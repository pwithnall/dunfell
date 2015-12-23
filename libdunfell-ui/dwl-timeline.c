/* vim:set et sw=2 cin cino=t0,f0,(0,{s,>2s,n-s,^-s,e2s: */
/*
 * Copyright © Philip Withnall 2015 <philip@tecnocode.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * SECTION:dwl-timeline
 * @short_description: Dunfell timeline renderer
 * @stability: Unstable
 * @include: libdunfell-ui/dwl-timeline.h
 *
 * TODO
 *
 * Since: UNRELEASED
 */

#include "config.h"

#include <errno.h>
#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include "dfl-main-context.h"
#include "dfl-thread.h"
#include "dfl-time-sequence.h"
#include "dfl-types.h"
#include "dwl-timeline.h"


static void dwl_timeline_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec);
static void dwl_timeline_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec);
static void dwl_timeline_dispose (GObject *object);
static gboolean dwl_timeline_draw (GtkWidget *widget,
                                   cairo_t   *cr);
static void dwl_timeline_get_preferred_width (GtkWidget *widget,
                                              gint      *minimum_width,
                                              gint      *natural_width);
static void dwl_timeline_get_preferred_height (GtkWidget *widget,
                                               gint      *minimum_height,
                                               gint      *natural_height);

static void add_default_css (GtkStyleContext *context);

struct _DwlTimeline
{
  GtkWidget parent;

  GPtrArray/*<owned DflMainContext>*/ *main_contexts;  /* owned */
  GPtrArray/*<owned DflThread>*/ *threads;  /* owned */

  gfloat zoom;
};

typedef enum
{
  PROP_ZOOM = 1,
} DwlTimelineProperty;

G_DEFINE_TYPE (DwlTimeline, dwl_timeline, GTK_TYPE_WIDGET)

static void
dwl_timeline_class_init (DwlTimelineClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = dwl_timeline_get_property;
  object_class->set_property = dwl_timeline_set_property;
  object_class->dispose = dwl_timeline_dispose;

  widget_class->draw = dwl_timeline_draw;
  widget_class->get_preferred_width = dwl_timeline_get_preferred_width;
  widget_class->get_preferred_height = dwl_timeline_get_preferred_height;

  /* TODO: Proper accessibility support. */
  gtk_widget_class_set_accessible_role (widget_class, ATK_ROLE_CHART);
  gtk_widget_class_set_css_name (widget_class, "timeline");

  /**
   * DwlTimeline:zoom:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_ZOOM,
                                   g_param_spec_float ("zoom", "Zoom",
                                                       "Zoom level.",
                                                       1.0, G_MAXFLOAT, 1.0,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_STATIC_STRINGS));
}

static void
dwl_timeline_init (DwlTimeline *self)
{
  gtk_widget_set_has_window (GTK_WIDGET (self), FALSE);

  self->zoom = 1.0;

  add_default_css (gtk_widget_get_style_context (GTK_WIDGET (self)));
}

static void
dwl_timeline_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  DwlTimeline *self = DWL_TIMELINE (object);

  switch ((DwlTimelineProperty) property_id)
    {
    case PROP_ZOOM:
      g_value_set_float (value, self->zoom);
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
dwl_timeline_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  DwlTimeline *self = DWL_TIMELINE (object);

  switch ((DwlTimelineProperty) property_id)
    {
    case PROP_ZOOM:
      self->zoom = g_value_get_float (value);
      gtk_widget_queue_resize (GTK_WIDGET (self));
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
dwl_timeline_dispose (GObject *object)
{
  DwlTimeline *self = DWL_TIMELINE (object);

  g_clear_pointer (&self->main_contexts, g_ptr_array_unref);
  g_clear_pointer (&self->threads, g_ptr_array_unref);

  /* Chain up to the parent class */
  G_OBJECT_CLASS (dwl_timeline_parent_class)->dispose (object);
}

/**
 * dwl_timeline_new:
 * @threads: (element-type DflThread): TODO
 * @main_contexts: (element-type DflMainContext): TODO
 *
 * TODO
 *
 * Returns: (transfer full): a new #DwlTimeline
 * Since: UNRELEASED
 */
DwlTimeline *
dwl_timeline_new (GPtrArray *threads,
                  GPtrArray *main_contexts)
{
  DwlTimeline *timeline = NULL;

  /* TODO: Properties. */
  timeline = g_object_new (DWL_TYPE_TIMELINE, NULL);

  timeline->threads = g_ptr_array_ref (threads);
  timeline->main_contexts = g_ptr_array_ref (main_contexts);

  return timeline;
}

