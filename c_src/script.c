/*
#  Created by Boyd Multerer on 2021-03-09.
#  Copyright © 2021 Kry10 Limited. All rights reserved.
#
*/

#include <string.h>

#include "common.h"
#include "utils.h"
#include "comms.h"
#include "script.h"
#include "image.h"
#include "font.h"

//---------------------------------------------------------
typedef struct _script_t
{
  sid_t id;
  data_t script;
  tommy_hashlin_node node;
} script_t;

// #define HASH_ID(id)  tommy_inthash_u32(id)
#define HASH_ID(id) tommy_hash_u32(0, id.p_data, id.size)

tommy_hashlin scripts = {0};

//---------------------------------------------------------
void init_scripts(void)
{
  // init the hash table
  tommy_hashlin_init(&scripts);
}

//=============================================================================
// internal utilities for working with the script hash

// isolate all knowledge of the hash table implementation to these functions

//---------------------------------------------------------
static int _comparator(const void *p_arg, const void *p_obj)
{
  const sid_t *p_id = p_arg;
  const script_t *p_script = p_obj;
  return (p_id->size != p_script->id.size) || memcmp(p_id->p_data, p_script->id.p_data, p_id->size);
}

//---------------------------------------------------------
script_t *get_script(sid_t id)
{
  return tommy_hashlin_search(
      &scripts,
      _comparator,
      &id,
      HASH_ID(id));
}

//---------------------------------------------------------
void do_delete_script(sid_t id)
{
  script_t *p_script = get_script(id);
  if (p_script)
  {
    tommy_hashlin_remove_existing(&scripts, &p_script->node);
    free(p_script);
  }
}

//---------------------------------------------------------
void put_script(int *p_msg_length)
{

  // read in the length of the id, which is in the first four bytes
  uint32_t id_length;
  read_bytes_down(&id_length, sizeof(uint32_t), p_msg_length);

  // initialize a record to hold the script
  int struct_size = ALIGN_UP(sizeof(script_t), 8);
  int id_size = ALIGN_UP(id_length, 8);
  int alloc_size = struct_size + id_size + *p_msg_length;
  script_t *p_script = malloc(alloc_size);
  if (!p_script)
  {
    send_puts("Unable to allocate script");
    return;
  };

  // initialize the id
  p_script->id.size = id_length;
  p_script->id.p_data = ((void *)p_script) + struct_size;
  read_bytes_down(p_script->id.p_data, id_length, p_msg_length);

  // initialize the data
  p_script->script.size = *p_msg_length;
  p_script->script.p_data = ((void *)p_script) + struct_size + id_size;
  read_bytes_down(p_script->script.p_data, *p_msg_length, p_msg_length);

  // if there is already is a script with the same id, delete it
  do_delete_script(p_script->id);

  // insert the script into the tommy hash
  tommy_hashlin_insert(&scripts, &p_script->node, p_script, HASH_ID(p_script->id));

  // return the completed struct
  return;
}

//---------------------------------------------------------
void delete_script(int *p_msg_length)
{
  sid_t id;

  // read in the length of the id, which is in the first four bytes
  read_bytes_down(&id.size, sizeof(uint32_t), p_msg_length);

  // create a temporary buffer and read the id into it
  id.p_data = malloc(id.size);
  if (!id.p_data)
  {
    send_puts("Unable to allocate buffer for the id");
    return;
  };

  // read in the body of the id
  read_bytes_down(id.p_data, id.size, p_msg_length);

  // delete and free
  do_delete_script(id);

  free(id.p_data);
}

//---------------------------------------------------------
void reset_scripts()
{
  // deallocates all the objects iterating the hashtable
  tommy_hashlin_foreach(&scripts, free);

  // deallocates the hashtable
  tommy_hashlin_done(&scripts);

  // re-init the hash table
  tommy_hashlin_init(&scripts);
}

//=============================================================================
// rendering

static void put_msg_i(char *msg, int n)
{
  char buff[200];
  sprintf(buff, "%s %d", msg, n);
  send_puts(buff);
}

