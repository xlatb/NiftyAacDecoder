#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "AacBitReader.h"
#include "AacScalefactorDecoder.h"
#include "AacSpectrumDecoder.h"
#include "AacAudioTools.h"
#include "AacConstants.h"
#include "AacWindows.h"
#include "AacAudioBlock.h"

#include "AacDecoder.h"

#define AAC_MAX_SFB_COUNT     51

#define AAC_MAX_WINDOW_COUNT  8

#define AAC_MAX_WINDOW_GROUPS 8

#define AAC_MAX_PULSE_COUNT   4

#define AAC_MAX_TNS_ORDER_LONG_MAIN 20  // Long window, main profile
#define AAC_MAX_TNS_ORDER_LONG_LC   12  // Long window, low-complexity profile
#define AAC_MAX_TNS_ORDER_SHORT     7   // Short window, any profile

#define AAC_MAX_TNS_FILTER_COUNT    3

enum AacElementId
{
  AAC_ID_SCE = 0x0,  // Single channel element
  AAC_ID_CPE = 0x1,  // Channel pair element
  AAC_ID_CCE = 0x2,  // Coupling channel element
  AAC_ID_LFE = 0x3,  // Low frequency effect (subwoofer) element
  AAC_ID_DSE = 0x4,  // Data stream element
  AAC_ID_PCE = 0x5,  // Program config element
  AAC_ID_FIL = 0x6,  // Fill element
  AAC_ID_END = 0x7,  // End of data block
};

enum AacExtensionType
{
  AAC_EXT_FILL          = 0x0,  // Bitstream filler
  AAC_EXT_FILL_DATA     = 0x1,  // Bitstream data as filler
  AAC_EXT_DYNAMIC_RANGE = 0xB,  // Dynamic range control
  AAC_EXT_SBR_DATA      = 0xD,  // SBR enhancement
  AAC_EXT_SBR_DATA_CRC  = 0xE,  // SBR enhancement with CRC
};

enum
{
  AAC_TNS_FILTER_FLAG_DOWNWARD = 0x01,
  AAC_TNS_FILTER_FLAG_COMPRESS = 0x02,
};

struct AacIcsInfo
{
  AacWindowSequence windowSequence;
  AacWindowShape    windowShape;
  unsigned int      sfbCount;  // Scalefactor band count
  unsigned int      samplesPerWindow;  // Samples per window according to window size and sfbCount
  uint8_t           windowGroupBits;  // For short windows only
};

struct AacSectionInfo
{
  unsigned int windowCount;  // Must be 1 or 8

  unsigned int windowGroupCount;
  struct { uint8_t winStart; uint8_t winLength; uint16_t sampleCount; } windowGroups[AAC_MAX_WINDOW_GROUPS];

  uint8_t sfbCodebooks[AAC_MAX_WINDOW_GROUPS][AAC_MAX_SFB_COUNT];  // For each group, for each scalefactor band, the codebook

  struct
  {
    uint8_t count;
    struct { uint8_t sfbStart; uint8_t sfbLength; uint16_t sampleStart; uint16_t sampleCount; } sections[AAC_MAX_SFB_COUNT];
  } windowGroupSections[AAC_MAX_WINDOW_GROUPS];  // For each group, the sections
};

struct AacScalefactorInfo
{
  uint8_t scalefactors[AAC_MAX_WINDOW_GROUPS][AAC_MAX_SFB_COUNT];  // For each group, for each scalefactor band, the scalefactor [0..255]
};

struct AacPulseInfo
{
  uint8_t pulseCount;
  uint8_t pulseSfbStart;
  struct { uint8_t offset; uint8_t amplitude; } pulses[AAC_MAX_PULSE_COUNT];
};

struct AacTnsInfo
{
  uint8_t filterCount[AAC_MAX_WINDOW_COUNT];  // For each window, the number of filters [0..3]
  uint8_t coefficientBits[AAC_MAX_WINDOW_COUNT];  // [3..4]

  struct { uint8_t length; uint8_t order; uint8_t flags; uint8_t coefficients[AAC_MAX_TNS_ORDER_LONG_MAIN]; } filters[AAC_MAX_WINDOW_COUNT][AAC_MAX_TNS_FILTER_COUNT];  // For each window, for each filter, the filter info
};

