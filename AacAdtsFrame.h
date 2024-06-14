#include <stdlib.h>

#include "AacAdtsFrameHeader.h"
#include "AacBitReader.h"

#ifndef AAC_ADTS_FRAME_H
#define AAC_ADTS_FRAME_H

class AacAdtsFrame
{
  AacAdtsFrameHeader m_header;

  AacBitReader       m_reader;

public:
  AacAdtsFrame(void);

  void                      setHeader(const AacAdtsFrameHeader *header);
  const AacAdtsFrameHeader *getHeader(void) const { return &m_header; };

  size_t                    getSize(void) { return m_header.getFrameSize(); };

  AacBitReader             *getReader(void) { return &m_reader; };
};

#endif
