#include "AacConstants.h"

#ifndef AAC_AUDIO_TOOLS_H
#define AAC_AUDIO_TOOLS_H

namespace AacAudioTools
{
  extern void dequantize(int16_t quant[AAC_SPECTRAL_SAMPLE_SIZE_LONG], double dequant[AAC_SPECTRAL_SAMPLE_SIZE_LONG]);

  extern void IMDCTLong(double coefficients[AAC_SPECTRAL_SAMPLE_SIZE_LONG], double samples[AAC_XFORM_WIN_SIZE_LONG]);
  extern void IMDCTShort(double coefficients[AAC_SPECTRAL_SAMPLE_SIZE_SHORT], double samples[AAC_XFORM_WIN_SIZE_SHORT]);
};

#endif