struct AacDecodeInfo
{
  unsigned int        identifier;
  uint8_t             globalGain;

  AacIcsInfo         *ics;
  AacSectionInfo     *section;
  AacScalefactorInfo *sf;
  AacPulseInfo        pulse;
  AacTnsInfo          tns;
};

AacDecoder::AacDecoder(unsigned int sampleRate)
{
  m_sampleRate = sampleRate;
  m_sampleRateIndex = AacConstants::getIndexBySampleRate(sampleRate);

  m_scalefactorBandInfo = AacConstants::getScalefactorBandInfo(m_sampleRateIndex);

  m_blockCount = 0;
}

// ics_info
bool AacDecoder::decodeIcsInfo(AacBitReader *reader, AacIcsInfo *ics)
{
  unsigned int reserved = reader->readUInt(1);
  assert(reserved == 0);  // Always zero?

  ics->windowSequence = static_cast<AacWindowSequence>(reader->readUInt(2));
  ics->windowShape    = static_cast<AacWindowShape>(reader->readUInt(1));

  if (ics->windowSequence == AAC_WINSEQ_8_SHORT)
  {
    ics->sfbCount         = reader->readUInt(4);
    assert(ics->sfbCount <= m_scalefactorBandInfo->shortWindow->swbCount);
    ics->samplesPerWindow = m_scalefactorBandInfo->shortWindow->offsets[ics->sfbCount];

    ics->windowGroupBits  = reader->readUInt(7);

    return true;
  }

  ics->sfbCount = reader->readUInt(6);
  assert(ics->sfbCount <= m_scalefactorBandInfo->longWindow->swbCount);
  ics->samplesPerWindow = m_scalefactorBandInfo->longWindow->offsets[ics->sfbCount];

  bool predictorDataPresent = reader->readUInt(1);
  assert(predictorDataPresent == false);  // Not allowed in LC (low-complexity)

  return true;
}

// section_data
bool AacDecoder::decodeSectionInfo(AacBitReader *reader, const AacIcsInfo *ics, AacSectionInfo *sect)
{
  unsigned int sectionLengthBits;

  sect->windowGroupCount = 1;
  sect->windowGroups[0].winStart = 0;
  sect->windowGroups[0].winLength = 1;
  sect->windowGroups[0].sampleCount = 0;

  if (ics->windowSequence == AAC_WINSEQ_8_SHORT)
  {
    sectionLengthBits = 3;
    sect->windowCount = 8;

    // Decode groups bitmask
    for (int i = 6; i >= 0; i--)
    {
      if ((ics->windowGroupBits >> i & 0x01) == 0)
      {
        // New group
        sect->windowGroupCount++;
        sect->windowGroups[sect->windowGroupCount - 1].winStart = 7 - i;
        sect->windowGroups[sect->windowGroupCount - 1].winLength = 1;
        sect->windowGroups[sect->windowGroupCount - 1].sampleCount = 0;
      }
      else
      {
        // Existing group expands
        sect->windowGroups[sect->windowGroupCount - 1].winLength++;
      }
    }
  }
  else
  {
    sect->windowCount = 1;
    sectionLengthBits = 5;
  }

  unsigned int esc = (1 << sectionLengthBits) - 1;

  // For each group, read the huffman codebook number for each band
  unsigned int sampleStart = 0;  // Sample start index within bitstream
  for (unsigned int g = 0; g < sect->windowGroupCount; g++)
  {
    unsigned int k = 0;  // Current scalefactor band start point
    unsigned int sec = 0;  // Current section index within this group

    while (k < ics->sfbCount)
    {
      unsigned int codebook = reader->readUInt(4);

      // Read length of section
      // NOTE: A section length of zero is valid
      unsigned int len = 0;
      unsigned int l = reader->readUInt(sectionLengthBits);
      while (l == esc)
      {
        len += l;
        l = reader->readUInt(sectionLengthBits);
      }

      len += l;

      if (k + len > ics->sfbCount)
        return false;  // We've overflowed the scalefactor bands

      unsigned int sampleCount;
      if (ics->windowSequence != AAC_WINSEQ_8_SHORT)
        sampleCount = m_scalefactorBandInfo->longWindow->offsets[k + len] - m_scalefactorBandInfo->longWindow->offsets[k];  // One long window
      else
        sampleCount = (m_scalefactorBandInfo->shortWindow->offsets[k + len] - m_scalefactorBandInfo->shortWindow->offsets[k]) * sect->windowGroups[g].winLength;  // Eight short windows

      // Copy the codebook to each band as determined by the length
      for (unsigned int sfb = k; sfb < k + len; sfb++)
        sect->sfbCodebooks[g][sfb] = codebook;

      // Remember the extent of this section
      sect->windowGroupSections[g].sections[sec] = {.sfbStart = static_cast<uint8_t>(k), .sfbLength = static_cast<uint8_t>(len), .sampleStart = static_cast<uint16_t>(sampleStart), .sampleCount = static_cast<uint16_t>(sampleCount)};

      // Also update the sample count for the entire window group
      sect->windowGroups[g].sampleCount += sampleCount;

      k += len;

      sec++;
      if (sec >= AAC_MAX_SFB_COUNT)
        return false;  // Too many sections

      sampleStart += sampleCount;
      if (sampleStart > AAC_SPECTRAL_SAMPLE_SIZE_LONG)
        return false;  // Too many samples
    }

    sect->windowGroupSections[g].count = sec;
  }

  printf("Window groups: %d groups\n", sect->windowGroupCount);
  for (unsigned int g = 0; g < sect->windowGroupCount; g++)
  {
    printf("  windowGroups[%d]: winStart %d  winLength %d  sampleCount %d\n", g, sect->windowGroups[g].winStart, sect->windowGroups[g].winLength, sect->windowGroups[g].sampleCount);
    for (unsigned int sec = 0; sec < sect->windowGroupSections[g].count; sec++)
    {
      auto section = sect->windowGroupSections[g].sections[sec];
      printf("    group %d  section %d  codebook 0x%X  sfbStart %d  sfbLength %d  sampleStart %d  sampleCount %d\n", g, sec, sect->sfbCodebooks[g][section.sfbStart], section.sfbStart, section.sfbLength, section.sampleStart, section.sampleCount);
    }
  }

  return true;
}

