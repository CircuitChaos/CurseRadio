#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <unistd.h>
#include <ctime>
#include "curseradio.h"
#include "bandlimits.h"
#include "throw.h"
#include "util.h"

static const unsigned DEFAULT_WPM         = 25;
static const unsigned METER_POLL_INTERVAL = 100;
static const unsigned CAT_TIMEOUT         = 2000;
static const int32_t TUNE_INCREMENT_SLOW  = 10;
static const int32_t TUNE_INCREMENT_FAST  = 250;

void CurseRadio::run(const Cli &cli)
{
	signal(SIGPIPE, SIG_IGN);
	std::set<int> inrfds{ui.getFd()};

	if(!cli.getPrefix().empty() || !cli.getInfix().empty() || !cli.getSuffix().empty()) {
		exchange.reset(new Exchange(cli.getPrefix(), cli.getInfix(), cli.getSuffix()));
	}

	if(exchange && !cli.getCallsign().empty()) {
		presets.reset(new Presets(cli.getCallsign()));
	}

	if(!cli.getCatPort().empty()) {
		catTimeoutTimer.reset(new Timer(CAT_TIMEOUT));
		inrfds.insert(catTimeoutTimer->getFd());

		cat.reset(new Cat(cli.getCatPort(), cli.getCatBaud(), catTimeoutTimer.get()));
		inrfds.insert(cat->getFd());

		// TODO maybe make this interval configurable from CLI
		catMeterTimer.reset(new Timer(METER_POLL_INTERVAL));
		inrfds.insert(catMeterTimer->getFd());
		catMeterTimer->start();
	}

	if(!cli.getPttPort().empty()) {
		ptt.reset(new Ptt(cli.getPttPort()));
		keyer.reset(new Keyer(cli.getWpm() ? cli.getWpm() : DEFAULT_WPM));
		inrfds.insert(keyer->getFd());
	}

	if(exchange && cat && !cli.getCallsign().empty() && !cli.getCbrFile().empty()) {
		logger.reset(new Logger(cli.getCallsign(), cli.getCbrFile()));
	}

	for(;;) {
		const std::set<int> outrfds(util::watch(inrfds, -1));
		if(util::inSet(outrfds, ui.getFd()) && uiEvt(ui.read())) {
			break;
		}

		if(cat && util::inSet(outrfds, cat->getFd())) {
			const std::vector<CatEvt> evts(cat->read());
			for(std::vector<CatEvt>::const_iterator i(evts.begin()); i != evts.end(); ++i) {
				catEvt(*i);
			}
		}

		if(keyer && util::inSet(outrfds, keyer->getFd())) {
			keyerEvt(keyer->read());
		}

		if(catMeterTimer && util::inSet(outrfds, catMeterTimer->getFd()) && catMeterTimer->read()) {
			cat->getMeter(METER_IDD);
		}

		if(catTimeoutTimer && util::inSet(outrfds, catTimeoutTimer->getFd()) && catTimeoutTimer->read()) {
			xthrow("CAT timeout");
		}
	}
}

