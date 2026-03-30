#pragma once

#include <string>
#include <vector>
#include <map>
#include <sys/socket.h>
#include "meters.h"
#include "fd.h"

class BroadcastPacket {
public:
	BroadcastPacket(const std::string &type);

	void add(const std::string &key, const std::string &value);
	std::vector<unsigned char> pack() const;

private:
	const std::string type;
	std::map<std::string, std::string> data;

	static void packString(std::vector<unsigned char> &packed, const std::string &s);
};

class Broadcaster {
public:
	Broadcaster(const std::string &host, const std::string &port);
	~Broadcaster();

	void sendPacket(const BroadcastPacket &packet);

private:
	Fd fd;
	sockaddr addr;
	socklen_t addrlen{0};
};