static inline byte get_byte(void *p, uint32_t offset)
{
  return *((byte *)(p + offset));
}

static inline unsigned short get_uint16(void *p, uint32_t offset)
{
  return ntoh_ui16(*((unsigned short *)(p + offset)));
}

static inline unsigned int get_uint32(void *p, uint32_t offset)
{
  return ntoh_ui32(*((unsigned int *)(p + offset)));
}

static inline float get_float(void *p, uint32_t offset)
{
  return ntoh_f32(*((float *)(p + offset)));
}

int padded_advance(int size)
{
  switch (size % 4)
  {
  case 0:
    return size;
  case 1:
    return size + 3;
  case 2:
    return size + 2;
  case 3:
    return size + 1;
  default:
    size;
  };
}

void render_text(char *p_text, unsigned int size)
{
  LOG("Script: render_text\n");
  LOG("|       size %d\n", size);
  LOG("|       text: %s\n", p_text);
}

int render_sprites(void *p, int i, uint16_t param)
{
  // get the count of rects
  uint32_t count = get_uint32(p, i);
  i += sizeof(uint32_t);

  // get the id
  sid_t id;
  id.size = param;
  id.p_data = p + i;
  i += padded_advance(param);

  // loop the draw commands and draw each
  for (int n = 0; n < count; n++)
  {
    float sx = get_float(p, i);
    float sy = get_float(p, i + 4);
    float sw = get_float(p, i + 8);
    float sh = get_float(p, i + 12);
    float dx = get_float(p, i + 16);
    float dy = get_float(p, i + 20);
    float dw = get_float(p, i + 24);
    float dh = get_float(p, i + 28);

    draw_image(id, sx, sy, sw, sh, dx, dy, dw, dh);

    i += 32;
  }

  return i;
}

