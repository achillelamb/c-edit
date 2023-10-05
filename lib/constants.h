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
#include <fcntl.h>


/*** defines ***/

#ifndef CONSTANTS_H
#define CONSTANTS_H

#define QUIT 'q'
#define CTRL_KEY(k) ((k) & 0x1f)
#define ESCAPE_PREFIX "\x1b["
#define BUFFER_INIT {NULL, 0}
#define VERSION "0.0.314"
#define TAB '\t'
#define TAB_LEN 4
#define SPACE ' '
#define CARRIAGE '\r'
#define SAVE 's'
#define QUIT_TIMES 2
#define ESC '\x1b'

enum Keys {
  BACKSPACE = 127,
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
  int dirty;
};

extern struct Config configuration;

#endif // CONSTANTS_H