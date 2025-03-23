#pragma once

#include <string>
#include "fd.h"

class Ptt {
public:
	Ptt(const std::string &pttPort);
	~Ptt();

	void keyDown();
	void keyUp();

private:
	Fd fd;
	bool setDtr(bool on);
};