// scale_factor_data
// § 8.3.2.5
bool AacDecoder::decodeScalefactorInfo(AacBitReader *reader, AacDecodeInfo *info)
{
  uint8_t sf = info->globalGain;

  auto sfd = AacScalefactorDecoder(reader);

  // For each group, read scalefactors for each scalefactor band
  printf("Scalefactors:\n");
  for (unsigned int g = 0; g < info->section->windowGroupCount; g++)
  {
    for (unsigned int sfb = 0; sfb < info->ics->sfbCount; sfb++)
    {
      unsigned int hcb = info->section->sfbCodebooks[g][sfb];
      if (hcb == AAC_HCB_ZERO)
        continue;  // Not an active band

      if ((hcb == AAC_HCB_INTENSITY) || (hcb == AAC_HCB_INTENSITY2))
      {
        info->sf->scalefactors[g][sfb] = 0;
        abort();  // TODO: Don't we have to read dpcm_is_position[] here?
        continue;  // Intensity stereo doesn't code scalefactors
      }

      int sfOffset;
      if (!sfd.decode(&sfOffset))
        return false;  // Huffman decode failure

      if ((sfOffset < 0) && (-sfOffset > sf))
        return false;  // Would underflow
      else if ((sfOffset > 0) && ((sfOffset + sf) > UINT8_MAX))
         return false;  // Would overflow

      sf += sfOffset;
      info->sf->scalefactors[g][sfb] = sf;
      printf("  g %d  hcb %d  sfb %d  sfOffset %d  sf %d\n", g, hcb, sfb, sfOffset, sf);
    }
  }

  return true;
}

// pulse_data
bool AacDecoder::decodePulseInfo(AacBitReader *reader, AacDecodeInfo *info)
{
  bool hasPulses = reader->readUInt(1);
  if (!hasPulses)
  {
    info->pulse.pulseCount = 0;
    return true;
  }

  // Pulses are disallowed when the window sequence is AAC_WINSEQ_8_SHORT
  if (info->ics->windowSequence == AAC_WINSEQ_8_SHORT)
    return false;  // Cannot combine pulses with short windows

  unsigned int pulseCount = reader->readUInt(2) + 1;
  info->pulse.pulseCount = pulseCount;

  printf("pulseCount %d\n", pulseCount);
  for (unsigned int p = 0; p < pulseCount; p++)
  {
    uint8_t offset = reader->readUInt(5);
    uint8_t amplitude = reader->readUInt(4);
    printf("pulse %d  offset %d  amplitude %d\n", p, offset, amplitude);
    info->pulse.pulses[p].offset = offset;
    info->pulse.pulses[p].amplitude = amplitude;
  }

  return true;
}

