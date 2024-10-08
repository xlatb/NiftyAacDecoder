#include <endian.h>

#include <bit>

#include "AacAudioBlock.h"

void AacAudioBlock::alloc(void)
{
  size_t neededSize = sizeof(int16_t) * AAC_AUDIO_BLOCK_SAMPLE_COUNT * m_channelCount;
  if (m_size >= neededSize)
    return;  // Sample buffer is already large enough

  if (m_samples != NULL)
  {
    delete [] m_samples;
    m_samples = NULL;
  }

  unsigned int elementCount = AAC_AUDIO_BLOCK_SAMPLE_COUNT * m_channelCount;
  m_samples = new int16_t[elementCount];
  m_size = sizeof(int16_t) * elementCount;
}

void AacAudioBlock::prepare(unsigned int sampleRate, unsigned int channelCount, std::endian endianness)
{
  m_sampleRate = sampleRate;

  // If the channel count changes, we may need to reallocate the sample buffer
  if (m_channelCount != channelCount)
  {
    m_channelCount = channelCount;
    alloc();
  }

  m_endianness = endianness;
}

size_t AacAudioBlock::getSampleBuffer(int16_t **buf)
{
  *buf = m_samples;

  // NOTE: Internal sample buffer may be oversized, so we don't return m_size
  return sizeof(int16_t) * AAC_AUDIO_BLOCK_SAMPLE_COUNT * m_channelCount;
}

void AacAudioBlock::switchEndianness(std::endian e)
{
  if (m_endianness == e)
    return;

  if ((e != std::endian::little) && (e != std::endian::big))
    abort();  // We don't support mixed endianness

  // Swap the bytes in each 16-bit sample
  // NOTE: We use std::bit_cast because the result of shifting negative
  //  integers is undefined.
  for (unsigned int s = 0; s < AAC_AUDIO_BLOCK_SAMPLE_COUNT * m_channelCount; s++)
    m_samples[s] = (std::bit_cast<uint16_t>(m_samples[s]) >> 8) | ((std::bit_cast<uint16_t>(m_samples[s]) & 0x00FF) << 8);

  m_endianness = e;
}