static void
add_default_css (GtkStyleContext *context)
{
  GtkCssProvider *provider = NULL;
  GError *error = NULL;
  const gchar *css;

  css =
    "timeline.thread_guide { color: #cccccc }\n"
    "timeline.thread { color: rgb(139, 142, 143) }\n"
    "timeline.main_context_dispatch { background-color: red; "
                                     "border: 1px solid black }\n";

  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (provider, css, -1, &error);
  g_assert_no_error (error);

  gtk_style_context_add_provider (context, GTK_STYLE_PROVIDER (provider),
                                  GTK_STYLE_PROVIDER_PRIORITY_FALLBACK);

  g_object_unref (provider);
}

static gint
timestamp_to_pixels (DwlTimeline  *self,
                     DflTimestamp  timestamp)
{
  return timestamp * self->zoom;
}

static gint
duration_to_pixels (DwlTimeline *self,
                    DflDuration  duration)
{
  return duration * self->zoom;
}

#define THREAD_MIN_WIDTH 100 /* pixels */
#define THREAD_NATURAL_WIDTH 140 /* pixels */
#define HEADER_HEIGHT 100 /* pixels */
#define FOOTER_HEIGHT 30 /* pixels */
#define MAIN_CONTEXT_ACQUIRED_WIDTH 3 /* pixels */
#define MAIN_CONTEXT_DISPATCH_WIDTH 10 /* pixels */

static gboolean
dwl_timeline_draw (GtkWidget *widget,
                   cairo_t   *cr)
{
  DwlTimeline *self = DWL_TIMELINE (widget);
  GtkStyleContext *context;
  gint widget_width;
  guint i, n_threads;
  DflTimestamp min_timestamp, max_timestamp;

  context = gtk_widget_get_style_context (widget);
  widget_width = gtk_widget_get_allocated_width (widget);

  /* TODO: Move these pre-calculations elsewhere */
  n_threads = self->threads->len;
  min_timestamp = G_MAXUINT64;
  max_timestamp = 0;

  for (i = 0; i < n_threads; i++)
    {
      DflThread *thread = self->threads->pdata[i];
      min_timestamp = MIN (min_timestamp, dfl_thread_get_new_timestamp (thread));
      max_timestamp = MAX (max_timestamp, dfl_thread_get_free_timestamp (thread));
    }

  /* Draw the threads. */
  for (i = 0; i < n_threads; i++)
    {
      DflThread *thread = self->threads->pdata[i];
      gdouble thread_centre;
      PangoLayout *layout = NULL;
      gchar *text = NULL;
      PangoRectangle layout_rect;

      thread_centre = widget_width / n_threads * (2 * i + 1) / 2;

      /* Guide line for the entire length of the thread. */
      gtk_style_context_add_class (context, "thread_guide");
      gtk_render_line (context, cr,
                       thread_centre,
                       HEADER_HEIGHT + timestamp_to_pixels (self, 0),
                       thread_centre,
                       HEADER_HEIGHT + timestamp_to_pixels (self, max_timestamp - min_timestamp));
      gtk_style_context_remove_class (context, "thread_guide");

      /* Line for the actual live length of the thread, plus its label. */
      gtk_style_context_add_class (context, "thread");
      gtk_render_line (context, cr,
                       thread_centre,
                       HEADER_HEIGHT + timestamp_to_pixels (self, dfl_thread_get_new_timestamp (thread) - min_timestamp),
                       thread_centre,
                       HEADER_HEIGHT + timestamp_to_pixels (self, dfl_thread_get_free_timestamp (thread) - min_timestamp));
      gtk_style_context_remove_class (context, "thread");

      /* Thread label. */
      gtk_style_context_add_class (context, "thread_header");

      text = g_strdup_printf ("Thread %" G_GUINT64_FORMAT, dfl_thread_get_id (thread));
      layout = gtk_widget_create_pango_layout (widget, text);
      g_free (text);

      pango_layout_get_pixel_extents (layout, NULL, &layout_rect);

      gtk_render_layout (context, cr,
                         thread_centre - layout_rect.width / 2,
                         HEADER_HEIGHT / 2 - layout_rect.height / 2,
                         layout);
      g_object_unref (layout);

      gtk_style_context_remove_class (context, "thread_header");
    }

  /* Draw the main contexts on top. */
  for (i = 0; i < self->main_contexts->len; i++)
    {
      DflMainContext *main_context = self->main_contexts->pdata[i];
      DflTimeSequenceIter iter;
      DflTimestamp timestamp;
      DflThreadOwnershipData *data;
      GdkRGBA color;

      /* Iterate through the thread ownership events. */
      gtk_style_context_add_class (context, "main_context");
      cairo_save (cr);

      gtk_style_context_get_color (context, gtk_widget_get_state_flags (widget),
                                   &color);

      cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
      cairo_set_line_width (cr, MAIN_CONTEXT_ACQUIRED_WIDTH);
      cairo_new_path (cr);

      /* TODO: Set the start timestamp according to the clip area */
      dfl_main_context_thread_ownership_iter (main_context, &iter, 0);

      while (dfl_time_sequence_iter_next (&iter, &timestamp, (gpointer *) &data))
        {
          gdouble thread_centre;
          gint timestamp_y;
          guint thread_index;

          /* TODO: This should not be so slow. */
          for (thread_index = 0; thread_index < n_threads; thread_index++)
            {
              if (dfl_thread_get_id (self->threads->pdata[thread_index]) ==
                  data->thread_id)
                break;
            }
          g_assert (thread_index < n_threads);

          thread_centre = widget_width / n_threads * (2 * thread_index + 1) / 2;
          timestamp_y = HEADER_HEIGHT + timestamp_to_pixels (self, timestamp - min_timestamp);

          cairo_line_to (cr,
                         thread_centre + 0.5,
                         timestamp_y + 0.5);
          cairo_line_to (cr,
                         thread_centre + 0.5,
                         timestamp_y +
                         duration_to_pixels (self, data->duration) + 0.5);
        }

      gdk_cairo_set_source_rgba (cr, &color);
      cairo_stroke (cr);

      cairo_restore (cr);
      gtk_style_context_remove_class (context, "main_context");

      /* Iterate through the dispatch events. */
      gtk_style_context_add_class (context, "main_context_dispatch");

      /* TODO: Set the start timestamp according to the clip area */
      dfl_main_context_dispatch_iter (main_context, &iter, 0);

      while (dfl_time_sequence_iter_next (&iter, &timestamp, (gpointer *) &data))
        {
          gdouble thread_centre, dispatch_width, dispatch_height;
          gint timestamp_y;
          guint thread_index;

          /* TODO: This should not be so slow. */
          for (thread_index = 0; thread_index < n_threads; thread_index++)
            {
              if (dfl_thread_get_id (self->threads->pdata[thread_index]) ==
                  data->thread_id)
                break;
            }
          g_assert (thread_index < n_threads);

          thread_centre = widget_width / n_threads * (2 * thread_index + 1) / 2;
          timestamp_y = HEADER_HEIGHT + timestamp_to_pixels (self, timestamp - min_timestamp);

          dispatch_width = MAIN_CONTEXT_DISPATCH_WIDTH;
          dispatch_height = duration_to_pixels (self, data->duration);

          gtk_render_background (context, cr,
                                 thread_centre - dispatch_width / 2.0,
                                 timestamp_y,
                                 dispatch_width,
                                 dispatch_height);
          gtk_render_frame (context, cr,
                            thread_centre - dispatch_width / 2.0,
                            timestamp_y,
                            dispatch_width,
                            dispatch_height);
        }

      gtk_style_context_remove_class (context, "main_context_dispatch");
    }

  return FALSE;
}

