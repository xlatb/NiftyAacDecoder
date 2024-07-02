#include "AacConstants.h"
#include "AacStructs.h"

#ifndef AAC_AUDIO_TOOLS_H
#define AAC_AUDIO_TOOLS_H

namespace AacAudioTools
{
  extern void dequantize(const int16_t quant[AAC_SPECTRAL_SAMPLE_SIZE_LONG], double dequant[AAC_SPECTRAL_SAMPLE_SIZE_LONG]);

  extern void IMDCTLong(const double coefficients[AAC_SPECTRAL_SAMPLE_SIZE_LONG], double samples[AAC_XFORM_WIN_SIZE_LONG]);
  extern void IMDCTShort(const double coefficients[AAC_SPECTRAL_SAMPLE_SIZE_SHORT], double samples[AAC_XFORM_WIN_SIZE_SHORT]);

  extern void window(const double window[], double samples[], unsigned int count);

  extern void tnsFilterUpwards(double *coefficients, unsigned int sampleCount, unsigned int order, const double lpc[]);
  extern void tnsFilterDownwards(double *coefficients, unsigned int sampleCount, unsigned int order, const double lpc[]);
};

#endif
