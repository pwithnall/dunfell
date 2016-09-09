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
 * SECTION:viewer-window
 * @short_description: #GtkApplicationWindow subclass for a viewer window
 * @stability: Unstable
 * @include: viewer/viewer-window.h
 *
 * TODO
 *
 * Since: 0.1.0
 */

#include "config.h"

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "libdunfell/model.h"
#include "libdunfell/parser.h"
#include "libdunfell-ui/statistics-pane.h"
#include "libdunfell-ui/timeline.h"
#include "viewer/viewer-window.h"


static void dfv_viewer_window_get_property (GObject      *object,
                                            guint         property_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);
static void dfv_viewer_window_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);
static void dfv_viewer_window_dispose      (GObject      *object);
static void dfv_viewer_window_clear_file   (DfvViewerWindow *self,
                                            const GError    *error);
static void dfv_viewer_window_set_file     (DfvViewerWindow *self,
                                            GFile           *file);
static void open_button_clicked            (GtkButton *button,
                                            gpointer   user_data);
static void record_button_clicked          (GtkButton *button,
                                            gpointer   user_data);

struct _DfvViewerWindow
{
  GtkApplicationWindow parent;

  GCancellable *open_cancellable;  /* owned; non-NULL iff loading a file */
  GFile *file;  /* owned; NULL iff no file is loaded */

  GtkStack *main_stack;
  GtkWidget *timeline_scrolled_window;
  GtkPaned *main_paned;
  GtkWidget *timeline;  /* NULL iff not loaded */
  GtkWidget *statistics_pane;  /* (nullable); NULL iff not loaded */
  GtkWidget *home_page_box;
};

G_DEFINE_TYPE (DfvViewerWindow, dfv_viewer_window, GTK_TYPE_APPLICATION_WINDOW)

typedef enum
{
  PROP_FILE = 1,
} DfvViewerWindowProperty;

static void
dfv_viewer_window_class_init (DfvViewerWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  /* Set up the composite widget. */
  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/gnome/Dunfell/Viewer/ui/viewer-window.ui");
  gtk_widget_class_bind_template_child (widget_class,
                                        DfvViewerWindow, main_stack);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        timeline_scrolled_window);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        main_paned);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        home_page_box);
  gtk_widget_class_bind_template_callback (widget_class, open_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class, record_button_clicked);

  object_class->get_property = dfv_viewer_window_get_property;
  object_class->set_property = dfv_viewer_window_set_property;
  object_class->dispose = dfv_viewer_window_dispose;

  /**
   * DfvViewerWindow:file:
   *
   * TODO
   *
   * Since: 0.1.0
   */
  g_object_class_install_property (object_class, PROP_FILE,
                                   g_param_spec_object ("file",
                                                        "File",
                                                        "TODO",
                                                        G_TYPE_FILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));
}

static void
dfv_viewer_window_init (DfvViewerWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  /* Set the initial stack page. */
  gtk_stack_set_visible_child_name (self->main_stack, "intro");
}

