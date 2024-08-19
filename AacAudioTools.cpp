#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "AacConstants.h"

#define restrict __restrict

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

  void window(const double *restrict window, double *restrict samples, unsigned int count)
  {
    for (unsigned int s = 0; s < count; s++)
      samples[s] *= window[s];
  }

  void transformTnsCoefficients(const int8_t quant[], double lpc[], unsigned int bitCount, unsigned int order)
  {
    double dequant[AAC_MAX_TNS_ORDER_LONG_MAIN + 1];  // Dequantized TNS coefficients
    double b[AAC_MAX_TNS_ORDER_LONG_MAIN + 1];

    // Inverse quantization
    double iqfac   = ((1 << (bitCount - 1)) - 0.5) / (M_PI / 2.0);
    double iqfac_m = ((1 << (bitCount - 1)) + 0.5) / (M_PI / 2.0);
    for (unsigned int o = 0; o < order; o++)
    {
      dequant[o] = sin(quant[o] / ((quant[o] >= 0) ? iqfac : iqfac_m));
      printf("  TNS: coef[%d] %d  dequant %f\n", o, quant[o], dequant[o]);
    }

    // Conversion to LPC
    // The standard is not very forthcoming about what is happening here. It
    //  only provides pseudocode for this transformation.
    lpc[0] = 1.0;
    for (unsigned int o = 1; o <= order; o++)
    {
      for (unsigned int i = 1; i < o; i++)
        b[i] = lpc[i] + (dequant[o - 1] * lpc[o - i]);

      for (unsigned int i = 1; i < o; i++)
        lpc[i] = b[i];

      lpc[o] = dequant[o - 1];
    }

    // NOTE: We end up with 1 more LPC coefficient than our 'order'
    for (unsigned int o = 0; o <= order; o++)
     printf("  TNS: lpc[%d] %f\n", o, lpc[o]);
  }

  void tnsFilterUpwards(double *coefficients, unsigned int sampleCount, unsigned int order, const double lpc[])
  {
    assert(order > 0);
    assert(order < AAC_MAX_TNS_ORDER_LONG_MAIN);

    // ISO 13818-7 is pretty terse about this process. It says:
    // - Simple all-pole filter of order “order” defined by
    //   y(n) = x(n) - lpc[1]*y(n-1) - ... - lpc[order]*y(n-order)
    // - The output data is written over the input data (“in-place operation”)
    //
    // My understanding is:
    // 1. x() is the input samples and y() is the output samples.
    // 2. The "order" is the number of filter components. Each one has an
    //  associated coefficient (here stored in lpc[1] through lpc[order]).
    // 3. The term "all-pole" means this is purely a feedback filter. There
    //  are no feedforward components.
    // 4. The presence of feedback components means the filter is an IIR
    //  (infinite impulse response) filter.
    // 5. lpc[0] is always 1.0 so we can skip that multiplication.
    // 6. We could rewrite the above as a summation:
    //                order
    //  y(n) = x(n) -   Σ   lpc[i] * y(n - i)
    //                i = 1

    for (unsigned int n = 0; n < sampleCount; n++)
    {
      double y = coefficients[n];
      //printf("TNS: starting sample: n %d  y %f\n", n, y);

      for (unsigned int i = 1; (i <= order) && (i <= n); i++)
      {
        //printf("  prior sample: i %d  n %d  lpc[%d] %f  sample[%d] %f  product %f\n", i, n - i, i, lpc[i], n - i, coefficients[n - i], lpc[i] * coefficients[n - i]);
        y -= lpc[i] * coefficients[n - i];
      }

      //printf("  final y %f\n", y);

      coefficients[n] = y;
    }
  }

  void tnsFilterDownwards(double *coefficients, unsigned int sampleCount, unsigned int order, const double lpc[])
  {
    assert(order > 0);
    assert(order < AAC_MAX_TNS_ORDER_LONG_MAIN);

    // See comments at tnsFilterUpwards(). This version just runs through the
    //  samples from high to low.
    // We use '(coefficients - n)[0]' below rather than 'coefficients[-n]' so
    //  that n can remain unsigned.

    for (unsigned int n = 0; n < sampleCount; n++)
    {
      double y = (coefficients - n)[0];
      //printf("TNS: starting sample: n %d  y %f\n", n, y);

      for (unsigned int i = 1; (i <= order) && (i <= n); i++)
      {
        //printf("  prior sample: i %d  n %d  lpc[%d] %f  sample[%d] %f  product %f\n", i, n + i, i, lpc[i], n + i, (coefficients - n + i)[0], lpc[i] * (coefficients - n + i)[0]);
        y -= lpc[i] * (coefficients - n + i)[0];
      }

      //printf("  final y %f\n", y);

      (coefficients - n)[0] = y;
    }

  }

};

