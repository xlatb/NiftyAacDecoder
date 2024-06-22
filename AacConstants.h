#include <stdint.h>

#ifndef AAC_CONSTANTS_H
#define AAC_CONSTANTS_H

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

// Special Huffman codebook indices
enum
{
  AAC_HCB_ZERO       = 0,   // ZERO_HCB
  AAC_HCB_FIRST_PAIR = 5,   // FIRST_PAIR_HCB
  AAC_HCB_ESC        = 11,  // ESC_HCB
  AAC_HCB_INTENSITY2 = 14,  // INTENSITY_HCB2
  AAC_HCB_INTENSITY  = 15,  // INTENSITY_HCB
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
  AAC_WINSHAPE_SIN = 0x0,
  AAC_WINSHAPE_KBD = 0x1,  // Kaiser-Bessel derived
};

// NOTE: We add one extra element to the end of offsets[] with the total
//  transform length (1024 for long windows and 128 for short windows).
//  This allows us to easily find the width of any swb by subtracting
//  the current offset from the following offset.
struct AacScalefactorBandOffsets
{
  unsigned int swbCount;     // Number of scalefactor window bands (swb)
  uint16_t     offsets[];    // The swb offsets (TODO: Rename to swbOffsets?)
};

struct AacScalefactorBandInfo
{
  const AacScalefactorBandOffsets *shortWindow;
  const AacScalefactorBandOffsets *longWindow;
};

namespace AacConstants
{
  extern unsigned int       getSampleRateByIndex(AacSampleRateIndex index);
  extern AacSampleRateIndex getIndexBySampleRate(unsigned int sampleRate);

  extern const AacScalefactorBandInfo *getScalefactorBandInfo(AacSampleRateIndex index);

  extern const char *getWindowSequenceName(AacWindowSequence sequence);

  extern const char *getWindowShapeName(AacWindowShape shape);
};

#endif
