#include "constants.h"

void die(char *s);

void disable_raw_mode();

void enable_raw_mode();

int editor_read_key();

int get_window_size(int *rows, int *cols);