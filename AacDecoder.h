#include <stdint.h>

#ifndef AAC_DECODER_H
#define AAC_DECODER_H

class AacBitReader;

struct AacIcsInfo;
struct AacSectionInfo;
struct AacDecodeInfo;

class AacDecoder
{
public:
  AacDecoder(void) {};

  bool decodeIcsInfo(AacBitReader *reader, AacIcsInfo *info);
  bool decodeSectionInfo(AacBitReader *reader, const AacIcsInfo *info, AacSectionInfo *sect);
  bool decodeScalefactorInfo(AacBitReader *reader, AacDecodeInfo *info);
  bool decodePulseInfo(AacBitReader *reader, AacDecodeInfo *info);
  bool decodeTnsInfo(AacBitReader *reader, AacDecodeInfo *info);
  bool decodeSpectralData(AacBitReader *reader, AacDecodeInfo *info);

  bool decodeBlock(AacBitReader *reader);

  bool decodeElementFIL(AacBitReader *reader);
  bool decodeElementSCE(AacBitReader *reader);
  bool decodeElementCPE(AacBitReader *reader);
};

#endif
