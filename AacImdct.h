#ifndef AAC_IMDCT_H
#define AAC_IMDCT_H

void AacImdctLong(const double coefficients[AAC_SPECTRAL_SAMPLE_SIZE_LONG], double samples[AAC_XFORM_WIN_SIZE_LONG]);
void AacImdctShort(const double coefficients[AAC_SPECTRAL_SAMPLE_SIZE_SHORT], double samples[AAC_XFORM_WIN_SIZE_SHORT]);

#endif