bool CurseRadio::uiEvt(const UiEvt &evt)
{
	switch(evt.type) {
		case UiEvt::EVT_NONE:
			break;

		case UiEvt::EVT_QUIT:
			return true;

		case UiEvt::EVT_FREQ_UP_SLOW:
		case UiEvt::EVT_FREQ_UP_FAST:
		case UiEvt::EVT_FREQ_DOWN_SLOW:
		case UiEvt::EVT_FREQ_DOWN_FAST:
		case UiEvt::EVT_FREQ_RESET: {
			if(!cat) {
				ui.print("CAT disabled");
				break;
			}

			cat->getFreq();
			ScheduledCatOp op(ScheduledCatOp::OP_FREQ_ADJUST);
			switch(evt.type) {
				case UiEvt::EVT_FREQ_UP_SLOW:
					op.freqIncrement = TUNE_INCREMENT_SLOW;
					break;

				case UiEvt::EVT_FREQ_UP_FAST:
					op.freqIncrement = TUNE_INCREMENT_FAST;
					break;

				case UiEvt::EVT_FREQ_DOWN_SLOW:
					op.freqIncrement = -TUNE_INCREMENT_SLOW;
					break;

				case UiEvt::EVT_FREQ_DOWN_FAST:
					op.freqIncrement = -TUNE_INCREMENT_FAST;
					break;

				case UiEvt::EVT_FREQ_RESET:
					op.freqIncrement = 0;
					break;

				default:
					xthrow("UI event %d is unexpectedhere", evt.type);
					break;
			}

			schedOps.push_back(op);
			break;
		}

		case UiEvt::EVT_BAND:
			xassert(evt.band, "Band expected, but not found");

			if(!cat) {
				ui.print("CAT disabled");
				break;
			}

			cat->setBand(evt.band.value());
			break;

		case UiEvt::EVT_MODE:
			xassert(evt.mode, "Mode expected, but not found");

			if(!cat) {
				ui.print("CAT disabled");
				break;
			}

			cat->setMode(evt.mode.value());
			break;

		case UiEvt::EVT_BW_DOWN:
		case UiEvt::EVT_BW_UP:
			if(!cat) {
				ui.print("CAT disabled");
				break;
			}

			cat->getWidth();
			schedOps.push_back(ScheduledCatOp((evt.type == UiEvt::EVT_BW_DOWN) ? ScheduledCatOp::OP_BW_DOWN : ScheduledCatOp::OP_BW_UP));
			break;

		case UiEvt::EVT_SWAP:
			if(!cat) {
				ui.print("CAT disabled");
				break;
			}

			ui.print("Swapping VFO");
			cat->swapVfo();
			break;

		case UiEvt::EVT_SHOW_PRESETS:
			if(!presets || !exchange) {
				ui.print("Preset support disabled");
			}
			else {
				ui.print("Number of presets: %zu", presets->numPresets());
				xassert(presets->numPresets() <= 10, "Too many presets");
				for(size_t i(0); i < presets->numPresets(); ++i) {
					ui.print("Preset #%zu: %s", (i + 1) % 10, presets->getPreset(i, exchange->get()).c_str());
				}
			}
			break;

		case UiEvt::EVT_SEND_PRESET: {
			if(!presets || !exchange || !keyer) {
				ui.print("Cannot send preset -- preset or keyer support disabled");
				break;
			}

			xassert(evt.preset, "Preset in event not set");
			if(evt.preset >= presets->numPresets()) {
				ui.print("Preset not defined");
				break;
			}

			const std::string preset(presets->getPreset(evt.preset.value(), exchange->get()));
			ui.print("Sending preset: %s", preset.c_str());
			keyer->send(preset);
			break;
		}

		case UiEvt::EVT_SHOW_XCHG:
			if(!exchange) {
				ui.print("Exchange support disabled");
				break;
			}
			ui.print("Current exchange: %s", exchange->get().c_str());
			break;

		case UiEvt::EVT_CHECK_CALL:
			xassert(evt.checkCall, "Expecting checkCall field");
			if(!logger) {
				ui.print("Cannot check call -- logging disabled");
				break;
			}

			if(logger->exists(evt.checkCall.value())) {
				ui.print("Callsign %s already in log", evt.checkCall.value().c_str());
			}
			else {
				ui.print("Callsign %s not in log", evt.checkCall.value().c_str());
			}
			break;

		case UiEvt::EVT_LOG: {
			xassert(evt.logCall && evt.logXchg, "Expecting at least call and exchange in log event");

			if(!logger) {
				ui.print("Cannot log -- logging disabled");
				break;
			}

			if(logger->exists(evt.logCall.value())) {
				ui.print("Warning: call %s already exists in log", evt.logCall.value().c_str());
			}

			Logger::Entry e;
			if(frozenTime) {
				e.ts       = frozenTime;
				frozenTime = 0;
			}
			else {
				e.ts = time(nullptr);
			}

			e.freq = 0;       // doesn't matter -- will be filled in event
			e.mode = MODE_CW; // doesn't matter -- will be filled in event
			// sentRst will be filled in event based on mode
			e.sentXchg = exchange->get();
			e.rcvdCall = evt.logCall.value();
			if(evt.logRst) {
				e.rcvdRst = evt.logRst.value();
			}
			e.rcvdXchg = evt.logXchg.value();

			cat->getFreq();
			schedOps.push_back(e);
			break;
		}

		case UiEvt::EVT_FREEZE_TIME:
			frozenTime = time(nullptr);
			break;

		case UiEvt::EVT_SEND_TEXT:
			xassert(evt.text, "Expecting text to send in event");

			if(!keyer) {
				ui.print("Cannot send text -- keyer support disabled");
				break;
			}

			ui.print("Sending text: %s", evt.text.value().c_str());
			keyer->send(evt.text.value());
			break;

		case UiEvt::EVT_WPM_UP:
			if(!keyer) {
				ui.print("Cannot increase WPM -- keyer support disabled");
				break;
			}

			ui.print("Keyer speed: %u WPM", keyer->wpmUp());
			break;

		case UiEvt::EVT_WPM_DOWN:
			if(!keyer) {
				ui.print("Cannot decrease WPM -- keyer support disabled");
				break;
			}

			ui.print("Keyer speed: %u WPM", keyer->wpmDown());
			break;

		case UiEvt::EVT_SEND_ABORT:
			if(!keyer) {
				ui.print("Cannot abort sending -- keyer support disabled");
				break;
			}

			if(!keyer->isSending()) {
				ui.print("Cannot abort sending -- keyer is not sending");
				break;
			}

			keyer->abortSending();
			break;

		case UiEvt::EVT_ZERO_IN:
			if(!cat) {
				ui.print("CAT disabled");
				break;
			}

			ui.print("Zeroing in");
			cat->zin();
			break;

		default:
			xthrow("Unknown UI event type %d", evt.type);
			break;
	}

	return false;
}

