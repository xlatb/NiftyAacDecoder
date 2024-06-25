#include <stdint.h>

#include "AacConstants.h"

#ifndef AAC_DECODER_H
#define AAC_DECODER_H

class AacBitReader;
class AacAudioBlock;

struct AacIcsInfo;
struct AacSectionInfo;
struct AacDecodeInfo;

class AacDecoder
{
  unsigned int       m_sampleRate;
  AacSampleRateIndex m_sampleRateIndex;

  const AacScalefactorBandInfo *m_scalefactorBandInfo;

  unsigned int m_blockCount;

  AacWindowShape m_previousWindowShape;

  bool decodeIcsInfo(AacBitReader *reader, AacIcsInfo *info);
  bool decodeSectionInfo(AacBitReader *reader, const AacIcsInfo *info, AacSectionInfo *sect);
  bool decodeScalefactorInfo(AacBitReader *reader, AacDecodeInfo *info);
  bool decodePulseInfo(AacBitReader *reader, AacDecodeInfo *info);
  bool decodeTnsInfo(AacBitReader *reader, AacDecodeInfo *info);
  bool decodeSpectralData(AacBitReader *reader, AacDecodeInfo *info, int16_t quant[]);

  bool decodeAudioLongWindow(AacBitReader *reader, AacDecodeInfo *info, AacAudioBlock *audio);
  bool decodeAudioShortWindow(AacBitReader *reader, AacDecodeInfo *info, AacAudioBlock *audio);

  bool decodeElementFIL(AacBitReader *reader);
  bool decodeElementSCE(AacBitReader *reader, AacAudioBlock *audio);
  bool decodeElementCPE(AacBitReader *reader, AacAudioBlock *audio);

public:
  AacDecoder(unsigned int sampleRate);

  bool decodeBlock(AacBitReader *reader, AacAudioBlock *audio);

  unsigned int getSampleRate(void) { return m_sampleRate; };
};

#endif