// tns_data()
bool AacDecoder::decodeTnsInfo(AacBitReader *reader, AacDecodeInfo *info)
{
  bool hasTns = reader->readUInt(1);
  if (!hasTns)
    return true;

  // § 14.2.1
  unsigned int filterCountBits;
  unsigned int lengthBits;
  unsigned int orderBits;
  unsigned int maxOrder;
  if (info->ics->windowSequence == AAC_WINSEQ_8_SHORT)
  {
    filterCountBits = 1;
    lengthBits = 4;
    orderBits = 3;
    maxOrder = AAC_MAX_TNS_ORDER_SHORT;
  }
  else
  {
    filterCountBits = 2;
    lengthBits = 6;
    orderBits = 5;
    maxOrder = AAC_MAX_TNS_ORDER_LONG_LC;
  }

  for (unsigned int w = 0; w < info->section->windowCount; w++)
  {
    unsigned int filterCount = reader->readUInt(filterCountBits);
    info->tns.filterCount[w] = filterCount;

    if (filterCount > 0)
      info->tns.coefficientBits[w] = reader->readUInt(1) + 3;

    for (unsigned int f = 0; f < filterCount; f++)
    {
      unsigned int length = reader->readUInt(lengthBits);
      unsigned int order = reader->readUInt(orderBits);

      if (order > maxOrder)
        return false;  // Order too high

      info->tns.filters[w][f].length = length;
      info->tns.filters[w][f].order  = order;
      info->tns.filters[w][f].flags  = 0;

      if (order)
      {
        if (reader->readUInt(1))
          info->tns.filters[w][f].flags |= AAC_TNS_FILTER_FLAG_DOWNWARD;

        if (reader->readUInt(1))
          info->tns.filters[w][f].flags |= AAC_TNS_FILTER_FLAG_COMPRESS;

        for (unsigned int o = 0; o < order; o++)
        {
          unsigned coefficientBits = info->tns.coefficientBits[w] - ((info->tns.filters[w][f].flags & AAC_TNS_FILTER_FLAG_COMPRESS) ? 1 : 0);
          info->tns.filters[w][f].coefficients[o] = reader->readUInt(coefficientBits);
          // TODO: Coefficient decoding per § 14.3
        }
      }
    }
  }

  return true;
}

