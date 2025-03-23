#pragma once

#include <ncurses.h>

class CursesWindow {
public:
	~CursesWindow();
	CursesWindow &operator=(WINDOW *w);
	operator WINDOW *();

private:
	WINDOW *wnd{nullptr};
};
