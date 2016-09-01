/* vim:set et sw=2 cin cino=t0,f0,(0,{s,>2s,n-s,^-s,e2s: */
/*
 * Copyright © Philip Withnall 2015, 2016 <philip@tecnocode.co.uk>
 * Copyright © Collabora Ltd. 2016
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
 * SECTION:timeline
 * @short_description: Dunfell timeline renderer
 * @stability: Unstable
 * @include: libdunfell-ui/timeline.h
 *
 * TODO
 *
 * Since: 0.1.0
 */

#include "config.h"

#include <errno.h>
#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include "libdunfell/main-context.h"
#include "libdunfell/model.h"
#include "libdunfell/source.h"
#include "libdunfell/task.h"
#include "libdunfell/thread.h"
#include "libdunfell/time-sequence.h"
#include "libdunfell/types.h"
#include "libdunfell-ui/enums.h"
#include "libdunfell-ui/timeline.h"


static void dwl_timeline_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec);
static void dwl_timeline_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec);
static void dwl_timeline_dispose (GObject *object);
static void dwl_timeline_realize (GtkWidget *widget);
static void dwl_timeline_unrealize (GtkWidget *widget);
static void dwl_timeline_map (GtkWidget *widget);
static void dwl_timeline_unmap (GtkWidget *widget);
static void dwl_timeline_size_allocate (GtkWidget     *widget,
                                        GtkAllocation *allocation);
static gboolean dwl_timeline_draw (GtkWidget *widget,
                                   cairo_t   *cr);
static void dwl_timeline_get_preferred_width (GtkWidget *widget,
                                              gint      *minimum_width,
                                              gint      *natural_width);
static void dwl_timeline_get_preferred_height (GtkWidget *widget,
                                               gint      *minimum_height,
                                               gint      *natural_height);
static gboolean dwl_timeline_scroll_event (GtkWidget      *widget,
                                           GdkEventScroll *event);
static gboolean dwl_timeline_motion_notify_event (GtkWidget      *widget,
                                                  GdkEventMotion *event);
static gboolean dwl_timeline_button_release_event (GtkWidget      *widget,
                                                   GdkEventButton *event);
static gboolean dwl_timeline_move_selected (DwlTimeline              *timeline,
                                            DwlSelectionMovementStep  step,
                                            gint                      distance);

static void add_default_css (GtkStyleContext *context);
static void update_cache    (DwlTimeline     *self);

#define ZOOM_MIN 0.001
#define ZOOM_MAX 1000.0

typedef enum
{
  ELEMENT_NONE,
  ELEMENT_SOURCE,
  ELEMENT_CONTEXT_DISPATCH,
  ELEMENT_TASK,
} DwlTimelineElement;

struct _DwlTimeline
{
  GtkWidget parent;

  GdkWindow *event_window;  /* owned */

  DflModel *model;  /* (ownership full) */

  GPtrArray/*<owned DflMainContext>*/ *main_contexts;  /* owned */
  GPtrArray/*<owned DflThread>*/ *threads;  /* owned */
  GPtrArray/*<owned DflSource>*/ *sources;  /* owned */
  GPtrArray/*<owned DflTask>*/ *tasks;  /* owned */

  gfloat zoom;  /* pixels per unit time */

  /* Cached dimensions. */
  DflTimestamp min_timestamp;
  DflTimestamp max_timestamp;
  DflDuration duration;

  /* Current hover item. */
  struct {
    DwlTimelineElement type;
    guint index;
    DflTimeSequenceIter *iter;  /* owned */
  } hover_element;

  /* Currently selected item. */
  struct {
    DwlTimelineElement type;
    guint index;
    DflTimeSequenceIter *iter;  /* owned */
  } selected_element;
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
  GtkBindingSet *binding_set;

  object_class->get_property = dwl_timeline_get_property;
  object_class->set_property = dwl_timeline_set_property;
  object_class->dispose = dwl_timeline_dispose;

  widget_class->realize = dwl_timeline_realize;
  widget_class->unrealize = dwl_timeline_unrealize;
  widget_class->map = dwl_timeline_map;
  widget_class->unmap = dwl_timeline_unmap;
  widget_class->size_allocate = dwl_timeline_size_allocate;
  widget_class->draw = dwl_timeline_draw;
  widget_class->get_preferred_width = dwl_timeline_get_preferred_width;
  widget_class->get_preferred_height = dwl_timeline_get_preferred_height;
  widget_class->scroll_event = dwl_timeline_scroll_event;
  widget_class->motion_notify_event = dwl_timeline_motion_notify_event;
  widget_class->button_release_event = dwl_timeline_button_release_event;

  /* TODO: Proper accessibility support. */
  gtk_widget_class_set_accessible_role (widget_class, ATK_ROLE_CHART);
  gtk_widget_class_set_css_name (widget_class, "timeline");

  /**
   * DwlTimeline:zoom:
   *
   * TODO
   *
   * Since: 0.1.0
   */
  g_object_class_install_property (object_class, PROP_ZOOM,
                                   g_param_spec_float ("zoom", "Zoom",
                                                       "Zoom level.",
                                                       ZOOM_MIN, ZOOM_MAX, 1.0,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_STATIC_STRINGS));

  /**
   * DwlTimeline::move-selected:
   * @box: the #DwlTimeline on which the signal is emitted
   * @step: the granularity of the move, as a #DwlSelectionMovementStep
   * @count: the number of @step units to move
   *
   * The ::move-selected signal is a
   * [keybinding signal][GtkBindingSignal]
   * which gets emitted when the user initiates movement of the selection
   * cursor in the timeline, which indicates the currently selected element.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control the timeline
   * programmatically.
   *
   * The default bindings are:
   * - N moves to the next element
   * - P moves to the previous element
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event;
   *    %FALSE to propagate the event further.
   * Since: UNRELEASED
   */
  g_signal_new ("move-selected", G_TYPE_FROM_CLASS (klass),
                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                0, NULL, NULL, NULL,
                G_TYPE_BOOLEAN, 2,
                DWL_TYPE_SELECTION_MOVEMENT_STEP, G_TYPE_INT);

  /* Key bindings. */
  binding_set = gtk_binding_set_by_class (klass);

  gtk_binding_entry_add_signal (binding_set, GDK_KEY_n, 0,
                                "move-selected", 2,
                                DWL_TYPE_SELECTION_MOVEMENT_STEP,
                                DWL_SELECTION_MOVEMENT_SIBLING,
                                G_TYPE_INT, 1);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_p, 0,
                                "move-selected", 2,
                                DWL_TYPE_SELECTION_MOVEMENT_STEP,
                                DWL_SELECTION_MOVEMENT_SIBLING,
                                G_TYPE_INT, -1);
}

