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

/*** data ***/
struct Config {
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

void draw_rows(int rows, struct Buffer *b) {
  int y;
  for (y = 0; y < rows; y++) {
    buf_append(b, "~", 1);
    if (y < rows - 1) {
      buf_append(b, "\r\n", 2);
    }
  }
}

void clear_screen(struct Buffer *b) {
  char clear[4] = ESCAPE_PREFIX;
  buf_append(b, strcat(clear, "2J"), 4);
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

void refresh_screen() {
  struct Buffer buf = BUFFER_INIT;
  hide_cursor(&buf);
  clear_screen(&buf);
  reset_cursor(&buf);
  draw_rows(configuration.screenrows, &buf);
  reset_cursor(&buf); 
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

char editor_read_key() {
  int nread;
  char c; 
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }
  return c;
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
  char c = editor_read_key();
  switch (c) {
    case CTRL_KEY(QUIT):
      refresh_screen();
      exit(0);
      break;
  }
}

/*** init ***/

void init_editor() {
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