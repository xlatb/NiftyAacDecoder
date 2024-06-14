#include "AacAdtsFrame.h"

AacAdtsFrame::AacAdtsFrame(void)
{
  m_reader = AacBitReader(NULL, 0);
}

void AacAdtsFrame::setHeader(const AacAdtsFrameHeader *header)
{
  m_header = *header;  // Copy underlying object, not pointer

  m_reader = AacBitReader(m_header.getPayloadBytes(), m_header.getPayloadSize());
}
