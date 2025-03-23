#include <unistd.h>
#include "cli.h"
#include "throw.h"
#include "version.h"

void Cli::help()
{
	static const char helpstr[] =
	    "\n"
	    "Syntax: curseradio [options]\n"
	    "\n"
	    "Options:\n"
	    "  -h: show help and exit\n"
	    "  -v: show version and exit\n"
	    "  -s <callsign>: your callsign (for presets and logging)\n"
	    "  -f <file>: Cabrillo file for logging\n"
	    "  -c <port>: radio CAT port\n"
	    "  -b <rate>: radio CAT port baudrate\n"
	    "  -p <port>: radio PTT port\n"
	    "  -w <wpm>: initial keyer speed\n"
	    "  -P <prefix>: contest exchange prefix\n"
	    "  -I <infix>: contest exchange infix\n"
	    "  -S <suffix>: contest exchange suffix\n"
	    "\n"
	    "Exchange prefix and suffix are fixed parts of the exchange. They can \n"
	    "be skipped if only numbers are exchanged.\n"
	    "\n"
	    "Exchange infix is the variable, numeric part of the exchange. It is \n"
	    "incremented in every logged QSO. It can also be empty (then this \n"
	    "feature is disabled). Prefix it with zeroes to enforce certain minimum \n"
	    "length of the exchange (for example, 001).\n"
	    "\n"
	    "If Cabrillo file is specified, then header and footer has to be added \n"
	    "to it manually. Program rereads and rewrites Cabrillo file every time \n"
	    "a QSO is logged.\n"
	    "\n"
	    "If callsign is not specified, then presets and logging will be disabled.\n"
	    "If Cabrillo file is not specified, then logging will be disabled.\n"
	    "If CAT port is not specified, then radio functions will be disabled.";

	version();
	puts(helpstr);
}

void Cli::version()
{
	printf("CurseRadio version %s built %s\n", version::getVersion().c_str(), version::getBuild().c_str());
	printf("Newest version: https://github.com/CircuitChaos/CurseRadio\n");
}

Cli::Cli(int argc, char *const argv[])
{
	int opt;
	while((opt = getopt(argc, argv, ":hvs:f:c:b:p:w:P:I:S:")) != -1 && !exitFlag) {
		switch(opt) {
			case '?':
				xthrow("-%c: option not recognized", optopt);
				break;

			case ':':
				xthrow("-%c: option requires argument", optopt);
				break;

			case 'h':
				help();
				exitFlag = true;
				break;

			case 'v':
				version();
				exitFlag = true;
				break;

			case 's':
				callsign = optarg;
				break;

			case 'f':
				cbrFile = optarg;
				break;

			case 'c':
				catPort = optarg;
				break;

			case 'b':
				/* No sanity checking, cat class will do it */
				catBaud = atoi(optarg);
				break;

			case 'p':
				pttPort = optarg;
				break;

			case 'w':
				/* No sanity checking, keyer class will do it */
				wpm = atoi(optarg);
				break;

			case 'P':
				prefix = optarg;
				break;

			case 'I':
				infix = optarg;
				break;

			case 'S':
				suffix = optarg;
				break;

			default:
				xthrow("Unknown value returned by getopt(): %d", opt);
				break;
		}
	}

	xassert(optind == argc, "Excessive arguments");
}

bool Cli::shouldExit() const
{
	return exitFlag;
}

std::string Cli::getCatPort() const
{
	return catPort;
}

std::string Cli::getPttPort() const
{
	return pttPort;
}

unsigned Cli::getCatBaud() const
{
	return catBaud;
}

std::string Cli::getCbrFile() const
{
	return cbrFile;
}

std::string Cli::getCallsign() const
{
	return callsign;
}

std::string Cli::getPrefix() const
{
	return prefix;
}

std::string Cli::getInfix() const
{
	return infix;
}

std::string Cli::getSuffix() const
{
	return suffix;
}

unsigned Cli::getWpm() const
{
	return wpm;
}
