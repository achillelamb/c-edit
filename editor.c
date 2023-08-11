/*** includes ***/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <time.h>
#include <stdarg.h>

/*** defines ***/

#define QUIT 'q'
#define CTRL_KEY(k) ((k) & 0x1f)
#define ESCAPE_PREFIX "\x1b["
#define BUFFER_INIT {NULL, 0}
#define VERSION "0.02.3"
#define TAB '\t'
#define TAB_LEN 4
#define SPACE ' '

enum Keys {
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
  PAGE_UP,
  PAGE_DOWN,
  HOME_KEY,
  END_KEY,
};

/*** data ***/

typedef struct Render {
  int size;
  char *render;
} Render;
typedef struct Row {
  int size;
  char *chars;
  Render render;
} Row;

struct Config {
  int cx, cy;
  int render_x;
  int screenrows;
  int screencols;
  struct termios initial;
  int num_rows;
  Row *row;
  int row_offset;
  int col_offset;
  char *file_name;
  char status_msg[80];
  time_t status_time;
};

struct Config configuration;

/*** append buffer ***/

struct Buffer {
  char *buf;
  int len;
};


void buf_append(struct Buffer *ab, const char *s, int len) {
  char *new = realloc(ab->buf, ab->len + len);
  if (new == NULL) return;
  memcpy(&new[ab->len], s, len);
  ab->buf = new;
  ab->len += len;
}

void buf_free(struct Buffer *ab) {
  free(ab->buf);
}


/*** output ***/

void clearln(struct Buffer *b) {
  char clear[3] = ESCAPE_PREFIX;
  buf_append(b, strcat(clear, "K"), 3);
}


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
  int len = snprintf(status, sizeof(status), "%.20s - %d lines",
    configuration.file_name ? configuration.file_name : "[New File]", configuration.num_rows);
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

/*** terminal ***/

void die(char *s) {
  refresh_screen();
  perror(s);
  exit(1);
}

void disable_raw_mode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &configuration.initial) == -1) {
    die("tcsetattr");
  }
}

void enable_raw_mode() {
  if (tcgetattr(STDIN_FILENO, &configuration.initial) == -1) die("tcgetattr");
  atexit(disable_raw_mode);
  struct termios raw = configuration.initial;
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= CS8;
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1; // the unit has a value of 100 ms
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int editor_read_key() {
  int nread;
  char c; 
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }
  if (c == ESCAPE_PREFIX[0]) {
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return ESCAPE_PREFIX[0];
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return ESCAPE_PREFIX[0];
    if (seq[0] == ESCAPE_PREFIX[1]) {
      if (seq[1] >= '0' && seq[1] <= '9') {
        if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
        if (seq[2] == '~') {
          switch (seq[1]) {
            case '1': return HOME_KEY;
            case '3': return DEL_KEY;
            case '4': return END_KEY;
            case '5': return PAGE_UP;
            case '6': return PAGE_DOWN;
            case '7': return HOME_KEY;
            case '8': return END_KEY;
          }
        }
      } else {
        switch (seq[1]) {
          case 'A': return ARROW_UP;
          case 'B': return ARROW_DOWN;
          case 'C': return ARROW_RIGHT;
          case 'D': return ARROW_LEFT;
          case 'H': return HOME_KEY;
          case 'F': return END_KEY;
        }
      }
    } else if (seq[0] == 'O') {
      switch (seq[1]) {
        case 'H': return HOME_KEY;
        case 'F': return END_KEY;
      }
    }
    return ESCAPE_PREFIX[0];
  } else {
    return c;
  }
}

int get_window_size(int *rows, int *cols) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    return -1;
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/*** row operations ***/

void update_row(Row *row) {
  int tabs = 0;
  for (int j = 0; j < row->size; j++) {
    if (row->chars[j] == TAB) {
      tabs++;
    }
  }
  free(row->render.render);
  row->render.render = malloc(row->size + tabs*(TAB_LEN - 1) + 1);
  int idx = 0;
  for (int j = 0; j < row->size; j++) {
    if (row->chars[j] == TAB) {
      do {
        row->render.render[idx++] = SPACE;
      } while (idx % TAB_LEN != 0);
    } else {
      row->render.render[idx++] = row->chars[j];
    }
  }
  row->render.render[idx] = '\0';
  row->render.size = idx;
}

void append_row(char *s, size_t len) {
  configuration.row = realloc(configuration.row, sizeof(Row) * (configuration.num_rows + 1));
  int at = configuration.num_rows;
  configuration.row[at].size = len;
  configuration.row[at].chars = malloc(len + 1);
  memcpy(configuration.row[at].chars, s, len);
  configuration.row[at].chars[len] = '\0';
  configuration.row[at].render.size = 0;
  configuration.row[at].render.render = NULL;
  update_row(&configuration.row[at]);
  configuration.num_rows++;
}

/*** file i/o ***/

void open_file(char *filename) {
  free(configuration.file_name);
  configuration.file_name = strdup(filename);
  FILE *fp = fopen(filename, "r");
  if (!fp) die("fopen");
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  while ((linelen = getline(&line, &linecap, fp)) != -1) {
    while (linelen > 0 && (line[linelen - 1] == '\n' ||
                           line[linelen - 1] == '\r'))
      linelen--;
    append_row(line, linelen);
  }
  free(line);
  fclose(fp);
}

/*** input ***/

void editor_process_keypress() {
  int c = editor_read_key();
  switch (c) {
    case CTRL_KEY(QUIT):
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
  }
}

/*** init ***/

void init_editor() {
  configuration.cx = 0;
  configuration.cy = 0;
  configuration.num_rows = 0;
  configuration.row = NULL;
  configuration.row_offset = 0;
  configuration.col_offset = 0;
  configuration.render_x = 0;
  configuration.file_name = NULL;
  configuration.status_msg[0] = '\0';
  configuration.status_time = 0;
  if (get_window_size(&configuration.screenrows, &configuration.screencols) == -1) die("get_window_size");
  configuration.screenrows--; // status bar
  configuration.screenrows--; // status message
}

int main(int argc, char *argv[]) {
  enable_raw_mode();
  init_editor();
  if (argc >= 2){
    open_file(argv[1]);
  }

  set_status_message("Ctrl-Q: quit");
  while (1) {
    refresh_screen();
    editor_process_keypress();
  }
  return 0;
}