// spectral_data()
bool AacDecoder::decodeSpectralData(AacBitReader *reader, AacDecodeInfo *info, int16_t quant[])
{
  auto sd = AacSpectrumDecoder(reader);

  printf("--- FRAME %d ---\n", m_blockCount);

  printf("decodeSpectralData():  windowSequence %s  sfbCount %d\n", AacConstants::getWindowSequenceName(info->ics->windowSequence), info->ics->sfbCount);

  for (unsigned int g = 0; g < info->section->windowGroupCount; g++)  // Groups
  {
    printf("- group %d has %d sections\n", g, info->section->windowGroupSections[g].count);

    for (unsigned int s = 0; s < info->section->windowGroupSections[g].count; s++)  // Sections
    {
      // Sections are ranges of one or more scalefactor bands that use the same codebook

      auto codebook = info->section->sfbCodebooks[g][info->section->windowGroupSections[g].sections[s].sfbStart];
      if ((codebook == AAC_HCB_ZERO) || (codebook > AAC_HCB_ESC))
      {
        printf("  group %d  section %d  codebook %2d -- Skipping due to codebook\n", g, s, codebook);
        continue;
      }

      unsigned int sectionSfbStart = info->section->windowGroupSections[g].sections[s].sfbStart;
      unsigned int sectionSfbEnd   = sectionSfbStart + info->section->windowGroupSections[g].sections[s].sfbLength;
      assert(sectionSfbEnd <= info->ics->sfbCount);

      unsigned int sectionSampleStart = info->section->windowGroupSections[g].sections[s].sampleStart;
      unsigned int sectionSampleEnd = sectionSampleStart + info->section->windowGroupSections[g].sections[s].sampleCount;

      // TODO: Move these asserts back to the section decoding
      if (info->ics->windowSequence != AAC_WINSEQ_8_SHORT)
      {
        // One long window
        assert(sectionSfbStart <  m_scalefactorBandInfo->longWindow->swbCount);
        assert(sectionSfbEnd   <= m_scalefactorBandInfo->longWindow->swbCount);
      }
      else
      {
        // Eight short windows
        assert(sectionSfbStart <  m_scalefactorBandInfo->shortWindow->swbCount);
        assert(sectionSfbEnd   <= m_scalefactorBandInfo->shortWindow->swbCount);
      }

      printf("  group %d  section %d  codebook %2d  sectionSfbStart %2u  sectionSfbEnd %2u  sectionSampleStart %4u  sectionSampleEnd %4u\n", g, s, codebook, sectionSfbStart, sectionSfbEnd, sectionSampleStart, sectionSampleEnd);

      if (codebook < AAC_HCB_FIRST_PAIR)
      {
        // 4-tuple decode
        for (unsigned int k = sectionSampleStart; k < sectionSampleEnd; k += 4)  // Sample index within group
        {
          // TODO: If decode4() took an int16_t, we could point it directly at quant[] and get rid of v[]?
          int v[4];
          sd.decode4(codebook, v);
          unsigned int dstIndex = k;
          printf("    dstIndex %d  sample %d: w %d  x %d  y %d  z %d\n", dstIndex, k, v[0], v[1], v[2], v[3]);
          quant[dstIndex++] = v[0];
          quant[dstIndex++] = v[1];
          quant[dstIndex++] = v[2];
          quant[dstIndex++] = v[3];
        }
      }
      else
      {
        // 2-tuple decode
        for (unsigned int k = sectionSampleStart; k < sectionSampleEnd; k += 2)
        {
          // TODO: If decode2() took an int16_t, we could point it directly at quant[] and get rid of v[]?
          int v[2];
          sd.decode2(codebook, v);
          unsigned int dstIndex = k;
          printf("    dstIndex %d  sample %d: y %d z %d\n", dstIndex, k, v[0], v[1]);
          quant[dstIndex++] = v[0];
          quant[dstIndex++] = v[1];
        }
      }
    }
  }

  // Deinterlace short blocks if needed
  if (info->ics->windowSequence == AAC_WINSEQ_8_SHORT)
  {
    // With long windows, we have a single window holding 1024 spectral
    //  samples. With short windows, we need eight temporal windows, each with
    //  128 samples.
    // The samples are stored in order by window group, then scalefactor band,
    //  then the windows within the window group. We re-order them to instead
    //  be stored by window. then scalefactor band.
    // See figure 6 and § 8.3.5
    // See also quant_to_spec() in § 9.3
    int16_t interlaced[AAC_SPECTRAL_SAMPLE_SIZE_LONG];
    memcpy(interlaced, quant, sizeof(int16_t) * AAC_SPECTRAL_SAMPLE_SIZE_LONG);

    unsigned int samplesPerWindow = info->ics->samplesPerWindow;
    printf("sampleIndicesPerWindow: %d\n", samplesPerWindow);

    for (unsigned int g = 0; g < info->section->windowGroupCount; g++)  // Groups
    {
      unsigned int winCount = info->section->windowGroups[g].winLength;  // Count of windows within group
      unsigned int groupSampleStart = info->section->windowGroupSections[g].sections[0].sampleStart;

      for (unsigned int winOffset = 0; winOffset < winCount; winOffset++)  // Window offset within group
      {
        unsigned int win = info->section->windowGroups[g].winStart + winOffset;

        unsigned int srcIndex = groupSampleStart + winOffset;
        unsigned int dstIndex = (win * AAC_SPECTRAL_SAMPLE_SIZE_SHORT);

        for (unsigned int s = 0; s < samplesPerWindow; s++)
        {
          printf("Deinterlace: group %d  winCount %d  winOffset %d  win  %d: %d ← %d\n", g, winCount, winOffset, win, dstIndex, srcIndex);
          quant[dstIndex] = interlaced[srcIndex];
          dstIndex++;
          srcIndex += winCount;
        }

        for (unsigned int s = samplesPerWindow; s < AAC_SPECTRAL_SAMPLE_SIZE_SHORT; s++)
        {
          printf("Deinterlace: group %d   win %d  zerofill: %d\n", g, win, dstIndex);
          quant[dstIndex++] = 0;
        }
      }
    }
  }

  // TODO: Max value of elements of quant is 8191. Should we be saturating them?
  for (unsigned int i = 0; i < 1024; i++)
  {
    printf("  quant[%d]: %d\n", i, quant[i]);
  }

  return true;
}

