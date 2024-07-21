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
#include "AacStructs.h"
#include "AacChannelDecoder.h"

#include "AacDecoder.h"

#define AAC_MONO_CHANNEL_COUNT   1
#define AAC_STEREO_CHANNEL_COUNT 2

// This is not standard. We just use it to bias the signed stereo position
//  for intensity stereo so that it can be stored in an uint8_t. That way we
//  can share the storage with the scalefactors.
#define AAC_STEREO_POSITION_BIAS 128

AacDecoder::AacDecoder(unsigned int sampleRate)
{
  m_sampleRate = sampleRate;
  m_sampleRateIndex = AacConstants::getIndexBySampleRate(sampleRate);

  m_scalefactorBandInfo = AacConstants::getScalefactorBandInfo(m_sampleRateIndex);

  m_blockCount = 0;
}

// program_config_element
bool AacDecoder::readProgramConfigInfo(AacBitReader *reader, AacProgramConfigInfo *pce)
{
  pce->instance        = reader->readUInt(4);
  pce->profile         = reader->readUInt(2);
  pce->sampleRateIndex = reader->readUInt(4);

  pce->frontChannelElementCount = reader->readUInt(4);
  assert(pce->frontChannelElementCount <= AAC_PCE_MAX_FRONT_CHANNEL_ELEMENTS);

  pce->sideChannelElementCount = reader->readUInt(4);
  assert(pce->sideChannelElementCount <= AAC_PCE_MAX_SIDE_CHANNEL_ELEMENTS);

  pce->rearChannelElementCount = reader->readUInt(4);
  assert(pce->rearChannelElementCount <= AAC_PCE_MAX_REAR_CHANNEL_ELEMENTS);

  pce->lfeChannelElementCount = reader->readUInt(2);
  assert(pce->lfeChannelElementCount <= AAC_PCE_MAX_LFES);

  pce->dseElementCount = reader->readUInt(3);
  assert(pce->dseElementCount <= AAC_PCE_MAX_DSES);

  pce->channelCouplingElementCount = reader->readUInt(4);
  assert(pce->channelCouplingElementCount <= AAC_PCE_MAX_CCES);

  pce->hasMonoMixdown = reader->readUInt(1);
  if (pce->hasMonoMixdown)
    pce->monoMixdown = reader->readUInt(4);

  pce->hasStereoMixdown = reader->readUInt(1);
  if (pce->hasStereoMixdown)
    pce->stereoMixdown = reader->readUInt(4);

  pce->hasMatrixMixdown = reader->readUInt(1);
  if (pce->hasMatrixMixdown)
  {
    pce->matrixMixdownIndex = reader->readUInt(2);
    pce->pseudoSurroundEnabled = reader->readUInt(1);
  }

  // Identifiers for each front channel element
  for (unsigned int i = 0; i < pce->frontChannelElementCount; i++)
  {
    AacElementId type = reader->readUInt(1) ? AAC_ID_CPE : AAC_ID_SCE;
    uint8_t instance = reader->readUInt(4);

    pce->frontChannelElements[i].type     = type;
    pce->frontChannelElements[i].instance = instance;
  }

  // Identifiers for each side channel element
  for (unsigned int i = 0; i < pce->sideChannelElementCount; i++)
  {
    AacElementId type = reader->readUInt(1) ? AAC_ID_CPE : AAC_ID_SCE;
    uint8_t instance = reader->readUInt(4);

    pce->sideChannelElements[i].type     = type;
    pce->sideChannelElements[i].instance = instance;
  }

  // Identifiers for each rear channel element
  for (unsigned int i = 0; i < pce->rearChannelElementCount; i++)
  {
    AacElementId type = reader->readUInt(1) ? AAC_ID_CPE : AAC_ID_SCE;
    uint8_t instance = reader->readUInt(4);

    pce->rearChannelElements[i].type     = type;
    pce->rearChannelElements[i].instance = instance;
  }

  // Identifiers for each LFE (subwoofer) channel element
  for (unsigned int i = 0; i < pce->lfeChannelElementCount; i++)
    pce->lfeChannelElements[i] = reader->readUInt(4);

  // Identifiers for each DSE instance
  for (unsigned int i = 0; i < pce->lfeChannelElementCount; i++)
    pce->lfeChannelElements[i] = reader->readUInt(4);

  // Comment field
  reader->alignToBit(0);
  unsigned int commentLength = reader->readUInt(8);
  for (unsigned int i = 0; i < commentLength; i++)
    pce->comment += reader->readByte();

  return true;
}

