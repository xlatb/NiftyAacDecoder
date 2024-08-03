#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "WavWriter.h"

#define WAV_HEADER_SIZE 44

bool WavWriter::open(const char *filename, unsigned int channelCount, unsigned int bitsPerSample, unsigned int sampleRate)
{
  if (m_file)
    close();

  m_file = fopen(filename, "w");
  if (!m_file)
    return false;

  m_channelCount = channelCount;
  m_bitsPerSample = bitsPerSample;
  m_sampleRate = sampleRate;

  if (!writeHeader())
    return false;

  m_valid = true;
  m_bytesWritten = 0;
  return true;
}

void WavWriter::close(void)
{
  if (m_file && m_valid)
    writeLength();

  if (m_file)
  {
    fclose(m_file);
    m_file = NULL;
  }

  m_channelCount = 0;
  m_bitsPerSample = 0;
  m_sampleRate = 0;

  m_valid = false;
  m_bytesWritten = 0;
}

bool WavWriter::writeHeader(void)
{
  uint8_t header[WAV_HEADER_SIZE] = {};

  memcpy(header + 0,  "RIFF", 4);
  memcpy(header + 8,  "WAVE", 4);

  // Start fmt chunk
  memcpy(header + 12, "fmt ", 4);

  header[16] = 16;  // Length of fmt payload
  header[20] = 1;  // PCM

  // Channel count
  header[22] = m_channelCount & 0xFF;
  header[23] = m_channelCount >> 8;

  // Sample rate
  header[24] = m_sampleRate & 0xFF;
  header[25] = (m_sampleRate >> 8) & 0xFF;
  header[26] = (m_sampleRate >> 16) & 0xFF;
  header[27] = (m_sampleRate >> 24) & 0xFF;

  // Bytes per second
  unsigned int bytesPerSecond = (m_sampleRate * m_bitsPerSample * m_channelCount) >> 3;
  header[28] = bytesPerSecond & 0xFF;
  header[29] = (bytesPerSecond >> 8) & 0xFF;
  header[30] = (bytesPerSecond >> 16) & 0xFF;
  header[31] = (bytesPerSecond >> 24) & 0xFF;

  // Bytes per sampling interval
  unsigned int bytesPerSamplingInterval = (m_bitsPerSample * m_channelCount) >> 3;
  header[32] = bytesPerSamplingInterval & 0xFF;
  header[33] = (bytesPerSamplingInterval >> 8) & 0xFF;

  // Bits per sample
  unsigned int bitsPerSample = 16;
  header[34] = bitsPerSample & 0xFF;
  header[35] = (bitsPerSample >> 8) & 0xFF;

  // Start data chunk
  memcpy(header + 36, "data", 4);

  size_t count = fwrite(header, 1, WAV_HEADER_SIZE, m_file);
  if (count != WAV_HEADER_SIZE)
  {
    close();
    return false;
  }

  return true;
}

bool WavWriter::writeLength(void)
{
  if (!(m_file && m_valid))
    return false;

  if (fseek(m_file, WAV_HEADER_SIZE - 4, SEEK_SET))
  {
    m_valid = false;
    return false;
  }

  uint8_t size[4];
  size_t count;

  size[0] = m_bytesWritten & 0xFF;
  size[1] = (m_bytesWritten >> 8) & 0xFF;
  size[2] = (m_bytesWritten >> 16) & 0xFF;
  size[3] = (m_bytesWritten >> 24) & 0xFF;

  count = fwrite(size, 1, 4, m_file);
  if (count != 4)
  {
    m_valid = false;
    return false;
  }

  if (fseek(m_file, 4, SEEK_SET))
  {
    m_valid = false;
    return false;
  }

  unsigned int riffSize = m_bytesWritten + WAV_HEADER_SIZE - 8;
  size[0] = riffSize & 0xFF;
  size[1] = (riffSize >> 8) & 0xFF;
  size[2] = (riffSize >> 16) & 0xFF;
  size[3] = (riffSize >> 24) & 0xFF;

  count = fwrite(size, 1, 4, m_file);
  if (count != 4)
  {
    m_valid = false;
    return false;
  }

  return true;
}

bool WavWriter::write(uint8_t *samples, size_t size)
{
  assert(m_valid);

  size_t count = fwrite(samples, 1, size, m_file);
  if (count != size)
  {
    close();
    return false;
  }

  m_bytesWritten += size;
  assert(m_bytesWritten < INT_MAX);

  return true;
}
