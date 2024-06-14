#include <stdint.h>

#ifndef AAC_DECODER_H
#define AAC_DECODER_H

class AacBitReader;

struct AacIcsInfo;
struct AacSectionInfo;
struct AacScalefactorInfo;

class AacDecoder
{
public:
  AacDecoder(void) {};

  bool decodeIcsInfo(AacBitReader *reader, AacIcsInfo *info);
  bool decodeSectionInfo(AacBitReader *reader, const AacIcsInfo *info, AacSectionInfo *sect);
  bool decodeScalefactorInfo(AacBitReader *reader, const AacSectionInfo *sect, AacScalefactorInfo *sf);

  bool decodeBlock(AacBitReader *reader);

  bool decodeElementFIL(AacBitReader *reader);
  bool decodeElementSCE(AacBitReader *reader);
  bool decodeElementCPE(AacBitReader *reader);
};

#endif