// ics_info
bool AacDecoder::decodeIcsInfo(AacBitReader *reader, AacIcsInfo *ics)
{
  unsigned int reserved = reader->readUInt(1);
  assert(reserved == 0);  // Always zero?

  ics->windowSequence = static_cast<AacWindowSequence>(reader->readUInt(2));
  ics->windowShape    = static_cast<AacWindowShape>(reader->readUInt(1));

  ics->windowGroupCount = 1;
  ics->windowGroups[0].winStart = 0;
  ics->windowGroups[0].winLength = 1;

  if (ics->windowSequence == AAC_WINSEQ_8_SHORT)
  {
    // Short windows

    ics->sfbCount         = reader->readUInt(4);
    assert(ics->sfbCount <= m_scalefactorBandInfo->shortWindow->swbCount);
    ics->samplesPerWindow = m_scalefactorBandInfo->shortWindow->offsets[ics->sfbCount];

    unsigned int windowGroupBits  = reader->readUInt(7);

    ics->windowCount = 8;
    ics->isLongWindow = false;

    // Decode groups bitmask
    for (int i = 6; i >= 0; i--)
    {
      if ((windowGroupBits >> i & 0x01) == 0)
      {
        // New group
        ics->windowGroupCount++;
        ics->windowGroups[ics->windowGroupCount - 1].winStart = 7 - i;
        ics->windowGroups[ics->windowGroupCount - 1].winLength = 1;
      }
      else
      {
        // Existing group expands
        ics->windowGroups[ics->windowGroupCount - 1].winLength++;
      }
    }
  }
  else
  {
    // Long windows

    ics->sfbCount = reader->readUInt(6);
    assert(ics->sfbCount <= m_scalefactorBandInfo->longWindow->swbCount);
    ics->samplesPerWindow = m_scalefactorBandInfo->longWindow->offsets[ics->sfbCount];

    bool predictorDataPresent = reader->readUInt(1);
    assert(predictorDataPresent == false);  // Not allowed in LC (low-complexity)

    ics->windowCount = 1;
    ics->isLongWindow = true;
  }

  return true;
}

// Decodes M/S (Main/Side) mask information from a CPE (Channel Pair Element).
bool AacDecoder::decodeMsMaskInfo(AacBitReader *reader, const AacIcsInfo *ics, AacMsMaskInfo *msMask)
{
  auto type = static_cast<AacMsMaskType>(reader->readUInt(2));
  if (type == AAC_MS_MASK_RESERVED)
    return false;  // Invalid M/S mask type

  msMask->type = type;
  if (type == AAC_MS_MASK_SUBBAND)
  {
    // We leverage the fact that the number of groups is at most 8, and pack
    //  each SFB's flags into one byte. Group zero's flag is in the least-
    //  significant bit.

    memset(msMask->sfbMask, 0, sizeof(msMask->sfbMask[0]) * ics->sfbCount);

    for (unsigned int g = 0; g < ics->windowGroupCount; g++)
    {
      for (unsigned int sfb = 0; sfb < ics->sfbCount; sfb++)
      {
        if (reader->readUInt(1))
          msMask->sfbMask[sfb] |= (0x1 << g);
      }
    }
  }

  return true;
}

