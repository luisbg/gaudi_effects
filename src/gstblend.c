/*
 * GStreamer
 * Copyright (C) 2010 Luis de Bethencourt <luis@debethencourt.com>
 *
 * Blend - previous frames weighted average video effect.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
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

/**
 * SECTION:element-blend
 *
 * Blend does a weighted average of previous frames of a video stream in
 * realtime.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v videotestsrc ! blend ! ffmpegcolorspace ! autovideosink
 * ]| This pipeline shows the effect of blend on a test stream
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <math.h>
#include <gst/gst.h>
#include <gst/controller/gstcontroller.h>

#include "gstplugin.h"
#include "gstblend.h"

#include <gst/video/video.h>

GST_DEBUG_CATEGORY_STATIC (gst_blend_debug);
#define GST_CAT_DEFAULT gst_blend_debug

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define CAPS_STR GST_VIDEO_CAPS_BGRx ";" GST_VIDEO_CAPS_RGBx
#else
#define CAPS_STR GST_VIDEO_CAPS_xRGB ";" GST_VIDEO_CAPS_xBGR
#endif

/* Filter signals and args. */
enum
{
  LAST_SIGNAL
};

enum
{
  PROP_0 = 0,
  PROP_EDGE_A,
  PROP_EDGE_B,
  PROP_SILENT
};

/* Initializations */

#define DEFAULT_EDGE_A 200
#define DEFAULT_EDGE_B 1

static gint gate_int (gint value, gint min, gint max);
void setup_cos_table (void);
static inline int abs_int (int val);
static void transform (GstBlend *filter, guint32 * src, guint32 * dest, gint video_area,
    gint edge_a, gint edge_b);

/* The capabilities of the inputs and outputs. */

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (CAPS_STR)
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (CAPS_STR)
    );

GST_BOILERPLATE (GstBlend, gst_blend, GstVideoFilter,
    GST_TYPE_VIDEO_FILTER);

static void gst_blend_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_blend_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_blend_set_caps (GstBaseTransform * btrans,
    GstCaps * incaps, GstCaps * outcaps);
static GstFlowReturn gst_blend_transform (GstBaseTransform * btrans,
    GstBuffer * in_buf, GstBuffer * out_buf);

/* GObject vmethod implementations */

static void
gst_blend_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_set_details_simple (element_class,
      "Blend",
      "Filter/Effect/Video",
      "Blend breaks the colors of the video signal.",
      "Luis de Bethencourt <luis@debethencourt.com>");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_factory));
}

