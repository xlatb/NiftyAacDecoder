#ifndef AAC_SPECTRUM_DECODER_H
#define AAC_SPECTRUM_DECODER_H

class AacBitReader;

class AacSpectrumDecoder
{
  AacBitReader *m_reader;

public:
  AacSpectrumDecoder(AacBitReader *reader) : m_reader(reader) {};

  bool decode2(unsigned int tableNum, int out[2]);
  bool decode4(unsigned int tableNum, int out[4]);

  unsigned int decodeEscape(void);
};

#endif
