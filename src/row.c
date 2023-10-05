#include "../lib/row.h"

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

void insert_row(int at, char *s, size_t len) {
  if (at < 0 || at > configuration.num_rows) return;
  configuration.row = realloc(configuration.row, sizeof(Row) * (configuration.num_rows + 1));
  memmove(&configuration.row[at + 1], &configuration.row[at], sizeof(Row) * (configuration.num_rows - at));
  configuration.row[at].size = len;
  configuration.row[at].chars = malloc(len + 1);
  memcpy(configuration.row[at].chars, s, len);
  configuration.row[at].chars[len] = '\0';
  configuration.row[at].render.size = 0;
  configuration.row[at].render.render = NULL;
  update_row(&configuration.row[at]);
  configuration.num_rows++;
  configuration.dirty = 1;
}

void free_row(Row *row) {
  free(row->render.render);
  free(row->chars);
}

void del_row(int at) {
  if (at < 0 || at >= configuration.num_rows) return;
  free_row(&configuration.row[at]);
  memmove(&configuration.row[at], &configuration.row[at + 1], sizeof(Row) * (configuration.num_rows - at - 1));
  configuration.num_rows--;
  configuration.dirty = 1;
}

void row_insert_char(Row *row, int at, int c) {
  if (at < 0 || at > row->size) at = row->size;
  row->chars = realloc(row->chars, row->size + 2);
  memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
  row->size++;
  row->chars[at] = c;
  update_row(row);
  configuration.dirty = 1;
}

void row_append_string(Row *row, char *s, size_t len) {
  row->chars = realloc(row->chars, row->size + len + 1);
  memcpy(&row->chars[row->size], s, len);
  row->size += len;
  row->chars[row->size] = '\0';
  update_row(row);
  configuration.dirty++;
}

void row_del_char(Row *row, int at) { 
  if (at < 0 || at >= row->size) return;
  memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
  row->size--;
  update_row(row);
  configuration.dirty = 1;
}