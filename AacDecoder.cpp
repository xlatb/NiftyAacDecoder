// TODO: Delete
       #include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>
       #include <unistd.h>



#include <assert.h>
#include <stdio.h>
#include <math.h>

#include "AacBitReader.h"
#include "AacScalefactorDecoder.h"
#include "AacSpectrumDecoder.h"
#include "AacConstants.h"
#include "AacWindows.h"

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
  uint8_t           windowGroupBits;  // For short windows only
};

struct AacSectionInfo
{
  unsigned int windowCount;
  unsigned int windowGroupCount;
  uint8_t      windowGroupLengths[AAC_MAX_WINDOW_GROUPS];

  uint8_t      sfbCodebooks[AAC_MAX_WINDOW_GROUPS][AAC_MAX_SFB_COUNT];  // For each group, for each scalefactor band, the codebook

  struct { uint8_t count; struct { uint8_t start; uint8_t length; } sections[AAC_MAX_SFB_COUNT]; } windowGroupSections[AAC_MAX_WINDOW_GROUPS];  // For each group, the sections
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
    ics->sfbCount        = reader->readUInt(4);
    ics->windowGroupBits = reader->readUInt(7);
    return true;
  }

  ics->sfbCount = reader->readUInt(6);

  bool predictorDataPresent = reader->readUInt(1);
  assert(predictorDataPresent == false);  // Not allowed in LC (low-complexity)

  return true;
}

// section_data
bool AacDecoder::decodeSectionInfo(AacBitReader *reader, const AacIcsInfo *ics, AacSectionInfo *sect)
{
  unsigned int sectionLengthBits;

  sect->windowGroupCount = 1;
  sect->windowGroupLengths[0] = 1;

  if (ics->windowSequence == AAC_WINSEQ_8_SHORT)
  {
    sectionLengthBits = 3;
    sect->windowCount = 8;

    // Decode groups bitmask
    for (int i = 7; i >= 0; i--)
    {
      if ((ics->windowGroupBits >> i & 0x01) == 0)
      {
        // New group
        sect->windowGroupCount++;
        sect->windowGroupLengths[sect->windowGroupCount - 1] = 1;
      }
      else
      {
        // Existing group expands
        sect->windowGroupLengths[sect->windowGroupCount - 1]++;
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
  for (unsigned int g = 0; g < sect->windowGroupCount; g++)
  {
    unsigned int k = 0;  // Current scalefactor band start point
    unsigned int s = 0;  // Current section index within this group

    while (k < ics->sfbCount)
    {
      unsigned int codebook = reader->readUInt(4);
      unsigned int len = 0;

      unsigned int l = reader->readUInt(sectionLengthBits);
      while (l == esc)
      {
        len += l;
        l = reader->readUInt(sectionLengthBits);
      }

      if (l == 0)
        return false;  // Zero-length sections could allow s to overflow our arrays
      else if (k + l > ics->sfbCount)
        return false;  // We've overflowed the scalefactor bands

      len += l;
      printf("group %d  section %d  codebook 0x%X  start %d  length %d\n", g, s, codebook, k, len);

      // Copy the codebook to each band as determined by the length
      for (unsigned int sfb = k; sfb < k + len; sfb++)
        sect->sfbCodebooks[g][sfb] = codebook;

      // Remember the extent of this section
      sect->windowGroupSections[g].sections[s] = {.start = static_cast<uint8_t>(k), .length = static_cast<uint8_t>(len)};

      k += len;
      s++;
    }

    sect->windowGroupSections[g].count = s;
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
      printf("g %d  sfb %d  sfOffset %d  sf %d\n", g, sfb, sfOffset, sf);
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

    if (filterCount> 0)
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
          info->tns.filters[w][f].coefficients[o] = reader->readUInt(orderBits);
        }
      }
    }
  }

  return true;
}

