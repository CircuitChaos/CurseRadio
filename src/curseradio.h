#pragma once

#include <memory>
#include "cli.h"
#include "ui.h"
#include "exchange.h"
#include "presets.h"
#include "cat.h"
#include "timer.h"
#include "ptt.h"
#include "keyer.h"
#include "logger.h"
#include "mode.h"
#include "meters.h"

class CurseRadio {
public:
	void run(const Cli &cli);

private:
	Ui ui;
	std::unique_ptr<Exchange> exchange;
	std::unique_ptr<Presets> presets;
	std::unique_ptr<Cat> cat;
	std::unique_ptr<Timer> catTimeoutTimer;
	std::unique_ptr<Timer> catMeterTimer;
	std::unique_ptr<Ptt> ptt;
	std::unique_ptr<Keyer> keyer;
	std::unique_ptr<Logger> logger;

	std::optional<uint32_t> curFreq;  /* Current frequency, updated by CAT meter timer */
	std::optional<Mode> curMode;      /* Current mode, updated by CAT meter timer */
	std::optional<time_t> frozenTime; /* Time frozen by UI when 'l' is pressed */

	/* Meters being read, scheduled for sending to UI */
	std::map<meters::Meter, uint8_t> schedMeters;

	/* Returns true if need to exit */
	bool uiEvt(const UiEvt &evt);
	void catEvt(const CatEvt &evt);
	void keyerEvt(const KeyerEvt &evt);
};
