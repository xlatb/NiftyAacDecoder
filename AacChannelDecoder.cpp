#include <string.h>

#include "AacChannelDecoder.h"

AacChannelDecoder::AacChannelDecoder(void)
{
  reset();
}

void AacChannelDecoder::reset(void)
{
  memset(m_oldSamples, 0, sizeof(m_oldSamples[0]) * AAC_AUDIO_SAMPLE_OUTPUT_COUNT);

  m_blockCount = 0;
}
