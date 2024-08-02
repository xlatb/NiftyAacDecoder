#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "AacConstants.h"

#define restrict __restrict

// Naïve implementation of DCT-II. Very slow!
static void dct_ii_naive(const double *restrict input, double *restrict output, const unsigned int N)
{
  double f = M_PI / N;

  for (unsigned int k = 0; k < N; k++)
  {
    double sum = 0.0;

    for (unsigned int n = 0; n < N; n++)
    {
      double v = input[n] * cos(f * (n + 0.5) * k);
      sum += v;
    }

    output[k] = sum;
  }
}

// Naïve implementation of DCT-IV. Very slow!
static void dct_iv_naive(const double *restrict input, double *restrict output, const unsigned int N)
{
  double f = M_PI / N;

  for (unsigned int k = 0; k < N; k++)
  {
    double sum = 0.0;

    for (unsigned int n = 0; n < N; n++)
    {
      double v = input[n] * cos(f * (n + 0.5) * (k + 0.5));
      sum += v;
    }

    output[k] = sum;
  }
}

// Naïve implementation of IMDCT. Very slow!
// The output is twice the length of the input.
static void imdct_naive(const double *restrict input, double *restrict output, unsigned int inputCount)
{
  unsigned int outputCount = inputCount << 1;

  double n0 = ((outputCount / 2.0) + 1.0) / 2.0;

  for (unsigned int s = 0; s < outputCount; s++)  // Audio samples
  {
    double sum = 0.0;
    for (unsigned int k = 0; k < inputCount; k++)  // Spectral coefficients
    {
      double v = input[k] * cos(((M_PI * 2.0) / outputCount) * (s + n0) * (k + 0.5));
      sum += v;
    }

    double sample = (2.0 / outputCount) * sum;
    output[s] = sample;
  }

  return;
}

// This faster IMDCT implementation works as follows:
//
// • IMDCT with N inputs gives back 2N outputs, but there is redundancy in
//   the output. We can instead perform a DCT-IV of length N, and derive
//   the extra IMDCT outputs via mirroring and negation.
// • The DCT-IV of length N can be implemented in terms of a single DCT-II
//   of length N, with some pre-processing and post-processing.
// • DCT-II of length N can be recursively broken down into two DCT-IIs of
//   length N/2. When N reaches 2, the two-point DCT is easy to calculate.

static void dct_ii_zhijin(const double *restrict input, double *restrict output, const unsigned int N);

// Recursive DCT-II.
// Based on "Recursive Algorithms for Discrete Cosine Transform" by Zhijin
//  & Huisheng.
// N must be a power of two because we're subdividing the problem into halves
//  at each step, and the base case is length 2.
static void dct_ii_zhijin(const double *restrict input, double *restrict output, const unsigned int N)
{
  if (N == 2)
  {
    // Trivial case, two-point DCT
    output[0] = input[0] + input[1];
    output[1] = sqrt(0.5) * (input[0] - input[1]);
    return;
  }

  assert(((N - 1) & N) == 0);  // Must be a power of two

  const unsigned int halfN = N >> 1;

  // Generate g[] and h[]
  double g[halfN];
  double h[halfN];
  for (unsigned int n = 0; n < halfN; n++)
  {
    g[n] = input[n] + input[N - n - 1];
    h[n] = input[n] - input[N - n - 1];
  }

  // Calculate G[] from g[], which gives us the even indices
  double G[halfN];
  dct_ii_zhijin(g, G, halfN);

  // Copy even results to output
  for (unsigned int k = 0; k < halfN; k++)
    output[k * 2] = G[k];

  // generate b[]
  double b[halfN];
  for (unsigned int n = 0; n < halfN; n++)
  {
    b[n] = h[n] * 2.0 * cos((M_PI / (2 * N)) * (2 * n + 1));
  }

  // Calculate B[] from b[], which gives us the odd indices
  double B[halfN];
  dct_ii_zhijin(b, B, halfN);

  // Copy odd results to output
  output[1] = B[0] / 2.0;
  for (unsigned int k = 1; k < halfN; k++)
    output[k * 2 + 1] = B[k] - output[k * 2 - 1];
}

// Performs DCT-IV based on an underlying DCT-II.
// This technique is from "A unified computing kernel for MDCT/IMDCT in
//  modern audio coding standards" by Tan Li, R. Zhang, R. Yang, Heyun
//  Huang, and Fuhuei Lin.
static void dct_iv_via_dct_ii(const double *restrict input, double *restrict output, const unsigned int N)
{
  // Transform input for DCT-II
  double input2[N];
  for (unsigned int n = 0; n < N; n++)
    input2[n] = 2.0 * cos((M_PI * (2 * n + 1)) / (4 * N)) * input[n];

  // Run the DCT-II
  double output2[N];
  dct_ii_zhijin(input2, output2, N);

  // Transform output for DCT-IV
  output[0] = output2[0] * 0.5;
  for (unsigned int n = 1; n < N; n++)
    output[n] = output2[n] - output[n - 1];
}

// Perform IMDCT based on DCT-IV.
// The output is twice the length of the input.
static void imdctViaDctIV(const double *restrict input, double *restrict output, unsigned int inputCount)
{
  const unsigned int outputCount = inputCount << 1;

  // Quarter output counts
  const unsigned int q1 = outputCount >> 2;
  const unsigned int q2 = outputCount >> 1;
  const unsigned int q3 = q1 + q2;

  // TODO: Fixed size?
  double dct[inputCount];

  dct_iv_via_dct_ii(input, dct, inputCount);

  // Use first quarter of DCT-IV to derive last quarter of IMDCT
  for (unsigned int n = 0; n < q1; n++)
    output[q3 + n] = -dct[n];

  // Use second quarter of DCT-IV to derive final quarter of IMDCT
  for (unsigned int n = q1; n < q2; n++)
    output[n - q1] = dct[n];

  // Second quarter - First quarter mirrored and negated
  for (unsigned int n = 0; n < q1; n++)
    output[q1 + n] = -output[q1 - 1 - n];

  // Third quarter - Fourth quarter mirrored
  for (unsigned int n = 0; n < q1; n++)
    output[q3 - n - 1] = output[q3 + n];

  // Rescale outputs
  const double scaleFactor = 1.0 / inputCount;
  for (unsigned int n = 0; n < outputCount; n++)
    output[n] = output[n] * scaleFactor;
}

// IMDCT for long windows
void AacImdctLong(const double coefficients[AAC_SPECTRAL_SAMPLE_SIZE_LONG], double samples[AAC_XFORM_WIN_SIZE_LONG])
{
  imdctViaDctIV(coefficients, samples, AAC_SPECTRAL_SAMPLE_SIZE_LONG);
}

// IMDCT for short windows
void AacImdctShort(const double coefficients[AAC_SPECTRAL_SAMPLE_SIZE_SHORT], double samples[AAC_XFORM_WIN_SIZE_SHORT])
{
  imdctViaDctIV(coefficients, samples, AAC_SPECTRAL_SAMPLE_SIZE_SHORT);
}
