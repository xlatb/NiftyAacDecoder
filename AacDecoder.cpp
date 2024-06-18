#include <assert.h>
#include <stdio.h>

#include "AacBitReader.h"
#include "AacScalefactorDecoder.h"

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
  AAC_HCB_ZERO       = 0,   // ZERO_HCB
  AAC_HCB_FIRST_PAIR = 5,   // FIRST_PAIR_HCB
  AAC_HCB_ESC        = 11,  // ESC_HCB
  AAC_HCB_INTENSITY2 = 14,  // INTENSITY_HCB2
  AAC_HCB_INTENSITY  = 15,  // INTENSITY_HCB
};

enum
{
  AAC_TNS_FILTER_FLAG_DOWNWARD = 0x01,
  AAC_TNS_FILTER_FLAG_COMPRESS = 0x02,
};

struct scalefactorBandInfo
{
  unsigned int bandCount;
  uint16_t     offsets[];
};

// Table 45
scalefactorBandInfo sfb_long_44100_48000 =
{
  .bandCount = 49,
  .offsets = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72, 80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292, 320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736, 768, 800, 832, 864, 896, 928},
};

// Table 46
scalefactorBandInfo sfb_short_32000_44100_48000 =
{
  .bandCount = 14,
  .offsets = {0, 4, 8, 12, 16, 20, 28, 36, 44, 56, 68, 80, 96, 112},
};

// Table 47
scalefactorBandInfo sfb_long_32000 =
{
  .bandCount = 51,
  .offsets = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72, 80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292, 320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736, 768, 800, 832, 864, 896, 928, 960, 992},
};

// Table 48
scalefactorBandInfo sfb_long_8000 =
{
  .bandCount = 40,
  .offsets = {0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 172, 188, 204, 220, 236, 252, 268, 288, 308, 328, 348, 372, 396, 420, 448, 476, 508, 544, 580, 620, 664, 712, 764, 820, 880, 944},
};

// Table 49
scalefactorBandInfo sfb_short_8000 =
{
  .bandCount = 15,
  .offsets = {0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 60, 72, 88, 108},
};

// Table 50
scalefactorBandInfo sfb_long_12000_16000 =
{
  .bandCount = 43,
  .offsets = {0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 100, 112, 124, 136, 148, 160, 172, 184, 196, 212, 228, 244, 260, 280, 300, 320, 344, 368, 396, 424, 456, 492, 532, 572, 616, 664, 716, 772, 832, 896, 960},
};

// Table 51
scalefactorBandInfo sfb_short_12000_16000 =
{
  .bandCount = 15,
  .offsets = {0, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 60, 72, 88, 108},
};

// Table 52
scalefactorBandInfo sfb_long_22050_24000 =
{
  .bandCount = 47,
  .offsets = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 52, 60, 68, 76, 84, 92, 100, 108, 116, 124, 136, 148, 160, 172, 188, 204, 220, 240, 260, 284, 308, 336, 364, 396, 432, 468, 508, 552, 600, 652, 704, 768, 832, 896, 960},
};

// Table 53
scalefactorBandInfo sfb_short_22050_24000 =
{
  .bandCount = 15,
  .offsets = {0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64, 76, 92, 108},
};

// Table 54
scalefactorBandInfo sfb_long_64000 =
{
  .bandCount = 47,
  .offsets = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 64, 72, 80, 88, 100, 112, 124, 140, 156, 172, 192, 216, 240, 268, 304, 344, 384, 424, 464, 504, 544, 584, 624, 664, 704, 744, 784, 824, 864, 904, 944, 984},
};

// Table 55
scalefactorBandInfo sfb_short_64000 =
{
  .bandCount = 12,
  .offsets = {0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92},
};

// Table 56
scalefactorBandInfo sfb_long_88200_96000 =
{
  .bandCount = 41,
  .offsets = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 64, 72, 80, 88, 96, 108, 120, 132, 144, 156, 172, 188, 212, 240, 276, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960},
};

// Table 57
scalefactorBandInfo sfb_short_88200_96000 =
{
  .bandCount = 12,
  .offsets = {0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92},
};

// Table 44
enum AacWindowSequence
{
  AAC_WINSEQ_LONG       = 0x0,
  AAC_WINSEQ_LONG_START = 0x1,
  AAC_WINSEQ_8_SHORT    = 0x2,
  AAC_WINSEQ_LONG_STOP  = 0x3,
};

enum AacWindowShape
{
  AAC_WINSHAPE_SINE = 0x0,
  AAC_WINSHAPE_KB   = 0x1,  // Kaiser-Bessel
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

  uint8_t      sfbCodebooks[AAC_MAX_WINDOW_GROUPS][AAC_MAX_SFB_COUNT];

  // TODO: Substruct
  uint8_t      sectionCounts[AAC_MAX_WINDOW_GROUPS];  // For each group, the number of subband "sections"
  uint8_t      sectionStarts[AAC_MAX_WINDOW_GROUPS][AAC_MAX_SFB_COUNT];  // For each group, for each section, the starting scalefilter band index
  uint8_t      sectionLengths[AAC_MAX_WINDOW_GROUPS][AAC_MAX_SFB_COUNT];  // For each group, for each section, the number of scalefilter bands
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
      printf("Section %d  codebook 0x%X  start %d  length %d\n", s, codebook, k, len);

      // Copy the codebook to each band as determined by the length
      for (unsigned int sfb = k; sfb < k + len; sfb++)
        sect->sfbCodebooks[g][sfb] = codebook;

      // Remember the extent of this section
      sect->sectionStarts[g][s] = static_cast<uint8_t>(k);
      sect->sectionLengths[g][s] = static_cast<uint8_t>(len);

      k += len;
      s++;
    }

    sect->sectionCounts[g] = s;
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
  for (unsigned int g = 0; g < info->section->windowGroupCount; g++)
  {
    for (unsigned int s = 0; s < info->section->sectionCounts[g]; s++)
    {
      auto codebook = info->section->sfbCodebooks[g][info->section->sectionStarts[g][s]];

      printf("group %d  section %d  codebook %d\n", g, s, codebook);
    }
  }

  abort();
}

bool AacDecoder::decodeBlock(AacBitReader *reader)
{
  while (!reader->isComplete())
  {
    unsigned int id = reader->readUInt(3);
    printf("Element ID: 0x%X\n", id);

    switch (id)
    {
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
  while (count)
  {
    //unsigned int extensionType = reader->readUInt(4);
    //printf("  extension type 0x%X\n", extensionType);
    reader->readUInt(8);
    count--;
  }

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

  abort();
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

