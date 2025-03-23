#include "file.h"

File::File(FILE *fp)
    : fp(fp)
{
}

File::~File()
{
	if(fp) {
		fclose(fp);
	}
}

File::operator FILE *() const
{
	return fp;
}
