#include <math.h>

#include "AacConstants.h"

constexpr double dequantizePower = 4.0 / 3.0;

namespace AacAudioTools
{
  // Dequantize (§ 10.3)
  void dequantize(int16_t quant[AAC_SPECTRAL_SAMPLE_SIZE_LONG], double dequant[AAC_SPECTRAL_SAMPLE_SIZE_LONG])
  {
    for (unsigned int s = 0; s < AAC_SPECTRAL_SAMPLE_SIZE_LONG; s++)
    {
      // This is really just raising the value to the power of (4/3) but
      //  also preserving the sign of negative values.
      dequant[s] = pow(abs(quant[s]), dequantizePower) * ((quant[s] > 0) ? 1.0 : -1.0);
    }
  }
};
