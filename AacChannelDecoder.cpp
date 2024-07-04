#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "AacStructs.h"
#include "AacWindows.h"
#include "AacAudioTools.h"

#include "AacChannelDecoder.h"

AacChannelDecoder::AacChannelDecoder(AacChannelOrdinal ordinal, AacSampleRateIndex sampleRateIndex)
{
  m_ordinal = ordinal;
  m_sampleRateIndex = sampleRateIndex;

  m_scalefactorBandInfo = AacConstants::getScalefactorBandInfo(m_sampleRateIndex);

  reset();
}

void AacChannelDecoder::reset(void)
{
  memset(m_oldSamples, 0, sizeof(m_oldSamples[0]) * AAC_AUDIO_SAMPLE_OUTPUT_COUNT);

  m_blockCount = 0;
}

bool AacChannelDecoder::applyTnsLongWindow(double coefficients[AAC_SPECTRAL_SAMPLE_SIZE_LONG], const AacDecodeInfo *info)
{
  printf("TNS for long window...\n");

  unsigned int w = 0;  // TODO

  // NOTE: We start counting the filter's bands from here, even if this
  //  block's sfbCount was lower.
  unsigned int sfbEnd = m_scalefactorBandInfo->longWindow->swbCount;

  for (unsigned int f = 0; f < info->tns.filterCount[w]; f++)
  {
    const auto &filter = info->tns.filters[w][f];

    unsigned int tnsMaxBand = AacConstants::getLongWindowTnsMaxBandByIndex(m_sampleRateIndex);

    unsigned int sfbStart = (filter.sfbCount > sfbEnd) ? 0 : sfbEnd - filter.sfbCount;

    unsigned int sampleStart = m_scalefactorBandInfo->longWindow->offsets[std::min(tnsMaxBand, std::min(sfbStart, info->ics->sfbCount))];
    unsigned int sampleEnd   = m_scalefactorBandInfo->longWindow->offsets[std::min(tnsMaxBand, std::min(sfbEnd, info->ics->sfbCount))];
    unsigned int sampleCount = sampleEnd - sampleStart;

    printf("  filter %d  isDownward %s  sfbStart %d  sfbEnd %d  tnsMaxBand %d  sampleStart %d  sampleEnd %d  sampleCount %d\n", f, (filter.isDownward ? "true" : "false"), sfbStart, sfbEnd, tnsMaxBand, sampleStart, sampleEnd, sampleCount);

    if (sampleCount == 0)
      continue;  // No work to do

    double lpc[AAC_MAX_TNS_ORDER_LONG_MAIN + 1];  // "Linear prediction coding" coefficients
    assert(filter.order <= AAC_MAX_TNS_ORDER_LONG_MAIN);
    AacAudioTools::transformTnsCoefficients(filter.coefficients, lpc, info->tns.coefficientBits[w], filter.order);

    if (filter.isDownward)
      AacAudioTools::tnsFilterDownwards(coefficients + sampleEnd - 1, sampleCount, filter.order, lpc);
    else
      AacAudioTools::tnsFilterUpwards(coefficients + sampleStart, sampleCount, filter.order, lpc);

    sfbEnd = sfbStart;
  }

  return true;
}

