/*
#  Created by Boyd Multerer on 2021-09-18
#  Adapted from the old scenic_driver_glfw from 2018...
#  Copyright © 2018-2021 Kry10 Limited. All rights reserved.
#
*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#include <fcntl.h> //O_BINARY
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../types.h"
#include "../utils.h"
#include "../comms.h"
#include "device.h"

#define STDIN_FILENO 0

typedef struct
{
  float last_x;
  float last_y;

  int window_width;
  int window_height;
  int frame_width;
  int frame_height;
  float ratio_x;
  float ratio_y;

  device_info_t *p_info;
} window_data_t;

window_data_t g_window_data = {0};

// //=============================================================================
// // main setup

//---------------------------------------------------------
int device_init(const device_opts_t *p_opts, device_info_t *p_info)
{
  // set up one-time features of the window
  g_window_data.p_info = p_info;
  g_window_data.last_x = -1.0f;
  g_window_data.last_y = -1.0f;

  g_window_data.window_width = p_opts->width;
  g_window_data.window_height = p_opts->height;
  g_window_data.ratio_x = 1.0f;
  g_window_data.ratio_y = 1.0f;

  p_info->width = g_window_data.window_width;
  p_info->height = g_window_data.window_height;

#ifdef _MSC_VER
  _setmode(_fileno(stdin), O_BINARY);
  _setmode(_fileno(stdout), O_BINARY);
#endif

  LOG("Device: init\n");
  LOG("|       width %d\n", g_window_data.window_width);
  LOG("|       height %d\n", g_window_data.window_height);
  return 0;
}

//---------------------------------------------------------
// tear down one-time features of the window
int device_close(device_info_t *p_info)
{
  LOG("Device: close\n");
  return 0;
}

//---------------------------------------------------------
void device_clear_color(float red, float green, float blue, float alpha)
{
  LOG("Device: clear color %f %f %f %f\n", red, green, blue, alpha);
}

//---------------------------------------------------------
void device_begin_render()
{
  LOG("Device: begin render\n");
}

void device_end_render()
{
  LOG("Device: end render\n");
}

//---------------------------------------------------------
void device_poll()
{
  LOG("Device: device poll\n");
}