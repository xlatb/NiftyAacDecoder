#include <stdio.h>
#include <stdbool.h>

#ifndef WAV_WRITER_H
#define WAV_WRITER_H

class WavWriter
{
  FILE        *m_file = NULL;
  bool         m_valid = false;

  unsigned int m_channelCount = 0;
  unsigned int m_bitsPerSample = 0;
  unsigned int m_sampleRate = 0;

  unsigned int m_bytesWritten = 0;

  bool writeHeader(void);

  bool writeLength(void);

public:
  bool open(const char *filename, unsigned int channelCount, unsigned int bitsPerSample, unsigned int sampleRate);
  void close(void);

  bool write(uint8_t *samples, size_t size);

  bool isOpen(void) { return (m_file && m_valid); };
};

#endif