static void
dwl_timeline_get_preferred_width (GtkWidget *widget,
                                  gint      *minimum_width,
                                  gint      *natural_width)
{
  DwlTimeline *self = DWL_TIMELINE (widget);
  guint n_threads;

  n_threads = self->threads->len;

  if (minimum_width != NULL)
    *minimum_width = MAX (1, n_threads * THREAD_MIN_WIDTH);
  if (natural_width != NULL)
    *natural_width = MAX (1, n_threads * THREAD_NATURAL_WIDTH);
}

static void
dwl_timeline_get_preferred_height (GtkWidget *widget,
                                   gint      *minimum_height,
                                   gint      *natural_height)
{
  DwlTimeline *self = DWL_TIMELINE (widget);
  DflTimestamp min_timestamp = G_MAXUINT64, max_timestamp = 0;
  gint height;
  guint i;

  /* What’s the maximum height of any of the threads? */
  for (i = 0; i < self->threads->len; i++)
    {
      DflThread *thread = self->threads->pdata[i];

      min_timestamp = MIN (min_timestamp,
                           dfl_thread_get_new_timestamp (thread));
      max_timestamp = MAX (max_timestamp,
                           dfl_thread_get_free_timestamp (thread));
    }

  g_assert (max_timestamp >= min_timestamp);

  height = timestamp_to_pixels (self, max_timestamp - min_timestamp);

  if (height > 0)
    height += HEADER_HEIGHT + FOOTER_HEIGHT;

  if (minimum_height != NULL)
    *minimum_height = MAX (1, height);
  if (natural_height != NULL)
    *natural_height = MAX (1, height);
}
