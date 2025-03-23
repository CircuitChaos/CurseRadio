#include <unistd.h>
#include <sys/socket.h>
#include "socketpair.h"
#include "throw.h"

SocketPair::SocketPair()
{
	xassert(!socketpair(AF_UNIX, SOCK_STREAM, 0, sv), "Could not create socket pair: %m");
	xassert(sv[0] >= 0 && sv[1] >= 0, "One of sockets from socket pair is invalid");
}

SocketPair::~SocketPair()
{
	close1();
	close0();
}

int SocketPair::get0() const
{
	return sv[0];
}

int SocketPair::get1() const
{
	return sv[1];
}

void SocketPair::close0()
{
	doClose(0);
}

void SocketPair::close1()
{
	doClose(1);
}

void SocketPair::doClose(int which)
{
	mutex.lock();
	if(sv[which] != -1) {
		close(sv[which]);
		sv[which] = -1;
	}
	mutex.unlock();
}
