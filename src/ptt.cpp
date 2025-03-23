#include <fcntl.h>
#include <sys/ioctl.h>
#include "throw.h"
#include "ptt.h"

Ptt::Ptt(const std::string &pttPort)
    : fd(open(pttPort.c_str(), O_RDWR | O_NOCTTY))
{
	xassert(setDtr(false), "Could not clear DTR: %m");
}

Ptt::~Ptt()
{
	setDtr(false);
}

void Ptt::keyDown()
{
	xassert(setDtr(true), "Could not set DTR: %m");
}

void Ptt::keyUp()
{
	xassert(setDtr(false), "Could not clear DTR: %m");
}

bool Ptt::setDtr(bool on)
{
	const int dtr(TIOCM_DTR);
	return ioctl(fd, on ? TIOCMBIS : TIOCMBIC, &dtr) == 0;
}
