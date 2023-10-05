#include "constants.h"
#include "buffer.h"

void clearln(struct Buffer *b);

int row_cx_to_render_x(Row *row, int cx);

void scroll();

void draw_rows(int rows, struct Buffer *b);

void draw_status_bar(struct Buffer *b);

void draw_message_bar(struct Buffer *b);

void reset_cursor(struct Buffer *b);

void hide_cursor(struct Buffer *b);

void show_cursor(struct Buffer *b);

void move_cursor(int key);

void refresh_screen();

void set_status_message(const char *fmt, ...);