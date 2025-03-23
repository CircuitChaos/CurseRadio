#pragma once

#include <mutex>

class SocketPair {
public:
	SocketPair();
	~SocketPair();

	int get0() const;
	int get1() const;

	/* These functions are thread-safe */
	void close0();
	void close1();

private:
	std::mutex mutex;
	int sv[2];

	void doClose(int which);
};
