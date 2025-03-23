#include "curseswindow.h"

CursesWindow::~CursesWindow()
{
	if(wnd) {
		delwin(wnd);
	}
}

CursesWindow &CursesWindow::operator=(WINDOW *w)
{
	wnd = w;
	return *this;
}

CursesWindow::operator WINDOW *()
{
	return wnd;
}
