#include <string.h>

#include "AacAdtsFrameHeader.h"
#include "AacAdtsFrame.h"

#include "AacAdtsFrameReader.h"

#define ID3_HEADER_SIZE 10

bool AacAdtsFrameReader::isAtFrameHeader(void)
{
  if ((m_position + AAC_ADTS_FRAME_HEADER_SIZE) >= m_size)
    return false;  // Not enough room for a frame header

  return AacAdtsFrameHeader::isFrameHeader(m_bytes + m_position);
}

bool AacAdtsFrameReader::readFrameHeader(AacAdtsFrameHeader *header)
{
  if (!isAtFrameHeader())
    return false;

  header->setBytes(m_bytes + m_position);
  return true;
}

bool AacAdtsFrameReader::readFrame(AacAdtsFrame *frame)
{
  if (!isAtFrameHeader())
    return false;

  auto header = AacAdtsFrameHeader(m_bytes + m_position);

  if ((m_position + header.getFrameSize()) > m_size)
    return false;  // Not enough space left for entire frame

  frame->setHeader(&header);
  return true;
}

size_t AacAdtsFrameReader::findNextFrame(void)
{
  size_t remainingSize = getRemainingSize();
  if (remainingSize == 0)
    return 0;  // EOF

  size_t initialPosition = m_position;

  while (remainingSize >= 1)
  {
    // Start looking from the next byte in the stream
    m_position++;
    remainingSize--;

    // Scan for first byte of syncword
    const uint8_t *start = m_bytes + m_position;
    const uint8_t *found = static_cast<const uint8_t *>(memchr(start, 0xFF, remainingSize));
    if (found == NULL)
    {
      m_position = m_size;
      break;  // Not found
    }

    // Move forward to new position
    size_t skipCount = found - start;
    m_position += skipCount;
    remainingSize -= skipCount;

    // Is it large enough for a frame header?
    if (remainingSize < AAC_ADTS_FRAME_HEADER_SIZE)
    {
      m_position = m_size;
      break;  // Not enough remaining space
    }

    // Check if it looks like a frame header
    if (AacAdtsFrameHeader::isFrameHeader(found))
      break;  // Found it
  }

  return m_position - initialPosition;
}

// Attempt to skip an ID3v2 header. These are normally at the start of the
//  file.
// The earlier ID3v1 header was placed at the end of the file, and is less
//  likely to contain false positive syncwords.
// https://mutagen-specs.readthedocs.io/en/latest/id3/id3v2.3.0.html
size_t AacAdtsFrameReader::skipID3(void)
{
  if (getRemainingSize() < ID3_HEADER_SIZE)
    return 0;  // Not enough space for ID3v2 header

  if (memcmp(m_bytes + m_position, "ID3", 3))
    return 0;  // Signature mismatch

  if ((m_bytes[m_position + 3] > 0x09) || (m_bytes[m_position + 4] > 0x09))
    return 0;  // Unreasonably-high version number components

  for (int i = 6; i < ID3_HEADER_SIZE; i++)
    if (m_bytes[m_position + i] & 0x80)
      return 0;  // Size bytes not allowed to have high bits set

  // The size is a four-byte big-endian number where the high bit of each byte
  //  is ignored, giving us a 28-bit result. The stored size does not include
  //  the size of the header.
  size_t size = ID3_HEADER_SIZE;
  size += m_bytes[m_position + 6] << 21;
  size += m_bytes[m_position + 7] << 14;
  size += m_bytes[m_position + 8] << 7;
  size += m_bytes[m_position + 9];

  advance(size);
  return size;
}
