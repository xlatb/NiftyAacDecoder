#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "AacConstants.h"

constexpr double dequantizePower = 4.0 / 3.0;

namespace AacAudioTools
{
  // Dequantize (§ 10.3)
  void dequantize(const int16_t quant[AAC_SPECTRAL_SAMPLE_SIZE_LONG], double dequant[AAC_SPECTRAL_SAMPLE_SIZE_LONG])
  {
    for (unsigned int s = 0; s < AAC_SPECTRAL_SAMPLE_SIZE_LONG; s++)
    {
      // This is really just raising the value to the power of (4/3) but
      //  also preserving the sign of negative values.
      dequant[s] = pow(abs(quant[s]), dequantizePower) * ((quant[s] > 0) ? 1.0 : -1.0);
    }
  }

  // IMDCT for long windows
  void IMDCTLong(const double coefficients[AAC_SPECTRAL_SAMPLE_SIZE_LONG], double samples[AAC_XFORM_WIN_SIZE_LONG])
  {
    constexpr double n0 = ((AAC_XFORM_WIN_SIZE_LONG / 2.0) + 1.0) / 2.0;

    for (unsigned int s = 0; s < AAC_XFORM_WIN_SIZE_LONG; s++)  // Audio samples
    {
      double sum = 0.0;
      for (unsigned int k = 0; k < AAC_SPECTRAL_SAMPLE_SIZE_LONG; k++)  // Spectral coefficients
      {
        double v = coefficients[k] * cos(((M_PI * 2.0) / AAC_XFORM_WIN_SIZE_LONG) * (s + n0) * (k + 0.5));
        sum += v;
      }

      //sum /= 2.0; // TODO: TEST - check output level
      double sample = (2.0 / AAC_SPECTRAL_SAMPLE_SIZE_LONG) * sum;  // TODO: AAC_XFORM_WIN_SIZE_LONG?
      samples[s] = sample;
      printf("  samples[%d] = %.3f  sum %.3f\n", s, sample, sum);
    }

  }

  // IMDCT for short windows
  void IMDCTShort(const double coefficients[AAC_SPECTRAL_SAMPLE_SIZE_SHORT], double samples[AAC_XFORM_WIN_SIZE_SHORT])
  {
    constexpr double n0 = ((AAC_XFORM_WIN_SIZE_SHORT / 2.0) + 1.0) / 2.0;

    for (unsigned int s = 0; s < AAC_XFORM_WIN_SIZE_SHORT; s++)  // Audio samples
    {
      double sum = 0.0;
      for (unsigned int k = 0; k < AAC_SPECTRAL_SAMPLE_SIZE_SHORT; k++)  // Spectral coefficients
      {
        double v = coefficients[k] * cos(((M_PI * 2.0) / AAC_XFORM_WIN_SIZE_SHORT) * (s + n0) * (k + 0.5));
        sum += v;
      }

      //sum /= 2.0; // TODO: TEST - check output level
      double sample = (2.0 / AAC_SPECTRAL_SAMPLE_SIZE_SHORT) * sum;  // TODO: AAC_XFORM_WIN_SIZE_LONG?
      samples[s] = sample;
      printf("  samples[%d] = %.3f  sum %.3f\n", s, sample, sum);
    }

  }

  void window(const double window[], double samples[], unsigned int count)
  {
    for (unsigned int s = 0; s < count; s++)
      samples[s] *= window[s];
  }
};