// section_data
bool AacDecoder::decodeSectionInfo(AacBitReader *reader, AacDecodeInfo *info)
{
  unsigned int sectionLengthBits = (info->ics->windowSequence == AAC_WINSEQ_8_SHORT) ? 3 : 5;

  unsigned int esc = (1 << sectionLengthBits) - 1;

  // For each group, read the huffman codebook number for each band
  unsigned int sampleStart = 0;  // Sample start index within bitstream
  for (unsigned int g = 0; g < info->ics->windowGroupCount; g++)
  {
    unsigned int k = 0;  // Current scalefactor band start point
    unsigned int sec = 0;  // Current section index within this group

    info->section.windowGroups[g].sampleCount = 0;

    while (k < info->ics->sfbCount)
    {
      uint8_t codebook = reader->readUInt(4);

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

      if (k + len > info->ics->sfbCount)
        return false;  // We've overflowed the scalefactor bands

      unsigned int sampleCount;
      if (info->ics->windowSequence != AAC_WINSEQ_8_SHORT)
        sampleCount = m_scalefactorBandInfo->longWindow->offsets[k + len] - m_scalefactorBandInfo->longWindow->offsets[k];  // One long window
      else
        sampleCount = (m_scalefactorBandInfo->shortWindow->offsets[k + len] - m_scalefactorBandInfo->shortWindow->offsets[k]) * info->ics->windowGroups[g].winLength;  // Eight short windows

      // Copy the codebook to each band as determined by the length
      for (unsigned int sfb = k; sfb < k + len; sfb++)
        info->section.sfbCodebooks[g][sfb] = codebook;

      uint16_t winSampleStart, winSampleCount;
      if (info->ics->windowSequence != AAC_WINSEQ_8_SHORT)
      {
        // Long window
        winSampleStart = m_scalefactorBandInfo->longWindow->offsets[k];
        winSampleCount = m_scalefactorBandInfo->longWindow->offsets[k + len] - winSampleStart;
      }
      else
      {
        // Short window
        winSampleStart = m_scalefactorBandInfo->shortWindow->offsets[k];
        winSampleCount = m_scalefactorBandInfo->shortWindow->offsets[k + len] - winSampleStart;
      }

      // Remember the extent of this section
      info->section.windowGroupSections[g].sections[sec] = {.sfbStart = static_cast<uint8_t>(k), .sfbLength = static_cast<uint8_t>(len), .sampleStart = static_cast<uint16_t>(sampleStart), .sampleCount = static_cast<uint16_t>(sampleCount), .winSampleStart = winSampleStart, .winSampleCount = winSampleCount, .codebook = codebook};

      // Also update the sample count for the entire window group
      info->section.windowGroups[g].sampleCount += sampleCount;

      k += len;

      sec++;
      if (sec >= AAC_MAX_SFB_COUNT)
        return false;  // Too many sections

      sampleStart += sampleCount;
      if (sampleStart > AAC_SPECTRAL_SAMPLE_SIZE_LONG)
        return false;  // Too many samples
    }

    info->section.windowGroupSections[g].count = sec;
  }

  //printf("Window groups: %d groups\n", info->ics->windowGroupCount);
  for (unsigned int g = 0; g < info->ics->windowGroupCount; g++)
  {
    //printf("  windowGroups[%d]: winStart %d  winLength %d  sampleCount %d\n", g, info->ics->windowGroups[g].winStart, info->ics->windowGroups[g].winLength, info->section.windowGroups[g].sampleCount);
    for (unsigned int sec = 0; sec < info->section.windowGroupSections[g].count; sec++)
    {
      auto section = info->section.windowGroupSections[g].sections[sec];
      //printf("    group %d  section %d  codebook 0x%X  sfbStart %d  sfbLength %d  sampleStart %d  sampleCount %d  winSampleStart %d  winSampleCount %d\n", g, sec, info->section.sfbCodebooks[g][section.sfbStart], section.sfbStart, section.sfbLength, section.sampleStart, section.sampleCount, section.winSampleStart, section.winSampleCount);
    }
  }

  return true;
}

