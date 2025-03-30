#pragma once

#include <cstdint>

enum Band {
	BAND_160,
	BAND_80,
	BAND_40,
	BAND_30,
	BAND_20,
	BAND_17,
	BAND_15,
	BAND_12,
	BAND_10,
	BAND_6,
	BAND_GEN,
	BAND_MW,
};

namespace band {

uint32_t getMinByBand(Band band);
uint32_t getMaxByBand(Band band);
Band getBandByFreq(uint32_t freq);

} // namespace band
