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
#include "defs.h"
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

	struct ScheduledCatOp {
		enum OpType {
			/* EVT_WIDTH will trigger setWidth() */
			OP_BW_UP,
			OP_BW_DOWN,

			/* EVT_FREQ will trigger getMode(). EVT_MODE will trigger writing log entry */
			OP_LOG,

			/* EVT_FREQ will trigger setFreq() */
			OP_FREQ_ADJUST,
		};

		/* Non-const, because copy operator must be created */
		OpType type;
		std::optional<Logger::Entry> logEntry;
		std::optional<int32_t> freqIncrement; /* if 0 then freq is reset */

		ScheduledCatOp(OpType type)
		    : type(type) {}

		ScheduledCatOp(const Logger::Entry &logEntry)
		    : type(OP_LOG), logEntry(logEntry) {}
	};

	/* Scheduled CAT operations (waiting for CAT events) */
	std::vector<ScheduledCatOp> schedOps;
	uint32_t schedFreq{0}; /* used internally by evtCat */
	time_t frozenTime{0};

	/* Meters being read, scheduled for sending to UI */
	std::map<meters::Meter, uint8_t> schedMeters;

	/* Returns true if need to exit */
	bool uiEvt(const UiEvt &evt);
	void catEvt(const CatEvt &evt);
	void keyerEvt(const KeyerEvt &evt);
};