bool AacDecoder::decodeAudioLongWindow(AacBitReader *reader, AacDecodeInfo *info, AacAudioBlock *audio)
{
  int16_t quant[AAC_SPECTRAL_SAMPLE_SIZE_LONG] = {};  // Quantized spectal values

  if (!decodeSpectralData(reader, info, quant))
    return false;

  // Dequantize
  double dequant[AAC_SPECTRAL_SAMPLE_SIZE_LONG];
  AacAudioTools::dequantize(quant, dequant);

  // Rescale (§ 11.3.3)
  double x_rescal[AAC_SPECTRAL_SAMPLE_SIZE_LONG] = {};
  for (unsigned int g = 0; g < info->section->windowGroupCount; g++)  // Groups
  {
    unsigned int winCount = info->section->windowGroups[g].winLength;  // Count of windows within group

    for (unsigned int sfb = 0; sfb < info->ics->sfbCount; sfb++)
    {
      unsigned int sfbSampleStart, sfbSampleCount;
      if (info->ics->windowSequence != AAC_WINSEQ_8_SHORT)
      {
        // Long window
        sfbSampleStart = m_scalefactorBandInfo->longWindow->offsets[sfb];
        sfbSampleCount = m_scalefactorBandInfo->longWindow->offsets[sfb + 1] - sfbSampleStart;
      }
      else
      {
        // Short window
        sfbSampleStart = m_scalefactorBandInfo->shortWindow->offsets[sfb];
        sfbSampleCount = m_scalefactorBandInfo->shortWindow->offsets[sfb + 1] - sfbSampleStart;
      }

      double gain = pow(2, 0.25 * (info->sf->scalefactors[g][sfb] - 100));

      for (unsigned int winOffset = 0; winOffset < winCount; winOffset++)
      {
        unsigned int win = info->section->windowGroups[g].winStart + winOffset;

        // NOTE: The win variable should always be 0 for a long window, so this should be safe.
        unsigned int winSampleStart = win * AAC_SPECTRAL_SAMPLE_SIZE_SHORT;

        unsigned int sampleBase = winSampleStart + sfbSampleStart;
        for (unsigned int k = 0; k < sfbSampleCount; k++)
        {
          x_rescal[sampleBase + k] = dequant[sampleBase + k] * gain;
          printf("x_rescal[%d] = %f  group %d  sfb %d  gain %f\n", sampleBase + k, x_rescal[sampleBase + k], g, sfb, gain);
        }
      }
    }
  }

  // IMDCT
  printf("Frame %d samples\n", m_blockCount);
  double samples[AAC_XFORM_WIN_SIZE_LONG];
  if (info->ics->windowSequence != AAC_WINSEQ_8_SHORT)
  {
    // One long window
    AacAudioTools::IMDCTLong(x_rescal, samples);
  }
  else
  {
    // Eight short windows
    for (unsigned int w = 0; w < 8; w++)
      AacAudioTools::IMDCTShort(x_rescal + (w * AAC_SPECTRAL_SAMPLE_SIZE_SHORT), samples + (w * AAC_XFORM_WIN_SIZE_SHORT));
  }

  // Windowing (§ 15.3.2)
  if (m_blockCount == 0)
    m_previousWindowShape = info->ics->windowShape;

  if (info->ics->windowSequence != AAC_WINSEQ_8_SHORT)
  {
    // Long windows

    const double *leftWindow = AacWindows::getLeftWindow(m_previousWindowShape, info->ics->windowSequence);
    AacAudioTools::window(leftWindow, samples, AAC_XFORM_HALFWIN_SIZE_LONG);

    const double *rightWindow = AacWindows::getRightWindow(info->ics->windowShape, info->ics->windowSequence);
    AacAudioTools::window(rightWindow, samples + AAC_XFORM_HALFWIN_SIZE_LONG, AAC_XFORM_HALFWIN_SIZE_LONG);
  }
  else
  {
    // Short windows

    const double *leftWindow = AacWindows::getLeftWindow(m_previousWindowShape, info->ics->windowSequence);
    AacAudioTools::window(leftWindow, samples, AAC_XFORM_HALFWIN_SIZE_SHORT);

    const double *rightWindow = AacWindows::getRightWindow(info->ics->windowShape, info->ics->windowSequence);
    AacAudioTools::window(rightWindow, samples + AAC_XFORM_HALFWIN_SIZE_SHORT, AAC_XFORM_HALFWIN_SIZE_SHORT);

    leftWindow = AacWindows::getLeftWindow(info->ics->windowShape, info->ics->windowSequence);
    AacAudioTools::window(leftWindow, samples + (AAC_XFORM_HALFWIN_SIZE_SHORT * 2), AAC_XFORM_HALFWIN_SIZE_SHORT);
    AacAudioTools::window(rightWindow, samples + (AAC_XFORM_HALFWIN_SIZE_SHORT * 3), AAC_XFORM_HALFWIN_SIZE_SHORT);
    AacAudioTools::window(leftWindow, samples + (AAC_XFORM_HALFWIN_SIZE_SHORT * 4), AAC_XFORM_HALFWIN_SIZE_SHORT);
    AacAudioTools::window(rightWindow, samples + (AAC_XFORM_HALFWIN_SIZE_SHORT * 5), AAC_XFORM_HALFWIN_SIZE_SHORT);
    AacAudioTools::window(leftWindow, samples + (AAC_XFORM_HALFWIN_SIZE_SHORT * 6), AAC_XFORM_HALFWIN_SIZE_SHORT);
    AacAudioTools::window(rightWindow, samples + (AAC_XFORM_HALFWIN_SIZE_SHORT * 7), AAC_XFORM_HALFWIN_SIZE_SHORT);

    // Internal overlap of short windows
    double input[AAC_XFORM_WIN_SIZE_LONG];
    memcpy(input, samples, sizeof(double) * AAC_XFORM_WIN_SIZE_LONG);

    for (unsigned int s = 0; s < 448; s++)
      samples[s] = 0.0;

    for (unsigned int s = 0; s < 128; s++)
      samples[s + 448] = input[s];
    for (unsigned int s = 0; s < 128; s++)
      samples[s + 576] = input[s + 128] + input[s + 256];
    for (unsigned int s = 0; s < 128; s++)
      samples[s + 704] = input[s + 384] + input[s + 512];
    for (unsigned int s = 0; s < 128; s++)
      samples[s + 832] = input[s + 640] + input[s + 768];
    for (unsigned int s = 0; s < 128; s++)
      samples[s + 960] = input[s + 896] + input[s + 1024];
    for (unsigned int s = 0; s < 128; s++)
      samples[s + 1088] = input[s + 1152] + input[s + 1280];
    for (unsigned int s = 0; s < 128; s++)
      samples[s + 1216] = input[s + 1408] + input[s + 1536];
    for (unsigned int s = 0; s < 128; s++)
      samples[s + 1344] = input[s + 1664] + input[s + 1792];
    for (unsigned int s = 0; s < 128; s++)
      samples[s + 1472] = input[s + 1920];

    for (unsigned int s = 0; s < 448; s++)
      samples[s + 1600] = 0.0;
  }

  // Overlapping with previous samples (§ 15.3.3)
  static double oldSamples[1024] = {};  // TODO: Constant for length, move to member variable
  for (unsigned int s = 0; s < 1024; s++)
  {
    auto tmp = samples[s];
    samples[s] += oldSamples[s];
    printf("  overlap[%d]: transform %.3f  old %.3f  sum %.3f\n", s, tmp, oldSamples[s], samples[s]);
  }

  // Save second half of previous samples for next time
  for (unsigned int s = 0; s < 1024; s++)
    oldSamples[s] = samples[s + 1024];

  // Convert to int16
  int16_t final[1024];  // TODO: Constant for length
  for (unsigned int s = 0; s < 1024; s++)
  {
    if (samples[s] > 0)
      final[s] = static_cast<int16_t>(samples[s] + 0.5);
    else
      final[s] = static_cast<int16_t>(samples[s] - 0.5);
  }

  // Write the first 1024 samples to the audio buffer
  int16_t *buf;
  audio->prepare(m_sampleRate, 1);  // 1 channel
  audio->getSampleBuffer(&buf);
  memcpy(buf, final, sizeof(int16_t) * 1024);

  // Remember window shape for next block
  m_previousWindowShape = info->ics->windowShape;

  return true;
}