// scale_factor_data
// § 8.3.2.5
bool AacDecoder::decodeScalefactorInfo(AacBitReader *reader, AacDecodeInfo *info)
{
  uint8_t sf = info->globalGain;

  int sp = 0;  // Stereo position for intensity stereo
  bool hasIntensityStereo = false;

  int ne;  // Noise energy for PNS
  bool hasNoise = false;

  auto sfd = AacScalefactorDecoder(reader);

  // For each group, read scalefactors for each scalefactor band
  printf("Scalefactors:\n");
  for (unsigned int g = 0; g < info->ics->windowGroupCount; g++)
  {
    for (unsigned int sfb = 0; sfb < info->ics->sfbCount; sfb++)
    {
      unsigned int hcb = info->section.sfbCodebooks[g][sfb];
      if (hcb == AAC_HCB_ZERO)
      {
        info->sf.scalefactors[g][sfb] = 0;
        continue;  // Not an active band
      }

      int offset;

      if (AAC_IS_INTENSITY_CODEBOOK(hcb))
      {
        // Intensity stereo position info

        if (!sfd.decode(&offset))
          return false;  // Huffman decode failure

        sp += offset;
        info->sf.scalefactors[g][sfb] = sp + AAC_STEREO_POSITION_BIAS;
        //printf("  g %d  hcb 0x%X  sfb %2d  type IS  spOffset %2d  sp %d\n", g, hcb, sfb, offset, sp);
        hasIntensityStereo = true;
      }
      else if (hcb == AAC_HCB_NOISE)
      {
        // PNS (Perceptual Noise Substitution)

        if (!hasNoise)
        {
          ne = reader->readUInt(9);  // Noise start point
          hasNoise = true;
        }
        else
        {
          if (!sfd.decode(&offset))
            return false;  // Huffman decode failure

          ne += offset;
          //info->sf.scalefactors[g][sfb] = ne;
        }
        //printf("  g %d  hcb 0x%X  sfb %2d  type PNS  ne %d\n", g, hcb, sfb, ne);
      }
      else if (AAC_IS_UNKNOWN_CODEBOOK(hcb))
      {
        // Unknown codebook
        // Some codebook numbers alter the bitstream layout, so it's not safe to continue.
        return false;
      }
      else
      {
        // Normal scalefactor info

        if (!sfd.decode(&offset))
          return false;  // Huffman decode failure

        if ((offset < 0) && (-offset > sf))
          return false;  // Would underflow
        else if ((offset > 0) && ((offset + sf) > UINT8_MAX))
          return false;  // Would overflow

        sf += offset;
        info->sf.scalefactors[g][sfb] = sf;
        //printf("  g %d  hcb 0x%X  sfb %2d  type SF  sfOffset %2d  sf %d\n", g, hcb, sfb, offset, sf);
      }
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
  {
    info->tns.isEnabled = false;
    return true;
  }

  info->tns.isEnabled = true;

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

  // § 14.3
  for (unsigned int w = 0; w < info->ics->windowCount; w++)
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

      info->tns.filters[w][f].sfbCount = length;
      info->tns.filters[w][f].order    = order;

      if (order)
      {
        info->tns.filters[w][f].isDownward = reader->readUInt(1);

        bool compress = reader->readUInt(1);

        unsigned readBits = info->tns.coefficientBits[w] - ((compress) ? 1 : 0);

        for (unsigned int o = 0; o < order; o++)
        {
          int8_t coefficient = reader->readUInt(readBits);
          //printf("TNS coefficient %d readbits %d\n", (int) coefficient, readBits);

          if (coefficient & (1 << (readBits - 1)))
            coefficient = (int8_t) (((uint8_t) coefficient) | ~((1U << readBits) - 1));  // Sign extend

          info->tns.filters[w][f].coefficients[o] = coefficient;
        }
      }
    }
  }

  printf("TNS enabled:\n");
  for (unsigned int w = 0; w < info->ics->windowCount; w++)
  {
    printf("  window %d: %d filters\n", w, info->tns.filterCount[w]);
    for (unsigned int f = 0; f < info->tns.filterCount[w]; f++)
    {
      auto filter = info->tns.filters[w][f];
      printf("    sfbCount %2d  order %2d  downward %s  coefficients ", filter.sfbCount, filter.order, (filter.isDownward ? "true" : "false"));
      for (unsigned int o = 0; o < filter.order; o++)
        printf("%2d  ", filter.coefficients[o]);
      printf("\n");
    }
  }

  return true;
}

