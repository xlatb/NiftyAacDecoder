#include <stdlib.h>
#include <stdint.h>

#ifndef AAC_AUDIO_BLOCK_H
#define AAC_AUDIO_BLOCK_H

constexpr unsigned int AAC_AUDIO_BLOCK_SAMPLE_COUNT = 1024;

class AacAudioBlock
{
  unsigned int m_sampleRate;
  unsigned int m_channelCount;

  int16_t     *m_samples;
  size_t       m_size;

  void alloc(void);

public:
  AacAudioBlock(void) : m_sampleRate(0), m_channelCount(0), m_samples(NULL), m_size(0) {};

  void           prepare(unsigned int sampleRate, unsigned int channelCount);

  size_t         getSampleBuffer(int16_t **buf);

  unsigned int   getSampleRate(void) { return m_sampleRate; };
  unsigned int   getChannelCount(void) { return m_channelCount; };
  unsigned int   getSampleCount(void) { return m_channelCount * AAC_AUDIO_BLOCK_SAMPLE_COUNT; };
  const int16_t *getSamples(void) { return m_samples; };
};

#endif