static void
dfv_viewer_window_get_property (GObject     *object,
                                guint        property_id,
                                GValue      *value,
                                GParamSpec  *pspec)
{
  DfvViewerWindow *self = DFV_VIEWER_WINDOW (object);

  switch ((DfvViewerWindowProperty) property_id)
    {
    case PROP_FILE:
      g_value_set_object (value, self->file);
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
dfv_viewer_window_set_property (GObject           *object,
                                guint              property_id,
                                const GValue      *value,
                                GParamSpec        *pspec)
{
  DfvViewerWindow *self = DFV_VIEWER_WINDOW (object);

  switch ((DfvViewerWindowProperty) property_id)
    {
    case PROP_FILE:
      dfv_viewer_window_set_file (self, g_value_get_object (value));
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
dfv_viewer_window_dispose (GObject *object)
{
  DfvViewerWindow *self = DFV_VIEWER_WINDOW (object);

  dfv_viewer_window_clear_file (self, NULL);
  g_assert (self->open_cancellable == NULL);
  g_assert (self->file == NULL);

  G_OBJECT_CLASS (dfv_viewer_window_parent_class)->dispose (object);
}

/**
 * dfv_viewer_window_new:
 * @application: TODO
 *
 * TODO
 *
 * Returns: (transfer full): a new #DfvViewerWindow
 * Since: 0.1.0
 */
DfvViewerWindow *
dfv_viewer_window_new (GtkApplication *application)
{
  g_return_val_if_fail (GTK_IS_APPLICATION (application), NULL);

  return g_object_new (DFV_TYPE_VIEWER_WINDOW,
                       "application", application,
                       "file", NULL,
                       NULL);
}

/**
 * dfv_viewer_window_new_for_file:
 * @application: TODO
 * @file: TODO
 *
 * TODO
 *
 * Returns: (transfer full): a new #DfvViewerWindow
 * Since: 0.1.0
 */
DfvViewerWindow *
dfv_viewer_window_new_for_file (GtkApplication *application,
                                GFile          *file)
{
  g_return_val_if_fail (GTK_IS_APPLICATION (application), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);

  return g_object_new (DFV_TYPE_VIEWER_WINDOW,
                       "application", application,
                       "file", file,
                       NULL);
}

/**
 * dfv_viewer_window_get_pane_name:
 * @self: a #DfvViewerWindow
 *
 * TODO
 *
 * Returns: TODO
 * Since: 0.1.0
 */
const gchar *
dfv_viewer_window_get_pane_name (DfvViewerWindow *self)
{
  const gchar *out;

  g_return_val_if_fail (DFV_IS_VIEWER_WINDOW (self), NULL);

  out = gtk_stack_get_visible_child_name (self->main_stack);
  g_assert (out != NULL);

  return out;
}

/**
 * dfv_viewer_window_open:
 * @self: a #DfvViewerWindow
 * @file: (nullable): TODO
 *
 * TODO
 *
 * Since: 0.1.0
 */
void
dfv_viewer_window_open (DfvViewerWindow *self,
                        GFile           *file)
{
  GtkWidget *dialog;

  g_return_if_fail (DFV_IS_VIEWER_WINDOW (self));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  if (file == NULL)
    {
      /* Show a file chooser. */
      dialog = gtk_file_chooser_dialog_new (_("Open File"),
                                            GTK_WINDOW (self),
                                            GTK_FILE_CHOOSER_ACTION_OPEN,
                                            _("_Cancel"),
                                            GTK_RESPONSE_CANCEL,
                                            _("_Open"),
                                            GTK_RESPONSE_ACCEPT,
                                            NULL);

      if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
        file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
      else
        file = NULL;

      gtk_widget_destroy (dialog);
    }
  else
    {
      /* File already provided. */
      g_object_ref (file);
    }

  /* Operation cancelled? */
  if (file == NULL)
    return;

  /* Load and display the file. */
  dfv_viewer_window_set_file (self, file);
  g_object_unref (file);
}

/**
 * dfv_viewer_window_record:
 * @self: a #DfvViewerWindow
 *
 * TODO
 *
 * Since: 0.1.0
 */
void
dfv_viewer_window_record (DfvViewerWindow *self)
{
  GtkWidget *dialog = NULL;

  g_return_if_fail (DFV_IS_VIEWER_WINDOW (self));

  /* FIXME: This needs to be expanded to provide a launch dialogue and use
   * GSubprocessLauncher. */
  dialog = gtk_message_dialog_new (GTK_WINDOW (self),
                                   GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_INFO,
                                   GTK_BUTTONS_CLOSE,
                                   _("Record an application by running it "
                                     "under dunfell-record, then open the "
                                     "resulting /tmp/dunfell.log log file "
                                     "here."));
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

static void
open_button_clicked (GtkButton *button,
                     gpointer   user_data)
{
  DfvViewerWindow *self = DFV_VIEWER_WINDOW (user_data);

  dfv_viewer_window_open (self, NULL);
}

static void
record_button_clicked (GtkButton *button,
                       gpointer   user_data)
{
  DfvViewerWindow *self = DFV_VIEWER_WINDOW (user_data);

  dfv_viewer_window_record (self);
}

static void set_file_cb1 (GObject      *source_object,
                          GAsyncResult *result,
                          gpointer      user_data);
static void set_file_cb2 (GObject      *source_object,
                          GAsyncResult *result,
                          gpointer      user_data);
static void set_file_cb_name (GObject      *source_object,
                              GAsyncResult *result,
                              gpointer      user_data);

static void
info_bar_response_cb (GtkInfoBar *info_bar,
                      gint        response_id,
                      gpointer    user_data)
{
  DfvViewerWindow *self = DFV_VIEWER_WINDOW (user_data);

  /* Remove the info bar. */
  g_assert (response_id == GTK_RESPONSE_CLOSE);

  gtk_container_remove (GTK_CONTAINER (self->home_page_box),
                        GTK_WIDGET (info_bar));
}

/* If @error is non-%NULL, an error infobar will be shown to indicate what went
 * wrong. */
static void
dfv_viewer_window_clear_file (DfvViewerWindow *self,
                              const GError    *error)
{
  g_return_if_fail (DFV_IS_VIEWER_WINDOW (self));

  if (error != NULL)
    {
      GtkInfoBar *info_bar = NULL;
      GtkWidget *content_area, *error_label;

      info_bar = GTK_INFO_BAR (gtk_info_bar_new ());
      gtk_info_bar_set_show_close_button (info_bar, TRUE);
      gtk_info_bar_set_message_type (info_bar, GTK_MESSAGE_ERROR);

      error_label = gtk_label_new (error->message);
      gtk_label_set_ellipsize (GTK_LABEL (error_label), PANGO_ELLIPSIZE_END);

      content_area = gtk_info_bar_get_content_area (info_bar);
      gtk_container_add (GTK_CONTAINER (content_area), error_label);

      gtk_box_pack_start (GTK_BOX (self->home_page_box), GTK_WIDGET (info_bar),
                          FALSE, FALSE, 0);

      g_signal_connect (info_bar, "response", (GCallback) info_bar_response_cb,
                        self);

      gtk_widget_show_all (GTK_WIDGET (info_bar));
    }

  if (self->file == NULL)
    return;

  g_cancellable_cancel (self->open_cancellable);
  g_clear_object (&self->open_cancellable);

  g_clear_object (&self->file);
  g_object_notify (G_OBJECT (self), "file");

  gtk_window_set_title (GTK_WINDOW (self), _("Dunfell Viewer"));
  gtk_stack_set_visible_child_name (self->main_stack, "intro");

  g_clear_pointer (&self->timeline, gtk_widget_destroy);
}

static void
dfv_viewer_window_set_file (DfvViewerWindow *self,
                            GFile           *file)
{
  GCancellable *cancellable = NULL;

  g_return_if_fail (DFV_IS_VIEWER_WINDOW (self));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  if (!g_set_object (&self->file, file))
    return;

  /* Start loading. */
  gtk_stack_set_visible_child_name (self->main_stack, "loading");
  g_object_notify (G_OBJECT (self), "file");

  cancellable = g_cancellable_new ();

  g_cancellable_cancel (self->open_cancellable);
  g_set_object (&self->open_cancellable, cancellable);

  /* Query the file’s name for the window title. */
  g_file_query_info_async (file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                           G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                           G_PRIORITY_DEFAULT, cancellable,
                           set_file_cb_name, self);

  /* Open the file. */
  g_file_read_async (file, G_PRIORITY_DEFAULT, cancellable, set_file_cb1, self);

  g_object_unref (cancellable);
}

static void
set_file_cb1 (GObject      *source_object,
              GAsyncResult *result,
              gpointer      user_data)
{
  DfvViewerWindow *self;
  GFile *file;
  g_autoptr (DflParser) parser = NULL;
  g_autoptr (GFileInputStream) stream = NULL;
  g_autoptr (GError) error = NULL;

  file = G_FILE (source_object);
  self = DFV_VIEWER_WINDOW (user_data);

  stream = g_file_read_finish (file, result, &error);

  if (error != NULL)
    {
      dfv_viewer_window_clear_file (self, error);
      return;
    }

  /* Parse the log into an event sequence. */
  parser = dfl_parser_new ();

  dfl_parser_load_from_stream_async (parser, G_INPUT_STREAM (stream),
                                     self->open_cancellable,
                                     set_file_cb2, self);
}

static void
set_file_cb2 (GObject      *source_object,
              GAsyncResult *result,
              gpointer      user_data)
{
  DfvViewerWindow *self;
  DflParser *parser;
  DflEventSequence *sequence;
  g_autoptr (DflModel) model = NULL;
  GError *child_error = NULL;

  self = DFV_VIEWER_WINDOW (user_data);
  parser = DFL_PARSER (source_object);

  /* Error? */
  dfl_parser_load_from_stream_finish (parser, result, &child_error);

  if (child_error != NULL)
    {
      dfv_viewer_window_clear_file (self, child_error);
      g_error_free (child_error);

      return;
    }

  sequence = dfl_parser_get_event_sequence (parser);
  model = dfl_model_new (sequence);

  /* Done. One last check for cancellation, then. Clear up the loading state. */
  if (g_cancellable_is_cancelled (self->open_cancellable))
    {
      dfv_viewer_window_clear_file (self, NULL);
      return;
    }

  g_clear_object (&self->open_cancellable);

  /* Create and show the timeline and statistics widgets. */
  self->timeline = GTK_WIDGET (dwl_timeline_new (model));
  gtk_container_add (GTK_CONTAINER (self->timeline_scrolled_window),
                     self->timeline);
  gtk_widget_show (self->timeline);

  self->statistics_pane = GTK_WIDGET (dwl_statistics_pane_new (model));
  gtk_paned_pack2 (self->main_paned, self->statistics_pane, FALSE, FALSE);
  gtk_widget_show (self->statistics_pane);

  gtk_stack_set_visible_child_name (self->main_stack, "timeline");
  gtk_widget_grab_focus (self->timeline);
}

static void
set_file_cb_name (GObject      *source_object,
                  GAsyncResult *result,
                  gpointer      user_data)
{
  DfvViewerWindow *self;
  GFile *file;
  g_autoptr (GFileInfo) file_info = NULL;
  g_autoptr (GError) error = NULL;

  self = DFV_VIEWER_WINDOW (user_data);
  file = G_FILE (source_object);

  file_info = g_file_query_info_finish (file, result, &error);

  if (error != NULL &&
      !g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      g_autofree gchar *parse_name = NULL;

      parse_name = g_file_get_parse_name (file);
      g_warning ("Error loading information for file ‘%s’: %s",
                 parse_name, error->message);

      return;
    }

  /* Only set the window title if this is the same file which was originally
   * being loaded — while this query operation was in progress, another file
   * might have been loaded. */
  if (self->file == file)
    {
      gtk_window_set_title (GTK_WINDOW (self),
                            g_file_info_get_display_name (file_info));
    }
}