bool AacChannelDecoder::applyTnsShortWindow(double coefficients[AAC_SPECTRAL_SAMPLE_SIZE_SHORT], const AacDecodeInfo *info)
{
  printf("TNS for short window...\n");

  for (unsigned int w = 0; w < info->ics->windowCount; w++)
  {
    // NOTE: We start counting the filter's bands from here, even if this
    //  block's sfbCount was lower.
    unsigned int sfbEnd = m_scalefactorBandInfo->shortWindow->swbCount;

    for (unsigned int f = 0; f < info->tns.filterCount[w]; f++)
    {
      const auto &filter = info->tns.filters[w][f];

      unsigned int tnsMaxBand = AacConstants::getShortWindowTnsMaxBandByIndex(m_sampleRateIndex);

      unsigned int sfbStart = (filter.sfbCount > sfbEnd) ? 0 : sfbEnd - filter.sfbCount;

      unsigned int sampleStart = m_scalefactorBandInfo->shortWindow->offsets[std::min(tnsMaxBand, std::min(sfbStart, info->ics->sfbCount))];
      unsigned int sampleEnd   = m_scalefactorBandInfo->shortWindow->offsets[std::min(tnsMaxBand, std::min(sfbEnd, info->ics->sfbCount))];
      unsigned int sampleCount = sampleEnd - sampleStart;

      printf("  filter %d  isDownward %s  sfbStart %d  sfbEnd %d  tnsMaxBand %d  sampleStart %d  sampleEnd %d  sampleCount %d\n", f, (filter.isDownward ? "true" : "false"), sfbStart, sfbEnd, tnsMaxBand, sampleStart, sampleEnd, sampleCount);

      if (sampleCount == 0)
        continue;  // No work to do

      double lpc[AAC_MAX_TNS_ORDER_SHORT + 1];  // "Linear prediction coding" coefficients
      assert(filter.order <= AAC_MAX_TNS_ORDER_SHORT);
      AacAudioTools::transformTnsCoefficients(filter.coefficients, lpc, info->tns.coefficientBits[w], filter.order);

      if (filter.isDownward)
        AacAudioTools::tnsFilterDownwards(coefficients + (w * AAC_SPECTRAL_SAMPLE_SIZE_SHORT) + sampleEnd - 1, sampleCount, filter.order, lpc);
      else
        AacAudioTools::tnsFilterUpwards(coefficients + (w * AAC_SPECTRAL_SAMPLE_SIZE_SHORT) + sampleStart, sampleCount, filter.order, lpc);

      sfbEnd = sfbStart;
    }
  }

  return true;
}

