#include <stdint.h>
#include <stdio.h>

#include <algorithm>

#include "AacBitReader.h"

static uint8_t bitmasks[] = {0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF};

unsigned int AacBitReader::readUInt(unsigned int bitCount)
{
  unsigned int readCount = 0;
  unsigned int v = 0;

  while (readCount < bitCount)
  {
    if (isComplete())
      return 0;  // EOF

    unsigned int bitsLeftInByte = 8 - m_bit;
    unsigned int bitsLeftToRead = bitCount - readCount;
    unsigned int bitsToRead = std::min(bitsLeftInByte, bitsLeftToRead);

    unsigned int readShift = bitsLeftInByte - bitsToRead;
    unsigned int writeShift = bitCount - readCount - bitsToRead;

    uint8_t bitmask = bitmasks[bitsToRead] >> m_bit;
    uint8_t bits = (m_bytes[m_position] & bitmask) >> readShift;

    v |= bits << writeShift;
    readCount += bitsToRead;

    m_bit += bitsToRead;
    if (m_bit > 7)
    {
      m_bit = 0;
      m_position++;
    }
  }

  return v;
}

void AacBitReader::skipBits(unsigned int count)
{
  // Skip forward by bytes
  unsigned int byteCount = count >> 3;
  m_position += byteCount;

  // Skip forward by bits
  m_bit = m_bit + (count & 0x07);
  if (m_bit >= 8)
  {
    m_bit = m_bit % 8;
    m_position++;
  }

  if (m_position > m_size)
    m_position = m_size;

  return;
}

void AacBitReader::dumpPosition(void)
{
  printf("AacBitReader position %zu (0x%zX) bit %d\n", m_position, m_position, m_bit);
}
