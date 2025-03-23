#pragma once

#include <string>

class Cli {
public:
	Cli(int argc, char *const argv[]);

	bool shouldExit() const;
	std::string getCatPort() const;
	unsigned getCatBaud() const;
	std::string getPttPort() const;
	std::string getCbrFile() const;
	std::string getCallsign() const;
	std::string getPrefix() const;
	std::string getInfix() const;
	std::string getSuffix() const;
	unsigned getWpm() const;

private:
	bool exitFlag{false};
	std::string catPort;
	unsigned catBaud{0};
	std::string pttPort;
	std::string cbrFile;
	std::string callsign;
	std::string prefix;
	std::string infix;
	std::string suffix;
	unsigned wpm{0};

	void help();
	void version();
};
