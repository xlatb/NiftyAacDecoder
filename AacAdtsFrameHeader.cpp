#include <assert.h>
#include <stdio.h>

#include "AacAdtsFrameHeader.h"

const char *versionNames[] = {"MPEG-4", "MPEG-2"};
const char *profileNames[] = {"Main", "LC", "SSR", "?"};

// Table 35
static const unsigned int sampleRateMap[] =
{
  96000,  // 0
  88200,
  64000,
  48000,

  44100,  // 4
  32000,
  24000,
  22050,

  16000,  // 8
  12000,
  11025,
  8000,

  0,      // 12
  0,
  0,
  0,
};

//struct AacChannelConfiguration
//{
//  unsigned int numFullChannels;
//  unsigned int numSubwooferChannels;
//};

// Channel configuration (Table 42)
static const AacChannelConfiguration channelConfigurations[] =
{
  {0, 0},  // 0x0: Channels defined elsewhere
  {1, 0},  // 0x1: Mono
  {2, 0},  // 0x2: Stereo
  {3, 0},  // 0x3: 3 front
  {4, 0},  // 0x4: 3 front, 1 back
  {5, 0},  // 0x5: 3 front, 2 back
  {5, 1},  // 0x6: 3 front, 2 back, 1 subwoofer ("5.1")
  {7, 1},  // 0x7: 3 front, 2 side, 2 back, 1 subwoofer ("7.1")
};

// Given a pointer to some memory at least AAC_ADTS_FRAME_HEADER_SIZE in
//  size, returns true if it looks like a frame header.
bool AacAdtsFrameHeader::isFrameHeader(const uint8_t *bytes)
{
  if (bytes[0] != 0xFF)
    return false;  // First byte of syncword mismatch

  if ((bytes[1] & 0xF0) != 0xF0)
    return false;  // Second byte of syncword mismatch

  if (((bytes[1] >> 1) & 0x03) != 0x00)
    return false;  // Invalid audio layer

  if (((bytes[2] >> 2) & 0x0F) == 0x0F)
    return false;  // Invalid sample rate index

  return true;
}

unsigned int AacAdtsFrameHeader::getSampleRate(void) const
{
  return sampleRateMap[getSampleRateIndex()];
}

const AacChannelConfiguration *AacAdtsFrameHeader::getChannelConfiguration(void) const
{
  return &channelConfigurations[getChannelConfigurationIndex()];
}

const uint8_t *AacAdtsFrameHeader::getPayloadBytes(void) const
{
  assert(getDataBlockCount() == 1);

  return m_bytes + AAC_ADTS_FRAME_HEADER_SIZE + (hasCrcProtection() ? 2 : 0);
}

size_t AacAdtsFrameHeader::getPayloadSize(void) const
{
  assert(getDataBlockCount() == 1);

  return getFrameSize() - AAC_ADTS_FRAME_HEADER_SIZE - (hasCrcProtection() ? 2 : 0);
}

void AacAdtsFrameHeader::dump(void) const
{
  auto channelConfig = getChannelConfiguration();

  printf("--- AAC audio frame ---\n");
  printf("MPEG version    : %d (%s)\n", getVersionId(), versionNames[getVersionId()]);
  printf("Has CRC         : %s\n", hasCrcProtection() ? "true" : "false");
  printf("Profile         : %d (%s)\n", getProfile(), profileNames[getProfile()]);
  printf("Sample rate     : %d Hz\n", getSampleRate());
  printf("Channel config  : %d (%d.%d)\n", getChannelConfigurationIndex(), channelConfig->fullChannelCount, channelConfig->subwooferChannelCount);
  printf("Frame size      : %zd\n", getFrameSize());
  printf("Data block count: %d\n", getDataBlockCount());
}
