#pragma once

#include <string>

enum Mode {
	MODE_SSB_1,  /* 1 */
	MODE_SSB_2,  /* 2 */
	MODE_CW_1,   /* 3 */
	MODE_FM,     /* 4 */
	MODE_AM,     /* 5 */
	MODE_RTTY_1, /* 6 */
	MODE_CW_2,   /* 7 */
	MODE_DATA_1, /* 8 */
	MODE_RTTY_2, /* 9 */
	MODE_FM_N,   /* B */
	MODE_DATA_2, /* C */
	MODE_AM_N,   /* D */
};

std::string getModeName(Mode mode);
