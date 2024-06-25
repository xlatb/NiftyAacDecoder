#include "AacConstants.h"

#ifndef AAC_AUDIO_TOOLS_H
#define AAC_AUDIO_TOOLS_H

namespace AacAudioTools
{
  extern void dequantize(int16_t quant[AAC_SPECTRAL_SAMPLE_SIZE_LONG], double dequant[AAC_SPECTRAL_SAMPLE_SIZE_LONG]);
};

#endif
