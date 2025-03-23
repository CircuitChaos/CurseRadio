#pragma once

#include <cstdint>
#include "defs.h"

namespace bandlimits {

uint32_t getMinByBand(Band band);
uint32_t getMaxByBand(Band band);
Band getBandByFreq(uint32_t freq);

} // namespace bandlimits
