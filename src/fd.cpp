#include <unistd.h>
#include "fd.h"

Fd::Fd(int fd)
    : fd(fd)
{
}

Fd::~Fd()
{
	if(fd != -1) {
		close(fd);
	}
}

Fd::operator int() const
{
	return fd;
}
