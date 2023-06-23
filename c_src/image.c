/*
#  Created by Boyd Multerer on 2021-03-28
#  Copyright © 2021 Kry10 Limited. All rights reserved.
#
*/

#include <string.h>
#include "common.h"

#include "comms.h"

#include "types.h"
#include "utils.h"
#include "image.h"
#include "comms.h"

#define HASH_ID(id) tommy_hash_u32(0, id.p_data, id.size)

#define REPEAT_XY (NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY)

tommy_hashlin images = {0};

//---------------------------------------------------------
typedef struct _image_t
{
  sid_t id;
  uint32_t nvg_id;
  uint32_t width;
  uint32_t height;
  uint32_t format;
  void *p_pixels;
  tommy_hashlin_node node;
} image_t;

//---------------------------------------------------------
void init_images(void)
{
  LOG("Image: init\n");
  // init the hash table
  tommy_hashlin_init(&images);
}

//=============================================================================
// internal utilities for working with the image id map
// isolate all knowledge of the hash table implementation to these functions

//---------------------------------------------------------
static int _comparator(const void *p_arg, const void *p_obj)
{
  const sid_t *p_id = p_arg;
  const image_t *p_img = p_obj;
  return (p_id->size != p_img->id.size) || memcmp(p_id->p_data, p_img->id.p_data, p_id->size);
}

//---------------------------------------------------------
// image_t* get_image( uint32_t driver_id ) {
image_t *get_image(sid_t id)
{
  return tommy_hashlin_search(
      &images,
      _comparator,
      &id,
      HASH_ID(id));
}

//---------------------------------------------------------
void image_free(image_t *p_image, void *_unused)
{
  if (p_image)
  {
    tommy_hashlin_remove_existing(&images, &p_image->node);
    LOG("Image: free image %d", p_image->nvg_id);

    if (p_image->p_pixels)
    {
      free(p_image->p_pixels);
    }

    free(p_image);
  }
}

//---------------------------------------------------------
void reset_images()
{
  LOG("Image: reset images\n");

  // deallocates all the objects iterating the hashtable
  tommy_hashlin_foreach_arg(&images, (tommy_foreach_arg_func *)image_free, NULL);

  // deallocates the hashtable
  tommy_hashlin_done(&images);

  // re-init the hash table
  tommy_hashlin_init(&images);
}

//---------------------------------------------------------
int read_pixels(void *p_pixels, uint32_t width, uint32_t height, uint32_t format_in, int *p_msg_length)
{
  // read incoming data into a temporary buffer
  int buffer_size = *p_msg_length;
  void *p_buffer = malloc(buffer_size);
  if (!p_buffer)
  {
    send_puts("Unable to alloc temporary pixel buffer!!");
    return -1;
  }
  read_bytes_down(p_buffer, buffer_size, p_msg_length);

  unsigned int pixel_count = width * height;
  unsigned int src_i;
  unsigned int dst_i;
  int x, y, comp;
  void *p_temp = NULL;

  switch (format_in)
  {
  case 0: // encoded file format
    p_temp = (void *)stbi_load_from_memory(p_buffer, buffer_size, &x, &y, &comp, 4);
    if (p_temp && (x != width || y != height))
    {
      send_puts("Image size mismatch!!");
      free(p_temp);
      return -1;
    }
    memcpy(p_pixels, p_temp, pixel_count * 4);
    free(p_temp);
    p_temp = NULL;
    break;

  case 1: // gray scale
    for (unsigned int i = 0; i < pixel_count; i++)
    {
      dst_i = i * 4;
      ((char *)p_pixels)[dst_i] = ((char *)p_buffer)[i];
      ((char *)p_pixels)[dst_i + 1] = ((char *)p_buffer)[i];
      ((char *)p_pixels)[dst_i + 2] = ((char *)p_buffer)[i];
      ((char *)p_pixels)[dst_i + 3] = 0xff;
    }
    break;

  case 2: // gray + alpha
    for (unsigned int i = 0; i < pixel_count; i++)
    {
      dst_i = i * 4;
      src_i = i * 2;
      ((char *)p_pixels)[dst_i] = ((char *)p_buffer)[src_i];
      ((char *)p_pixels)[dst_i + 1] = ((char *)p_buffer)[src_i];
      ((char *)p_pixels)[dst_i + 2] = ((char *)p_buffer)[src_i];
      ((char *)p_pixels)[dst_i + 3] = ((char *)p_buffer)[src_i + 1];
    }
    break;

  case 3: // rgb
    for (unsigned int i = 0; i < pixel_count; i++)
    {
      dst_i = i * 4;
      src_i = i * 3;
      ((char *)p_pixels)[dst_i] = ((char *)p_buffer)[src_i];
      ((char *)p_pixels)[dst_i + 1] = ((char *)p_buffer)[src_i + 1];
      ((char *)p_pixels)[dst_i + 2] = ((char *)p_buffer)[src_i + 2];
      ((char *)p_pixels)[dst_i + 3] = 0xff;
    }
    break;

  case 4: // rgba
    memcpy(p_pixels, p_buffer, pixel_count * 4);
    break;
  }

  // clean up
  if (p_buffer)
    free(p_buffer);
  return 0;
}

