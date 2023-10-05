#include "constants.h"

void update_row(Row *row);

void insert_row(int at, char *s, size_t len);

void free_row(Row *row);

void del_row(int at);

void row_insert_char(Row *row, int at, int c);

void row_append_string(Row *row, char *s, size_t len);

void row_del_char(Row *row, int at);