// spectral_data()
bool AacDecoder::decodeSpectralData(AacBitReader *reader, AacDecodeInfo *info)
{
  auto sd = AacSpectrumDecoder(reader);

  int16_t x_quant[1024] = {};

  printf("decodeSpectralData():  windowSequence %s  sfbCount %d\n", AacConstants::getWindowSequenceName(info->ics->windowSequence), info->ics->sfbCount);

  for (unsigned int g = 0; g < info->section->windowGroupCount; g++)  // Groups
  {
    printf("- group %d has %d sections\n", g, info->section->windowGroupSections[g].count);
    for (unsigned int s = 0; s < info->section->windowGroupSections[g].count; s++)  // Sections
    {
      // Sections are ranges of one or more scalefactor bands that use the same codebook

      auto codebook = info->section->sfbCodebooks[g][info->section->windowGroupSections[g].sections[s].start];
      if ((codebook == AAC_HCB_ZERO) || (codebook > AAC_HCB_ESC))
      {
        printf("  group %d  section %d  codebook %2d -- Skipping due to codebook\n", g, s, codebook);
        continue;
      }

      unsigned int sectionSfbStart = info->section->windowGroupSections[g].sections[s].start;
      unsigned int sectionSfbEnd   = sectionSfbStart + info->section->windowGroupSections[g].sections[s].length;
      assert(sectionSfbEnd <= info->ics->sfbCount);

      unsigned int sectionSampleStart;
      unsigned int sectionSampleEnd;
      if (info->ics->windowSequence != AAC_WINSEQ_8_SHORT)
      {
        // One long window
        assert(sectionSfbStart <  m_scalefactorBandInfo->longWindow->swbCount);
        assert(sectionSfbEnd   <= m_scalefactorBandInfo->longWindow->swbCount);
        sectionSampleStart = m_scalefactorBandInfo->longWindow->offsets[sectionSfbStart];
        sectionSampleEnd   = m_scalefactorBandInfo->longWindow->offsets[sectionSfbEnd];
      }
      else
      {
        // Eight short windows
        assert(sectionSfbStart <  m_scalefactorBandInfo->shortWindow->swbCount);
        assert(sectionSfbEnd   <= m_scalefactorBandInfo->shortWindow->swbCount);
        // TODO: We need to look up the sect_sfb_offset[g][s]
        abort();  // TODO
      }

      printf("  group %d  section %d  codebook %2d  sectionSfbStart %2u  sectionSfbEnd %2u  sectionSampleStart %4u  sectionSampleEnd %4u\n", g, s, codebook, sectionSfbStart, sectionSfbEnd, sectionSampleStart, sectionSampleEnd);


      if (codebook < AAC_HCB_FIRST_PAIR)
      {
        // 4-tuple decode
        for (unsigned int k = sectionSampleStart; k < sectionSampleEnd; k += 4)
        {
          // TODO: If decode4() took an int16_t, we could point it directly at x_quant[] and get rid of v[]?
          int v[4];
          sd.decode4(codebook, v);
          printf("    sample %d: w %d  x %d  y %d  z %d\n", k, v[0], v[1], v[2], v[3]);
          x_quant[k + 0] = v[0];
          x_quant[k + 1] = v[1];
          x_quant[k + 2] = v[2];
          x_quant[k + 3] = v[3];
        }
      }
      else
      {
        // 2-tuple decode
        for (unsigned int k = sectionSampleStart; k < sectionSampleEnd; k += 2)
        {
          // TODO: If decode2() took an int16_t, we could point it directly at x_quant[] and get rid of v[]?
          int v[2];
          sd.decode2(codebook, v);
          printf("    sample %d: y %d z %d\n", k, v[0], v[1]);
          x_quant[k + 0] = v[0];
          x_quant[k + 1] = v[1];
        }
      }
    }
  }

  // TODO: Max value of elements of x_quant is 8191. Should we be saturating them?

  // Dequantize (§ 10.3)
  double x_invquant[1024];  // TODO: A constant for length
  for (unsigned int i = 0; i < 1024; i++)
  {
    // This is really just raising the value to the power of (4/3) but
    //  also preserving the sign of negative values.
    // TODO: Calculate 4/3 outside the loop.
    x_invquant[i] = pow(abs(x_quant[i]), 4.0/3.0) * ((x_quant[i] > 0) ? 1.0 : -1.0);
  }

  // Rescale (§ 11.3.3)
  double x_rescal[1024] = {};
  for (unsigned int g = 0; g < info->section->windowGroupCount; g++)  // Groups
  {
    for (unsigned int sfb = 0; sfb < info->ics->sfbCount; sfb++)
    {
      unsigned int swbStart, swbWidth;
      if (info->ics->windowSequence != AAC_WINSEQ_8_SHORT)
      {
        // Long window
        swbStart = m_scalefactorBandInfo->longWindow->offsets[sfb];
        swbWidth = m_scalefactorBandInfo->longWindow->offsets[sfb + 1] - swbStart;
      }
      else
      {
        // Short window
        // TODO: Need to adjust start by group number on short windows?
        swbStart = m_scalefactorBandInfo->shortWindow->offsets[sfb];
        swbWidth = m_scalefactorBandInfo->shortWindow->offsets[sfb + 1] - swbStart;
      }

      for (unsigned int w = 0; w < info->section->windowGroupLengths[g]; w++)
      {
        double gain = pow(2, 0.25 * (info->sf->scalefactors[g][sfb] - 100));

        for (unsigned int k = 0; k < swbWidth; k++)
        {
          x_rescal[swbStart + k] = x_invquant[swbStart + k] * gain;
          printf("x_rescal[%d] = %f  group %d  sfb %d  gain %f\n", swbStart + k, x_rescal[swbStart + k], g, sfb, gain);
        }
      }
    }
  }

  // TODO: De-interleave the spectral samples when we have short bands?
  // See figure 6 and § 8.3.5
  // Is quant_to_spec() in § 9.3 also related?

  // IMDCT
  constexpr size_t size = 2048;
  double samples[size];
  double half = ((size / 2.0) + 1) / 2.0;
  for (unsigned int s = 0; s < size; s++)  // Audio samples
  {
    double sum = 0.0;
    unsigned int spectralCount = size >> 1;
    for (unsigned int k = 0; k < spectralCount; k++)  // Spectral coefficients
    {
      double v = x_rescal[k] * cos(((M_PI * 2.0) / size) * (s + half) * (k + 0.5));
      sum += v;
      //printf("  filterbank: s %d  k %d  x_rescal %f  v %f  sum %f\n", s, k, x_rescal[k], v, sum);
    }

    sum *= 2.0;  // TEST!!!
    double sample = (2.0 / size) * sum;
    samples[s] = sample;
    printf("samples[%d] = %f  sum %.3f\n", s, sample, sum);
  }

  // Windowing (§ 15.3.2)
  if (m_blockCount == 0)
    m_previousWindowShape = info->ics->windowShape;

  if (info->ics->windowSequence != AAC_WINSEQ_8_SHORT)
  {
    // Long windows

    const double *leftWindow = AacWindows::getLeftWindow(m_previousWindowShape, info->ics->windowSequence);
    for (unsigned int s = 0; s < AAC_XFORM_HALFWIN_SIZE_LONG; s++)
      samples[s] *= leftWindow[s];

    const double *rightWindow = AacWindows::getRightWindow(m_previousWindowShape, info->ics->windowSequence);
    for (unsigned int s = 0; s < AAC_XFORM_HALFWIN_SIZE_LONG; s++)
      samples[AAC_XFORM_HALFWIN_SIZE_LONG + s] *= rightWindow[s];
  }
  else
  {
    // TODO: Windowing of short windows
  }

  // Overlapping with previous samples (§ 15.3.3)
  static double oldSamples[1024] = {};  // TODO: Constant for length, move to member variable
  for (unsigned int s = 0; s < 1024; s++)
  {
    auto tmp = samples[s];
    samples[s] += oldSamples[s];
    printf("overlap[%d]: transform %f  old %f  sum %f\n", s, tmp, oldSamples[s], samples[s]);
  }

  // Save second half of previous samples for next time
  for (unsigned int s = 0; s < 1024; s++)
    oldSamples[s] = samples[s + 1024];

  // Convert to int16
  // TODO: We only need to convert the first 1024
  int16_t final[2048];
  for (unsigned int s = 0; s < 2048; s++)
  {
    if (samples[s] > 0)
      final[s] = static_cast<int16_t>(samples[s] + 0.5);
    else
      final[s] = static_cast<int16_t>(samples[s] - 0.5);
  }

  int fd = open("out.pcm", O_WRONLY|O_APPEND|O_CREAT, 0600);
  if (fd < 1)
  {
    perror("open()");
    abort();
  }

  unsigned int writeSize = size / 2;  // Only write the first 1024 samples
  if (write(fd, final, writeSize * sizeof(int16_t)) != writeSize * sizeof(int16_t))
  {
    fprintf(stderr, "write(): Short write\n");
    abort();
  }

  close(fd);

  // Remember window shape for next block
  m_previousWindowShape = info->ics->windowShape;

  return true;
}

bool AacDecoder::decodeBlock(AacBitReader *reader)
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
      if (!decodeElementSCE(reader))
        return false;
      break;
    case AAC_ID_CPE:
      if (!decodeElementCPE(reader))
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
bool AacDecoder::decodeElementSCE(AacBitReader *reader)
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

  printf("window seq       : %d\n", ics.windowSequence);
  printf("window shape     : %d\n", ics.windowShape);
  printf("scalefactor bands: %d\n", ics.sfbCount);

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

  if (!decodeSpectralData(reader, &info))
    return false;

  // TODO: Decode audio

  return true;
}

// Channel pair element
bool AacDecoder::decodeElementCPE(AacBitReader *reader)
{
  unsigned int identifier = reader->readUInt(4);

  bool commonWindow = reader->readUInt(1);

  printf("-- CPE --\n");
  printf("identifier : %d\n", identifier);
  printf("commonWindow: %s\n", commonWindow ? "true" : "false");
  abort();
}

