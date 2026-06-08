#include <vector>
#include <utility>
#include <algorithm>
#include "throw.h"
#include "band.h"

struct Entry {
	Band band;
	uint32_t min;
	uint32_t beacon;
	uint32_t max;
};

/* Vector, not map, because must be ordered (GEN must be last) */
static const std::vector<Entry> bands = {
    /* Only bands supported by FT-891. Valid for IARU 1 region only.
     *
     * For frequencies < 10 MHz, beacon is shifted 3 kHz up, because
     * beacon is the FT8 frequency which is always in USB.
     */
    {BAND_160, 1810000, 1843000, 2000000},
    {BAND_80, 3500000, 3576000, 3800000},
    {BAND_60, 5351500, 5360000, 5366500},
    {BAND_40, 7000000, 7077000, 7200000},
    {BAND_30, 10100000, 10136000, 10150000},
    {BAND_20, 14000000, 14074000, 14350000},
    {BAND_17, 18068000, 18100000, 18168000},
    {BAND_15, 21000000, 21074000, 21450000},
    {BAND_12, 24890000, 24915000, 24990000},
    {BAND_10, 28000000, 28074000, 29700000},
    {BAND_6, 50000000, 50313000, 54000000},
};

static const Entry &getByBand(Band band)
{
	const std::vector<Entry>::const_iterator i(std::find_if(bands.begin(), bands.end(), [band](const auto &entry) { return entry.band == band; }));
	xassert(i != bands.end(), "Band %d not found", band);
	return *i;
}

uint32_t band::getMinByBand(Band band)
{
	return getByBand(band).min;
}

uint32_t band::getMaxByBand(Band band)
{
	return getByBand(band).max;
}

uint32_t band::getBeaconByBand(Band band)
{
	return getByBand(band).beacon;
}

Band band::getBandByFreq(uint32_t freq)
{
	const std::vector<Entry>::const_iterator i(std::find_if(bands.begin(), bands.end(), [freq](const auto &entry) { return freq >= entry.min && freq <= entry.max; }));
	xassert(i != bands.end(), "Band for freq %u not found", freq);
	return i->band;
}
