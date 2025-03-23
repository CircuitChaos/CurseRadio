#pragma once

#include <cstdio>

class File {
public:
	File(FILE *fp);
	~File();

	operator FILE *() const;

private:
	FILE *fp;
};