static void
dwl_timeline_init (DwlTimeline *self)
{
  self->zoom = 1.0;

  add_default_css (gtk_widget_get_style_context (GTK_WIDGET (self)));

  gtk_widget_set_can_focus (GTK_WIDGET (self), TRUE);
  gtk_widget_set_has_window (GTK_WIDGET (self), FALSE);

  g_signal_connect (self, "move-selected",
                    (GCallback) dwl_timeline_move_selected, NULL);
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
      dwl_timeline_set_zoom (self, g_value_get_float (value));
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
dwl_timeline_dispose (GObject *object)
{
  DwlTimeline *self = DWL_TIMELINE (object);

  g_clear_object (&self->model);
  g_clear_pointer (&self->sources, g_ptr_array_unref);
  g_clear_pointer (&self->main_contexts, g_ptr_array_unref);
  g_clear_pointer (&self->threads, g_ptr_array_unref);
  g_clear_pointer (&self->tasks, g_ptr_array_unref);
  g_clear_pointer (&self->hover_element.iter, dfl_time_sequence_iter_free);
  g_clear_pointer (&self->selected_element.iter, dfl_time_sequence_iter_free);

  /* Chain up to the parent class */
  G_OBJECT_CLASS (dwl_timeline_parent_class)->dispose (object);
}

/**
 * dwl_timeline_new:
 * @model: (transfer none): TODO
 *
 * TODO
 *
 * Returns: (transfer full): a new #DwlTimeline
 * Since: 0.1.0
 */
DwlTimeline *
dwl_timeline_new (DflModel *model)
{
  DwlTimeline *timeline = NULL;

  /* TODO: Properties. */
  timeline = g_object_new (DWL_TYPE_TIMELINE, NULL);

  timeline->model = g_object_ref (model);

  timeline->threads = dfl_model_dup_threads (model);
  timeline->main_contexts = dfl_model_dup_main_contexts (model);
  timeline->sources = dfl_model_dup_sources (model);
  timeline->tasks = dfl_model_dup_tasks (model);

  update_cache (timeline);

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
    "timeline.millisecond_marker { color: #d3d7cf }\n"
    "timeline.millisecond_marker_label { color: #babdb6 }\n"
    "timeline.ten_millisecond_marker { color: #babdb6 }\n"
    "timeline.ten_millisecond_marker_label { color: #888a85 }\n"
    "timeline.hundred_millisecond_marker { color: #888a85 }\n"
    "timeline.hundred_millisecond_marker_label { color: #555753 }\n"
    "timeline.thousand_millisecond_marker { color: #555753 }\n"
    "timeline.thousand_millisecond_marker_label { color: #2e3436 }\n"
    "timeline.main_context_dispatch { background-color: #3465a4; "
                                     "border: 1px solid #2e3436 }\n"
    "timeline.main_context_dispatch_hover { background-color: #729fcf }\n"
    "timeline.main_context_dispatch_selected { background-color: #729fcf }\n"
    "timeline.source { background-color: #c17d11 }\n"
    "timeline.source_hover { background-color: #e9b96e }\n"
    "timeline.source_selected { background-color: #73d216 }\n"
    "timeline.source_unattached { background-color: #cc0000 }\n"
    "timeline.source_dispatch { background-color: #73d216; "
                              " border: 1px solid #2e3436 }\n"
    "timeline.source_dispatch_line { color: #555753 }\n"
    "timeline.task_new { background-color: #edd400 }\n"
    "timeline.task_new_hover { background-color: #fce94f }\n"
    "timeline.task_new_selected { background-color: #73d216 }\n"
    "timeline.task_return_line { color: #555753 }\n"
    "timeline.task_propagate_line { color: #555753 }\n";

  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (provider, css, -1, &error);
  g_assert_no_error (error);

  gtk_style_context_add_provider (context, GTK_STYLE_PROVIDER (provider),
                                  GTK_STYLE_PROVIDER_PRIORITY_FALLBACK);

  g_object_unref (provider);
}

#define THREAD_MIN_WIDTH 100 /* pixels */
#define THREAD_NATURAL_WIDTH 140 /* pixels */
#define HEADER_HEIGHT 100 /* pixels */
#define FOOTER_HEIGHT 30 /* pixels */
#define MAIN_CONTEXT_ACQUIRED_WIDTH 3 /* pixels */
#define MAIN_CONTEXT_DISPATCH_WIDTH 10 /* pixels */
#define SOURCE_BORDER_WIDTH 1 /* pixel */
#define SOURCE_OFFSET 20 /* pixels */
#define SOURCE_WIDTH 10 /* pixels */
#define SOURCE_DISPATCH_WIDTH 2 /* pixels */
#define SOURCE_NAME_OFFSET 30 /* pixels */
#define SOURCE_DISPATCH_DETAILS_OFFSET 10 /* pixels */
#define SOURCE_ATTACH_DESTROY_WIDTH 1 /* pixel */
#define TASK_BORDER_WIDTH 2 /* pixels */
#define TASK_OFFSET 20 /* pixels */
#define TASK_WIDTH 12 /* pixels */
#define TASK_SOURCE_TAG_OFFSET 30 /* pixels */
#define TASK_CALLBACK_OFFSET 30 /* pixels */
#define LEFT_GUTTER_WIDTH 70 /* pixels */
#define LEFT_GUTTER_RIGHT_PADDING 5 /* pixels */
#define AUTO_SCROLL_MARGIN 0.1 /* × viewport height */

/* Calculate various values from the data model we have (the threads, main
 * contexts and sources). The calculated values will be used frequently when
 * drawing. */
static void
update_cache (DwlTimeline *self)
{
  guint i;
  DflTimestamp min_timestamp, max_timestamp;

  min_timestamp = G_MAXUINT64;
  max_timestamp = 0;

  for (i = 0; i < self->threads->len; i++)
    {
      DflThread *thread = self->threads->pdata[i];
      min_timestamp = MIN (min_timestamp, dfl_thread_get_new_timestamp (thread));
      max_timestamp = MAX (max_timestamp, dfl_thread_get_free_timestamp (thread));
    }

  if (self->threads->len == 0)
    min_timestamp = 0;

  g_assert (max_timestamp >= min_timestamp);

  /* Update the cache. */
  self->min_timestamp = min_timestamp;
  self->max_timestamp = max_timestamp;
  self->duration = max_timestamp - min_timestamp;
}

static gint
timestamp_to_y (DwlTimeline  *self,
                DflTimestamp  timestamp)
{
  g_return_val_if_fail (timestamp <= G_MAXINT / self->zoom, G_MAXINT);
  return HEADER_HEIGHT + timestamp * self->zoom;
}

static DflTimestamp
y_to_timestamp (DwlTimeline *self,
                gint         y)
{
  g_return_val_if_fail (y > HEADER_HEIGHT, 0);
  return (y - HEADER_HEIGHT) / self->zoom;
}

static DflDuration
pixels_to_duration (DwlTimeline *self,
                    gint         pixels)
{
  return pixels / self->zoom;
}

static gint
duration_to_pixels (DwlTimeline *self,
                    DflDuration  duration)
{
  g_return_val_if_fail (duration <= G_MAXINT / self->zoom, G_MAXINT);
  return duration * self->zoom;
}

static gboolean
pixel_is_timestamp (DwlTimeline *self,
                    gint         pixel)
{
  return (pixel > HEADER_HEIGHT &&
          pixel <= HEADER_HEIGHT + duration_to_pixels (self, self->duration));
}

static void
dwl_timeline_realize (GtkWidget *widget)
{
  DwlTimeline *self = DWL_TIMELINE (widget);
  GdkWindow *parent_window;
  GdkWindowAttr attributes;
  gint attributes_mask;
  GtkAllocation allocation;

  gtk_widget_set_realized (widget, TRUE);
  parent_window = gtk_widget_get_parent_window (widget);
  gtk_widget_set_window (widget, parent_window);
  g_object_ref (parent_window);

  gtk_widget_get_allocation (widget, &allocation);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_ONLY;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.event_mask = gtk_widget_get_events (widget) |
                          GDK_SCROLL_MASK | GDK_POINTER_MOTION_MASK |
                          GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK;
  attributes_mask = GDK_WA_X | GDK_WA_Y;

  self->event_window = gdk_window_new (parent_window,
                                       &attributes,
                                       attributes_mask);
  gtk_widget_register_window (widget, self->event_window);
}

static void
dwl_timeline_unrealize (GtkWidget *widget)
{
  DwlTimeline *self = DWL_TIMELINE (widget);

  if (self->event_window != NULL)
    {
      gtk_widget_unregister_window (widget, self->event_window);
      gdk_window_destroy (self->event_window);
      self->event_window = NULL;
    }

  GTK_WIDGET_CLASS (dwl_timeline_parent_class)->unrealize (widget);
}

static void
dwl_timeline_map (GtkWidget *widget)
{
  DwlTimeline *self = DWL_TIMELINE (widget);

  GTK_WIDGET_CLASS (dwl_timeline_parent_class)->map (widget);

  if (self->event_window)
    gdk_window_show (self->event_window);
}

static void
dwl_timeline_unmap (GtkWidget *widget)
{
  DwlTimeline *self = DWL_TIMELINE (widget);

  if (self->event_window)
    gdk_window_hide (self->event_window);

  GTK_WIDGET_CLASS (dwl_timeline_parent_class)->unmap (widget);
}

static void
dwl_timeline_size_allocate (GtkWidget     *widget,
                            GtkAllocation *allocation)
{
  DwlTimeline *self = DWL_TIMELINE (widget);

  gtk_widget_set_allocation (widget, allocation);

  if (gtk_widget_get_realized (widget))
    gdk_window_move_resize (self->event_window,
                            allocation->x,
                            allocation->y,
                            allocation->width,
                            allocation->height);
}

static guint
thread_id_to_index (DwlTimeline *self,
                    DflThreadId  thread_id)
{
  guint thread_index, n_threads;

  n_threads = self->threads->len;

  /* TODO: This should not be so slow. */
  for (thread_index = 0; thread_index < n_threads; thread_index++)
    {
      if (dfl_thread_get_id (self->threads->pdata[thread_index]) == thread_id)
        break;
    }
  g_assert (thread_index < n_threads);

  return thread_index;
}

/* Get the X coordinate of the centre of the given thread. */
static gint
thread_index_to_centre (DwlTimeline *self,
                        guint        thread_index)
{
  gint widget_width;

  widget_width = gtk_widget_get_allocated_width (GTK_WIDGET (self));

  return (widget_width - LEFT_GUTTER_WIDTH) /
         self->threads->len * (2 * thread_index + 1) / 2 +
         LEFT_GUTTER_WIDTH;
}

/* Draw a line from point 1 to point 2, first moving horizontally from point 1,
 * then drawing a curved corner, then moving vertically to point 2. */
static void
draw_cornered_line (DwlTimeline *self,
                    cairo_t     *cr,
                    gdouble      p1_x,
                    gdouble      p1_y,
                    gdouble      p2_x,
                    gdouble      p2_y)
{
  GtkStyleContext *context;
  GdkRGBA color;
  gdouble centre_of_arc_x, centre_of_arc_y;
  gdouble arc_angle_start, arc_angle_finish;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));

  if (p1_x < p2_x)
    {
      centre_of_arc_x = p2_x - SOURCE_WIDTH / 2.0;
      arc_angle_finish = 0.0;
    }
  else
    {
      centre_of_arc_x = p2_x + SOURCE_WIDTH / 2.0;
      arc_angle_finish = M_PI;
    }

  if (p1_y < p2_y)
    {
      centre_of_arc_y = p1_y + SOURCE_WIDTH / 2.0;
      arc_angle_start = 3.0 * M_PI / 2.0;
    }
  else
    {
      centre_of_arc_y = p1_y - SOURCE_WIDTH / 2.0;
      arc_angle_start = M_PI / 2.0;
    }

  cairo_new_path (cr);
  cairo_move_to (cr,
                 p1_x + 0.5,
                 p1_y + 0.5);
  cairo_line_to (cr,
                 centre_of_arc_x + 0.5,
                 p1_y + 0.5);

  if (arc_angle_start > arc_angle_finish)
    cairo_arc_negative (cr,
                        centre_of_arc_x + 0.5,
                        centre_of_arc_y + 0.5,
                        SOURCE_WIDTH / 2.0,
                        arc_angle_start,
                        arc_angle_finish);
  else
    cairo_arc (cr,
               centre_of_arc_x + 0.5,
               centre_of_arc_y + 0.5,
               SOURCE_WIDTH / 2.0,
               arc_angle_start,
               arc_angle_finish);

  cairo_line_to (cr,
                 p2_x + 0.5,
                 p2_y + 0.5);

  gtk_style_context_get_color (context,
                               gtk_widget_get_state_flags (GTK_WIDGET (self)),
                               &color);
  gdk_cairo_set_source_rgba (cr, &color);
  cairo_stroke (cr);
}

