#include <stdint.h>

#include <string>

#include "AacConstants.h"

#ifndef AAC_STRUCTS_H
#define AAC_STRUCTS_H

struct AacInstanceIdentifier
{
  AacElementId type;
  uint8_t      instance;
};

struct AacProgramConfigInfo
{
  uint8_t instance;
  uint8_t profile;
  uint8_t sampleRateIndex;

  uint8_t frontChannelElementCount;
  AacInstanceIdentifier frontChannelElements[AAC_PCE_MAX_FRONT_CHANNEL_ELEMENTS];

  uint8_t sideChannelElementCount;
  AacInstanceIdentifier sideChannelElements[AAC_PCE_MAX_SIDE_CHANNEL_ELEMENTS];

  uint8_t rearChannelElementCount;
  AacInstanceIdentifier rearChannelElements[AAC_PCE_MAX_REAR_CHANNEL_ELEMENTS];

  uint8_t lfeChannelElementCount;
  uint8_t lfeChannelElements[AAC_PCE_MAX_LFES];

  uint8_t dseElementCount;
  uint8_t dseElements[AAC_PCE_MAX_DSES];

  uint8_t channelCouplingElementCount;
  struct { bool isIndependentlySiwtched; uint8_t instance; } channelCouplingElements[AAC_PCE_MAX_CCES];

  bool    hasMonoMixdown;
  uint8_t monoMixdown;

  bool    hasStereoMixdown;
  uint8_t stereoMixdown;

  bool    hasMatrixMixdown;
  uint8_t matrixMixdownIndex;
  bool    pseudoSurroundEnabled;

  std::string comment;
};

struct AacIcsInfo
{
  AacWindowSequence windowSequence;
  AacWindowShape    windowShape;

  bool              isLongWindow;

  unsigned int      sfbCount;  // Scalefactor band count
  unsigned int      samplesPerWindow;  // Samples per window according to window size and sfbCount

  unsigned int      windowCount;  // Must be 1 or 8
  unsigned int      windowGroupCount;  // Number of window groups

  struct { uint8_t winStart; uint8_t winLength; } windowGroups[AAC_MAX_WINDOW_GROUPS];
};

// Main/Side mask for joint stereo.
struct AacMsMaskInfo
{
  AacMsMaskType type;
  uint8_t       sfbMask[AAC_MAX_SFB_COUNT];  // Group zero in low bit, etc
};

struct AacSectionInfo
{
  struct { uint16_t sampleCount; } windowGroups[AAC_MAX_WINDOW_GROUPS];

  // TODO: I think it would be more efficient to track this per section rather
  //  than per SFB. It only varies per section.
  // TODO: Actually, not sure. Might remove 'codebook' below inside sections[].
  uint8_t sfbCodebooks[AAC_MAX_WINDOW_GROUPS][AAC_MAX_SFB_COUNT];  // For each group, for each scalefactor band, the codebook

  struct
  {
    uint8_t count;
    struct
    {
      uint8_t  sfbStart;
      uint8_t  sfbLength;  // TODO: sfbCount?
      uint16_t sampleStart;  // TODO: intSampleStart?
      uint16_t sampleCount;  // TODO: intSampleCount?
      uint16_t winSampleStart;  // Post-deinterlace starting position of this section within each window
      uint16_t winSampleCount;  // Post-deinterlace count of samples within each window
      uint8_t  codebook;
    } sections[AAC_MAX_SFB_COUNT];
  } windowGroupSections[AAC_MAX_WINDOW_GROUPS];  // For each group, the sections
};

struct AacScalefactorInfo
{
  uint8_t scalefactors[AAC_MAX_WINDOW_GROUPS][AAC_MAX_SFB_COUNT];  // For each group, for each scalefactor band, the scalefactor [0..255]
};

struct AacPulseInfo
{
  uint8_t pulseCount;
  uint8_t pulseSfbStart;
  struct { uint8_t offset; uint8_t amplitude; } pulses[AAC_MAX_PULSE_COUNT];
};

struct AacTnsInfo
{
  bool    isEnabled;
  uint8_t filterCount[AAC_MAX_WINDOW_COUNT];  // For each window, the number of filters [0..3]
  uint8_t coefficientBits[AAC_MAX_WINDOW_COUNT];  // [3..4]

  struct AacTnsFilter
  {
    uint8_t sfbCount;
    uint8_t order;  // If zero, it is a placeholder and the direction/coefficients are undefined
    bool    isDownward;
    int8_t  coefficients[AAC_MAX_TNS_ORDER_LONG_MAIN];
  } filters[AAC_MAX_WINDOW_COUNT][AAC_MAX_TNS_FILTER_COUNT];  // For each window, for each filter, the filter info;
};

struct AacDecodeInfo
{
  unsigned int        identifier;
  uint8_t             globalGain;

  AacIcsInfo         *ics;
  AacSectionInfo      section;
  AacScalefactorInfo  sf;
  AacPulseInfo        pulse;
  AacTnsInfo          tns;
};

#endif
