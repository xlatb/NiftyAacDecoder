#include <stdlib.h>
#include <stdint.h>

#ifndef AAC_ADTS_FRAME_HEADER_H
#define AAC_ADTS_FRAME_HEADER_H

#define AAC_ADTS_FRAME_HEADER_SIZE 7

enum AacAdtsProfile
{
  AAC_PROFILE_MAIN    = 0x0,
  AAC_PROFILE_LC      = 0x1,  // Low complexity
  AAC_PROFILE_SSR     = 0x2,  // Scalable sampling rate
  AAC_PROFILE_UNKNOWN = 0x3,
};

struct AacChannelConfiguration
{
  unsigned int fullChannelCount;
  unsigned int subwooferChannelCount;
};

class AacAdtsFrameHeader
{
  const uint8_t *m_bytes;

  uint8_t getVersionId(void) const { return (m_bytes[1] >> 3) & 0x01; };

  uint8_t getLayerId(void) const { return (m_bytes[1] >> 1) & 0x03; };

  uint8_t getSampleRateIndex(void) const { return (m_bytes[2] >> 2) & 0x0F; };

  uint8_t getChannelConfigurationIndex(void) const { return ((m_bytes[2] & 0x01) << 2) | (m_bytes[3] >> 6); };

public:
  static bool isFrameHeader(const uint8_t *bytes);

  AacAdtsFrameHeader(void) : m_bytes(NULL) {};
  AacAdtsFrameHeader(const uint8_t bytes[AAC_ADTS_FRAME_HEADER_SIZE]) : m_bytes(bytes) {};

  void setBytes(const uint8_t *bytes) { m_bytes = bytes; };

  bool hasCrcProtection(void) const { return !(m_bytes[1] & 0x01); };

  AacAdtsProfile getProfile(void) const { return static_cast<AacAdtsProfile>(m_bytes[2] >> 6); };

  unsigned int getSampleRate(void) const;

  const AacChannelConfiguration *getChannelConfiguration(void) const;

  size_t getFrameSize(void) const { return ((m_bytes[3] & 0x03) << 11) | (m_bytes[4] << 3) | (m_bytes[5] >> 5); };

  unsigned int getDataBlockCount(void) const { return (m_bytes[6] & 0x03) + 1; };

  const uint8_t *getPayloadBytes(void) const;
  size_t         getPayloadSize(void) const;

  void dump(void) const;
};

#endif
