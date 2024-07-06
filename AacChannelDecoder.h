#include "AacConstants.h"

#ifndef AAC_CHANNEL_DECODER_H
#define AAC_CHANNEL_DECODER_H

enum AacChannelOrdinal
{
  AAC_CHANNEL_FIRST,   // Solo channel or first (left) channel of a stereo pair
  AAC_CHANNEL_SECOND,  // Second (right) channel of a stereo pair
};

class AacBitReader;

class AacChannelDecoder
{
  AacChannelOrdinal m_ordinal;

  AacSampleRateIndex m_sampleRateIndex;

  const AacScalefactorBandInfo *m_scalefactorBandInfo;

  // The right-hand set of audio samples from the previous block, for blending
  //  with the following block.
  double m_oldSamples[AAC_AUDIO_SAMPLE_OUTPUT_COUNT];

  // The window shape of the previous block.
  AacWindowShape m_previousWindowShape;

  unsigned int m_blockCount;

  bool applyTnsLongWindow(double coefficients[AAC_SPECTRAL_SAMPLE_SIZE_LONG], const AacDecodeInfo *info);
  bool applyTnsShortWindow(double coefficients[AAC_SPECTRAL_SAMPLE_SIZE_LONG], const AacDecodeInfo *info);

  bool decodeAudioLongWindow(AacBitReader *reader, const AacDecodeInfo *info, double spec[AAC_SPECTRAL_SAMPLE_SIZE_LONG], int16_t *audio, size_t audioStride);
  bool decodeAudioShortWindow(AacBitReader *reader, const AacDecodeInfo *info, double spec[AAC_SPECTRAL_SAMPLE_SIZE_LONG], int16_t *audio, size_t audioStride);

public:
  AacChannelDecoder(AacChannelOrdinal ordinal, AacSampleRateIndex sampleRateIndex);

  void reset(void);

  bool decodeAudio(AacBitReader *reader, const AacDecodeInfo *info, double spec[AAC_SPECTRAL_SAMPLE_SIZE_LONG], int16_t *audio, size_t audioStride);
};

#endif
