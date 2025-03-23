#include "curseswindow.h"

CursesWindow::~CursesWindow()
{
	if(win) {
		delwin(win);
	}
}

CursesWindow &CursesWindow::operator=(WINDOW *w)
{
	win = w;
	return *this;
}

CursesWindow::operator WINDOW *()
{
	return win;
}
