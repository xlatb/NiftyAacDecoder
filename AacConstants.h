#ifndef AAC_CONSTANTS_H
#define AAC_CONSTANTS_H

enum
{
  AAC_HCB_ZERO       = 0,   // ZERO_HCB
  AAC_HCB_FIRST_PAIR = 5,   // FIRST_PAIR_HCB
  AAC_HCB_ESC        = 11,  // ESC_HCB
  AAC_HCB_INTENSITY2 = 14,  // INTENSITY_HCB2
  AAC_HCB_INTENSITY  = 15,  // INTENSITY_HCB
};

enum AacSampleRateIndex : unsigned int
{
  AAC_SAMPLE_RATE_96000 = 0,
  AAC_SAMPLE_RATE_88200 = 1,
  AAC_SAMPLE_RATE_64000 = 2,
  AAC_SAMPLE_RATE_48000 = 3,
  AAC_SAMPLE_RATE_44100 = 4,
  AAC_SAMPLE_RATE_32000 = 5,
  AAC_SAMPLE_RATE_24000 = 6,
  AAC_SAMPLE_RATE_22050 = 7,
  AAC_SAMPLE_RATE_16000 = 8,
  AAC_SAMPLE_RATE_12000 = 9,
  AAC_SAMPLE_RATE_11025 = 10,
  AAC_SAMPLE_RATE_8000  = 11,
};

namespace AacConstants
{
  extern unsigned int       getSampleRateByIndex(AacSampleRateIndex index);
  extern AacSampleRateIndex getIndexBySampleRate(unsigned int sampleRate);
};

#endif
