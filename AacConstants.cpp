#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include <array>

#include "AacConstants.h"

namespace AacConstants
{
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

  unsigned int getSampleRateByIndex(AacSampleRateIndex index)
  {
    if (index > std::size(sampleRateMap))
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

};