// spectral_data()
bool AacDecoder::decodeSpectralData(AacBitReader *reader, AacDecodeInfo *info, double spec[AAC_SPECTRAL_SAMPLE_SIZE_LONG])
{
  auto sd = AacSpectrumDecoder(reader);

  printf("--- FRAME %d ---\n", m_blockCount);

  printf("decodeSpectralData():  windowSequence %s  sfbCount %d\n", AacConstants::getWindowSequenceName(info->ics->windowSequence), info->ics->sfbCount);

  int16_t quant[AAC_SPECTRAL_SAMPLE_SIZE_LONG] = {};  // Quantized spectal values  // TODO: Don't pre-zero. We can zero as we go.
  for (unsigned int g = 0; g < info->ics->windowGroupCount; g++)  // Groups
  {
    printf("- group %d has %d sections\n", g, info->section.windowGroupSections[g].count);

    for (unsigned int s = 0; s < info->section.windowGroupSections[g].count; s++)  // Sections
    {
      // Sections are ranges of one or more scalefactor bands that use the same codebook

      auto codebook = info->section.sfbCodebooks[g][info->section.windowGroupSections[g].sections[s].sfbStart];
      if ((codebook == AAC_HCB_ZERO) || (codebook > AAC_HCB_ESC))
      {
        printf("  group %d  section %d  codebook %2d -- Skipping due to codebook\n", g, s, codebook);
        continue;
      }

      unsigned int sectionSfbStart = info->section.windowGroupSections[g].sections[s].sfbStart;
      unsigned int sectionSfbEnd   = sectionSfbStart + info->section.windowGroupSections[g].sections[s].sfbLength;
      assert(sectionSfbEnd <= info->ics->sfbCount);

      unsigned int sectionSampleStart = info->section.windowGroupSections[g].sections[s].sampleStart;
      unsigned int sectionSampleEnd = sectionSampleStart + info->section.windowGroupSections[g].sections[s].sampleCount;

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
          //printf("    dstIndex %d  sample %d: w %d  x %d  y %d  z %d\n", dstIndex, k, v[0], v[1], v[2], v[3]);
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
          //printf("    dstIndex %d  sample %d: y %d z %d\n", dstIndex, k, v[0], v[1]);
          quant[dstIndex++] = v[0];
          quant[dstIndex++] = v[1];
        }
      }
    }
  }

  // TODO: Max abs(value) of each element of quant is 8191. Should we be saturating them?
  for (unsigned int i = 0; i < 1024; i++)
  {
    //printf("  quant[%d]: %d\n", i, quant[i]);
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
    memset(quant, 0, sizeof(int16_t) * AAC_SPECTRAL_SAMPLE_SIZE_LONG);  // TODO HACK

    unsigned int srcIndex = 0;
    for (unsigned int g = 0; g < info->ics->windowGroupCount; g++)  // Groups
    {
      unsigned int winCount = info->ics->windowGroups[g].winLength;  // Count of windows within group

      for (unsigned int sfb = 0; sfb < info->ics->sfbCount; sfb++)  // Each SFB
      {
        for (unsigned int winOffset = 0; winOffset < winCount; winOffset++)  // Window offset within group
        {
          unsigned int win = info->ics->windowGroups[g].winStart + winOffset;

          unsigned int sfbSampleStart = m_scalefactorBandInfo->shortWindow->offsets[sfb];
          unsigned int sfbSampleCount = m_scalefactorBandInfo->shortWindow->offsets[sfb + 1] - sfbSampleStart;

          unsigned int dstIndex = (win * AAC_SPECTRAL_SAMPLE_SIZE_SHORT) + sfbSampleStart;
          for (unsigned int s = 0; s < sfbSampleCount; s++)
          {
            //printf("Deinterlace: group %d  winCount %d  winOffset %d  win  %d  sfb %d: %d ← %d\n", g, winCount, winOffset, win, sfb, dstIndex, srcIndex);
            quant[dstIndex++] = interlaced[srcIndex++];
          }
        }
      }
    }

    //for (unsigned int i = 0; i < 1024; i++)
    //  printf("  deinterlaced[%d]: %d\n", i, quant[i]);
  }

  // Dequantize
  double dequant[AAC_SPECTRAL_SAMPLE_SIZE_LONG];
  AacAudioTools::dequantize(quant, dequant);

  // Rescale (§ 11.3.3)
  double x_rescal[AAC_SPECTRAL_SAMPLE_SIZE_LONG] = {};
  for (unsigned int g = 0; g < info->ics->windowGroupCount; g++)  // Groups
  {
    unsigned int winCount = info->ics->windowGroups[g].winLength;  // Count of windows within group

    // TODO: Would probably be smarter to iterate over sections, not SFBs
    for (unsigned int sfb = 0; sfb < info->ics->sfbCount; sfb++)
    {
      unsigned int hcb = info->section.sfbCodebooks[g][sfb];
      if ((hcb == AAC_HCB_ZERO) || AAC_IS_INTENSITY_CODEBOOK(hcb))
        continue;  // No scalefactor for this band

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

      // TODO: Since the scalefactors are limited to 8 bits, we could have a LUT for the gain
      double gain = pow(2, 0.25 * (info->sf.scalefactors[g][sfb] - 100));
      //printf("  Rescale group %d  sfb %d  sfbSampleStart %d  sfbSampleCount %d  gain %f\n", g, sfb, sfbSampleStart, sfbSampleCount, gain);

      for (unsigned int winOffset = 0; winOffset < winCount; winOffset++)
      {
        unsigned int win = info->ics->windowGroups[g].winStart + winOffset;

        // NOTE: The win variable should always be 0 for a long window, so this should be safe.
        unsigned int winSampleStart = win * AAC_SPECTRAL_SAMPLE_SIZE_SHORT;

        unsigned int sampleBase = winSampleStart + sfbSampleStart;
        for (unsigned int k = 0; k < sfbSampleCount; k++)
        {
          x_rescal[sampleBase + k] = dequant[sampleBase + k] * gain;
          //printf("    x_rescal[%d] = %f  group %d  sfb %d  dequant %f  gain %f\n", sampleBase + k, x_rescal[sampleBase + k], g, sfb, dequant[sampleBase + k], gain);
        }
      }
    }
  }

  // TODO: Remove
  memcpy(spec, x_rescal, sizeof(double) * AAC_SPECTRAL_SAMPLE_SIZE_LONG);

  return true;
}

