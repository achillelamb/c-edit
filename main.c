/*** includes ***/
#include "lib/constants.h"
#include "lib/output.h"
#include "lib/buffer.h"
#include "lib/terminal.h"
#include "lib/editor.h"
#include "lib/file.h"

struct Config configuration;

/*** init ***/

void init_editor() {
  configuration.cx = 0;
  configuration.cy = 0;
  configuration.num_rows = 0;
  configuration.row = NULL;
  configuration.row_offset = 0;
  configuration.col_offset = 0;
  configuration.render_x = 0;
  configuration.file_name = NULL;
  configuration.status_msg[0] = '\0';
  configuration.status_time = 0;
  if (get_window_size(&configuration.screenrows, &configuration.screencols) == -1) die("get_window_size");
  configuration.screenrows--; // status bar
  configuration.screenrows--; // status message
  configuration.dirty = 0; 
}

int main(int argc, char *argv[]) {
  enable_raw_mode();
  init_editor();
  if (argc >= 2){
    open_file(argv[1]);
  }

  set_status_message("Ctrl-Q: quit | Ctrl-S: save");
  while (1) {
    refresh_screen();
    editor_process_keypress();
  }
  return 0;
}