void CurseRadio::catEvt(const CatEvt &evt)
{
	switch(evt.type) {
		case CatEvt::EVT_ERROR:
			xassert(evt.error, "Expected error field not found");
			xthrow("CAT error: %s", evt.error.value().c_str());
			break;

		case CatEvt::EVT_METER:
			xassert(evt.meter, "Expected meter field not found");

			switch(evt.meter.value().first) {
				case METER_IDD:
					if(!evt.meter.value().second) {
						/* If IDD = 0, then we're in RX mode -- only read signal */
						cat->getMeter(METER_SIG);
					}
					else {
						/* Store IDD and go to ALC -> COMP -> PWR -> SWR */
						schedMeters[evt.meter.value().first] = evt.meter.value().second;
						cat->getMeter(METER_ALC);
					}
					break;

				case METER_SWR:
				case METER_SIG:
					/* In RX mode only signal is read, in TX reading ends on SWR */
					schedMeters[evt.meter.value().first] = evt.meter.value().second;
					ui.updateMeters(schedMeters);
					schedMeters.clear();
					catMeterTimer->start();
					break;

				case METER_ALC:
					schedMeters[evt.meter.value().first] = evt.meter.value().second;
					cat->getMeter(METER_COMP);
					break;

				case METER_COMP:
					schedMeters[evt.meter.value().first] = evt.meter.value().second;
					cat->getMeter(METER_PWR);
					break;

				case METER_PWR:
					schedMeters[evt.meter.value().first] = evt.meter.value().second;
					cat->getMeter(METER_SWR);
					break;

				default:
					xthrow("Unknown meter %d", evt.meter.value().first);
					break;
			}
			break;

		case CatEvt::EVT_FREQ: {
			xassert(evt.freq, "Expecting frequency in event");
			xassert(!schedOps.empty(), "Frequency read, but no scheduled operation");
			const ScheduledCatOp op(schedOps[0]);

			switch(op.type) {
				case ScheduledCatOp::OP_FREQ_ADJUST: {
					xassert(op.freqIncrement, "Increment not found");
					uint32_t freq(evt.freq.value());
					const int32_t increment(op.freqIncrement.value());
					const Band band(bandlimits::getBandByFreq(freq));

					if(increment) {
						freq += increment;
						if(freq < bandlimits::getMinByBand(band)) {
							freq = bandlimits::getMinByBand(band);
						}
						else if(freq > bandlimits::getMaxByBand(band)) {
							freq = bandlimits::getMaxByBand(band);
						}
					}
					else {
						freq = bandlimits::getMinByBand(band);
					}

					ui.print("Setting frequency to %d.%03d kHz", freq / 1000, freq % 1000);
					schedOps.erase(schedOps.begin());
					cat->setFreq(freq);
					break;
				}

				case ScheduledCatOp::OP_LOG:
					schedFreq = evt.freq.value();
					cat->getMode();
					break;

				default:
					xthrow("Frequency read, but scheduled operation is %d", op.type);
					break;
			}
			break;
		}

		case CatEvt::EVT_MODE: {
			xassert(evt.mode, "Expecting mode in event");
			xassert(!schedOps.empty(), "Mode read, but no scheduled operation");
			const ScheduledCatOp op(schedOps[0]);
			schedOps.erase(schedOps.begin());
			xassert(op.type == ScheduledCatOp::OP_LOG, "Mode read, but scheduled operation is not logging");
			xassert(op.logEntry, "Log entry not found");
			xassert(logger, "Got EVT_MODE, but logger is disabled");

			Logger::Entry le(op.logEntry.value());
			le.mode = evt.mode.value();
			if(le.sentRst.empty()) {
				le.sentRst = (le.mode == MODE_CW) ? "599" : "59";
			}
			if(le.rcvdRst.empty()) {
				le.rcvdRst = (le.mode == MODE_CW) ? "599" : "59";
			}
			le.freq = schedFreq;

			ui.print("Logged QSO: %s", logger->log(le).c_str());
			schedFreq = 0;
			if(exchange->next()) {
				ui.print("Log entry written, next exchange: %s", exchange->get().c_str());
			}
			else {
				ui.print("Log entry written, exchange does not change");
			}
			break;
		}

		case CatEvt::EVT_WIDTH: {
			xassert(evt.width, "Expecting bandwidth in event");
			xassert(!schedOps.empty(), "Bandwidth read, but no scheduled operation");
			const ScheduledCatOp::OpType type(schedOps[0].type);
			schedOps.erase(schedOps.begin());

			if(type == ScheduledCatOp::OP_BW_UP) {
				/* TODO increase bandwidth, if possible, call setWidth */
			}
			else if(type == ScheduledCatOp::OP_BW_DOWN) {
				/* TODO decrease bandwidth, if possible, call setWidth */
			}
			else {
				xthrow("Bandwidth read, but scheduled operation is %d", type);
			}
			break;
		}

		default:
			xthrow("Unknown CAT event %d", evt.type);
			break;
	}
}

void CurseRadio::keyerEvt(const KeyerEvt &evt)
{
	xassert(ptt, "Keyer event without PTT, this shouldn't happen");

	switch(evt.type) {
		case KeyerEvt::EVT_NONE:
			break;

		case KeyerEvt::EVT_KEY_DOWN:
			ptt->keyDown();
			break;

		case KeyerEvt::EVT_KEY_UP:
			ptt->keyUp();
			break;

		default:
			xthrow("Unknown keyer event %d", evt.type);
			break;
	}
}

int main(int argc, char *const argv[])
{
	try {
		Cli cli(argc, argv);
		if(cli.shouldExit()) {
			return EXIT_SUCCESS;
		}

		CurseRadio cr;
		cr.run(cli);
	}
	catch(const std::runtime_error &e) {
		fprintf(stderr, "Fatal error: %s\n", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

// TODO some command to decrement exchange
