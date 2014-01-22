/*
 * Copyright (c) 2013, Freescale Semiconductor, Inc. All rights reserved.
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

#ifndef __GST_VPU_DEC_OBJECT_H__
#define __GST_VPU_DEC_OBJECT_H__

#include <gst/video/gstvideodecoder.h>
#include "vpu_wrapper.h"
#include "video-tsm/mfw_gst_ts.h"

G_BEGIN_DECLS

#define GST_TYPE_VPU_DEC_OUTPUT_FORMAT (gst_vpu_dec_output_format_get_type ())

#define GST_TYPE_VPU_DEC_OBJECT             (gst_vpu_dec_object_get_type())
#define GST_VPU_DEC_OBJECT(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VPU_DEC_OBJECT,GstVpuDecObject))
#define GST_VPU_DEC_OBJECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VPU_DEC_OBJECT,GstVpuDecObjectClass))
#define GST_VPU_DEC_OBJECT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_VPU_DEC_OBJECT, GstVpuDecObjectClass))
#define GST_VPU_DEC_OBJECT_CAST(obj)        ((GstVpuDecObject *)obj)
#define GST_IS_VPU_DEC_OBJECT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VPU_DEC_OBJECT))
#define GST_IS_VPU_DEC_OBJECT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VPU_DEC_OBJECT))

typedef struct _GstVpuDecObject GstVpuDecObject;
typedef struct _GstVpuDecObjectClass GstVpuDecObjectClass;

#define GST_VPU_DEC_LOW_LATENCY(o)           ((o)->low_latency)
#define GST_VPU_DEC_OUTPUT_FORMAT(o)         ((o)->output_format)
#define GST_VPU_DEC_FRAME_DROP(o)            ((o)->frame_drop)
#define GST_VPU_DEC_FRAMES_PLUS(o)           ((o)->frame_plus)
#define GST_VPU_DEC_MIN_BUF_CNT(o)           ((o)->min_buf_cnt)
#define GST_VPU_DEC_ACTUAL_BUF_CNT(o)        ((o)->actual_buf_cnt)
#define GST_VPU_DEC_BUF_ALIGNMENT(o)         ((o)->buf_align)
#define GST_VPU_DEC_VIDEO_ALIGNMENT(o)       ((o)->video_align)
#define GST_VPU_DEC_NEED_WAIT(o)             ((o)->need_wait)
 
typedef enum {
  STATE_NULL    = 0,
  STATE_LOADED,
  STATE_ALLOCATED_INTERNAL_BUFFER,
  STATE_OPENED,
  STATE_FLUSHED,
  STATE_REGISTRIED_FRAME_BUFFER,
  STATE_MAX
} VpuDecState;

struct _GstVpuDecObject {
  GstObject parent;

  /* vpudec will get/set */
  gboolean low_latency;
  guint output_format;
  gboolean frame_drop;
  guint frame_plus;
  guint min_buf_cnt;
  guint actual_buf_cnt;
  guint buf_align;
  GstVideoAlignment video_align;
  gboolean need_wait;

  GstVideoCodecState *input_state;
  GstVideoCodecState *output_state;

  /* vpu_dec_object parameter */
  VpuDecHandle handle;
  VpuDecInitInfo init_info;
  VpuMemInfo mem_info;
  VpuFrameBuffer *vpuframebuffers;
  gint width_paded;
  gint height_paded;
  VpuDecState state;
	GList * internal_virt_mem;
	GList * internal_phy_mem;
	GList * mv_mem;
  GstVideoFormat output_format_decided;
	GHashTable *frame2gstbuffer_table;
	GHashTable *gstbuffer2frame_table;
	GList * gstbuffer_in_vpudec;
  gboolean use_new_tsm;
  gint framerate_n;
  gint framerate_d;
  gint mosaic_cnt;
  gboolean is_mjpeg;
  gboolean new_segment;
  void *tsm;
  TSMGR_MODE tsm_mode;
};

struct _GstVpuDecObjectClass {
  GstObjectClass parent_class;
};

GType gst_vpu_dec_output_format_get_type (void);
GType gst_vpu_dec_object_get_type (void);

/* create/destroy */
GstVpuDecObject *	gst_vpu_dec_object_new (void);
void gst_vpu_dec_object_destroy (GstVpuDecObject * vpu_decobject);

GstCaps* gst_vpu_dec_object_get_sink_caps (void);
GstCaps* gst_vpu_dec_object_get_src_caps (void);

gboolean gst_vpu_dec_object_open (GstVpuDecObject * vpu_dec_object);
gboolean gst_vpu_dec_object_close (GstVpuDecObject * vpu_dec_object);
gboolean gst_vpu_dec_object_start (GstVpuDecObject * vpu_dec_object);
gboolean gst_vpu_dec_object_stop (GstVpuDecObject * vpu_dec_object);
gboolean gst_vpu_dec_object_config (GstVpuDecObject * vpu_dec_object, \
    GstVideoDecoder * bdec, GstVideoCodecState * state);
GstFlowReturn gst_vpu_dec_object_decode (GstVpuDecObject * vpu_dec_object, \
    GstVideoDecoder * bdec, GstVideoCodecFrame * frame);
gboolean gst_vpu_dec_object_flush (GstVpuDecObject * vpu_dec_object);

G_END_DECLS

#endif /* __GST_VPU_DEC_OBJECT_H__ */
