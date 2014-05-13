/*
 * Copyright (c) 2013-2014, Freescale Semiconductor, Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>

#include <linux/fb.h>
#include <linux/mxcfb.h>

#include "gstimxcommon.h"
#include "displays.h"
#include "gstsutils.h"
#include "gstimxv4l2.h"

GST_DEBUG_CATEGORY_EXTERN (overlay_sink_debug);
#define GST_CAT_DEFAULT overlay_sink_debug

#define KEY_DEVICE "device"
#define KEY_FMT "fmt"
#define KEY_WIDTH "width"
#define KEY_HEIGHT "height"
#define KEY_COLOR_KEY "color_key"
#define KEY_ALPHA "alpha"

#define DISPLAY_NUM_BUFFERS (3)

#define PAGE_SHIFT      12
#ifndef PAGE_SIZE
#define PAGE_SIZE       (1 << PAGE_SHIFT)
#endif
#define ROUNDUP2PAGESIZE(x)  (((x) + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1))

// handle for v4l2 display
typedef struct {
  gchar *name;
  gchar *device;
  gint fmt;
  gint w;
  gint h;
  guint alpha;
  gboolean enable_color_key;
  guint color_key;

  gpointer v4l2handle;
  PhyMemBlock memblk[DISPLAY_NUM_BUFFERS];
  gint first_request;
} DisplayHandle;

static guint string_to_fmt (char *value)
{
  guint fmt, a, b, c, d;

  a = value[0];
  b = value[1];
  c = value[2];
  d = value[3];

  fmt = (((a) << 0) | ((b) << 8) | ((c) << 16) | ((d) << 24));

  return fmt;
}

gint scan_displays(gpointer **phandle, gint *pcount)
{
  GstsutilsEntry *entry = NULL;
  gint group_count;
  gint i;
  gint count = 0;

  if (imx_chip_code () == CC_MX6Q)
    entry = gstsutils_init_entry ("/usr/share/imx_6q_display_config");
  else if (imx_chip_code () == CC_MX60)
    entry = gstsutils_init_entry ("/usr/share/imx_6sx_display_config");
  else {
    GST_ERROR ("Not supported platform.");
    return -1;
  }

  if(entry == NULL) {
    GST_ERROR ("scan display configuration file failed.");
    return -1;
  }

  group_count = gstsutils_get_group_count (entry);
  GST_DEBUG ("osink config group count (%d)", group_count);
  for (i=1; i<=group_count; i++) {
    GstsutilsGroup *group;
    DisplayHandle *hdisplay;
    gchar *name = NULL;
    gchar *device;
    gchar *fmt, *w, *h, *color_key, *alpha;

    color_key = w = h = NULL;

    if(!gstsutils_get_group_by_index(entry, i, &group)) {
      GST_ERROR ("gstsutils_get_group_by_index failed.\n");
      continue;
    }

    name = gstsutils_get_group_name (group); 
    if (!name)
      name = "unknown";

    if(!gstsutils_get_value_by_key(group, KEY_DEVICE, &device)) {
      GST_ERROR ("can find display %s device.", name);
      continue;
    }
    if(!gstsutils_get_value_by_key(group, KEY_FMT, &fmt)) {
      fmt = "RGBP"; //default rgb565
    }
    if(!gstsutils_get_value_by_key(group, KEY_WIDTH, &w)) {
      GST_DEBUG ("No width configured for display %s.", name);
    }
    if(!gstsutils_get_value_by_key(group, KEY_HEIGHT, &h)) {
      GST_DEBUG ("No height configured for display %s.", name);
    }

    if(!gstsutils_get_value_by_key(group, KEY_ALPHA, &alpha))
      alpha = "0";

    gstsutils_get_value_by_key(group, KEY_COLOR_KEY, &color_key);

    hdisplay = g_slice_alloc (sizeof(DisplayHandle));
    if (!hdisplay) {
      GST_ERROR ("allocate DisplayHandle for display %s failed.", name);
      return -1;
    }
    memset (hdisplay, 0, sizeof (DisplayHandle));

    hdisplay->name = g_strdup (name);
    hdisplay->device = g_strdup (device);
    hdisplay->fmt = string_to_fmt (fmt);
    hdisplay->alpha = atoi (alpha);
    if (color_key) {
      hdisplay->enable_color_key = TRUE;
      hdisplay->color_key = atoi (color_key);
    }
    else {
      hdisplay->enable_color_key = FALSE;
    }

    if (!w || !h) {
      gst_imx_v4l2_get_display_resolution (hdisplay->device, &hdisplay->w, &hdisplay->h);
    }
    else {
      hdisplay->w = atoi (w);
      hdisplay->h = atoi (h);
    }

    GST_DEBUG ("display(%s), device(%s), fmt(%d), res(%dx%d), ckey(%x), alpha(%d)",
        hdisplay->name, hdisplay->device, hdisplay->fmt, hdisplay->w, hdisplay->h, 
        hdisplay->color_key, hdisplay->alpha);

    phandle[count] = (gpointer)hdisplay;
    count ++;
    if (count >= MAX_DISPLAY)
      break;
  }

  *pcount = count;

  return 0;
}

void free_display (gpointer display)
{
  DisplayHandle *hdisplay = (DisplayHandle*) display;

  if (hdisplay) {
    if (hdisplay->name)
      g_free (hdisplay->name);
    if (hdisplay->device)
      g_free (hdisplay->device);
    g_slice_free1 (sizeof(DisplayHandle), hdisplay);
  }
}

gchar *get_display_name(gpointer display)
{
  DisplayHandle *hdisplay = (DisplayHandle*) display;

  return hdisplay->name;
}

gint get_display_format(gpointer display)
{
  DisplayHandle *hdisplay = (DisplayHandle*) display;

  return hdisplay->fmt;
}

gint get_display_res(gpointer display, gint *width, gint *height)
{
  DisplayHandle *hdisplay = (DisplayHandle*) display;

  *width = hdisplay->w;
  *height = hdisplay->h;

  return 0;
}

// v4l2 display implementation

gint init_display (gpointer display)
{
  DisplayHandle *hdisplay = (DisplayHandle*) display;
  IMXV4l2Rect rect;

  hdisplay->v4l2handle = gst_imx_v4l2_open_device (hdisplay->device, V4L2_BUF_TYPE_VIDEO_OUTPUT);
  if (!hdisplay->v4l2handle) {
    GST_ERROR ("gst_imx_v4l2_open_device %s failed.", hdisplay->device);
    return -1;
  }

  gst_imx_v4l2out_config_alpha (hdisplay->v4l2handle, hdisplay->alpha);
  gst_imx_v4l2out_config_color_key (hdisplay->v4l2handle, hdisplay->enable_color_key, hdisplay->color_key);

  rect.left = rect.top = 0;
  rect.width = hdisplay->w;
  rect.height = hdisplay->h;
  if (gst_imx_v4l2out_config_input (hdisplay->v4l2handle, hdisplay->fmt, hdisplay->w, hdisplay->h, &rect) < 0) {
    GST_ERROR ("configure v4l2 device %s input failed.", hdisplay->device);
    goto err;
  }

  if (gst_imx_v4l2out_config_output (hdisplay->v4l2handle, &rect, FALSE) < 0) {
    GST_ERROR ("configure v4l2 device %s output failed.", hdisplay->device);
    goto err;
  }

  if (gst_imx_v4l2_config_rotate (hdisplay->v4l2handle, 0) < 0) {
    GST_ERROR ("configure v4l2 device %s rotate to 0 failed.", hdisplay->device);
    goto err;
  }

  if (gst_imx_v4l2_set_buffer_count (hdisplay->v4l2handle, DISPLAY_NUM_BUFFERS, V4L2_MEMORY_MMAP) < 0) {
    GST_ERROR ("configure v4l2 device %s buffer count failed.", hdisplay->device);
    goto err;
  }

  gint i;
  for (i=0; i<DISPLAY_NUM_BUFFERS; i++) {
    if (gst_imx_v4l2_allocate_buffer (hdisplay->v4l2handle, &hdisplay->memblk[i]) < 0) {
      GST_ERROR ("allocate memory from v4l2 device %s failed.", hdisplay->device);
      goto err;
    }
  }

  clear_display (display);

  return 0;

err:
    deinit_display (hdisplay->v4l2handle);
    return -1;
}

void deinit_display (gpointer display)
{
  DisplayHandle *hdisplay = (DisplayHandle*) display;
  gint i;

  for (i=0; i<DISPLAY_NUM_BUFFERS; i++) {
    if (hdisplay->memblk[i].vaddr) {
      gst_imx_v4l2_free_buffer (hdisplay->v4l2handle, &hdisplay->memblk[i]);
      memset (&hdisplay->memblk[i], 0, sizeof (PhyMemBlock));
    }
  }

  gst_imx_v4l2_close_device (hdisplay->v4l2handle);
  hdisplay->v4l2handle = NULL;

  return;
}

gint clear_display (gpointer display)
{
  DisplayHandle *hdisplay = (DisplayHandle*) display;
  gint i;

  for (i=0; i<DISPLAY_NUM_BUFFERS; i++) {
    memset (hdisplay->memblk[i].vaddr, 0, hdisplay->memblk[i].size);
  }

  return 0;
}

gint get_next_display_buffer (gpointer display, SurfaceBuffer *buffer)
{
  DisplayHandle *hdisplay = (DisplayHandle*) display;
  PhyMemBlock *memblk = NULL;
  gint index;

  if (hdisplay->first_request < DISPLAY_NUM_BUFFERS) {
    memblk = &hdisplay->memblk[hdisplay->first_request];
    hdisplay->first_request ++;
  }
  else {
    if (gst_imx_v4l2_dequeue_v4l2memblk (hdisplay->v4l2handle, &memblk) < 0) {
      GST_ERROR ("get buffer from %s failed.", hdisplay->device);
      return -1;
    } 
  }

  if (!memblk) {
    GST_ERROR ("get display buffer failed.");
    return -1;
  }

  memcpy (buffer, memblk, sizeof (SurfaceBuffer));

  //GST_DEBUG ("get display buffer, vaddr (%p) paddr (%p).", buffer->vaddr, buffer->paddr);

  return 0;
}

gint flip_display_buffer (gpointer display, SurfaceBuffer *buffer)
{
  DisplayHandle *hdisplay = (DisplayHandle*) display;
  PhyMemBlock *memblk = NULL;
  gint i;

  //GST_DEBUG ("flip display buffer, vaddr (%p) paddr (%p).", buffer->vaddr, buffer->paddr);

  for (i=0; i<DISPLAY_NUM_BUFFERS; i++) {
    if (buffer->vaddr == hdisplay->memblk[i].vaddr)
      memblk = &hdisplay->memblk[i];
  }

  if (!memblk) {
    GST_ERROR ("invalid display buffer.");
    return -1;
  }

  if (gst_imx_v4l2_queue_v4l2memblk (hdisplay->v4l2handle, memblk, GST_VIDEO_FRAME_FLAG_NONE) < 0) {
    GST_ERROR ("flip display buffer failed.");
    return -1;
  }

  return 0;
}
