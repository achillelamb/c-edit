/*** includes ***/
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>

/*** defines ***/

#define QUIT 'q'
#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/

struct termios initial;

/*** terminal ***/

void die(char *s) {
  perror(s);
  exit(1);
}

void disable_raw_mode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &initial) == -1) {
    die("tcsetattr");
  }
}

void enable_raw_mode() {
  if (tcgetattr(STDIN_FILENO, &initial) == -1) die("tcgetattr");
  atexit(disable_raw_mode);
  struct termios raw = initial;
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

/*** input ***/

void editor_process_keypress() {
  char c = editor_read_key();
  switch (c) {
    case CTRL_KEY(QUIT):
      exit(0);
      break;
  }
}

/*** init ***/

int main() {
  enable_raw_mode();
  while (1) {
    editor_process_keypress();
  }
  return 0;
}