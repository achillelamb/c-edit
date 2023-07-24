/*** includes ***/
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

/*** defines ***/

#define QUIT 'q'
#define CTRL_KEY(k) ((k) & 0x1f)
#define ESCAPE_PREFIX "\x1b["
#define BUFFER_INIT {NULL, 0}
#define VERSION "0.02.3"

enum Keys {
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  PAGE_UP,
  PAGE_DOWN,
  HOME_KEY,
  END_KEY,
};

/*** data ***/
struct Config {
  int cx, cy;
  int screenrows;
  int screencols;
  struct termios initial;
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

void draw_rows(int rows, struct Buffer *b) {
  int y;
  for (y = 0; y < rows; y++) {
    if (y == rows / 3) {
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
    clearln(b);
    if (y < rows - 1) {
      buf_append(b, "\r\n", 2);
    }
  }
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
  switch (key)
  {
  case ARROW_LEFT:
    if (configuration.cx > 0)
      configuration.cx--;
    break;
  case ARROW_RIGHT:
    if (configuration.cx < configuration.screencols - 1)
      configuration.cx++;
    break;
  case ARROW_UP:
    if (configuration.cy > 0)
      configuration.cy--;
    break;
  case ARROW_DOWN:
    if (configuration.cy < configuration.screenrows - 1)
      configuration.cy++;
    break;
  default:
    break;
  }
}

void refresh_screen() {
  struct Buffer buf = BUFFER_INIT;
  hide_cursor(&buf);
  reset_cursor(&buf);
  draw_rows(configuration.screenrows, &buf);

  char mv_cursor[32];
  snprintf(mv_cursor, sizeof(mv_cursor), "\x1b[%d;%dH", configuration.cy + 1, configuration.cx + 1);
  buf_append(&buf, mv_cursor, strlen(mv_cursor));

  show_cursor(&buf);
  write(STDOUT_FILENO, buf.buf, buf.len);
  buf_free(&buf);
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
      configuration.cx = configuration.screencols - 1;
      break;
    case PAGE_UP:
    case PAGE_DOWN:
      {
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
  if (get_window_size(&configuration.screenrows, &configuration.screencols) == -1) die("get_window_size");
}

int main() {
  enable_raw_mode();
  init_editor();
  while (1) {
    refresh_screen();
    editor_process_keypress();
  }
  return 0;
}