#include <stdint.h>

#include <unordered_map>

#include "AacConstants.h"

#ifndef AAC_DECODER_H
#define AAC_DECODER_H

class AacBitReader;
class AacAudioBlock;
class AacChannelDecoder;

struct AacIcsInfo;
struct AacMsMaskInfo;
struct AacSectionInfo;
struct AacTnsFilter;
struct AacDecodeInfo;

class AacDecoder
{
  unsigned int       m_sampleRate;
  AacSampleRateIndex m_sampleRateIndex;

  const AacScalefactorBandInfo *m_scalefactorBandInfo;  // TODO: Remove?

  unsigned int m_blockCount;

  AacWindowShape m_previousWindowShape;

  // Channel decoders
  std::unordered_map<uint8_t, AacChannelDecoder *>    m_sceDecoders;
  std::unordered_map<uint8_t, AacChannelDecoder *[2]> m_cpeDecoders;

  bool decodeIcsInfo(AacBitReader *reader, AacIcsInfo *info);
  bool decodeMsMaskInfo(AacBitReader *reader, const AacIcsInfo *ics, AacMsMaskInfo *msMask);
  bool decodeSectionInfo(AacBitReader *reader, AacDecodeInfo *info);
  bool decodeScalefactorInfo(AacBitReader *reader, AacDecodeInfo *info);
  bool decodePulseInfo(AacBitReader *reader, AacDecodeInfo *info);
  bool decodeTnsInfo(AacBitReader *reader, AacDecodeInfo *info);

  bool decodeSpectralData(AacBitReader *reader, AacDecodeInfo *info, double spec[AAC_SPECTRAL_SAMPLE_SIZE_LONG]);

  AacChannelDecoder *getSceChannelDecoder(uint8_t instance);
  void               getCpeChannelDecoders(uint8_t instance, AacChannelDecoder *decoders[2]);

  bool applyMsJointStereo(const AacDecodeInfo *info, const AacMsMaskInfo *msMask, double leftSpec[AAC_SPECTRAL_SAMPLE_SIZE_LONG], double rightSpec[AAC_SPECTRAL_SAMPLE_SIZE_LONG]);


  bool decodeElementFIL(AacBitReader *reader);
  bool decodeElementSCE(AacBitReader *reader, AacAudioBlock *audio);
  bool decodeElementCPE(AacBitReader *reader, AacAudioBlock *audio);

  void dumpInfo(AacDecodeInfo *info);

public:
  AacDecoder(unsigned int sampleRate);
  // TODO: Destructor

  bool decodeBlock(AacBitReader *reader, AacAudioBlock *audio);

  unsigned int getSampleRate(void) { return m_sampleRate; };
};

#endif
