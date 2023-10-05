#include "../lib/editor.h"
#include "../lib/row.h"
#include "../lib/terminal.h"
#include "../lib/output.h"
#include "../lib/file.h"

void editor_insert_char(int c) {
  if (configuration.cy == configuration.num_rows) {
    insert_row(configuration.num_rows, "", 0);
  }
  row_insert_char(&configuration.row[configuration.cy], configuration.cx, c);
  configuration.cx++;
}

void del_char() {
  if (configuration.cy == configuration.num_rows) return;
  if (configuration.cx == 0 && configuration.cy == 0) return;
  Row *row = &configuration.row[configuration.cy];
  if (configuration.cx > 0) {
    row_del_char(row, --configuration.cx);
  } else {
    configuration.cx = configuration.row[configuration.cy - 1].size;
    row_append_string(&configuration.row[configuration.cy - 1], row->chars, row->size);
    del_row(configuration.cy);
    configuration.cy--;
  }
}

void insert_newline() {
  if (configuration.cx == 0) {
    insert_row(configuration.cy, "", 0);
  } else {
    Row *row = &configuration.row[configuration.cy];
    insert_row(configuration.cy + 1, &row->chars[configuration.cx], row->size - configuration.cx);
    row = &configuration.row[configuration.cy];
    row->size = configuration.cx;
    row->chars[row->size] = '\0';
    update_row(row);
  }
  configuration.cy++;
  configuration.cx = 0;
}

void editor_process_keypress() {
  static int quit_times = QUIT_TIMES;
  int c = editor_read_key();
  switch (c) {
    case CARRIAGE:
      insert_newline();
      break;
    case CTRL_KEY(QUIT):
      if (configuration.dirty && quit_times > 0) {
        set_status_message("!!! File has unsaved changes! "
          "Press Ctrl-Q %d more times to quit without saving.", quit_times);
        quit_times--;
        return; // early return such that quit_times is not reset at the end of the function.
      }
      refresh_screen();
      exit(0);
      break;
    case HOME_KEY:
      configuration.cx = 0;
      break;
    case END_KEY:
      if (configuration.cy < configuration.num_rows) {
        configuration.cx = configuration.row[configuration.cy].size;
      }
      break;
    case DEL_KEY:
      move_cursor(ARROW_RIGHT);
      __attribute__ ((fallthrough));
    case BACKSPACE:
    case CTRL_KEY('h'):
      del_char();
      break;
    case PAGE_UP:
    case PAGE_DOWN:
      {
        if (c == PAGE_UP) {
          configuration.cy = configuration.row_offset;
        } else if (c == PAGE_DOWN) {
          configuration.cy = configuration.row_offset + configuration.screenrows - 1;
          if (configuration.cy > configuration.num_rows) configuration.cy = configuration.num_rows;
        }
        int times = configuration.screenrows;
        while (times--)
          move_cursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
      }
      break;
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
      move_cursor(c);
      break;
    case CTRL_KEY(SAVE):
      save_file();
      break;
    case CTRL_KEY('l'):
    case ESC:
      break;
    default: 
      editor_insert_char(c);
      break;
  }
  quit_times = QUIT_TIMES;
}