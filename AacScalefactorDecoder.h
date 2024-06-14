#include <stdbool.h>

#ifndef AAC_SCALE_FACTOR_DECODER_H
#define AAC_SCALE_FACTOR_DECODER_H

class AacBitReader;

class AacScalefactorDecoder
{
  AacBitReader *m_reader;

public:
  AacScalefactorDecoder(AacBitReader *reader) : m_reader(reader) {};

  bool decode(int *scaleFactor);
};

#endif
