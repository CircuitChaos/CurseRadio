#pragma once

class Fd {
public:
	Fd(int fd = -1);
	~Fd();

	operator int() const;
	void reset(int newfd);

private:
	int fd;
};