static void
draw_source_dispatch_line (DwlTimeline           *self,
                           cairo_t               *cr,
                           DflSource             *source,
                           gdouble                source_x,
                           gdouble                source_y,
                           DflTimestamp           dispatch_timestamp,
                           DflSourceDispatchData *dispatch)
{
  gint timestamp_y;
  gdouble dispatch_width, dispatch_height;
  gdouble thread_centre;
  guint thread_index;
  GtkStyleContext *context;
  DflTimestamp min_timestamp;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));
  min_timestamp = self->min_timestamp;
  thread_index = thread_id_to_index (self, dispatch->thread_id);
  thread_centre = thread_index_to_centre (self, thread_index);
  timestamp_y = timestamp_to_y (self, dispatch_timestamp - min_timestamp);

  cairo_set_line_cap (cr, CAIRO_LINE_CAP_BUTT);
  cairo_set_line_width (cr, SOURCE_DISPATCH_WIDTH);

  gtk_style_context_add_class (context, "source_dispatch_line");
  draw_cornered_line (self, cr, thread_centre, timestamp_y, source_x, source_y);
  gtk_style_context_remove_class (context, "source_dispatch_line");

  /* Render the duration of the dispatch. */
  dispatch_width = MAIN_CONTEXT_DISPATCH_WIDTH;
  dispatch_height = duration_to_pixels (self, dispatch->duration);

  gtk_style_context_add_class (context, "source_dispatch");

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

  gtk_style_context_remove_class (context, "source_dispatch");

  /* Label the dispatch with the relevant callback function, but only if the
   * zoom level is high enough to accommodate it.. */
  if (self->zoom > 0.3 &&
      (dispatch->dispatch_name != NULL || dispatch->callback_name != NULL))
    {
      PangoLayout *layout = NULL;
      PangoRectangle layout_rect;
      gchar *text = NULL;

      gtk_style_context_add_class (context, "source_dispatch_details");

      text = g_strdup_printf ("%s\n%s", dispatch->dispatch_name,
                              dispatch->callback_name);
      layout = gtk_widget_create_pango_layout (GTK_WIDGET (self), text);
      g_free (text);

      pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);
      pango_layout_get_pixel_extents (layout, NULL, &layout_rect);

      gtk_render_layout (context, cr,
                         thread_centre + SOURCE_DISPATCH_DETAILS_OFFSET,
                         timestamp_y - layout_rect.height / 2.0,
                         layout);
      g_object_unref (layout);

      gtk_style_context_remove_class (context, "source_dispatch_details");
    }
}

static void
draw_source_attach_destroy_lines (DwlTimeline *self,
                                  cairo_t     *cr,
                                  DflSource   *source,
                                  gdouble      source_x,
                                  gdouble      source_y)
{
  gdouble thread_centre;
  guint thread_index;
  GtkStyleContext *context;
  DflTimestamp min_timestamp;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));
  min_timestamp = self->min_timestamp;

  cairo_set_line_cap (cr, CAIRO_LINE_CAP_BUTT);
  cairo_set_line_width (cr, SOURCE_ATTACH_DESTROY_WIDTH);

  /* Draw the attach line. */
  if (dfl_source_get_attach_timestamp (source) != 0)
    {
      gint attach_timestamp_y;

      thread_index = thread_id_to_index (self,
                                         dfl_source_get_attach_thread_id (source));
      thread_centre = thread_index_to_centre (self, thread_index);

      attach_timestamp_y = timestamp_to_y (self,
                                           dfl_source_get_attach_timestamp (source) - min_timestamp);

      gtk_style_context_add_class (context, "source_attach_line");
      draw_cornered_line (self, cr,
                          thread_centre, attach_timestamp_y,
                          source_x, source_y);
      gtk_style_context_remove_class (context, "source_attach_line");
    }

  /* Draw the attach line. */
  if (dfl_source_get_destroy_timestamp (source) != 0)
    {
      gint destroy_timestamp_y;

      thread_index = thread_id_to_index (self,
                                         dfl_source_get_destroy_thread_id (source));
      thread_centre = thread_index_to_centre (self, thread_index);

      destroy_timestamp_y = timestamp_to_y (self,
                                            dfl_source_get_destroy_timestamp (source) - min_timestamp);

      gtk_style_context_add_class (context, "source_destroy_line");
      draw_cornered_line (self, cr,
                          thread_centre, destroy_timestamp_y,
                          source_x, source_y);
      gtk_style_context_remove_class (context, "source_destroy_line");
    }
}

static void
draw_source_selected (DwlTimeline *self,
                      cairo_t     *cr,
                      DflSource   *source,
                      gdouble      source_x,
                      gdouble      source_y)
{
  GdkRGBA color;
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));

  /* Draw the source’s circle. */
  gtk_style_context_add_class (context, "source");
  gtk_style_context_add_class (context, "source_selected");

  cairo_save (cr);

  cairo_set_line_cap (cr, CAIRO_LINE_CAP_BUTT);
  cairo_set_line_width (cr, SOURCE_BORDER_WIDTH);
  cairo_new_path (cr);

  cairo_arc (cr,
             source_x,
             source_y,
             SOURCE_WIDTH / 2.0,
             0.0, 2 * M_PI);

  cairo_clip_preserve (cr);
  gtk_render_background (context, cr,
                         source_x - SOURCE_WIDTH / 2.0,
                         source_y - SOURCE_WIDTH / 2.0,
                         SOURCE_WIDTH,
                         SOURCE_WIDTH);

  gtk_style_context_get_color (context,
                               gtk_widget_get_state_flags (GTK_WIDGET (self)),
                               &color);
  gdk_cairo_set_source_rgba (cr, &color);
  cairo_stroke (cr);

  cairo_restore (cr);

  gtk_style_context_remove_class (context, "source_selected");
  gtk_style_context_remove_class (context, "source");

  /* Plonk a label next to it for its name. */
  if (dfl_source_get_name (source) != NULL)
    {
      PangoLayout *layout = NULL;
      PangoRectangle layout_rect;

      gtk_style_context_add_class (context, "source_name");

      layout = gtk_widget_create_pango_layout (GTK_WIDGET (self),
                                               dfl_source_get_name (source));

      pango_layout_get_pixel_extents (layout, NULL, &layout_rect);

      gtk_render_layout (context, cr,
                         source_x + SOURCE_NAME_OFFSET,
                         source_y - layout_rect.height / 2.0,
                         layout);
      g_object_unref (layout);

      gtk_style_context_remove_class (context, "source_name");
    }
}