//---------------------------------------------------------
void put_image(int *p_msg_length)
{
  // read in the fixed size data
  uint32_t id_length;
  uint32_t blob_size;
  uint32_t width;
  uint32_t height;
  uint32_t format;
  read_bytes_down(&id_length, sizeof(uint32_t), p_msg_length);
  read_bytes_down(&blob_size, sizeof(uint32_t), p_msg_length);
  read_bytes_down(&width, sizeof(uint32_t), p_msg_length);
  read_bytes_down(&height, sizeof(uint32_t), p_msg_length);
  read_bytes_down(&format, sizeof(uint32_t), p_msg_length);
  LOG("Image: put image\n");
  LOG("|      id_length %d\n", id_length);
  LOG("|      blob_size %d\n", blob_size);
  LOG("|      width %d\n", width);
  LOG("|      height %d\n", height);
  LOG("|      format %d\n", format);

  // read the id into a temp buffer
  void *p_temp_id = calloc(1, id_length + 1);
  if (!p_temp_id)
  {
    send_puts("Unable to allocate image p_temp_id");
    return;
  };
  read_bytes_down(p_temp_id, id_length, p_msg_length);
  sid_t id;
  id.size = id_length;
  id.p_data = p_temp_id;

  // get the existing image record, if there is one
  image_t *p_image = get_image(id);

  // if the height or width have changed, then we fail
  if (p_image && ((width != p_image->width) || (height != p_image->height)))
  {
    // send_puts("Cannot change image size");
    log_error("Cannot change image size");
    free(p_temp_id);
    return;
  }

  // if there is no existing record, create a new one
  if (!p_image)
  {
    // initialize a record to hold the image
    int struct_size = ALIGN_UP(sizeof(image_t), 8);
    // the +1 is so the id is null terminated
    int id_size = ALIGN_UP(id_length + 1, 8);
    int pixel_size = width * height * 4;
    int alloc_size = struct_size + id_size + pixel_size;

    p_image = malloc(alloc_size);
    if (!p_image)
    {
      send_puts("Unable to allocate image struct");
      free(p_temp_id);
      return;
    };

    // basic setup
    memset(p_image, 0, struct_size + id_size);
    p_image->width = width;
    p_image->height = height;
    p_image->format = format;

    // initialize the id
    p_image->id.size = id_length;
    p_image->id.p_data = ((void *)p_image) + struct_size;
    memcpy(p_image->id.p_data, p_temp_id, id_length);

    // initialize the pixel pointer
    p_image->p_pixels = ((void *)p_image) + struct_size + id_size;

    // get the image data in pixel format
    read_pixels(p_image->p_pixels, width, height, format, p_msg_length);

    // create an nvg texture from the pixel data
    LOG("Image: create new image\n");
    LOG("|      width %d\n", width);
    LOG("|      height %d\n", height);
    LOG("|      repeat mode REPEAT_XY\n");

    // save the image record into the tommyhash
    tommy_hashlin_insert(&images, &p_image->node, p_image, HASH_ID(p_image->id));
  }
  else
  {
    // the image already exists and is the right size.
    // can save some bit of work by replacing the pixels of the existing image
    read_pixels(p_image->p_pixels, width, height, format, p_msg_length);
    LOG("Image: update existing image %d\n", p_image->nvg_id);
  }

  free(p_temp_id);
}

//=============================================================================
// called when rendering scripts

//---------------------------------------------------------
void set_fill_image(sid_t id)
{
  // get the mapped nvg_id for this image_id
  image_t *p_image = get_image(id);
  if (!p_image)
    return;

  LOG("Image: set fill image %d\n", p_image->nvg_id);
}

//---------------------------------------------------------
void set_stroke_image(sid_t id)
{
  // get the mapped nvg_id for this image_id
  image_t *p_image = get_image(id);
  if (!p_image)
    return;

  LOG("Image: set stroke image %d\n", p_image->nvg_id);
}

//---------------------------------------------------------
// see: https://github.com/memononen/nanovg/issues/348
void draw_image(sid_t id,
                float sx, float sy, float sw, float sh,
                float dx, float dy, float dw, float dh)
{
  // get the mapped nvg_id for this driver_id
  image_t *p_image = get_image(id);
  if (!p_image)
    return;
  LOG("Image: draw image\n");
  LOG("|      id %d\n", p_image->nvg_id);
  LOG("|      sx %f\n", sx);
  LOG("|      sy %f\n", sy);
  LOG("|      sw %f\n", sw);
  LOG("|      sh %f\n", sh);
  LOG("|      dx %f\n", dx);
  LOG("|      dy %f\n", dy);
  LOG("|      dw %f\n", dw);
  LOG("|      dh %f\n", dh);
}
