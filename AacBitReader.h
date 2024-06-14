#include <stdint.h>
#include <stdlib.h>

#ifndef AAC_BIT_READER_H
#define AAC_BIT_READER_H

class AacBitReader
{
  const uint8_t *m_bytes;
  size_t         m_size;

  size_t         m_position;
  unsigned int   m_bit;  // Bit position counting from the left (0..7)

public:
  AacBitReader(void) : m_bytes(NULL), m_size(0), m_position(0), m_bit(0) {};
  AacBitReader(const uint8_t *bytes, size_t size) : m_bytes(bytes), m_size(size), m_position(0), m_bit(0) {};

  bool         isComplete(void) { return m_position >= m_size; };

  unsigned int readBit(void) { if (isComplete()) return 0; uint8_t byte = m_bytes[m_position]; unsigned int v = (byte >> m_bit) & 0x01; if (++m_bit > 7) { m_bit = 0; m_position++; }; return v; };

  unsigned int readUInt(unsigned int bitCount);

  void         dumpPosition(void);
};

#endif
