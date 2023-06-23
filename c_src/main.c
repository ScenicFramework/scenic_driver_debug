/*
#  Created by Boyd Multerer on 05/17/18.
#  Copyright © 2018 Kry10 Industries. All rights reserved.
#
*/

#include <unistd.h>

#include <stdio.h>
// #include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <stdint.h>
#include <assert.h>

#include "comms.h"
#include "types.h"
#include "image.h"
#include "font.h"
#include "script.h"

#include "device/device.h"

#define STDIN_FILENO 0
#define DEFAULT_SCREEN 0

device_info_t g_device_info = {0};

//---------------------------------------------------------
int main(int argc, char **argv)
{
  driver_data_t data = {
      .keep_going = true,
      .input_flags = 0,
      .last_x = 0,
      .last_y = 0,
      .root_script = 0,
      .p_tx_ids = NULL,
      .p_fonts = NULL,
      .global_tx = {1, 0, 0, 1, 0, 0},
      .cursor_tx = {1, 0, 0, 1, 0, 0},
      .cursor_pos = {0, 0},
      .f_show_cursor = 0};

  // super simple arg check
  if (argc != 10)
  {
    log_error("Wrong number of parameters");
    return 0;
  }

  // ingest the command line options
  device_opts_t opts = {0};
  opts.cursor = atoi(argv[1]);
  opts.layer = atoi(argv[2]);
  opts.global_opacity = atoi(argv[3]);
  opts.antialias = atoi(argv[4]);
  opts.debug_mode = atoi(argv[5]);
  opts.width = atoi(argv[6]);
  opts.height = atoi(argv[7]);
  opts.resizable = atoi(argv[8]);
  opts.title = argv[9];

  // init the hashtables
  init_scripts();
  init_fonts();
  init_images();

  int err = device_init(&opts, &g_device_info);
  if (err)
  {
    send_puts("Failed to initialize the device");
    return err;
  }

  // signal the app that the window is ready
  send_ready();

  /* Loop until the calling app closes the window */
  while (data.keep_going && !isCallerDown())
  {
    // check for incoming messages - blocks with a timeout
    handle_stdio_in(&data);
    device_poll();
  }

  device_close(&g_device_info);
  return 0;
}
