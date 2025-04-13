#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <unistd.h>
#include <ctime>
#include "curseradio.h"
#include "band.h"
#include "throw.h"
#include "util.h"

static const unsigned DEFAULT_WPM         = 25;
static const unsigned METER_POLL_INTERVAL = 100;
static const unsigned CAT_TIMEOUT         = 2000;
static const int32_t TUNE_INCREMENT_SLOW  = 10;
static const int32_t TUNE_INCREMENT_NORM  = 100;
static const int32_t TUNE_INCREMENT_FAST  = 1000;
static const int32_t TUNE_INCREMENT_XFAST = 10000;

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
			cat->getMeter(meters::METER_IDD);
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
		case UiEvt::EVT_FREQ_UP_NORM:
		case UiEvt::EVT_FREQ_UP_FAST:
		case UiEvt::EVT_FREQ_UP_XFAST:
		case UiEvt::EVT_FREQ_DOWN_SLOW:
		case UiEvt::EVT_FREQ_DOWN_NORM:
		case UiEvt::EVT_FREQ_DOWN_FAST:
		case UiEvt::EVT_FREQ_DOWN_XFAST:
		case UiEvt::EVT_FREQ_RESET: {
			if(!cat) {
				ui.print("CAT disabled");
				break;
			}

			if(!curFreq) {
				ui.print("Current frequency unknown yet");
				break;
			}

			uint32_t newFreq(curFreq.value());
			int32_t increment(0);
			const Band band(band::getBandByFreq(newFreq));

			switch(evt.type) {
				case UiEvt::EVT_FREQ_UP_SLOW:
					increment = TUNE_INCREMENT_SLOW;
					break;

				case UiEvt::EVT_FREQ_UP_NORM:
					increment = TUNE_INCREMENT_NORM;
					break;

				case UiEvt::EVT_FREQ_UP_FAST:
					increment = TUNE_INCREMENT_FAST;
					break;

				case UiEvt::EVT_FREQ_UP_XFAST:
					increment = TUNE_INCREMENT_XFAST;
					break;

				case UiEvt::EVT_FREQ_DOWN_SLOW:
					increment = -TUNE_INCREMENT_SLOW;
					break;

				case UiEvt::EVT_FREQ_DOWN_NORM:
					increment = -TUNE_INCREMENT_NORM;
					break;

				case UiEvt::EVT_FREQ_DOWN_FAST:
					increment = -TUNE_INCREMENT_FAST;
					break;

				case UiEvt::EVT_FREQ_DOWN_XFAST:
					increment = -TUNE_INCREMENT_XFAST;
					break;

				case UiEvt::EVT_FREQ_RESET:
					newFreq = band::getMinByBand(band);
					break;

				default:
					xthrow("UI event %d is unexpected here", evt.type);
					break;
			}

			if(increment) {
				newFreq += increment;
				if(newFreq < band::getMinByBand(band)) {
					newFreq = band::getMinByBand(band);
				}
				else if(newFreq > band::getMaxByBand(band)) {
					newFreq = band::getMaxByBand(band);
				}
			}

			cat->setFreq(newFreq);
			curFreq = newFreq;
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

		case UiEvt::EVT_FAN_MODE:
			xassert(evt.fanMode, "Fan mode expected, but not found");

			if(!cat) {
				ui.print("CAT disabled");
				break;
			}

			cat->setFanMode(evt.fanMode.value());
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
			unsigned presetNo(evt.preset.value());
			if(presetNo == 0) {
				presetNo = 9;
			}
			else {
				--presetNo;
			}

			const std::string preset(presets->getPreset(presetNo, exchange->get()));
			ui.print("Sending preset: %s", preset.c_str());
			keyer->send(preset);
			break;
		}

		case UiEvt::EVT_SHOW_XCHG:
			if(!exchange) {
				ui.print("Exchange support disabled");
				break;
			}
			ui.print("Next exchange: %s", exchange->get().c_str());
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

			if(!curFreq || !curMode) {
				ui.print("Current freq or mode unknown yet (no CAT?), cannot log");
				break;
			}

			if(logger->exists(evt.logCall.value())) {
				ui.print("Warning: call %s already exists in log", evt.logCall.value().c_str());
			}

			Logger::Entry e;
			if(frozenTime) {
				e.ts = frozenTime.value();
				frozenTime.reset();
			}
			else {
				e.ts = time(nullptr);
			}

			e.freq = curFreq.value();
			e.mode = curMode.value();
			if(e.mode == MODE_CW_1 || e.mode == MODE_CW_2) {
				e.sentRst = "599";
			}
			else {
				e.sentRst = "59";
			}
			e.sentXchg = exchange->get();
			e.rcvdCall = evt.logCall.value();
			if(evt.logRst) {
				e.rcvdRst = evt.logRst.value();
			}
			else {
				e.rcvdRst = e.sentRst;
			}
			e.rcvdXchg = evt.logXchg.value();

			ui.print("%s", logger->log(e).c_str());
			if(exchange->next()) {
				ui.print("Log entry written, next exchange: %s", exchange->get().c_str());
			}
			else {
				ui.print("Log entry written, exchange did not change");
			}
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

			if(curFreq) {
				ui.print("Zeroing in from %s", util::formatFreq(curFreq.value()).c_str());
			}
			else {
				ui.print("Zeroing in");
			}

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
				case meters::METER_IDD:
					if(!evt.meter.value().second) {
						/* If IDD = 0, then we're in RX mode:
						 * - read signal
						 * - read freq
						 * - read mode
						 */
						cat->getMeter(meters::METER_SIG);
					}
					else {
						/* IDD != 0, we're in TX mode:
						 * - store IDD
						 * - read ALC
						 * - read COMP
						 * - read PWR
						 * - read SWR
						 */
						schedMeters[evt.meter.value().first] = evt.meter.value().second;
						cat->getMeter(meters::METER_ALC);
					}
					break;

				case meters::METER_SIG:
					schedMeters[evt.meter.value().first] = evt.meter.value().second;
					cat->getFreq();
					break;

				case meters::METER_ALC:
					schedMeters[evt.meter.value().first] = evt.meter.value().second;
					cat->getMeter(meters::METER_COMP);
					break;

				case meters::METER_COMP:
					schedMeters[evt.meter.value().first] = evt.meter.value().second;
					cat->getMeter(meters::METER_PWR);
					break;

				case meters::METER_PWR:
					schedMeters[evt.meter.value().first] = evt.meter.value().second;
					cat->getMeter(meters::METER_SWR);
					break;

				case meters::METER_SWR:
					schedMeters[evt.meter.value().first] = evt.meter.value().second;
					ui.updateMeters(schedMeters, curFreq, curMode);
					schedMeters.clear();
					catMeterTimer->start();
					break;

				default:
					xthrow("Unknown meter %d", evt.meter.value().first);
					break;
			}
			break;

		case CatEvt::EVT_FREQ:
			xassert(evt.freq, "Expecting frequency in event");
			curFreq = evt.freq;
			cat->getMode();
			break;

		case CatEvt::EVT_MODE:
			xassert(evt.mode, "Expecting mode in event");
			curMode = evt.mode;
			ui.updateMeters(schedMeters, curFreq, curMode);
			schedMeters.clear();
			catMeterTimer->start();
			break;

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