static void
draw_task_circle (DwlTimeline *self,
                  cairo_t     *cr,
                  DflTask     *task,
                  gboolean     hovering,
                  gboolean     selected)
{
  gdouble thread_centre, task_x, task_y;
  guint thread_index;
  GdkRGBA color;
  GtkStyleContext *context;
  DflTimestamp min_timestamp;

  /* New task circle. */
  min_timestamp = self->min_timestamp;
  context = gtk_widget_get_style_context (GTK_WIDGET (self));

  thread_index = thread_id_to_index (self,
                                     dfl_task_get_new_thread_id (task));
  thread_centre = thread_index_to_centre (self, thread_index);

  gtk_style_context_add_class (context, "task_new");

  if (hovering)
    gtk_style_context_add_class (context, "task_new_hover");
  if (selected)
    gtk_style_context_add_class (context, "task_new_selected");

  cairo_save (cr);

  cairo_set_line_cap (cr, CAIRO_LINE_CAP_BUTT);
  cairo_set_line_width (cr, TASK_BORDER_WIDTH);
  cairo_new_path (cr);

  task_x = thread_centre + TASK_OFFSET;
  task_y = timestamp_to_y (self, dfl_task_get_new_timestamp (task) - min_timestamp);

  cairo_arc (cr,
             task_x,
             task_y,
             TASK_WIDTH / 2.0,
             0.0, 2 * M_PI);

  cairo_clip_preserve (cr);
  gtk_render_background (context, cr,
                         task_x - TASK_WIDTH / 2.0,
                         task_y - TASK_WIDTH / 2.0,
                         TASK_WIDTH,
                         TASK_WIDTH);

  gtk_style_context_get_color (context,
                               gtk_widget_get_state_flags (GTK_WIDGET (self)),
                               &color);
  gdk_cairo_set_source_rgba (cr, &color);
  cairo_stroke (cr);

  cairo_restore (cr);

  if (selected)
    gtk_style_context_remove_class (context, "task_new_selected");
  if (hovering)
    gtk_style_context_remove_class (context, "task_new_hover");

  gtk_style_context_remove_class (context, "task_new");

  /* Plonk labels next to it for its source tag and callback. */
  if (selected && dfl_task_get_source_tag_name (task) != NULL)
    {
      PangoLayout *layout = NULL;
      PangoRectangle layout_rect;

      gtk_style_context_add_class (context, "task_source_tag");

      layout = gtk_widget_create_pango_layout (GTK_WIDGET (self),
                                               dfl_task_get_source_tag_name (task));

      pango_layout_get_pixel_extents (layout, NULL, &layout_rect);

      gtk_render_layout (context, cr,
                         task_x + TASK_SOURCE_TAG_OFFSET,
                         task_y - layout_rect.height / 2.0,
                         layout);
      g_object_unref (layout);

      gtk_style_context_remove_class (context, "task_source_tag");
    }

  if (selected && dfl_task_get_callback_name (task) != NULL)
    {
      PangoLayout *layout = NULL;
      PangoRectangle layout_rect;
      gdouble task_return_x, task_return_y;
      gdouble return_thread_centre;
      guint return_thread_index;

      gtk_style_context_add_class (context, "task_callback");

      layout = gtk_widget_create_pango_layout (GTK_WIDGET (self),
                                               dfl_task_get_callback_name (task));

      pango_layout_get_pixel_extents (layout, NULL, &layout_rect);

      /* Work out the label position. If the task has returned, put the label
       * next to the return timestamp. If it hasn't, put it by the
       * g_task_new(). */
      if (dfl_task_get_return_thread_id (task) != 0)
        {
          return_thread_index = thread_id_to_index (self,
                                                    dfl_task_get_return_thread_id (task));
          return_thread_centre = thread_index_to_centre (self,
                                                         return_thread_index);

          task_return_x = return_thread_centre + TASK_OFFSET;
          task_return_y = timestamp_to_y (self, dfl_task_get_return_timestamp (task) - min_timestamp);
        }
      else
        {
          task_return_x = task_x;
          task_return_y = task_y + layout_rect.height;
        }

      gtk_render_layout (context, cr,
                         task_return_x + TASK_CALLBACK_OFFSET,
                         task_return_y - layout_rect.height / 2.0,
                         layout);
      g_object_unref (layout);

      gtk_style_context_remove_class (context, "task_callback");
    }
}

static void
draw_task_return_propagate_lines (DwlTimeline *self,
                                  cairo_t     *cr,
                                  DflTask     *task)
{
  gdouble thread_centre;
  guint thread_index;
  GtkStyleContext *context;
  DflTimestamp min_timestamp;
  gdouble task_x, task_y;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));
  min_timestamp = self->min_timestamp;

  cairo_set_line_cap (cr, CAIRO_LINE_CAP_BUTT);
  cairo_set_line_width (cr, SOURCE_ATTACH_DESTROY_WIDTH);

  thread_index = thread_id_to_index (self,
                                     dfl_task_get_new_thread_id (task));
  thread_centre = thread_index_to_centre (self, thread_index);
  task_x = thread_centre + TASK_OFFSET;
  task_y = timestamp_to_y (self, dfl_task_get_new_timestamp (task) - min_timestamp);

  /* Draw the return line. */
  if (dfl_task_get_return_timestamp (task) != 0)
    {
      gint return_timestamp_y;

      thread_index = thread_id_to_index (self,
                                         dfl_task_get_return_thread_id (task));
      thread_centre = thread_index_to_centre (self, thread_index);

      return_timestamp_y = timestamp_to_y (self,
                                           dfl_task_get_return_timestamp (task) - min_timestamp);

      gtk_style_context_add_class (context, "task_return_line");
      draw_cornered_line (self, cr,
                          thread_centre, return_timestamp_y,
                          task_x, task_y);
      gtk_style_context_remove_class (context, "task_return_line");
    }

  /* Draw the propagate line. */
  if (dfl_task_get_propagate_timestamp (task) != 0)
    {
      gint propagate_timestamp_y;

      thread_index = thread_id_to_index (self,
                                         dfl_task_get_propagate_thread_id (task));
      thread_centre = thread_index_to_centre (self, thread_index);

      propagate_timestamp_y = timestamp_to_y (self,
                                              dfl_task_get_propagate_timestamp (task) - min_timestamp);

      gtk_style_context_add_class (context, "task_propagate_line");
      draw_cornered_line (self, cr,
                          thread_centre, propagate_timestamp_y,
                          task_x, task_y);
      gtk_style_context_remove_class (context, "task_propagate_line");
    }
}