//bool AacDecoder::decodeAudioShortWindow(AacBitReader *reader, AacDecodeInfo *info, AacAudioBlock *audio)
//{
//  int16_t quant[AAC_SPECTRAL_SAMPLE_SIZE_LONG] = {};  // Quantized spectal values
//
//  if (!decodeSpectralData(reader, info, quant))
//    return false;
//
//  abort();
//}

bool AacDecoder::decodeBlock(AacBitReader *reader, AacAudioBlock *audio)
{
  bool done = false;

  while (!done && !reader->isComplete())
  {
    unsigned int id = reader->readUInt(3);
    printf("Element ID: 0x%X\n", id);

    switch (id)
    {
    case AAC_ID_END:
      done = true;
      break;
    case AAC_ID_FIL:
      if (!decodeElementFIL(reader))
        return false;
      break;
    case AAC_ID_SCE:
      if (!decodeElementSCE(reader, audio))
        return false;
      break;
    case AAC_ID_CPE:
      if (!decodeElementCPE(reader, audio))
        return false;
      break;
    default:
      printf("Unhandled element ID %X\n", id);
      abort();
    }
  }

  m_blockCount++;

  reader->alignToBit(0);

  return true;
}

// Fill element
bool AacDecoder::decodeElementFIL(AacBitReader *reader)
{
  // Table 26
  unsigned int count = reader->readUInt(4);
  if (count == 15)
  {
    unsigned int extra = reader->readUInt(8);
    count += extra - 1;
  }

  printf("FIL count %d\n", count);
  reader->skipBytes(count);

  return true;
}

