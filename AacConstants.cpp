#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include <array>

#include "AacConstants.h"

namespace AacConstants
{
  // Table 45
  static const AacScalefactorBandOffsets sfb_long_44100_48000 =
  {
    .swbCount = 49,
    .offsets = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72, 80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292, 320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736, 768, 800, 832, 864, 896, 928, 1024},
  };

  // Table 46
  static const AacScalefactorBandOffsets sfb_short_32000_44100_48000 =
  {
    .swbCount = 14,
    .offsets = {0, 4, 8, 12, 16, 20, 28, 36, 44, 56, 68, 80, 96, 112, 128},
  };

  // Table 47
  static const AacScalefactorBandOffsets sfb_long_32000 =
  {
    .swbCount = 51,
    .offsets = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 48, 56, 64, 72, 80, 88, 96, 108, 120, 132, 144, 160, 176, 196, 216, 240, 264, 292, 320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736, 768, 800, 832, 864, 896, 928, 960, 992, 1024},
  };

  // Table 48
  static const AacScalefactorBandOffsets sfb_long_8000 =
  {
    .swbCount = 40,
    .offsets = {0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 172, 188, 204, 220, 236, 252, 268, 288, 308, 328, 348, 372, 396, 420, 448, 476, 508, 544, 580, 620, 664, 712, 764, 820, 880, 944, 1024},
  };

  // Table 49
  static const AacScalefactorBandOffsets sfb_short_8000 =
  {
    .swbCount = 15,
    .offsets = {0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 60, 72, 88, 108, 128},
  };

  // Table 50
  static const AacScalefactorBandOffsets sfb_long_11025_12000_16000 =
  {
    .swbCount = 43,
    .offsets = {0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 100, 112, 124, 136, 148, 160, 172, 184, 196, 212, 228, 244, 260, 280, 300, 320, 344, 368, 396, 424, 456, 492, 532, 572, 616, 664, 716, 772, 832, 896, 960, 1024},
  };

  // Table 51
  static const AacScalefactorBandOffsets sfb_short_11025_12000_16000 =
  {
    .swbCount = 15,
    .offsets = {0, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 60, 72, 88, 108, 128},
  };

  // Table 52
  static const AacScalefactorBandOffsets sfb_long_22050_24000 =
  {
    .swbCount = 47,
    .offsets = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 52, 60, 68, 76, 84, 92, 100, 108, 116, 124, 136, 148, 160, 172, 188, 204, 220, 240, 260, 284, 308, 336, 364, 396, 432, 468, 508, 552, 600, 652, 704, 768, 832, 896, 960, 1024},
  };

  // Table 53
  static const AacScalefactorBandOffsets sfb_short_22050_24000 =
  {
    .swbCount = 15,
    .offsets = {0, 4, 8, 12, 16, 20, 24, 28, 36, 44, 52, 64, 76, 92, 108, 128},
  };

  // Table 54
  static const AacScalefactorBandOffsets sfb_long_64000 =
  {
    .swbCount = 47,
    .offsets = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 64, 72, 80, 88, 100, 112, 124, 140, 156, 172, 192, 216, 240, 268, 304, 344, 384, 424, 464, 504, 544, 584, 624, 664, 704, 744, 784, 824, 864, 904, 944, 984, 1024},
  };

  // Table 55
  static const AacScalefactorBandOffsets sfb_short_64000 =
  {
    .swbCount = 12,
    .offsets = {0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 128},
  };

  // Table 56
  static const AacScalefactorBandOffsets sfb_long_88200_96000 =
  {
    .swbCount = 41,
    .offsets = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 64, 72, 80, 88, 96, 108, 120, 132, 144, 156, 172, 188, 212, 240, 276, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960, 1024},
  };

  // Table 57
  static const AacScalefactorBandOffsets sfb_short_88200_96000 =
  {
    .swbCount = 12,
    .offsets = {0, 4, 8, 12, 16, 20, 24, 32, 40, 48, 64, 92, 128},
  };

  static const AacScalefactorBandInfo scalefactorBandMap[] =
  {
    {&sfb_short_88200_96000,       &sfb_long_88200_96000},        // 96000
    {&sfb_short_88200_96000,       &sfb_long_88200_96000},        // 82000
    {&sfb_short_64000,             &sfb_long_64000},              // 64000
    {&sfb_short_32000_44100_48000, &sfb_long_44100_48000},        // 48000

    {&sfb_short_32000_44100_48000, &sfb_long_44100_48000},        // 44100
    {&sfb_short_32000_44100_48000, &sfb_long_32000},              // 32000
    {&sfb_short_22050_24000,       &sfb_long_22050_24000},        // 24000
    {&sfb_short_22050_24000,       &sfb_long_22050_24000},        // 22050

    {&sfb_short_11025_12000_16000, &sfb_long_11025_12000_16000},  // 16000
    {&sfb_short_11025_12000_16000, &sfb_long_11025_12000_16000},  // 12000
    {&sfb_short_11025_12000_16000, &sfb_long_11025_12000_16000},  // 11025
    {&sfb_short_8000,              &sfb_long_8000},               // 8000
  };

  // Table 35
  static const unsigned int sampleRateMap[] =
  {
    96000,  // 0
    88200,
    64000,
    48000,

    44100,  // 4
    32000,
    24000,
    22050,

    16000,  // 8
    12000,
    11025,
    8000,
  };

  // Arbitrary sample rates between the standard ones are allowed, and will
  //  be binned into a nearby sample rate index.
  // This table facilitates a binary search for the correct bin.
  // Min/max values are from Table 38.
  static const struct { unsigned int min; unsigned int max; AacSampleRateIndex index; } sampleRateIndexSearch[] =
  {
    {    0,     9390, AAC_SAMPLE_RATE_8000 },  // 11
    { 9391,    11501, AAC_SAMPLE_RATE_11025},  // 10
    {11502,    13855, AAC_SAMPLE_RATE_12000},  // 9
    {13856,    18782, AAC_SAMPLE_RATE_16000},  // 8
    {18783,    23003, AAC_SAMPLE_RATE_22050},  // 7
    {23004,    27712, AAC_SAMPLE_RATE_24000},  // 6
    {27713,    37565, AAC_SAMPLE_RATE_32000},  // 5
    {37566,    46008, AAC_SAMPLE_RATE_44100},  // 4
    {46009,    55425, AAC_SAMPLE_RATE_48000},  // 3
    {55426,    75131, AAC_SAMPLE_RATE_64000},  // 2
    {75132,    92016, AAC_SAMPLE_RATE_88200},  // 1
    {92017, UINT_MAX, AAC_SAMPLE_RATE_96000},  // 0
  };

  // Table 44
  static const char *windowSequenceNames[] =
  {
    "AAC_WINSEQ_LONG",        // 0x0
    "AAC_WINSEQ_LONG_START",  // 0x1
    "AAC_WINSEQ_8_SHORT",     // 0x2
    "AAC_WINSEQ_LONG_STOP",   // 0x3
  };

  static const char *windowShapeNames[] =
  {
    "AAC_WINSHAPE_SIN",  // 0x0
    "AAC_WINSHAPE_KBD",  // 0x1
  };

  unsigned int getSampleRateByIndex(AacSampleRateIndex index)
  {
    if (index >= std::size(sampleRateMap))
      return 0;  // Out of range

    return sampleRateMap[index];
  }

  // Given a sample rate in Hz, returns the index value.
  AacSampleRateIndex getIndexBySampleRate(unsigned int sampleRate)
  {
    unsigned int size = std::size(sampleRateIndexSearch);
    unsigned int jump = (size >> 1) + (size % 2);
    unsigned int i = jump;

    while (i < size)
    {
      auto sr = sampleRateIndexSearch[i];
      jump = (jump >> 1) + (jump % 2);

      //printf("Sample rate search for %u  index %u  jump %u  bin.min %u  bin.max %u  bin %u\n", sampleRate, i, jump, sr.min, sr.max, sampleRateMap[sr.index]);
      if (sampleRate >= sr.min)
      {
        if (sampleRate <= sr.max)
          return sr.index;
        else
          i += jump;
      }
      else
        i -= jump;
    }

    abort();  // Not reached
  }

  const AacScalefactorBandInfo *getScalefactorBandInfo(AacSampleRateIndex index)
  {
    if (index >= std::size(sampleRateMap))
      return NULL;  // Out of range

    return &scalefactorBandMap[index];
  }

  const char *getWindowSequenceName(AacWindowSequence sequence)
  {
    if (sequence >= std::size(windowSequenceNames))
      return NULL;  // Out of range

    return windowSequenceNames[sequence];
  }

  const char *getWindowShapeName(AacWindowShape shape)
  {
    if (shape >= std::size(windowShapeNames))
      return NULL;  // Out of range

    return windowShapeNames[shape];
  }

};