static gboolean
dwl_timeline_draw (GtkWidget *widget,
                   cairo_t   *cr)
{
  DwlTimeline *self = DWL_TIMELINE (widget);
  GtkStyleContext *context;
  gint widget_width, widget_height;
  guint i, n_threads;
  DflTimestamp min_timestamp, max_timestamp, t;
  DflTimestamp min_visible_timestamp, max_visible_timestamp;

  context = gtk_widget_get_style_context (widget);
  widget_width = gtk_widget_get_allocated_width (widget);
  widget_height = gtk_widget_get_allocated_height (widget);

  n_threads = self->threads->len;
  min_timestamp = self->min_timestamp;
  max_timestamp = self->max_timestamp;

  /* Work out our clipping. Once we implement GtkScrollable, this can be
   * removed. */
  min_visible_timestamp = min_timestamp;
  max_visible_timestamp = max_timestamp;

  if (GTK_IS_SCROLLABLE (gtk_widget_get_parent (widget)))
    {
      GtkScrollable *scrollable;
      GtkAdjustment *vadjustment;
      gdouble value, lower, upper;
      gint parent_widget_height;
      DflTimestamp visible_time, value_timestamp;

      scrollable = GTK_SCROLLABLE (gtk_widget_get_parent (GTK_WIDGET (self)));
      vadjustment = gtk_scrollable_get_vadjustment (scrollable);
      parent_widget_height = gtk_widget_get_allocated_height (GTK_WIDGET (scrollable));
      lower = gtk_adjustment_get_lower (vadjustment);
      upper = gtk_adjustment_get_upper (vadjustment);
      value = gtk_adjustment_get_value (vadjustment);

      /* Multiply by two to give some buffer either side of the visible area. */
      visible_time = (DflTimestamp) pixels_to_duration (self, parent_widget_height) * 2;
      value_timestamp = min_timestamp +
                        (max_timestamp - min_timestamp) /
                        (upper - lower) *
                        value;

      if (min_timestamp <= G_MAXUINT64 - visible_time &&
          min_timestamp + visible_time <= value_timestamp)
        min_visible_timestamp = value_timestamp - visible_time;
      else
        min_visible_timestamp = min_timestamp;

      if (max_timestamp >= visible_time &&
          max_timestamp - visible_time >= value_timestamp)
        max_visible_timestamp = value_timestamp + visible_time;
      else
        max_visible_timestamp = max_timestamp;
    }

  g_assert (min_timestamp <= max_timestamp);
  g_assert (min_visible_timestamp <= max_visible_timestamp);
  g_assert (min_timestamp <= min_visible_timestamp);
  g_assert (max_visible_timestamp <= max_timestamp);

  /* If there are no threads, there’s nothing to draw. */
  if (n_threads == 0)
    {
      PangoLayout *layout = NULL;
      PangoRectangle layout_rect;

      gtk_style_context_add_class (context, "message");

      layout = gtk_widget_create_pango_layout (GTK_WIDGET (self),
                                               "Log file is empty.");

      pango_layout_get_pixel_extents (layout, NULL, &layout_rect);

      gtk_render_layout (context, cr,
                         (widget_width - layout_rect.width) / 2.0,
                         (widget_height - layout_rect.height) / 2.0,
                         layout);
      g_object_unref (layout);

      gtk_style_context_remove_class (context, "message");

      return FALSE;
    }

  /* Draw the 1ms, 10ms and 100ms markers. Only draw the higher frequency
   * markers if there’s enough space to render them. */
  for (t = min_timestamp + ((min_visible_timestamp - min_timestamp) / 1000000) * 1000000;
       t <= max_visible_timestamp;
       t += (self->zoom <= 0.0011) ? 100000 : ((self->zoom <= 0.01) ? 10000 : 1000))
    {
      const gchar *line_class_name, *label_class_name;
      gdouble marker_y;
      PangoLayout *layout = NULL;
      g_autofree gchar *text = NULL;
      PangoRectangle layout_rect;

      /* Line. */
      if ((t - min_timestamp) % 1000000 == 0)
        {
          line_class_name = "thousand_millisecond_marker";
          label_class_name = "thousand_millisecond_marker_label";
        }
      else if ((t - min_timestamp) % 100000 == 0)
        {
          line_class_name = "hundred_millisecond_marker";
          label_class_name = "hundred_millisecond_marker_label";
        }
      else if ((t - min_timestamp) % 10000 == 0)
        {
          line_class_name = "ten_millisecond_marker";
          label_class_name = "ten_millisecond_marker_label";
        }
      else
        {
          line_class_name = "millisecond_marker";
          label_class_name = "millisecond_marker_label";
        }

      marker_y = timestamp_to_y (self, t - min_timestamp);

      /* Line. */
      gtk_style_context_add_class (context, line_class_name);
      gtk_render_line (context, cr,
                       LEFT_GUTTER_WIDTH,
                       marker_y,
                       widget_width,
                       marker_y);
      gtk_style_context_remove_class (context, line_class_name);

      /* Label. */
      gtk_style_context_add_class (context, label_class_name);

      text = g_strdup_printf ("%" G_GINT64_FORMAT " ms",
                              (t - min_timestamp) / 1000);
      layout = gtk_widget_create_pango_layout (widget, text);

      pango_layout_set_alignment (layout, PANGO_ALIGN_RIGHT);
      pango_layout_get_pixel_extents (layout, NULL, &layout_rect);

      gtk_render_layout (context, cr,
                         LEFT_GUTTER_WIDTH - LEFT_GUTTER_RIGHT_PADDING - layout_rect.width,
                         marker_y - layout_rect.height / 2,
                         layout);
      g_object_unref (layout);

      gtk_style_context_remove_class (context, label_class_name);
    }

  /* Draw the threads. */
  for (i = 0; i < n_threads; i++)
    {
      DflThread *thread = self->threads->pdata[i];
      gdouble thread_centre;
      PangoLayout *layout = NULL;
      gchar *text = NULL;
      PangoRectangle layout_rect;
      const gchar *thread_name;

      thread_centre = thread_index_to_centre (self, i);

      /* Guide line for the entire length of the thread. */
      gtk_style_context_add_class (context, "thread_guide");
      gtk_render_line (context, cr,
                       thread_centre,
                       timestamp_to_y (self, 0),
                       thread_centre,
                       timestamp_to_y (self, max_timestamp - min_timestamp));
      gtk_style_context_remove_class (context, "thread_guide");

      /* Line for the actual live length of the thread, plus its label. */
      gtk_style_context_add_class (context, "thread");
      gtk_render_line (context, cr,
                       thread_centre,
                       timestamp_to_y (self, dfl_thread_get_new_timestamp (thread) - min_timestamp),
                       thread_centre,
                       timestamp_to_y (self, dfl_thread_get_free_timestamp (thread) - min_timestamp));
      gtk_style_context_remove_class (context, "thread");

      /* Thread label. */
      gtk_style_context_add_class (context, "thread_header");

      thread_name = dfl_thread_get_name (thread);
      text = g_strdup_printf ("Thread %" G_GUINT64_FORMAT "\n%s",
                              dfl_thread_get_id (thread),
                              (thread_name != NULL) ? thread_name : "");
      layout = gtk_widget_create_pango_layout (widget, text);
      g_free (text);

      pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
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

      dfl_main_context_thread_ownership_iter (main_context, &iter,
                                              min_visible_timestamp);

      while (dfl_time_sequence_iter_next (&iter, &timestamp, (gpointer *) &data) &&
             timestamp <= max_visible_timestamp)
        {
          gdouble thread_centre;
          gint timestamp_y;
          guint thread_index;

          thread_index = thread_id_to_index (self, data->thread_id);
          thread_centre = thread_index_to_centre (self, thread_index);
          timestamp_y = timestamp_to_y (self, timestamp - min_timestamp);

          cairo_move_to (cr,
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

      dfl_main_context_dispatch_iter (main_context, &iter,
                                      min_visible_timestamp);

      while (dfl_time_sequence_iter_next (&iter, &timestamp, (gpointer *) &data) &&
             timestamp <= max_visible_timestamp)
        {
          gdouble thread_centre, dispatch_width, dispatch_height;
          gint timestamp_y;
          guint thread_index;

          thread_index = thread_id_to_index (self, data->thread_id);
          thread_centre = thread_index_to_centre (self, thread_index);
          timestamp_y = timestamp_to_y (self, timestamp - min_timestamp);

          dispatch_width = MAIN_CONTEXT_DISPATCH_WIDTH;
          dispatch_height = duration_to_pixels (self, data->duration);

          if (self->hover_element.type == ELEMENT_CONTEXT_DISPATCH &&
              self->hover_element.index == i &&
              dfl_time_sequence_iter_equal (self->hover_element.iter, &iter))
            gtk_style_context_add_class (context, "main_context_dispatch_hover");
          if (self->selected_element.type == ELEMENT_CONTEXT_DISPATCH &&
              self->selected_element.index == i &&
              dfl_time_sequence_iter_equal (self->selected_element.iter, &iter))
            gtk_style_context_add_class (context, "main_context_dispatch_selected");

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

          if (self->selected_element.type == ELEMENT_CONTEXT_DISPATCH &&
              self->selected_element.index == i &&
              dfl_time_sequence_iter_equal (self->selected_element.iter, &iter))
            gtk_style_context_remove_class (context, "main_context_dispatch_selected");
          if (self->hover_element.type == ELEMENT_CONTEXT_DISPATCH &&
              self->hover_element.index == i &&
              dfl_time_sequence_iter_equal (self->hover_element.iter, &iter))
            gtk_style_context_remove_class (context, "main_context_dispatch_hover");
        }

      gtk_style_context_remove_class (context, "main_context_dispatch");
    }

  /* Draw the sources either side. */
  for (i = 0; i < self->sources->len; i++)
    {
      DflSource *source = self->sources->pdata[i];
      gdouble thread_centre, source_x, source_y;
      guint thread_index;
      GdkRGBA color;
      DflTimestamp new_timestamp;

      new_timestamp = dfl_source_get_new_timestamp (source);

      if (new_timestamp < min_visible_timestamp ||
          new_timestamp > max_visible_timestamp)
        continue;

      thread_index = thread_id_to_index (self,
                                         dfl_source_get_new_thread_id (source));
      thread_centre = thread_index_to_centre (self, thread_index);

      /* Source circle. */
      gtk_style_context_add_class (context, "source");

      if (self->hover_element.type == ELEMENT_SOURCE &&
          self->hover_element.index == i)
        gtk_style_context_add_class (context, "source_hover");
      if (self->selected_element.type == ELEMENT_SOURCE &&
          self->selected_element.index == i)
        gtk_style_context_add_class (context, "source_selected");
      if (dfl_source_get_attach_main_context_id (source) == DFL_ID_INVALID)
        gtk_style_context_add_class (context, "source_unattached");

      cairo_save (cr);

      cairo_set_line_cap (cr, CAIRO_LINE_CAP_BUTT);
      cairo_set_line_width (cr, SOURCE_BORDER_WIDTH);
      cairo_new_path (cr);

      /* Calculate the centre of the source. */
      source_x = thread_centre - SOURCE_OFFSET;
      source_y = timestamp_to_y (self, new_timestamp - min_timestamp);

      cairo_arc (cr,
                 source_x,
                 source_y,
                 SOURCE_WIDTH / 2.0,
                 0.0, 2 * M_PI);

      cairo_clip_preserve (cr);
      gtk_render_background (context, cr,
                             source_x - SOURCE_WIDTH / 2.0,
                             source_y - SOURCE_WIDTH / 2.0,
                             SOURCE_WIDTH,
                             SOURCE_WIDTH);

      gtk_style_context_get_color (context, gtk_widget_get_state_flags (widget),
                                   &color);
      gdk_cairo_set_source_rgba (cr, &color);
      cairo_stroke (cr);

      cairo_restore (cr);

      if (dfl_source_get_attach_main_context_id (source) == DFL_ID_INVALID)
        gtk_style_context_remove_class (context, "source_unattached");
      if (self->selected_element.type == ELEMENT_SOURCE &&
          self->selected_element.index == i)
        gtk_style_context_remove_class (context, "source_selected");
      if (self->hover_element.type == ELEMENT_SOURCE &&
          self->hover_element.index == i)
        gtk_style_context_remove_class (context, "source_hover");

      gtk_style_context_remove_class (context, "source");
    }

  /* Draw the GTasks. */
  for (i = 0; i < self->tasks->len; i++)
    {
      DflTimestamp new_timestamp, return_timestamp;
      DflTask *task = self->tasks->pdata[i];

      new_timestamp = dfl_task_get_new_timestamp (task);
      return_timestamp = dfl_task_get_return_timestamp (task);

      if (return_timestamp < min_visible_timestamp ||
          new_timestamp > max_visible_timestamp)
        continue;

      draw_task_circle (self, cr, task,
                        self->hover_element.type == ELEMENT_TASK &&
                        self->hover_element.index == i,
                        self->selected_element.type == ELEMENT_TASK &&
                        self->selected_element.index == i);
    }

  /* Draw the dispatch lines for the selected source. */
  if (self->selected_element.type == ELEMENT_SOURCE)
    {
      DflSource *source = self->sources->pdata[self->selected_element.index];
      gdouble thread_centre, source_x, source_y;
      guint thread_index;
      DflTimeSequenceIter iter;
      DflTimestamp timestamp;
      DflSourceDispatchData *data;

      thread_index = thread_id_to_index (self,
                                         dfl_source_get_new_thread_id (source));
      thread_centre = thread_index_to_centre (self, thread_index);

      /* Calculate the centre of the source. */
      source_x = thread_centre - SOURCE_OFFSET;
      source_y = timestamp_to_y (self, dfl_source_get_new_timestamp (source) - min_timestamp);

      /* Render the source’s dispatches. Each dispatch is rendered as a
       * horizontal line from the thread where it occurs, across to line up with
       * the column containing the source, round the corner, then up to where
       * the source is rendered (the g_source_new()). */
      dfl_source_dispatch_iter (source, &iter, 0);

      while (dfl_time_sequence_iter_next (&iter, &timestamp, (gpointer *) &data))
        {
          draw_source_dispatch_line (self, cr, source, source_x, source_y,
                                     timestamp, data);
        }

      /* Render the source’s attach and destroy lines. */
      draw_source_attach_destroy_lines (self, cr, source, source_x, source_y);

      /* Re-render the source circle to make sure it’s on top. */
      draw_source_selected (self, cr, source, source_x, source_y);
    }
  else if (self->selected_element.type == ELEMENT_CONTEXT_DISPATCH)
    {
      g_autoptr (DflTimeSequenceIter) main_context_iter = NULL;
      DflTimestamp main_context_timestamp;
      DflMainContextDispatchData *main_context_data;

      /* For each of the sources in this main context dispatch, highlight them
       * and draw their dispatch lines. */
      main_context_iter = dfl_time_sequence_iter_copy (self->selected_element.iter);
      main_context_timestamp = dfl_time_sequence_iter_get_timestamp (self->selected_element.iter);
      main_context_data = dfl_time_sequence_iter_get_data (self->selected_element.iter);

      /* TODO: Speed this up. Perhaps move the list of dispatched sources into
       * DflMainContextDispatchData? */
      for (i = 0; i < self->sources->len; i++)
        {
          DflSource *source = self->sources->pdata[i];
          gdouble thread_centre, source_x, source_y;
          guint thread_index;
          DflTimeSequenceIter source_iter;
          DflTimestamp source_timestamp;
          DflSourceDispatchData *source_data;
          gboolean dispatch_drawn;

          thread_index = thread_id_to_index (self,
                                             dfl_source_get_new_thread_id (source));
          thread_centre = thread_index_to_centre (self, thread_index);

          /* Calculate the centre of the source. */
          source_x = thread_centre - SOURCE_OFFSET;
          source_y = timestamp_to_y (self, dfl_source_get_new_timestamp (source) - min_timestamp);

          /* Render the source’s dispatches. Each dispatch is rendered as a
           * horizontal line from the thread where it occurs, across to line up with
           * the column containing the source, round the corner, then up to where
           * the source is rendered (the g_source_new()). */

          dfl_source_dispatch_iter (source, &source_iter,
                                    dfl_time_sequence_iter_get_timestamp (main_context_iter));
          dispatch_drawn = FALSE;

          while (dfl_time_sequence_iter_next (&source_iter, &source_timestamp,
                                              (gpointer *) &source_data))
            {
              if (source_data->thread_id != main_context_data->thread_id ||
                  source_timestamp < main_context_timestamp ||
                  source_timestamp > main_context_timestamp +
                  main_context_data->duration)
                continue;

              draw_source_dispatch_line (self, cr, source, source_x, source_y,
                                         source_timestamp, source_data);
              dispatch_drawn = TRUE;
            }

          if (dispatch_drawn)
            draw_source_selected (self, cr, source, source_x, source_y);
        }
    }
  else if (self->selected_element.type == ELEMENT_TASK)
    {
      DflTask *task = self->tasks->pdata[self->selected_element.index];

      /* Render the task’s return and propagate lines. */
      draw_task_return_propagate_lines (self, cr, task);

      /* Re-render the task new circle to make sure it’s on top. */
      draw_task_circle (self, cr, task,
                        self->hover_element.type == ELEMENT_TASK &&
                        self->hover_element.index == self->selected_element.index,
                        TRUE);
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
    *minimum_width = MAX (1,
                          LEFT_GUTTER_WIDTH + n_threads * THREAD_MIN_WIDTH);
  if (natural_width != NULL)
    *natural_width = MAX (1,
                          LEFT_GUTTER_WIDTH + n_threads * THREAD_NATURAL_WIDTH);
}

static void
dwl_timeline_get_preferred_height (GtkWidget *widget,
                                   gint      *minimum_height,
                                   gint      *natural_height)
{
  DwlTimeline *self = DWL_TIMELINE (widget);
  gint height;

  /* What’s the maximum height of any of the threads? */
  height = timestamp_to_y (self, self->max_timestamp - self->min_timestamp);

  if (height > 0)
    height += FOOTER_HEIGHT;

  if (minimum_height != NULL)
    *minimum_height = MAX (1, height);
  if (natural_height != NULL)
    *natural_height = MAX (1, height);
}

#define SCROLL_SMOOTH_FACTOR_SCALE 2.0 /* pixels per unit zoom factor */

static gboolean
dwl_timeline_scroll_event (GtkWidget      *widget,
                           GdkEventScroll *event)
{
  DwlTimeline *self = DWL_TIMELINE (widget);

  /* If the user is holding down Ctrl, change the zoom level. Otherwise, pass
   * the scroll event through to other widgets. */
  if (event->state & GDK_CONTROL_MASK)
    {
      gdouble factor;
      gdouble delta;
      gfloat old_zoom;
      DflTimestamp old_focus_timestamp;
      gboolean use_focus_timestamp;

      switch (event->direction)
        {
        case GDK_SCROLL_UP:
          factor = 2.0;
          break;
        case GDK_SCROLL_DOWN:
          factor = 0.5;
          break;
        case GDK_SCROLL_SMOOTH:
          g_assert (gdk_event_get_scroll_deltas ((GdkEvent *) event, NULL,
                                                 &delta));

          /* Process the delta. */
          if (delta == 0.0)
            factor = 1.0;
          else if (delta > 0.0)
            factor = delta / SCROLL_SMOOTH_FACTOR_SCALE;
          else
            factor = SCROLL_SMOOTH_FACTOR_SCALE / -delta;

          break;
        case GDK_SCROLL_LEFT:
        case GDK_SCROLL_RIGHT:
        default:
          factor = 1.0;
          break;
        }

      /* Store some of the old state. */
      use_focus_timestamp = pixel_is_timestamp (self, event->y);

      if (use_focus_timestamp)
        old_focus_timestamp = y_to_timestamp (self, event->y);
      old_zoom = dwl_timeline_get_zoom (self);

      /* Set the updated zoom and schedule a redraw. */
      dwl_timeline_set_zoom (self, old_zoom * factor);

      /* Adjust the scroll position so the cursor continues to be focused on the
       * same point. They use the same units, modulo the zoom level.
       *
       * The fact that we check whether the parent widget is a GtkScrollable is
       * because we don’t yet implement it ourselves, and hence expect to be
       * implicitly packed in a GtkViewport. */
      if (GTK_IS_SCROLLABLE (gtk_widget_get_parent (GTK_WIDGET (self))) &&
          use_focus_timestamp)
        {
          GtkScrollable *scrollable;
          GtkAdjustment *vadjustment;
          gdouble new_position, old_position;

          scrollable = GTK_SCROLLABLE (gtk_widget_get_parent (GTK_WIDGET (self)));
          vadjustment = gtk_scrollable_get_vadjustment (scrollable);

          new_position = timestamp_to_y (self, old_focus_timestamp);
          old_position = event->y;

          gtk_adjustment_set_value (vadjustment,
                                    gtk_adjustment_get_value (vadjustment) -
                                    old_position +
                                    new_position);
        }

      return GDK_EVENT_STOP;
    }

  return GDK_EVENT_PROPAGATE;
}

static gboolean
dwl_timeline_motion_notify_event (GtkWidget      *widget,
                                  GdkEventMotion *event)
{
  /* Try and work out which part of the diagram we’re on top of. In the absence
   * of child actors, this is going to end up being a horrible mess of
   * hard-coded checks for collisions with various rendered primitives. */

  DwlTimeline *self = DWL_TIMELINE (widget);
  gint widget_width;
  guint i, n_threads;
  DflTimestamp min_timestamp;
  gdouble thread_width, nearest_thread_centre;
  guint nearest_thread_index;
  DwlTimelineElement new_hover_type = ELEMENT_NONE;
  guint new_hover_index = 0;
  g_autoptr (DflTimeSequenceIter) new_hover_iter = NULL;

  widget_width = gtk_widget_get_allocated_width (widget);
  n_threads = self->threads->len;
  min_timestamp = self->min_timestamp;

  /* If there are no threads, there’s nothing to do. */
  if (n_threads == 0)
    return GDK_EVENT_STOP;

  /* Find the nearest thread. */
  thread_width = (widget_width - LEFT_GUTTER_WIDTH) / n_threads;
  nearest_thread_index = (event->x - LEFT_GUTTER_WIDTH) / thread_width;
  nearest_thread_centre = thread_index_to_centre (self, nearest_thread_index);

  if (ABS (nearest_thread_centre - event->x) > SOURCE_OFFSET + SOURCE_WIDTH / 2.0)
    {
      /* No hover element found. */
      new_hover_type = ELEMENT_NONE;
      goto done;
    }

  /* Within nearest_thread_index’s column. Search for sources. */
  for (i = 0; i < self->sources->len; i++)
    {
      DflSource *source = self->sources->pdata[i];
      gdouble thread_centre, source_x, source_y;
      guint thread_index;

      thread_index = thread_id_to_index (self,
                                         dfl_source_get_new_thread_id (source));

      if (thread_index != nearest_thread_index)
        continue;

      thread_centre = thread_index_to_centre (self, thread_index);

      /* Calculate the centre of the source. */
      source_x = thread_centre - SOURCE_OFFSET;
      source_y = timestamp_to_y (self, dfl_source_get_new_timestamp (source) - min_timestamp);

      /* See if the event was within this source. */
      if (event->x >= source_x - SOURCE_WIDTH / 2.0 &&
          event->x <= source_x + SOURCE_WIDTH / 2.0 &&
          event->y >= source_y - SOURCE_WIDTH / 2.0 &&
          event->y <= source_y + SOURCE_WIDTH / 2.0)
        {
          new_hover_type = ELEMENT_SOURCE;
          new_hover_index = i;
          goto done;
        }
    }

  /* What about main context dispatches? */
  for (i = 0; i < self->main_contexts->len; i++)
    {
      DflMainContext *main_context = self->main_contexts->pdata[i];
      DflTimeSequenceIter iter;
      DflTimestamp timestamp;
      DflThreadOwnershipData *data;

      /* Iterate through the thread ownership events. */

      /* TODO: Set the start timestamp according to the clip area */
      dfl_main_context_dispatch_iter (main_context, &iter, 0);

      while (dfl_time_sequence_iter_next (&iter, &timestamp, (gpointer *) &data))
        {
          gdouble thread_centre, dispatch_width, dispatch_height;
          gdouble dispatch_left, dispatch_right, dispatch_top, dispatch_bottom;
          gint timestamp_y;
          guint thread_index;

          thread_index = thread_id_to_index (self, data->thread_id);
          thread_centre = thread_index_to_centre (self, thread_index);
          timestamp_y = timestamp_to_y (self, timestamp - min_timestamp);

          dispatch_width = MAIN_CONTEXT_DISPATCH_WIDTH;
          dispatch_height = duration_to_pixels (self, data->duration);

          dispatch_left = thread_centre - dispatch_width / 2.0;
          dispatch_right = thread_centre + dispatch_width / 2.0;
          dispatch_bottom = timestamp_y;
          dispatch_top = timestamp_y + dispatch_height;

          if (event->x >= dispatch_left &&
              event->x <= dispatch_right &&
              event->y >= dispatch_bottom &&
              event->y <= dispatch_top)
            {
              new_hover_type = ELEMENT_CONTEXT_DISPATCH;
              new_hover_index = i;
              new_hover_iter = dfl_time_sequence_iter_copy (&iter);
              goto done;
            }
        }
    }

  /* Search for tasks. */
  for (i = 0; i < self->tasks->len; i++)
    {
      DflTask *task = self->tasks->pdata[i];
      gdouble task_x, task_y;
      guint thread_index;

      thread_index = thread_id_to_index (self,
                                         dfl_task_get_new_thread_id (task));

      if (thread_index != nearest_thread_index)
        continue;

      /* Calculate the centre of the task circles. */
      task_x = thread_index_to_centre (self, thread_index) + TASK_OFFSET;
      task_y = timestamp_to_y (self, dfl_task_get_new_timestamp (task) - min_timestamp);

      /* See if the event was within the new circle for this task. */
      if (event->x >= task_x - TASK_WIDTH / 2.0 &&
          event->x <= task_x + TASK_WIDTH / 2.0 &&
          event->y >= task_y - TASK_WIDTH / 2.0 &&
          event->y <= task_y + TASK_WIDTH / 2.0)
        {
          new_hover_type = ELEMENT_TASK;
          new_hover_index = i;
          goto done;
        }
    }

  /* No hover element found. */
  if (self->hover_element.type != ELEMENT_NONE)
    {
      self->hover_element.type = ELEMENT_NONE;
      g_debug ("%s: clearing hover element", G_STRFUNC);
      gtk_widget_queue_draw (widget);
    }

done:
  if (new_hover_type != self->hover_element.type ||
      new_hover_index != self->hover_element.index ||
      (new_hover_type == ELEMENT_CONTEXT_DISPATCH &&
       self->hover_element.type == ELEMENT_CONTEXT_DISPATCH &&
       !dfl_time_sequence_iter_equal (self->hover_element.iter, new_hover_iter)))
    {
      g_debug ("%s: type: %u, index: %u, iter: %p (%" G_GUINT64_FORMAT "), "
               "changed: %s",
               G_STRFUNC, new_hover_type, new_hover_index, new_hover_iter,
               (new_hover_iter != NULL) ? dfl_time_sequence_iter_get_timestamp (new_hover_iter) : 0,
               "yes");

      self->hover_element.type = new_hover_type;
      self->hover_element.index = new_hover_index;

      g_clear_pointer (&self->hover_element.iter, dfl_time_sequence_iter_free);
      self->hover_element.iter = g_steal_pointer (&new_hover_iter);

      g_assert (self->hover_element.type != ELEMENT_CONTEXT_DISPATCH ||
                self->hover_element.iter != NULL);

      gtk_widget_queue_draw (widget);
    }

  return GDK_EVENT_STOP;
}

/* Returns %TRUE if the selected element has actually changed. */
static gboolean
dwl_timeline_set_selected_element (DwlTimeline         *self,
                                   DwlTimelineElement   type,
                                   guint                index,
                                   DflTimeSequenceIter *iter  /* transfer full */)
{
  gboolean changed;

  g_assert (DWL_IS_TIMELINE (self));
  g_assert (type != ELEMENT_CONTEXT_DISPATCH || iter != NULL);

  changed = (self->selected_element.type != type ||
             self->selected_element.index != index ||
             (self->selected_element.iter != NULL &&
              iter != NULL &&
              !dfl_time_sequence_iter_equal (self->selected_element.iter,
                                             iter)));

  g_debug ("%s: type: %u, index: %u, iter: %p (%" G_GUINT64_FORMAT "), "
           "changed: %s",
           G_STRFUNC, type, index, iter,
           (iter != NULL) ? dfl_time_sequence_iter_get_timestamp (iter) : 0,
           changed ? "yes" : "no");

  self->selected_element.type = type;
  self->selected_element.index = index;

  g_clear_pointer (&self->selected_element.iter, dfl_time_sequence_iter_free);
  if (iter != NULL)
    self->selected_element.iter = g_steal_pointer (&iter);

  return changed;
}

static gboolean
dwl_timeline_button_release_event (GtkWidget      *widget,
                                   GdkEventButton *event)
{
  DwlTimeline *self = DWL_TIMELINE (widget);

  /* Focus the widget? */
  if (gtk_widget_get_focus_on_click (widget) && !gtk_widget_has_focus (widget))
    gtk_widget_grab_focus (widget);

  /* If an element is being hovered over, turn it into the currently selected
   * element. Otherwise, clear the selection. */
  if (dwl_timeline_set_selected_element (self,
                                         self->hover_element.type,
                                         self->hover_element.index,
                                         (self->hover_element.iter != NULL) ? dfl_time_sequence_iter_copy (self->hover_element.iter) : NULL))
    gtk_widget_queue_draw (widget);

  return GDK_EVENT_STOP;
}

static void
dwl_timeline_scroll_to_timestamp (DwlTimeline  *self,
                                  DflTimestamp  timestamp)
{
  gint new_y;

  new_y = timestamp_to_y (self, timestamp - self->min_timestamp);

  if (GTK_IS_SCROLLABLE (gtk_widget_get_parent (GTK_WIDGET (self))))
    {
      GtkScrollable *scrollable;
      GtkAdjustment *vadjustment;
      gdouble current_value;
      gint parent_widget_height;

      scrollable = GTK_SCROLLABLE (gtk_widget_get_parent (GTK_WIDGET (self)));
      vadjustment = gtk_scrollable_get_vadjustment (scrollable);

      /* Is the given @y value already visible? If so, don’t scroll. */
      current_value = gtk_adjustment_get_value (vadjustment);
      parent_widget_height = gtk_widget_get_allocated_height (GTK_WIDGET (scrollable));

      g_debug ("%s: current_value: %f, parent_widget_height: %d, new_y: %d",
               G_STRFUNC, current_value, parent_widget_height, new_y);

      if (current_value + AUTO_SCROLL_MARGIN * parent_widget_height <= new_y &&
          current_value + (1.0 - AUTO_SCROLL_MARGIN) * parent_widget_height >= new_y)
        return;

      gtk_adjustment_set_value (vadjustment, new_y - parent_widget_height / 2);
    }
}

static void
dwl_timeline_scroll_to_selected (DwlTimeline *self)
{
  DflTimestamp timestamp;

  switch (self->selected_element.type)
    {
    case ELEMENT_CONTEXT_DISPATCH:
      timestamp = dfl_time_sequence_iter_get_timestamp (self->selected_element.iter);
      break;
    case ELEMENT_SOURCE:
      {
        DflSource *source = self->sources->pdata[self->selected_element.index];
        timestamp = dfl_source_get_new_timestamp (source);
        break;
      }
    case ELEMENT_TASK:
      {
        DflTask *task = self->tasks->pdata[self->selected_element.index];
        timestamp = dfl_task_get_new_timestamp (task);
        break;
      }
    case ELEMENT_NONE:
      /* Nothing to do. */
      return;
    default:
      g_assert_not_reached ();
    }

  dwl_timeline_scroll_to_timestamp (self, timestamp);
}

/* Version of move_selected_sibling() specially for %ELEMENT_CONTEXT_DISPATCH
 * elements. */
static void
move_selected_sibling_main_context_dispatch (DwlTimeline *self,
                                             gint         distance)
{
  g_autoptr (DflTimeSequenceIter) iter = NULL;
  DflTimestamp new_timestamp;
  guint i;
  gboolean changed = FALSE;

  g_assert (self->selected_element.type == ELEMENT_CONTEXT_DISPATCH);

  /* Grab the existing selected timestamp and iterate @distance further.
   * If we can go no further, clamp on the highest value. */
  iter = dfl_time_sequence_iter_copy (self->selected_element.iter);

  for (i = 0; i < (guint) ABS (distance); i++)
    {
      if ((distance > 0 &&
           !dfl_time_sequence_iter_next (iter, &new_timestamp, NULL)) ||
          (distance < 0 &&
           !dfl_time_sequence_iter_previous (iter, &new_timestamp, NULL)))
        break;

      changed = dwl_timeline_set_selected_element (self,
                                                   ELEMENT_CONTEXT_DISPATCH,
                                                   self->selected_element.index,
                                                   dfl_time_sequence_iter_copy (iter)) || changed;
    }

  if (changed)
    {
      dwl_timeline_scroll_to_selected (self);
      gtk_widget_queue_draw (GTK_WIDGET (self));
    }
}

/* Change the selection to a previous or following sibling of the current
 * selection. Must only be called if the current selection is valid. */
static void
move_selected_sibling (DwlTimeline *self,
                       gint         distance)
{
  GPtrArray/*<unknown>*/ *array;
  guint new_index;

  switch (self->selected_element.type)
    {
    case ELEMENT_CONTEXT_DISPATCH:
      /* Requires some special handling, because main context dispatches are
       * done by timestamp rather than by index. */
      move_selected_sibling_main_context_dispatch (self, distance);
      return;
    case ELEMENT_SOURCE:
      array = self->sources;
      break;
    case ELEMENT_TASK:
      array = self->tasks;
      break;
    case ELEMENT_NONE:
    default:
      g_assert_not_reached ();
    }

  /* Clamp to the bounds of the @array. */
  if (distance > 0 && (guint) distance >= array->len - self->selected_element.index)
    new_index = array->len - 1;
  else if (distance < 0 && self->selected_element.index < (guint) -distance)
    new_index = 0;
  else
    new_index = self->selected_element.index + distance;

  if (self->selected_element.index != new_index)
    {
      self->selected_element.index = new_index;
      dwl_timeline_scroll_to_selected (self);
      gtk_widget_queue_draw (GTK_WIDGET (self));
    }
}

/* Change the selection to an ancestor or descendent of the current
 * selection. Must only be called if the current selection is valid. */
static void
move_selected_ancestor (DwlTimeline *self,
                        gint         distance)
{
  /* FIXME: What kind of navigation do we want to implement here? */
}

static gboolean
dwl_timeline_move_selected (DwlTimeline              *timeline,
                            DwlSelectionMovementStep  step,
                            gint                      distance)
{
  g_debug ("%s: step: %u, distance: %d", G_STRFUNC, step, distance);

  /* Do we need to do anything? */
  if (distance == 0)
    return TRUE;

  /* If there is currently no selection, select the first source element
   * (arbitrarily). */
  if (timeline->selected_element.type == ELEMENT_NONE)
    {
      if (timeline->sources->len > 0)
        {
          g_assert (dwl_timeline_set_selected_element (timeline,
                                                       ELEMENT_SOURCE,
                                                       0, NULL));
        }
      else if (timeline->tasks->len > 0)
        {
          g_assert (dwl_timeline_set_selected_element (timeline,
                                                       ELEMENT_TASK,
                                                       0, NULL));
        }
      else if (timeline->main_contexts->len > 0)
        {
          DflTimeSequenceIter iter;

          dfl_main_context_thread_ownership_iter (timeline->main_contexts->pdata[0],
                                                  &iter, 0);

          if (!dfl_time_sequence_iter_next (&iter,
                                            NULL, NULL))
            return TRUE;

          g_assert (dwl_timeline_set_selected_element (timeline,
                                                       ELEMENT_CONTEXT_DISPATCH,
                                                       0,
                                                       dfl_time_sequence_iter_copy (&iter)));
        }

      dwl_timeline_scroll_to_selected (timeline);
      gtk_widget_queue_draw (GTK_WIDGET (timeline));

      return TRUE;
    }

  /* Otherwise, change the selection from the current one. */
  switch (step)
    {
    case DWL_SELECTION_MOVEMENT_SIBLING:
      move_selected_sibling (timeline, distance);
      break;
    case DWL_SELECTION_MOVEMENT_ANCESTOR:
      move_selected_ancestor (timeline, distance);
      break;
    default:
      g_assert_not_reached ();
    }

  return TRUE;
}

/**
 * dwl_timeline_get_zoom:
 * @self: a #DwlTimeline
 *
 * TODO
 *
 * Returns: TODO
 * Since: 0.1.0
 */
gfloat
dwl_timeline_get_zoom (DwlTimeline *self)
{
  g_return_val_if_fail (DWL_IS_TIMELINE (self), 1.0);

  return self->zoom;
}

/**
 * dwl_timeline_set_zoom:
 * @self: a #DwlTimeline
 * @zoom: new zoom value, between %ZOOM_MIN and %ZOOM_MAX
 *
 * TODO
 *
 * Returns: %TRUE if the zoom level changed, %FALSE otherwise
 * Since: 0.1.0
 */
gboolean
dwl_timeline_set_zoom (DwlTimeline *self,
                       gfloat       zoom)
{
  gfloat new_zoom;

  g_return_val_if_fail (DWL_IS_TIMELINE (self), FALSE);

  new_zoom = CLAMP (zoom, ZOOM_MIN, ZOOM_MAX);

  if (new_zoom == self->zoom)
    return FALSE;

  g_debug ("%s: Setting zoom to %f", G_STRFUNC, new_zoom);

  self->zoom = new_zoom;
  g_object_notify (G_OBJECT (self), "zoom");
  gtk_widget_queue_resize (GTK_WIDGET (self));

  return TRUE;
}
