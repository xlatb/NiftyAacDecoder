#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#ifndef AAC_ADTS_FRAME_READER_H
#define AAC_ADTS_FRAME_READER_H

class AacAdtsFrame;
class AacAdtsFrameHeader;

class AacAdtsFrameReader
{
  const uint8_t *m_bytes;
  size_t         m_size;

  size_t         m_position;

public:
  AacAdtsFrameReader(const uint8_t *bytes, size_t size) : m_bytes(bytes), m_size(size), m_position(0) {};

  size_t getPosition(void) { return m_position; };

  bool   advance(size_t count) { if (m_position + count > m_size) { m_position = m_size; return false; } m_position += count; return true; };

  bool   isAtFrameHeader(void);
  bool   isComplete(void) { return m_position >= m_size; };

  bool   readFrameHeader(AacAdtsFrameHeader *header);
  bool   readFrame(AacAdtsFrame *frame);

  size_t findNextFrame(void);

  size_t skipID3(void);

  size_t getRemainingSize(void) { if (m_position >= m_size) return 0; return m_size - m_position; };
};

#endif
