#include "AacConstants.h"

#ifndef AAC_CHANNEL_DECODER_H
#define AAC_CHANNEL_DECODER_H

class AacChannelDecoder
{
  // The right-hand set of audio samples from the previous block, for blending
  //  with the following block.
  double m_oldSamples[AAC_AUDIO_SAMPLE_OUTPUT_COUNT];

  // The window shape of the previous block.
  AacWindowShape m_previousWindowShape;

  unsigned int m_blockCount;

public:
  AacChannelDecoder(void);

  void reset(void);
};

#endif