//---------------------------------------------------------
void render_script(sid_t id)
{
  // get the script
  script_t *p_script = get_script(id);
  if (!p_script)
  {
    return;
  }

  // track the state pushes
  int push_count = 0;

  // setup
  void *p = p_script->script.p_data;
  int i = 0;

  while (i < p_script->script.size)
  {
    int op = get_uint16(p, i);
    int param = get_uint16(p, i + 2);
    i += 4;

    switch (op)
    {
    case 0x01: // draw_line
      LOG("Script: draw_line\n");
      LOG("|      SX %f\n", get_float(p, i));
      LOG("|      SY %f\n", get_float(p, i + 4));
      LOG("|      EX %f\n", get_float(p, i + 8));
      LOG("|      EY %f\n", get_float(p, i + 12));

      if (param & 2)
        LOG("Script: draw_line stroke\n");
      i += 16;
      break;
    case 0x02: // draw_triangle
      LOG("Script: draw_triangle\n");
      LOG("|      AX %f\n", get_float(p, i));
      LOG("|      AY %f\n", get_float(p, i + 4));
      LOG("|      BX %f\n", get_float(p, i + 8));
      LOG("|      BY %f\n", get_float(p, i + 12));
      LOG("|      CX %f\n", get_float(p, i + 16));
      LOG("|      CY %f\n", get_float(p, i + 20));

      if (param & 1)
        LOG("Script: draw_triangle fill\n");
      if (param & 2)
        LOG("Script: draw_triangle stroke\n");
      i += 24;
      break;
    case 0x03: // draw_quad
      LOG("Script: draw_quad\n");
      LOG("|      AX %f\n", get_float(p, i));
      LOG("|      AY %f\n", get_float(p, i + 4));
      LOG("|      BX %f\n", get_float(p, i + 8));
      LOG("|      BY %f\n", get_float(p, i + 12));
      LOG("|      CX %f\n", get_float(p, i + 16));
      LOG("|      CY %f\n", get_float(p, i + 20));
      LOG("|      DX %f\n", get_float(p, i + 24));
      LOG("|      DY %f\n", get_float(p, i + 28));

      if (param & 1)
        LOG("Script: draw_triangle fill\n");
      if (param & 2)
        LOG("Script: draw_triangle stroke\n");
      i += 32;
      break;
    case 0x04: // draw_rect
      LOG("Script: draw_rect\n");
      LOG("|      W %f\n", get_float(p, i));
      LOG("|      H %f\n", get_float(p, i + 4));

      if (param & 1)
        LOG("Script: draw_rect fill\n");
      if (param & 2)
        LOG("Script: draw_rect stroke\n");
      i += 8;
      break;
    case 0x05: // draw_rrect
      LOG("Script: draw_rrect\n");
      LOG("|      W %f\n", get_float(p, i));
      LOG("|      H %f\n", get_float(p, i + 4));
      LOG("|      R %f\n", get_float(p, i + 8));
      if (param & 1)
        LOG("Script: draw_rrect fill\n");
      if (param & 2)
        LOG("Script: draw_rrect stroke\n");
      i += 12;
      break;
    case 0x06: // draw_arc
      LOG("Script: draw_arc\n");
      LOG("|      R %f\n", get_float(p, i));
      LOG("|      A %f\n", get_float(p, i + 4));
      LOG("|      W %s\n", get_float(p, i + 4) > 0 ? "CW" : "CCW");

      if (param & 1)
        LOG("Script: draw_arc fill\n");
      if (param & 2)
        LOG("Script: draw_arc stroke\n");
      i += 8;
      break;
    case 0x07: // draw_sector
      nvgBeginPath(p_ctx);
      nvgMoveTo(p_ctx, 0, 0);
      nvgLineTo(p_ctx, get_float(p, i), 0);
      nvgArc(p_ctx,
             0, 0,
             get_float(p, i), 0, get_float(p, i + 4),
             get_float(p, i + 4) > 0 ? NVG_CW : NVG_CCW);
      nvgClosePath(p_ctx);
      if (param & 1)
        nvgFill(p_ctx);
      if (param & 2)
        nvgStroke(p_ctx);
      i += 8;
      break;
    case 0x08: // draw_circle
      nvgBeginPath(p_ctx);
      nvgCircle(p_ctx, 0, 0, get_float(p, i));
      if (param & 1)
        nvgFill(p_ctx);
      if (param & 2)
        nvgStroke(p_ctx);
      i += 4;
      break;
    case 0x09: // draw_ellipse
      nvgBeginPath(p_ctx);
      nvgEllipse(p_ctx, 0, 0, get_float(p, i), get_float(p, i + 4));
      if (param & 1)
        nvgFill(p_ctx);
      if (param & 2)
        nvgStroke(p_ctx);
      i += 8;
      break;
    case 0x0A: // draw_text - byte count is in param
      render_text(p + i, param);
      i += padded_advance(param);
      break;
    case 0x0B: // draw_sprites
      i = render_sprites(p, i, param);
      break;

    case 0x0F: // render_script
      // we can reuse the passed in id struct
      id.size = param;
      id.p_data = p + i;
      render_script(id);
      i += padded_advance(param);
      break;

    case 0x20: // begin_path
      LOG("Script: begin_path\n");
      break;
    case 0x21: // close_path
      LOG("Script: close_path\n");
      break;
    case 0x22: // fill
      LOG("Script: fill\n");
      break;
    case 0x23: // stroke
      LOG("Script: stroke\n");
      break;

    case 0x26: // move_to
      nvgMoveTo(p_ctx, get_float(p, i), get_float(p, i + 4));
      i += 8;
      break;
    case 0x27: // line_to
      nvgLineTo(p_ctx, get_float(p, i), get_float(p, i + 4));
      i += 8;
      break;
    case 0x28: // arc_to
      nvgArcTo(p_ctx,
               get_float(p, i), get_float(p, i + 4),      // x1, y1
               get_float(p, i + 8), get_float(p, i + 12), // x2, y2
               get_float(p, i + 16)                       // radius
      );
      i += 20;
      break;
    case 0x29: // bezier_to
      nvgBezierTo(p_ctx,
                  get_float(p, i), get_float(p, i + 4),      // c1x, c1y
                  get_float(p, i + 8), get_float(p, i + 12), // c2x, c2y
                  get_float(p, i + 16), get_float(p, i + 20) // x, y
      );
      i += 24;
      break;
    case 0x2A: // quadratic_to
      nvgQuadTo(p_ctx,
                get_float(p, i), get_float(p, i + 4),     // cx, cy
                get_float(p, i + 8), get_float(p, i + 12) // x, y
      );
      i += 16;
      break;

    case 0x40: // push_state
      push_count++;
      LOG("Script: push_state\n");
      break;
    case 0x41: // pop_state
      if (push_count > 0)
      {
        push_count--;
        LOG("Script: pop_state\n");
      }
      break;
    case 0x42: // pop_push_state
      if (push_count > 0)
      {
        push_count--;
        LOG("Script: pop_push_state pop\n");
      }
      push_count++;
      LOG("Script: pop_push_state push\n");
      break;

    // case 0x43:        // clear
    case 0x44: // scissor
      nvgScissor(p_ctx, 0, 0, get_float(p, i), get_float(p, i + 4));
      i += 8;
      break;

    case 0x50: // transform
      nvgTransform(
          p_ctx,
          get_float(p, i), get_float(p, i + 4),
          get_float(p, i + 8), get_float(p, i + 12),
          get_float(p, i + 16), get_float(p, i + 20));
      i += 24;
      break;
    case 0x51: // scale
      nvgScale(p_ctx, get_float(p, i), get_float(p, i + 4));
      i += 8;
      break;
    case 0x52: // rotate
      nvgRotate(p_ctx, get_float(p, i));
      i += 4;
      break;
    case 0x53: // translate
      nvgTranslate(p_ctx, get_float(p, i), get_float(p, i + 4));
      i += 8;
      break;

    case 0x60: // fill_color
      nvgFillColor(p_ctx, nvgRGBA(
                              get_byte(p, i), get_byte(p, i + 1), get_byte(p, i + 2), get_byte(p, i + 3)));
      i += 4;
      break;
    case 0x61: // fill_linear
      nvgFillPaint(p_ctx,
                   nvgLinearGradient(
                       p_ctx,
                       get_float(p, i), get_float(p, i + 4), get_float(p, i + 8), get_float(p, i + 12),
                       nvgRGBA(get_byte(p, i + 16), get_byte(p, i + 17), get_byte(p, i + 18), get_byte(p, i + 19)),
                       nvgRGBA(get_byte(p, i + 20), get_byte(p, i + 21), get_byte(p, i + 22), get_byte(p, i + 23))));
      i += 24;
      break;
    case 0x62: // fill_radial
      nvgFillPaint(p_ctx,
                   nvgRadialGradient(
                       p_ctx,
                       get_float(p, i), get_float(p, i + 4), get_float(p, i + 8), get_float(p, i + 12),
                       nvgRGBA(get_byte(p, i + 16), get_byte(p, i + 17), get_byte(p, i + 18), get_byte(p, i + 19)),
                       nvgRGBA(get_byte(p, i + 20), get_byte(p, i + 21), get_byte(p, i + 22), get_byte(p, i + 23))));
      i += 24;
      break;
    case 0x63: // fill_image
      // we can reuse the passed in id struct
      id.size = param;
      id.p_data = p + i;
      set_fill_image(p_ctx, id);
      i += padded_advance(param);
      break;
    case 0x64: // fill_stream
      // we can reuse the passed in id struct
      id.size = param;
      id.p_data = p + i;
      set_fill_image(p_ctx, id);
      i += padded_advance(param);
      break;

    case 0x70: // stroke width
      nvgStrokeWidth(p_ctx, param / 4.0);
      break;
    case 0x71: // stroke color
      nvgStrokeColor(p_ctx, nvgRGBA(
                                get_byte(p, i), get_byte(p, i + 1), get_byte(p, i + 2), get_byte(p, i + 3)));
      i += 4;
      break;
    case 0x72: // stroke_linear
      nvgStrokePaint(p_ctx,
                     nvgLinearGradient(
                         p_ctx,
                         get_float(p, i), get_float(p, i + 4), get_float(p, i + 8), get_float(p, i + 12),
                         nvgRGBA(get_byte(p, i + 16), get_byte(p, i + 17), get_byte(p, i + 18), get_byte(p, i + 19)),
                         nvgRGBA(get_byte(p, i + 20), get_byte(p, i + 21), get_byte(p, i + 22), get_byte(p, i + 23))));
      i += 24;
      break;
    case 0x73: // stroke_radial
      nvgStrokePaint(p_ctx,
                     nvgRadialGradient(
                         p_ctx,
                         get_float(p, i), get_float(p, i + 4), get_float(p, i + 8), get_float(p, i + 12),
                         nvgRGBA(get_byte(p, i + 16), get_byte(p, i + 17), get_byte(p, i + 18), get_byte(p, i + 19)),
                         nvgRGBA(get_byte(p, i + 20), get_byte(p, i + 21), get_byte(p, i + 22), get_byte(p, i + 23))));
      i += 24;
      break;
    case 0x74: // stroke_image
      // we can reuse the passed in id struct
      id.size = param;
      id.p_data = p + i;
      set_stroke_image(p_ctx, id);
      i += padded_advance(param);
      break;
    case 0x75: // stroke_stream
      // we can reuse the passed in id struct
      id.size = param;
      id.p_data = p + i;
      set_stroke_image(p_ctx, id);
      i += padded_advance(param);
      break;

    case 0x80: // line cap
      switch (param)
      {
      case 0x00:
        nvgLineCap(p_ctx, NVG_BUTT);
        break;
      case 0x01:
        nvgLineCap(p_ctx, NVG_ROUND);
        break;
      case 0x02:
        nvgLineCap(p_ctx, NVG_SQUARE);
        break;
      }
      break;
    case 0x81: // line join
      switch (param)
      {
      case 0x00:
        nvgLineJoin(p_ctx, NVG_BEVEL);
        break;
      case 0x01:
        nvgLineJoin(p_ctx, NVG_ROUND);
        break;
      case 0x02:
        nvgLineJoin(p_ctx, NVG_MITER);
        break;
      }
      break;
    case 0x82: // miter limit
      nvgMiterLimit(p_ctx, param);
      break;

    case 0x90: // font
      // we can reuse the passed in id struct
      id.size = param;
      id.p_data = p + i;
      set_font(id, p_ctx);
      i += padded_advance(param);
      break;
    case 0x91: // font_size
      nvgFontSize(p_ctx, param / 4.0);
      // font_size = param / 4
      // ctx.font = `${font_size}px ${font}`
      break;
    case 0x92: // text_align
      switch (param)
      {
      case 0x00: // left
        nvgTextAlignH(p_ctx, NVG_ALIGN_LEFT);
        break;
      case 0x01: // center
        nvgTextAlignH(p_ctx, NVG_ALIGN_CENTER);
        break;
      case 0x02: // right
        nvgTextAlignH(p_ctx, NVG_ALIGN_RIGHT);
        break;
      }
      break;
    case 0x93: // text_base
      switch (param)
      {
      case 0x00: // top
        nvgTextAlignV(p_ctx, NVG_ALIGN_TOP);
        break;
      case 0x01: // middle
        nvgTextAlignV(p_ctx, NVG_ALIGN_MIDDLE);
        break;
      case 0x02: // alphabetic
        nvgTextAlignV(p_ctx, NVG_ALIGN_BASELINE);
        break;
      case 0x03: // bottom
        nvgTextAlignV(p_ctx, NVG_ALIGN_BOTTOM);
        break;
      }
      break;

    default:
      put_msg_i("Unknown OP: ", op);
      break;
    }
  }

  // if there are unbalanced pushes, clear them
  while (push_count > 0)
  {
    push_count--;
    nvgRestore(p_ctx);
  }
}
