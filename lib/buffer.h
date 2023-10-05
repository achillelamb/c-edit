#include "constants.h"

#ifndef BUFFER_H
#define BUFFER_H
struct Buffer {
  char *buf;
  int len;
};

void buf_append(struct Buffer *ab, const char *s, int len);

void buf_free(struct Buffer *ab);
#endif