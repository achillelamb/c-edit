#include "../lib/output.h"

int row_cx_to_render_x(Row *row, int cx) {
  int rx = 0;
  for (int j = 0; j < cx; j++) {
    if (row->chars[j] == TAB)
      rx += (TAB_LEN - 1) - (rx % TAB_LEN);
    rx++;
  }
  return rx;
}

void scroll() {
  configuration.render_x = 0;
  if (configuration.render_x < configuration.num_rows) {
    configuration.render_x = row_cx_to_render_x(&configuration.row[configuration.cy], configuration.cx);
  }
  if (configuration.cy < configuration.row_offset) {
    configuration.row_offset = configuration.cy;
  }
  if (configuration.cy >= configuration.row_offset + configuration.screenrows) {
    configuration.row_offset = configuration.cy - configuration.screenrows + 1; 
  }
  if (configuration.render_x < configuration.col_offset) {
    configuration.col_offset = configuration.render_x;
  }
  if (configuration.render_x >= configuration.col_offset + configuration.screencols) {
    configuration.col_offset = configuration.render_x - configuration.screencols + 1;
  }
}

void draw_rows(int rows, struct Buffer *b) {
  int y;
  for (y = 0; y < rows; y++) {
    int filerow = y + configuration.row_offset;
    if (filerow >= configuration.num_rows) {
      if (configuration.num_rows == 0 && y == rows / 3) {
        char welcome[80];
        int welcomelen = snprintf(welcome, sizeof(welcome),
          "c-edit -- version %s", VERSION);
        if (welcomelen > configuration.screencols) welcomelen = configuration.screencols;
        int padding = (configuration.screencols - welcomelen) / 2;
        if (padding) {
          buf_append(b, "~", 1);
          padding--;
        }
        while (padding--) buf_append(b, " ", 1);
        buf_append(b, welcome, welcomelen);
      } else {
      buf_append(b, "~", 1);
      }
    } else {
      int len = configuration.row[filerow].render.size - configuration.col_offset;
      if (len < 0) len = 0;
      if (len > configuration.screencols) len = configuration.screencols;
      buf_append(b, &configuration.row[filerow].render.render[configuration.col_offset], len);
    }

    clearln(b);
    
    buf_append(b, "\r\n", 2);
    
  }
}

void draw_status_bar(struct Buffer *b) {
  char invert_colors[4] = ESCAPE_PREFIX;
  buf_append(b, strcat(invert_colors, "7m"), 4);
  char status[80], rstatus[80];
  int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
    configuration.file_name ? configuration.file_name : "[New File]", configuration.num_rows,
    configuration.dirty ? "(modified)" : "");
  int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d",
    configuration.cy + 1, configuration.num_rows);
  if (len > configuration.screencols) len = configuration.screencols;
  buf_append(b, status, len);
  while (len < configuration.screencols) {
    if (configuration.screencols - len == rlen) {
      buf_append(b, rstatus, rlen);
      break;
    } else {
      buf_append(b, " ", 1);
      len++;
    }
  }
  char restore_colors[3] = ESCAPE_PREFIX;
  buf_append(b, strcat(restore_colors, "m"), 3);
  buf_append(b, "\r\n", 2);
}

void draw_message_bar(struct Buffer *b) {
  clearln(b);
  int len = strlen(configuration.status_msg);
  if (len > configuration.screencols) len = configuration.screencols;
  if (len && time(NULL) - configuration.status_time < 5)
    buf_append(b, configuration.status_msg, len);
}

void clearln(struct Buffer *b) {
  char clear[3] = ESCAPE_PREFIX;
  buf_append(b, strcat(clear, "K"), 3);
}

void reset_cursor(struct Buffer *b) {
  char cursor_pos[3] = ESCAPE_PREFIX;
  buf_append(b, strcat(cursor_pos, "H"), 3);
}

void hide_cursor(struct Buffer *b) {
  char hide[6] = ESCAPE_PREFIX;
  buf_append(b, strcat(hide, "?25l"), 6);
}

void show_cursor(struct Buffer *b) {
  char show[6] = ESCAPE_PREFIX;
  buf_append(b, strcat(show, "?25h"), 6);
}

void move_cursor(int key) {
  Row *row = (configuration.cy >= configuration.num_rows) ? NULL : &configuration.row[configuration.cy];
  switch (key)
  {
  case ARROW_LEFT:
    if (configuration.cx > 0)
      configuration.cx--;
    else if (configuration.cy > 0) {
      configuration.cy--;
      configuration.cx = configuration.row[configuration.cy].size;
    }
    break;
  case ARROW_RIGHT:
      if (row && configuration.cx < row->size)
        configuration.cx++;
      else if (row && configuration.cx == row->size) {
        configuration.cy++;
        configuration.cx = 0;
      }
    break;
  case ARROW_UP:
    if (configuration.cy > 0)
      configuration.cy--;
    break;
  case ARROW_DOWN:
    if (configuration.cy < configuration.num_rows)
      configuration.cy++;
    break;
  default:
    break;
  }

  row = (configuration.cy >= configuration.num_rows) ? NULL : &configuration.row[configuration.cy];
  int rowlen = row ? row->size : 0;
  if (configuration.cx > rowlen) {
    configuration.cx = rowlen;
  }
}

void refresh_screen() {
  scroll();
  struct Buffer buf = BUFFER_INIT;
  hide_cursor(&buf);
  reset_cursor(&buf);
  draw_rows(configuration.screenrows, &buf);
  draw_status_bar(&buf);
  draw_message_bar(&buf);

  char mv_cursor[32];
  snprintf(mv_cursor, sizeof(mv_cursor), "\x1b[%d;%dH", (configuration.cy - configuration.row_offset) + 1, 
                                                        (configuration.render_x - configuration.col_offset) + 1);
  buf_append(&buf, mv_cursor, strlen(mv_cursor));

  show_cursor(&buf);
  write(STDOUT_FILENO, buf.buf, buf.len);
  buf_free(&buf);
}

void set_status_message(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(configuration.status_msg, sizeof(configuration.status_msg), fmt, ap);
  va_end(ap);
  configuration.status_time = time(NULL);
}