/* Initialize the blend's class. */
static void
gst_blend_class_init (GstBlendClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstBaseTransformClass *trans_class = (GstBaseTransformClass *) klass;

  gobject_class->set_property = gst_blend_set_property;
  gobject_class->get_property = gst_blend_get_property;

  g_object_class_install_property (gobject_class, PROP_EDGE_A,
      g_param_spec_uint ("edge-a", "Edge A",
          "First edge parameter", 0, 256, DEFAULT_EDGE_A,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_EDGE_B,
      g_param_spec_uint ("edge-b", "Edge B",
          "Second edge parameter", 0, 256, DEFAULT_EDGE_B,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  trans_class->set_caps = GST_DEBUG_FUNCPTR (gst_blend_set_caps);
  trans_class->transform = GST_DEBUG_FUNCPTR (gst_blend_transform);
}

/* Initialize the element,
 * instantiate pads and add them to element,
 * set pad calback functions, and
 * initialize instance structure.
 */
static void
gst_blend_init (GstBlend * filter, GstBlendClass * gclass)
{
  // filter->edge_a = DEFAULT_EDGE_A;
  // filter->edge_b = DEFAULT_EDGE_B;
  // filter->silent = FALSE;

  setup_cos_table ();
}

static void
gst_blend_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBlend *filter = GST_BLEND (object);

  switch (prop_id) {
    /*    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    case PROP_EDGE_A:
      filter->edge_a = g_value_get_uint (value);
      break;
    case PROP_EDGE_B:
      filter->edge_b = g_value_get_uint (value);
      break; */
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
gst_blend_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBlend *filter = GST_BLEND (object);

  GST_OBJECT_LOCK (filter);
  switch (prop_id) {
    case PROP_SILENT:
      /* g_value_set_boolean (value, filter->silent);
      break;
    case PROP_EDGE_A:
      g_value_set_uint (value, filter->edge_a);
      break;
    case PROP_EDGE_B:
      g_value_set_uint (value, filter->edge_b);
      break; */
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (filter);
}

/* GstElement vmethod implementations */

/* Handle the link with other elements. */
static gboolean
gst_blend_set_caps (GstBaseTransform * btrans, GstCaps * incaps,
    GstCaps * outcaps)
{
  GstBlend *filter = GST_BLEND (btrans);
  GstStructure *structure;
  gboolean ret = FALSE;

  structure = gst_caps_get_structure (incaps, 0);

  GST_OBJECT_LOCK (filter);
  if (gst_structure_get_int (structure, "width", &filter->width) &&
      gst_structure_get_int (structure, "height", &filter->height)) {
    gint area = filter->width * filter->height;

    g_free (filter->buffer);
    filter->buffer = (guint32 *) g_malloc0 (area * 2 * sizeof (guint32));

    filter->current_buffer = filter->buffer;
    filter->prev_buffer = filter->buffer + area;

    ret = TRUE;
  }
  GST_OBJECT_UNLOCK (filter);

  return ret;
}

/* Actual processing. */
static GstFlowReturn
gst_blend_transform (GstBaseTransform * btrans,
    GstBuffer * in_buf, GstBuffer * out_buf)
{
  GstBlend *filter = GST_BLEND (btrans);
  gint video_size, edge_a, edge_b;
  guint32 *src = (guint32 *) GST_BUFFER_DATA (in_buf);
  guint32 *dest = (guint32 *) GST_BUFFER_DATA (out_buf);
  GstClockTime timestamp;
  gint64 stream_time;

  /* GstController: update the properties */
  timestamp = GST_BUFFER_TIMESTAMP (in_buf);
  stream_time =
      gst_segment_to_stream_time (&btrans->segment, GST_FORMAT_TIME, timestamp);

  GST_DEBUG_OBJECT (filter, "sync to %" GST_TIME_FORMAT,
      GST_TIME_ARGS (timestamp));

  if (GST_CLOCK_TIME_IS_VALID (stream_time))
    gst_object_sync_values (G_OBJECT (filter), stream_time);
  /*
  GST_OBJECT_LOCK (filter);
  edge_a = filter->edge_a;
  edge_b = filter->edge_b;
  GST_OBJECT_UNLOCK (filter);
  */
  video_size = filter->width * filter->height;
  transform (filter, src, dest, video_size, edge_a, edge_b);

  return GST_FLOW_OK;
}

/* Entry point to initialize the plug-in.
 * Register the element factories and other features. */
gboolean
gst_blend_plugin_init (GstPlugin * blend)
{
  /* debug category for fltering log messages */
  GST_DEBUG_CATEGORY_INIT (gst_blend_debug, "blend", 0,
      "Template blend");

  return gst_element_register (blend, "blend", GST_RANK_NONE,
      GST_TYPE_BLEND);
}

/*** Now the image processing work.... ***/
/* Keep the values absolute. */
static inline int
abs_int (int val)
{
  if (val > 0) {
    return val;
  } else {
    return -val;
  }
}

/* Keep the values inbounds. */
static gint
gate_int (gint value, gint min, gint max)
{
  if (value < min) {
    return min;
  } else if (value > max) {
    return max;
  } else {
    return value;
  }
}

/* Transform processes each frame. */
static void
transform (GstBlend *filter, guint32 * src, guint32 * dest, gint video_area,
    gint edge_a, gint edge_b)
{
  guint32 in, out;
  guint32 red, green, blue;
  guint32 p_red, p_green, p_blue;
  guint32 *prev;
  gint x;

  prev = filter->prev_buffer;

  for (x = 0; x < video_area; x++) {
    in = *src++;
    out = *prev++;

    red = (in >> 16) & 0xff;
    green = (in >> 8) & 0xff;
    blue = (in) & 0xff;

    p_red = (out >> 16) & 0xff;
    p_green = (out >> 8) & 0xff;
    p_blue = (out) & 0xff;

    red = gate_int ((red * 0.66)+ (p_red * 0.33), 0, 255);
    green = gate_int ((green * 0.66) + (p_green * 0.33), 0, 255);
    blue = gate_int ((blue * 0.66) + (p_blue * 0.33), 0, 255);

    *prev = ((red << 16) | (green << 8) | blue);
  }

  memcpy (dest, filter->prev_buffer, video_area * sizeof (guint32));

  prev = filter->current_buffer;
  filter->current_buffer = filter->prev_buffer;
  filter->prev_buffer = prev;
}