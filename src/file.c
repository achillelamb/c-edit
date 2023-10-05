

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include "../lib/file.h"
#include "../lib/row.h"
#include "../lib/terminal.h"
#include "../lib/output.h"

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
    insert_row(configuration.num_rows, line, linelen);
  }
  free(line);
  fclose(fp);
  configuration.dirty = 0;
}

char *rows_to_string(int *buflen) {
  int totlen = 0;
  int j;
  for (j = 0; j < configuration.num_rows; j++)
    totlen += configuration.row[j].size + 1;
  *buflen = totlen;
  char *buf = malloc(totlen);
  char *p = buf;
  for (j = 0; j < configuration.num_rows; j++) {
    memcpy(p, configuration.row[j].chars, configuration.row[j].size);
    p += configuration.row[j].size;
    *p = '\n';
    p++;
  }
  return buf;
}

char *editor_prompt(char *prompt) {
  size_t bufsize = 1 << 7;
  char *buf = malloc(bufsize);
  size_t buflen = 0;
  buf[0] = '\0';
  while (1) {
    set_status_message(prompt, buf);
    refresh_screen();
    int c = editor_read_key();
    if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
      if (buflen != 0) buf[--buflen] = '\0';
    } else if (c == ESC) {
      set_status_message("");
      free(buf);
      return NULL;
    } else if (c == CARRIAGE) {
      if (buflen != 0) {
        set_status_message("");
        return buf;
      }
    } else if (!iscntrl(c) && c < 1 << 7) {
      if (buflen == bufsize - 1) {
        bufsize *= 2;
        buf = realloc(buf, bufsize);
      }
      buf[buflen++] = c;
      buf[buflen] = '\0';
    }
  }
}


void save_file() {
  if (configuration.file_name == NULL) {
    configuration.file_name = editor_prompt("Save as (ESC to cancel): %s");
      if (configuration.file_name == NULL) {
        set_status_message("Save aborted");
        return;
      }
  }
  int len;
  char *buf = rows_to_string(&len);
  int file_descriptor = open(configuration.file_name, O_RDWR | O_CREAT, 0644);
  if (file_descriptor != -1) {
    if (ftruncate(file_descriptor, len) != -1) {
      if (write(file_descriptor, buf, len) == len) {
        close(file_descriptor);
        free(buf);
        configuration.dirty = 0;
        set_status_message("%d bytes written on disk", len);
        return;
      }
    }
    close(file_descriptor);
  }
  free(buf);
  set_status_message("Cannot save! I/O Error: %s", strerror(errno));
}