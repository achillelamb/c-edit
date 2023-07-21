#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

struct termios initial;

void disable_raw_mode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &initial);
}

void enable_raw_mode() {
  tcgetattr(STDIN_FILENO, &initial);
  atexit(disable_raw_mode);
  struct termios raw = initial;
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= CS8;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
  enable_raw_mode();
  char c;
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
    if (iscntrl(c)) {
      printf("%d\r\n", c);
    } else {
      printf("%d ('%c')\r\n", c, c);
    }
  }
  return 0;
}