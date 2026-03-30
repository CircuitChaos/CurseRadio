#pragma once

#include <string>
#include <ctime>
#include <cstdint>
#include "mode.h"
#include "ui.h"

class Logger {
public:
	struct Entry {
		time_t ts;
		uint32_t freq;
		Mode mode;
		std::string sentRst;
		std::string sentXchg;
		std::string rcvdCall;
		std::string rcvdRst;
		std::string rcvdXchg;
	};

	Logger(const std::string &call, const std::string &cbrFile);

	std::string log(const Entry &e);
	bool checkIfExists(Ui *ui, const std::string &call, bool exactMatch) const;

private:
	const std::string call;
	const std::string cbrFile;
};
