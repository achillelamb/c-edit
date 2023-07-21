#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

struct termios initial;

void disable_raw_mode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &initial);
}

void enable_raw_mode() {
  tcgetattr(STDIN_FILENO, &initial);
  atexit(disable_raw_mode);
  struct termios raw = initial;
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
  enable_raw_mode();
  char c;
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
  return 0;
}