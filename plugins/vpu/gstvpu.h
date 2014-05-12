/*
 * Copyright (c) 2014, Freescale Semiconductor, Inc. All rights reserved.
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

#ifndef __GST_VPU_H__
#define __GST_VPU_H__

#include "vpu_wrapper.h"

G_BEGIN_DECLS

#define DEFAULT_FRAME_BUFFER_ALIGNMENT_H 16
#define DEFAULT_FRAME_BUFFER_ALIGNMENT_V 16
#define ALIGN(ptr,align)	((align) ? ((((guint32)(ptr))+(align)-1)/(align)*(align)) : ((guint32)(ptr)))

typedef struct
{
  VpuCodStd std;
  const gchar *mime;
} VPUMapper;

static VPUMapper vpu_mappers[] = {
  {VPU_V_MPEG4, "video/mpeg, mpegversion=(int)4"},
  {VPU_V_XVID, "video/x-xvid"},
  {VPU_V_H263, "video/x-h263"},
  {VPU_V_AVC, "video/x-h264"},
  {VPU_V_VC1, "video/x-wmv, wmvversion=(int)3, format=(string)WMV3"},
  {VPU_V_VC1_AP, "video/x-wmv, wmvversion=(int)3, format=(string)WVC1"},
  {VPU_V_MPEG2, "video/mpeg, systemstream=(boolean)false, mpegversion=(int){1,2}"},
  {VPU_V_MJPG, "image/jpeg"},
  {VPU_V_VP8, "video/x-vp8"},
  {-1, NULL}
};

typedef struct {
  VpuMemInfo mem_info;
	GList * internal_virt_mem;
	GList * internal_phy_mem;
} VpuInternalMem;

gint gst_vpu_find_std (GstCaps * caps);
gboolean gst_vpu_free_internal_mem (VpuInternalMem * vpu_internal_mem);
gboolean gst_vpu_allocate_internal_mem (VpuInternalMem * vpu_internal_mem);
gboolean gst_vpu_register_frame_buffer (GList * gstbuffer_in_vpudec, \
    GstVideoInfo *info, VpuFrameBuffer * vpuframebuffers);

G_END_DECLS

#endif /* __GST_VPU_H__ */