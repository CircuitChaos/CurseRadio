#pragma once

class Fd {
public:
	Fd(int fd);
	~Fd();

	operator int() const;

private:
	int fd;
};