bool AacChannelDecoder::decodeAudioLongWindow(AacBitReader *reader, const AacDecodeInfo *info, const int16_t quant[AAC_SPECTRAL_SAMPLE_SIZE_LONG], int16_t audio[AAC_AUDIO_SAMPLE_OUTPUT_COUNT])
{
  // Dequantize
  double dequant[AAC_SPECTRAL_SAMPLE_SIZE_LONG];
  AacAudioTools::dequantize(quant, dequant);

  // Rescale (§ 11.3.3)
  double x_rescal[AAC_SPECTRAL_SAMPLE_SIZE_LONG] = {};
  for (unsigned int g = 0; g < info->ics->windowGroupCount; g++)  // Groups
  {
    unsigned int winCount = info->ics->windowGroups[g].winLength;  // Count of windows within group

    for (unsigned int sfb = 0; sfb < info->ics->sfbCount; sfb++)
    {
      unsigned int hcb = info->section.sfbCodebooks[g][sfb];
      if ((hcb == AAC_HCB_ZERO) || (hcb == AAC_HCB_INTENSITY) || (hcb == AAC_HCB_INTENSITY2))
        continue;  // No scalefactor for this band

      unsigned int sfbSampleStart, sfbSampleCount;
      if (info->ics->windowSequence != AAC_WINSEQ_8_SHORT)
      {
        // Long window
        sfbSampleStart = m_scalefactorBandInfo->longWindow->offsets[sfb];
        sfbSampleCount = m_scalefactorBandInfo->longWindow->offsets[sfb + 1] - sfbSampleStart;
      }
      else
      {
        // Short window
        sfbSampleStart = m_scalefactorBandInfo->shortWindow->offsets[sfb];
        sfbSampleCount = m_scalefactorBandInfo->shortWindow->offsets[sfb + 1] - sfbSampleStart;
      }

      double gain = pow(2, 0.25 * (info->sf.scalefactors[g][sfb] - 100));
      //printf("  Rescale group %d  sfb %d  sfbSampleStart %d  sfbSampleCount %d  gain %f\n", g, sfb, sfbSampleStart, sfbSampleCount, gain);

      for (unsigned int winOffset = 0; winOffset < winCount; winOffset++)
      {
        unsigned int win = info->ics->windowGroups[g].winStart + winOffset;

        // NOTE: The win variable should always be 0 for a long window, so this should be safe.
        unsigned int winSampleStart = win * AAC_SPECTRAL_SAMPLE_SIZE_SHORT;

        unsigned int sampleBase = winSampleStart + sfbSampleStart;
        for (unsigned int k = 0; k < sfbSampleCount; k++)
        {
          x_rescal[sampleBase + k] = dequant[sampleBase + k] * gain;
          //printf("    x_rescal[%d] = %f  group %d  sfb %d  dequant %f  gain %f\n", sampleBase + k, x_rescal[sampleBase + k], g, sfb, dequant[sampleBase + k], gain);
        }
      }
    }
  }

  // TNS
  if (info->tns.isEnabled)
  {
    if (info->ics->windowSequence != AAC_WINSEQ_8_SHORT)
    {
      if (!applyTnsLongWindow(x_rescal, info))
        return false;
    }
    else
    {
      if (!applyTnsShortWindow(x_rescal, info))
        return false;
    }
  }

  // IMDCT
  printf("Frame %d samples\n", m_blockCount);
  double samples[AAC_XFORM_WIN_SIZE_LONG];
  if (info->ics->windowSequence != AAC_WINSEQ_8_SHORT)
  {
    // One long window
    AacAudioTools::IMDCTLong(x_rescal, samples);
  }
  else
  {
    // Eight short windows
    for (unsigned int w = 0; w < 8; w++)
      AacAudioTools::IMDCTShort(x_rescal + (w * AAC_SPECTRAL_SAMPLE_SIZE_SHORT), samples + (w * AAC_XFORM_WIN_SIZE_SHORT));
  }

  // Windowing (§ 15.3.2)
  if (m_blockCount == 0)
    m_previousWindowShape = info->ics->windowShape;

  if (info->ics->windowSequence != AAC_WINSEQ_8_SHORT)
  {
    // Long windows

    const double *leftWindow = AacWindows::getLeftWindow(m_previousWindowShape, info->ics->windowSequence);
    AacAudioTools::window(leftWindow, samples, AAC_XFORM_HALFWIN_SIZE_LONG);

    const double *rightWindow = AacWindows::getRightWindow(info->ics->windowShape, info->ics->windowSequence);
    AacAudioTools::window(rightWindow, samples + AAC_XFORM_HALFWIN_SIZE_LONG, AAC_XFORM_HALFWIN_SIZE_LONG);

  }
  else
  {
    // Short windows

    const double *leftWindow = AacWindows::getLeftWindow(m_previousWindowShape, info->ics->windowSequence);
    AacAudioTools::window(leftWindow, samples, AAC_XFORM_HALFWIN_SIZE_SHORT);

    const double *rightWindow = AacWindows::getRightWindow(info->ics->windowShape, info->ics->windowSequence);
    AacAudioTools::window(rightWindow, samples + AAC_XFORM_HALFWIN_SIZE_SHORT, AAC_XFORM_HALFWIN_SIZE_SHORT);

    leftWindow = AacWindows::getLeftWindow(info->ics->windowShape, info->ics->windowSequence);
    AacAudioTools::window(leftWindow, samples + (AAC_XFORM_HALFWIN_SIZE_SHORT *  2), AAC_XFORM_HALFWIN_SIZE_SHORT);  // 1
    AacAudioTools::window(rightWindow, samples + (AAC_XFORM_HALFWIN_SIZE_SHORT *  3), AAC_XFORM_HALFWIN_SIZE_SHORT);
    AacAudioTools::window(leftWindow, samples + (AAC_XFORM_HALFWIN_SIZE_SHORT *  4), AAC_XFORM_HALFWIN_SIZE_SHORT);  // 2
    AacAudioTools::window(rightWindow, samples + (AAC_XFORM_HALFWIN_SIZE_SHORT *  5), AAC_XFORM_HALFWIN_SIZE_SHORT);
    AacAudioTools::window(leftWindow, samples + (AAC_XFORM_HALFWIN_SIZE_SHORT *  6), AAC_XFORM_HALFWIN_SIZE_SHORT);  // 3
    AacAudioTools::window(rightWindow, samples + (AAC_XFORM_HALFWIN_SIZE_SHORT *  7), AAC_XFORM_HALFWIN_SIZE_SHORT);
    AacAudioTools::window(leftWindow, samples + (AAC_XFORM_HALFWIN_SIZE_SHORT *  8), AAC_XFORM_HALFWIN_SIZE_SHORT);  // 4
    AacAudioTools::window(rightWindow, samples + (AAC_XFORM_HALFWIN_SIZE_SHORT *  9), AAC_XFORM_HALFWIN_SIZE_SHORT);
    AacAudioTools::window(leftWindow, samples + (AAC_XFORM_HALFWIN_SIZE_SHORT * 10), AAC_XFORM_HALFWIN_SIZE_SHORT);  // 5
    AacAudioTools::window(rightWindow, samples + (AAC_XFORM_HALFWIN_SIZE_SHORT * 11), AAC_XFORM_HALFWIN_SIZE_SHORT);
    AacAudioTools::window(leftWindow, samples + (AAC_XFORM_HALFWIN_SIZE_SHORT * 12), AAC_XFORM_HALFWIN_SIZE_SHORT);  // 6
    AacAudioTools::window(rightWindow, samples + (AAC_XFORM_HALFWIN_SIZE_SHORT * 13), AAC_XFORM_HALFWIN_SIZE_SHORT);
    AacAudioTools::window(leftWindow, samples + (AAC_XFORM_HALFWIN_SIZE_SHORT * 14), AAC_XFORM_HALFWIN_SIZE_SHORT);  // 7
    AacAudioTools::window(rightWindow, samples + (AAC_XFORM_HALFWIN_SIZE_SHORT * 15), AAC_XFORM_HALFWIN_SIZE_SHORT);

    // Internal overlap of short windows
    double input[AAC_XFORM_WIN_SIZE_LONG];
    memcpy(input, samples, sizeof(double) * AAC_XFORM_WIN_SIZE_LONG);

    for (unsigned int s = 0; s < 448; s++)
      samples[s] = 0.0;

    for (unsigned int s = 0; s < 128; s++)
      samples[s + 448] = input[s];
    for (unsigned int s = 0; s < 128; s++)
      samples[s + 576] = input[s + 128] + input[s + 256];
    for (unsigned int s = 0; s < 128; s++)
      samples[s + 704] = input[s + 384] + input[s + 512];
    for (unsigned int s = 0; s < 128; s++)
      samples[s + 832] = input[s + 640] + input[s + 768];
    for (unsigned int s = 0; s < 128; s++)
      samples[s + 960] = input[s + 896] + input[s + 1024];
    for (unsigned int s = 0; s < 128; s++)
      samples[s + 1088] = input[s + 1152] + input[s + 1280];
    for (unsigned int s = 0; s < 128; s++)
      samples[s + 1216] = input[s + 1408] + input[s + 1536];
    for (unsigned int s = 0; s < 128; s++)
      samples[s + 1344] = input[s + 1664] + input[s + 1792];
    for (unsigned int s = 0; s < 128; s++)
      samples[s + 1472] = input[s + 1920];

    for (unsigned int s = 0; s < 448; s++)
      samples[s + 1600] = 0.0;

  }

  // TODO: Some window shapes leave samples[] with large regions of zeroes.
  // We could maybe take advantage of this when summing samples.

  // Overlapping with previous samples (§ 15.3.3)
  static double oldSamples[1024] = {};  // TODO: Constant for length, move to member variable
  for (unsigned int s = 0; s < 1024; s++)  // TODO: Constant
  {
    auto tmp = samples[s];
    samples[s] += oldSamples[s];
    printf("  overlap[%d]: transform %.3f  old %.3f  sum %.3f\n", s, tmp, oldSamples[s], samples[s]);
  }

  // Save second half of previous samples for next time
  for (unsigned int s = 0; s < 1024; s++)  // TODO: Constant
    oldSamples[s] = samples[s + 1024];

  // Convert to int16
  int16_t final[1024];  // TODO: Constant for length
  for (unsigned int s = 0; s < 1024; s++)  // TODO: Constant
  {
    if (samples[s] > 0)
    {
      if (samples[s] > INT16_MAX)
        final[s] = INT16_MAX;
      else
        final[s] = static_cast<int16_t>(samples[s] + 0.5);
    }
    else
    {
      if (samples[s] < INT16_MIN)
        final[s] = INT16_MIN;
      else
        final[s] = static_cast<int16_t>(samples[s] - 0.5);
    }
  }

  // Write the first 1024 samples to the audio buffer
  memcpy(audio, final, sizeof(int16_t) * 1024);

  // Remember window shape for next block
  m_previousWindowShape = info->ics->windowShape;

  m_blockCount++;

  return true;
}

bool AacChannelDecoder::decodeAudioShortWindow(AacBitReader *reader, const AacDecodeInfo *info, const int16_t quant[AAC_SPECTRAL_SAMPLE_SIZE_LONG], int16_t audio[AAC_AUDIO_SAMPLE_OUTPUT_COUNT])
{
  // TODO
  return decodeAudioLongWindow(reader, info, quant, audio);
}

bool AacChannelDecoder::decodeAudio(AacBitReader *reader, const AacDecodeInfo *info, const int16_t quant[AAC_SPECTRAL_SAMPLE_SIZE_LONG], int16_t audio[AAC_AUDIO_SAMPLE_OUTPUT_COUNT])
{
  if (info->ics->isLongWindow)
    return decodeAudioLongWindow(reader, info, quant, audio);
  else
    return decodeAudioShortWindow(reader, info, quant, audio);
}