AacChannelDecoder *AacDecoder::getSceChannelDecoder(uint8_t instance)
{
  if (auto found = m_sceDecoders.find(instance); found != m_sceDecoders.end())
    return found->second;

  auto cd = new AacChannelDecoder(AAC_CHANNEL_FIRST, m_sampleRateIndex);

  m_sceDecoders[instance] = cd;

  return cd;
}

void AacDecoder::getCpeChannelDecoders(uint8_t instance, AacChannelDecoder *decoders[2])
{
  if (auto found = m_cpeDecoders.find(instance); found != m_cpeDecoders.end())
  {
    decoders[0] = found->second[0];
    decoders[1] = found->second[1];
    return;
  }

  auto left  = new AacChannelDecoder(AAC_CHANNEL_FIRST, m_sampleRateIndex);
  auto right = new AacChannelDecoder(AAC_CHANNEL_SECOND, m_sampleRateIndex);

  auto item = m_cpeDecoders[instance];
  item[0] = left;
  item[1] = right;

  decoders[0] = left;
  decoders[1] = right;
  return;
}

// § 12.1.3 M/S (Main/Side) joint stereo
// In this scheme, the first channel ("left") contains the main audio, and
//  the second channel ("right") contains the delta between the channels.
bool AacDecoder::applyMsJointStereo(const AacDecodeInfo *info, const AacMsMaskInfo *msMask, double leftSpec[AAC_SPECTRAL_SAMPLE_SIZE_LONG], double rightSpec[AAC_SPECTRAL_SAMPLE_SIZE_LONG])
{
  if (msMask->type == AAC_MS_MASK_ZERO)
    return true;  // Nothing to do
  else if (msMask->type == AAC_MS_MASK_RESERVED)
    return false;  // Invalid mask type

  for (unsigned int g = 0; g < info->ics->windowGroupCount; g++)
  {
    unsigned int winCount = info->ics->windowGroups[g].winLength;  // Count of windows within group

    for (unsigned int winOffset = 0; winOffset < winCount; winOffset++)  // Window offset within group
    {
      for (unsigned int sfb = 0; sfb < info->ics->sfbCount; sfb++)  // Each SFB
      {
        auto hcb = info->section.sfbCodebooks[g][sfb];
        if (AAC_IS_INTENSITY_CODEBOOK(hcb))
          continue;  // This SFB uses intensity joint stereo, not M/S joint stereo

        if ((msMask->type == AAC_MS_MASK_SUBBAND) && !((msMask->sfbMask[sfb] >> g) & 0x01))
          continue;  // Joint stereo not enabled for this SFB

        unsigned int win = info->ics->windowGroups[g].winStart + winOffset;

        unsigned int sampleStart, sampleCount;
        if (info->ics->isLongWindow)
        {
          sampleStart = m_scalefactorBandInfo->longWindow->offsets[sfb];
          sampleCount = m_scalefactorBandInfo->longWindow->offsets[sfb + 1] - sampleStart;
        }
        else
        {
          sampleStart = m_scalefactorBandInfo->shortWindow->offsets[sfb];
          sampleCount = m_scalefactorBandInfo->shortWindow->offsets[sfb + 1] - sampleStart;
        }

        // NOTE: The win variable should always be 0 for a long window, so this should be safe.
        sampleStart = (win * AAC_SPECTRAL_SAMPLE_SIZE_SHORT) + sampleStart;

        for (unsigned int s = 0; s < sampleCount; s++)
        {
          double main = leftSpec[sampleStart + s];
          double side = rightSpec[sampleStart + s];
          double left  = main + side;
          double right = main - side;
          leftSpec[sampleStart + s] = left;
          rightSpec[sampleStart + s] = right;
        }
      }
    }
  }

  return true;
}

