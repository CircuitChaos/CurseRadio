#include <vector>
#include <utility>
#include <algorithm>
#include "throw.h"
#include "bandlimits.h"

struct Entry {
	Band band;
	uint32_t min;
	uint32_t max;
};

/* Vector, not map, because must be ordered (GEN must be last) */
static const std::vector<Entry> bands = {
    /* This is in kHz, it's translated later to Hz */
    /* Only bands supported by FT-891. Valid for IARU 1 region only. */
    {BAND_160, 1810, 2000},
    {BAND_80, 3500, 3800},
    {BAND_40, 7000, 7200},
    {BAND_30, 10100, 10150},
    {BAND_20, 14000, 14350},
    {BAND_17, 18068, 18168},
    {BAND_15, 21000, 21450},
    {BAND_12, 24890, 24990},
    {BAND_10, 28000, 29700},
    {BAND_6, 50000, 54000},
    {BAND_GEN, 30, 56000},
};

static const Entry &getByBand(Band band)
{
	const std::vector<Entry>::const_iterator i(std::find_if(bands.begin(), bands.end(), [band](const auto &entry) { return entry.band == band; }));
	xassert(i != bands.end(), "Band %d not found", band);
	return *i;
}

uint32_t bandlimits::getMinByBand(Band band)
{
	return getByBand(band).min * 1000;
}

uint32_t bandlimits::getMaxByBand(Band band)
{
	return getByBand(band).max * 1000;
}

Band bandlimits::getBandByFreq(uint32_t freq)
{
	const std::vector<Entry>::const_iterator i(std::find_if(bands.begin(), bands.end(), [freq](const auto &entry) { return freq >= entry.min * 1000 && freq <= entry.max * 1000; }));
	xassert(i != bands.end(), "Band for freq %u not found", freq);
	return i->band;
}