// Single channel element
bool AacDecoder::decodeElementSCE(AacBitReader *reader, AacAudioBlock *audio)
{
  AacIcsInfo ics;
  AacSectionInfo section;
  AacScalefactorInfo sf;

  AacDecodeInfo info;
  info.ics     = &ics;
  info.section = &section;
  info.sf      = &sf;

  info.identifier = reader->readUInt(4);
  info.globalGain = reader->readUInt(8);

  printf("-- SCE --\n");
  printf("identifier       : %d\n", info.identifier);
  printf("global gain      : %d\n", info.globalGain);

  if (!decodeIcsInfo(reader, &ics))
    return false;

  printf("window seq        : %d\n", ics.windowSequence);
  printf("window shape      : %d\n", ics.windowShape);
  printf("scalefactor bands : %d\n", ics.sfbCount);
  printf("samples per window: %d\n", ics.samplesPerWindow);

  if (!decodeSectionInfo(reader, &ics, &section))
    return false;

  printf("Window count     : %d\n", section.windowCount);

  if (!decodeScalefactorInfo(reader, &info))
    return false;

  if (!decodePulseInfo(reader, &info))
    return false;

  if (!decodeTnsInfo(reader, &info))
    return false;

  bool hasGainControl = reader->readUInt(1);
  if (hasGainControl)
    return false;  // Not allowed in LC profile

  if (section.windowCount == 1)
  {
    if (!decodeAudioLongWindow(reader, &info, audio))  // Long window
      return false;
  }
  else
  {
    if (!decodeAudioLongWindow(reader, &info, audio))  // Short window (TODO: Switch to decodAudioShortWindow)
      return false;
  }

  return true;
}

// Channel pair element
bool AacDecoder::decodeElementCPE(AacBitReader *reader, AacAudioBlock *audio)
{
  unsigned int identifier = reader->readUInt(4);

  bool commonWindow = reader->readUInt(1);

  printf("-- CPE --\n");
  printf("identifier : %d\n", identifier);
  printf("commonWindow: %s\n", commonWindow ? "true" : "false");
  abort();
}

