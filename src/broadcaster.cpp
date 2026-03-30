#include <algorithm>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include "broadcaster.h"
#include "util.h"
#include "throw.h"

BroadcastPacket::BroadcastPacket(const std::string &type) : type(type)
{
}

void BroadcastPacket::add(const std::string &key, const std::string &value)
{
	data[key] = value;
}

void BroadcastPacket::packString(std::vector<unsigned char> &packed, const std::string &s)
{
	xassert(s.find((char) 0) == std::string::npos, "NUL in string");
	std::copy(s.begin(), s.end(), std::back_inserter(packed));
	packed.push_back(0);
}

std::vector<unsigned char> BroadcastPacket::pack() const
{
	std::vector<unsigned char> rs;
	packString(rs, type);
	for(std::map<std::string, std::string>::const_iterator i = data.begin(); i != data.end(); ++i) {
		packString(rs, i->first);
		packString(rs, i->second);
	}
	return rs;
}

class GaiGuard {
public:
	GaiGuard(addrinfo *ai) : ai(ai)
	{
	}

	~GaiGuard()
	{
		freeaddrinfo(ai);
	}

	addrinfo *get() const
	{
		return ai;
	}

private:
	addrinfo *ai;
};

Broadcaster::Broadcaster(const std::string &host, const std::string &port)
{
	addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	addrinfo *ai;
	const int gaierr = getaddrinfo(host.c_str(), port.c_str(), &hints, &ai);
	xassert(!gaierr, "getaddrinfo() error: %s", gai_strerror(gaierr));

	GaiGuard gai(ai);

	int xfd = -1;
	addrinfo *p;
	for(p = ai; p; p = p->ai_next) {
		xfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(xfd != -1) {
			break;
		}
	}

	xassert(xfd, "Could not create any socket");
	fd.reset(xfd);
	xassert(p, "p is null but shouldn't be");
	xassert(p->ai_addr, "ai_addr is not set");
	xassert(p->ai_addrlen <= sizeof(addr), "ai_addrlen too large");

	const int bc = 1;
	xassert(setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &bc, sizeof(bc)) == 0, "setsockopt(SO_BROADCAST) failed: %m");

	memcpy(&addr, p->ai_addr, p->ai_addrlen);
	addrlen = p->ai_addrlen;

	BroadcastPacket bp("hello");
	sendPacket(bp);
}

Broadcaster::~Broadcaster()
{
	BroadcastPacket p("bye");
	sendPacket(p);
}

void Broadcaster::sendPacket(const BroadcastPacket &packet)
{
	const std::vector<unsigned char> data = packet.pack();
	xassert(sendto(fd, &data[0], data.size(), 0, &addr, addrlen) != -1, "sendto() failed: %m");
}