bool AacDecoder::applyIntensityJointStereo(const AacDecodeInfo *info, const AacMsMaskInfo *msMask, const double leftSpec[AAC_SPECTRAL_SAMPLE_SIZE_LONG], double rightSpec[AAC_SPECTRAL_SAMPLE_SIZE_LONG])
{
  printf("Intensity stereo:\n");
  for (unsigned int g = 0; g < info->ics->windowGroupCount; g++)
  {
    unsigned int winCount = info->ics->windowGroups[g].winLength;  // Count of windows within group

    for (unsigned int winOffset = 0; winOffset < winCount; winOffset++)  // Window offset within group
    {
      for (unsigned int sfb = 0; sfb < info->ics->sfbCount; sfb++)  // Each SFB
      {
        int polarity;

        auto hcb = info->section.sfbCodebooks[g][sfb];
        if (hcb == AAC_HCB_INTENSITY2)
          polarity = -1;
        else if (hcb == AAC_HCB_INTENSITY)
          polarity = 1;
        else
          continue;  // Not an intensity stereo SFB

        if ((msMask->type == AAC_MS_MASK_SUBBAND) && ((msMask->sfbMask[sfb] >> g) & 0x01))
          polarity = -polarity;

        int stereoPosition = info->sf.scalefactors[g][sfb] - AAC_STEREO_POSITION_BIAS;
        double scale = pow(0.5, (0.25 * stereoPosition)) * polarity;

        unsigned int win = info->ics->windowGroups[g].winStart + winOffset;

        unsigned int sampleStart, sampleCount;
        if (info->ics->isLongWindow)
        {
          sampleStart = m_scalefactorBandInfo->longWindow->offsets[sfb];
          sampleCount = m_scalefactorBandInfo->longWindow->offsets[sfb + 1] - sampleStart;
        }
        else
        {
          sampleStart = m_scalefactorBandInfo->shortWindow->offsets[sfb];
          sampleCount = m_scalefactorBandInfo->shortWindow->offsets[sfb + 1] - sampleStart;
        }

        printf("  g %d  win %d  sfb %d  stereoPosition %d  scale %f\n", g, win, sfb, stereoPosition, scale);

        // NOTE: The win variable should always be 0 for a long window, so this should be safe.
        sampleStart = (win * AAC_SPECTRAL_SAMPLE_SIZE_SHORT) + sampleStart;

        for (unsigned int s = 0; s < sampleCount; s++)
          rightSpec[sampleStart + s] = leftSpec[sampleStart + s] * scale;
      }
    }
  }

  return true;
}

bool AacDecoder::decodeBlock(AacBitReader *reader, AacAudioBlock *audio)
{
  bool done = false;

  while (!done && !reader->isComplete())
  {
    AacElementId id = static_cast<AacElementId>(reader->readUInt(3));
    printf("Element ID: 0x%X\n", id);

    switch (id)
    {
    case AAC_ID_END:
      done = true;
      break;
    case AAC_ID_FIL:  // Fill element
      if (!decodeElementFIL(reader))
        return false;
      break;
    case AAC_ID_SCE:  // Single channel element
      if (!decodeElementSCE(reader, audio))
        return false;
      break;
    case AAC_ID_CPE:  // Channel pair element
      if (!decodeElementCPE(reader, audio))
        return false;
      break;
    case AAC_ID_PCE:  // Program config element
      if (!decodeElementPCE(reader))
        return false;
      break;
    default:
      printf("Unhandled element ID %X (%s - %s)\n", id, AacConstants::getElementNameShort(id), AacConstants::getElementNameLong(id));
      abort();
    }
  }

  m_blockCount++;

  reader->alignToBit(0);

  return true;
}

// Program config element
bool AacDecoder::decodeElementPCE(AacBitReader *reader)
{
  AacProgramConfigInfo pce;

  if (!readProgramConfigInfo(reader, &pce))
    return false;

  printf("--- PCE ---\n");
  printf("instance       : %d\n", pce.instance);
  printf("profile        : %d\n", pce.profile);
  printf("sampleRateIndex: %d\n", pce.sampleRateIndex);
  printf("front channels : %d\n", pce.frontChannelElementCount);
  printf("side channels  : %d\n", pce.sideChannelElementCount);
  printf("rear channels  : %d\n", pce.rearChannelElementCount);
  printf("comment        : %s\n", pce.comment.c_str());

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

  AacDecodeInfo info;
  info.ics     = &ics;

  info.identifier = reader->readUInt(4);
  auto channelDecoder = getSceChannelDecoder(info.identifier);

  info.globalGain = reader->readUInt(8);

  if (!decodeIcsInfo(reader, &ics))
    return false;

  if (!decodeSectionInfo(reader, &info))
    return false;

  if (!decodeScalefactorInfo(reader, &info))
    return false;

  if (!decodePulseInfo(reader, &info))
    return false;

  if (!decodeTnsInfo(reader, &info))
    return false;

  bool hasGainControl = reader->readUInt(1);
  if (hasGainControl)
    return false;  // Not allowed in LC profile

  printf("-- SCE --\n");
  dumpInfo(&info);

  int16_t *buf;
  audio->prepare(m_sampleRate, AAC_MONO_CHANNEL_COUNT);
  audio->getSampleBuffer(&buf);

  double spec[AAC_SPECTRAL_SAMPLE_SIZE_LONG];  // Spectal samples
  if (!decodeSpectralData(reader, &info, spec))
    return false;

  if (!channelDecoder->decodeAudio(reader, &info, spec, buf, AAC_MONO_CHANNEL_COUNT))
    return false;

  return true;
}

