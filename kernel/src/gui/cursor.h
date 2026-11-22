#ifndef ZENITH_GUI_CURSOR_H
#define ZENITH_GUI_CURSOR_H

void cursor_draw(int x, int y);
void cursor_restore(void);
void cursor_move(int x, int y);
void cursor_invalidate(void);

#endif