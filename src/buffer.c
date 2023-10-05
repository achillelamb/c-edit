#include "../lib/buffer.h"

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