// Channel pair element
bool AacDecoder::decodeElementCPE(AacBitReader *reader, AacAudioBlock *audio)
{
  AacChannelDecoder *channelDecoders[AAC_STEREO_CHANNEL_COUNT];

  AacIcsInfo ics[AAC_STEREO_CHANNEL_COUNT];

  AacDecodeInfo info[AAC_STEREO_CHANNEL_COUNT];

  AacMsMaskInfo msMaskInfo;

  unsigned int identifier = reader->readUInt(4);
  getCpeChannelDecoders(identifier, channelDecoders);

  bool commonWindow = reader->readUInt(1);

  if (commonWindow)
  {
    if (!decodeIcsInfo(reader, &ics[0]))
      return false;

    if (!decodeMsMaskInfo(reader, &ics[0], &msMaskInfo))
      return false;
  }

  double spec[AAC_STEREO_CHANNEL_COUNT][AAC_SPECTRAL_SAMPLE_SIZE_LONG];  // Spectal samples

  int16_t *buf;
  audio->prepare(m_sampleRate, AAC_STEREO_CHANNEL_COUNT);
  audio->getSampleBuffer(&buf);

  // Read per-channel settings
  for (unsigned int ch = 0; ch < AAC_STEREO_CHANNEL_COUNT; ch++)
  {
    info[ch].identifier = identifier;
    info[ch].globalGain = reader->readUInt(8);

    if (commonWindow)
      info[ch].ics = &ics[0];
    else
    {
      if (!decodeIcsInfo(reader, &ics[ch]))
        return false;

      info[ch].ics = &ics[ch];
    }

    if (!decodeSectionInfo(reader, &info[ch]))
      return false;

    if (!decodeScalefactorInfo(reader, &info[ch]))
      return false;

    if (!decodePulseInfo(reader, &info[ch]))
      return false;

    if (!decodeTnsInfo(reader, &info[ch]))
      return false;

    bool hasGainControl = reader->readUInt(1);
    if (hasGainControl)
      return false;  // Not allowed in LC profile

    if (!decodeSpectralData(reader, &info[ch], spec[ch]))
      return false;
  }

  printf("commonWindow      : %s\n", commonWindow ? "true" : "false");

  printf("-- CPE left --\n");
  dumpInfo(&info[0]);

  printf("-- CPE right --\n");
  dumpInfo(&info[1]);

  // Joint stereo
  if (commonWindow)
  {
    // M/S (main/side) joint stereo
    if (msMaskInfo.type != AAC_MS_MASK_ZERO)
    {
      if (!applyMsJointStereo(&info[1], &msMaskInfo, spec[0], spec[1]))
        return false;
    }

    // Intensity stereo
    if (!applyIntensityJointStereo(&info[1], &msMaskInfo, spec[0], spec[1]))
      return false;
  }

  // Decode audio
  for (unsigned int ch = 0; ch < AAC_STEREO_CHANNEL_COUNT; ch++)
  {
    if (!channelDecoders[ch]->decodeAudio(reader, &info[ch], spec[ch], buf + ch, AAC_STEREO_CHANNEL_COUNT))
      return false;
  }

  return true;
}

void AacDecoder::dumpInfo(AacDecodeInfo *info)
{
  printf("identifier        : %d\n", info->identifier);
  printf("global gain       : %d\n", info->globalGain);
  printf("window seq        : %d (%s)\n", info->ics->windowSequence, AacConstants::getWindowSequenceName(info->ics->windowSequence));
  printf("window shape      : %d (%s)\n", info->ics->windowShape, AacConstants::getWindowShapeName(info->ics->windowShape));
  printf("scalefactor bands : %d\n", info->ics->sfbCount);
  printf("samples per window: %d\n", info->ics->samplesPerWindow);
  printf("window count      : %d\n", info->ics->windowCount);
  printf("pulse count       : %d\n", info->pulse.pulseCount);
  printf("TNS enabled       : %s\n", info->tns.isEnabled ? "true" : "false");
}
