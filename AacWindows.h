#include "AacConstants.h"

#ifndef AAC_WINDOWS_H
#define AAC_WINDOWS_H

namespace AacWindows
{
  extern const double *getLeftWindow(AacWindowShape shape, AacWindowSequence sequence);
  extern const double *getRightWindow(AacWindowShape shape, AacWindowSequence sequence);
};

#endif
