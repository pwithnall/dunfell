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
#include "libdunfell-ui/source-model.h"
#include "libdunfell-ui/statistics-pane.h"
#include "libdunfell-ui/task-model.h"
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
static void main_stack_notify_visible_child_name (GObject    *object,
                                                  GParamSpec *pspec,
                                                  gpointer    user_data);

typedef struct {
  const gchar *text;
  gint column;
} EmptyStringRendererData;

G_DEFINE_AUTOPTR_CLEANUP_FUNC (EmptyStringRendererData, g_free)

static void hex_renderer_cb (GtkTreeViewColumn *tree_column,
                             GtkCellRenderer   *cell,
                             GtkTreeModel      *tree_model,
                             GtkTreeIter       *iter,
                             gpointer           user_data);
static void number_renderer_cb (GtkTreeViewColumn *tree_column,
                                GtkCellRenderer   *cell,
                                GtkTreeModel      *tree_model,
                                GtkTreeIter       *iter,
                                gpointer           user_data);
static void empty_string_renderer_cb (GtkTreeViewColumn *tree_column,
                                      GtkCellRenderer   *cell,
                                      GtkTreeModel      *tree_model,
                                      GtkTreeIter       *iter,
                                      gpointer           user_data);

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
  GtkStack *file_stack;
  GtkStackSwitcher *file_stack_switcher;
  GtkHeaderBar *header_bar;

  /* Sources tree view. */
  GtkTreeView *sources_tree_view;
  GtkTreeViewColumn *sources_name_column;
  GtkCellRenderer *sources_name_renderer;
  GtkTreeViewColumn *sources_address_column;
  GtkCellRenderer *sources_address_renderer;
  GtkTreeViewColumn *sources_max_priority_column;
  GtkCellRenderer *sources_max_priority_renderer;
  GtkTreeViewColumn *sources_n_dispatches_column;
  GtkCellRenderer *sources_n_dispatches_renderer;
  GtkTreeViewColumn *sources_min_dispatch_duration_column;
  GtkCellRenderer *sources_min_dispatch_duration_renderer;
  GtkTreeViewColumn *sources_median_dispatch_duration_column;
  GtkCellRenderer *sources_median_dispatch_duration_renderer;
  GtkTreeViewColumn *sources_max_dispatch_duration_column;
  GtkCellRenderer *sources_max_dispatch_duration_renderer;

  /* Tasks tree view. */
  GtkTreeView *tasks_tree_view;
  GtkTreeViewColumn *tasks_address_column;
  GtkCellRenderer *tasks_address_renderer;
  GtkTreeViewColumn *tasks_run_duration_column;
  GtkCellRenderer *tasks_run_duration_renderer;
  GtkTreeViewColumn *tasks_thread_run_duration_column;
  GtkCellRenderer *tasks_thread_run_duration_renderer;
  GtkTreeViewColumn *tasks_thread_name_column;
  GtkCellRenderer *tasks_thread_name_renderer;
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
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        file_stack);

  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        sources_tree_view);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        sources_name_column);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        sources_name_renderer);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        sources_address_column);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        sources_address_renderer);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        sources_max_priority_column);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        sources_max_priority_renderer);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        sources_n_dispatches_column);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        sources_n_dispatches_renderer);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        sources_min_dispatch_duration_column);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        sources_min_dispatch_duration_renderer);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        sources_median_dispatch_duration_column);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        sources_median_dispatch_duration_renderer);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        sources_max_dispatch_duration_column);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        sources_max_dispatch_duration_renderer);

  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        tasks_tree_view);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        tasks_address_column);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        tasks_address_renderer);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        tasks_run_duration_column);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        tasks_run_duration_renderer);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        tasks_thread_run_duration_column);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        tasks_thread_run_duration_renderer);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        tasks_thread_name_column);
  gtk_widget_class_bind_template_child (widget_class, DfvViewerWindow,
                                        tasks_thread_name_renderer);

  gtk_widget_class_bind_template_callback (widget_class, open_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class, record_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           main_stack_notify_visible_child_name);

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
  g_autoptr (EmptyStringRendererData) empty_string_data = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));

  /* Set up the header bar and stack switcher. */
  self->header_bar = GTK_HEADER_BAR (gtk_header_bar_new ());
  gtk_header_bar_set_show_close_button (self->header_bar, TRUE);
  gtk_window_set_titlebar (GTK_WINDOW (self), GTK_WIDGET (self->header_bar));
  gtk_widget_show (GTK_WIDGET (self->header_bar));

  self->file_stack_switcher = GTK_STACK_SWITCHER (gtk_stack_switcher_new ());
  gtk_stack_switcher_set_stack (self->file_stack_switcher, self->file_stack);

  gtk_header_bar_pack_start (self->header_bar,
                             GTK_WIDGET (self->file_stack_switcher));

  gtk_widget_hide (GTK_WIDGET (self->file_stack_switcher));

  gtk_window_set_title (GTK_WINDOW (self), _("Dunfell Viewer"));
  gtk_header_bar_set_title (self->header_bar,
                            gtk_window_get_title (GTK_WINDOW (self)));

  /* Set the initial stack pages. */
  gtk_stack_set_visible_child_name (self->main_stack, "intro");
  gtk_stack_set_visible_child_name (self->file_stack, "timeline");

  gtk_header_bar_set_title (self->header_bar,
                            gtk_window_get_title (GTK_WINDOW (self)));

  /* Set up the sources tree view. */
  empty_string_data = g_new0 (EmptyStringRendererData, 1);
  empty_string_data->text = _("Unset");
  empty_string_data->column = 4;

  gtk_tree_view_column_set_cell_data_func (self->sources_name_column,
                                           self->sources_name_renderer,
                                           empty_string_renderer_cb,
                                           g_steal_pointer (&empty_string_data),
                                           g_free);
  gtk_tree_view_column_set_cell_data_func (self->sources_address_column,
                                           self->sources_address_renderer,
                                           hex_renderer_cb,
                                           GINT_TO_POINTER (0)  /* column index */,
                                           NULL);
  gtk_tree_view_column_set_cell_data_func (self->sources_max_priority_column,
                                           self->sources_max_priority_renderer,
                                           number_renderer_cb,
                                           GINT_TO_POINTER (11)  /* column index */,
                                           NULL);
  gtk_tree_view_column_set_cell_data_func (self->sources_n_dispatches_column,
                                           self->sources_n_dispatches_renderer,
                                           number_renderer_cb,
                                           GINT_TO_POINTER (12)  /* column index */,
                                           NULL);
  gtk_tree_view_column_set_cell_data_func (self->sources_min_dispatch_duration_column,
                                           self->sources_min_dispatch_duration_renderer,
                                           number_renderer_cb,
                                           GINT_TO_POINTER (13)  /* column index */,
                                           NULL);
  gtk_tree_view_column_set_cell_data_func (self->sources_median_dispatch_duration_column,
                                           self->sources_median_dispatch_duration_renderer,
                                           number_renderer_cb,
                                           GINT_TO_POINTER (14)  /* column index */,
                                           NULL);
  gtk_tree_view_column_set_cell_data_func (self->sources_max_dispatch_duration_column,
                                           self->sources_max_dispatch_duration_renderer,
                                           number_renderer_cb,
                                           GINT_TO_POINTER (15)  /* column index */,
                                           NULL);

  /* Set up the tasks tree view. */
  gtk_tree_view_column_set_cell_data_func (self->tasks_address_column,
                                           self->tasks_address_renderer,
                                           hex_renderer_cb,
                                           GINT_TO_POINTER (0)  /* column index */,
                                           NULL);
  gtk_tree_view_column_set_cell_data_func (self->tasks_run_duration_column,
                                           self->tasks_run_duration_renderer,
                                           number_renderer_cb,
                                           GINT_TO_POINTER (18)  /* column index */,
                                           NULL);
  gtk_tree_view_column_set_cell_data_func (self->tasks_thread_run_duration_column,
                                           self->tasks_thread_run_duration_renderer,
                                           number_renderer_cb,
                                           GINT_TO_POINTER (19),  /* column index */
                                           NULL);

  empty_string_data = g_new0 (EmptyStringRendererData, 1);
  empty_string_data->text = _("Task not threaded");
  empty_string_data->column = 16;

  gtk_tree_view_column_set_cell_data_func (self->tasks_thread_name_column,
                                           self->tasks_thread_name_renderer,
                                           empty_string_renderer_cb,
                                           g_steal_pointer (&empty_string_data),
                                           g_free);
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

static void
hex_renderer_cb (GtkTreeViewColumn *tree_column,
                 GtkCellRenderer   *cell,
                 GtkTreeModel      *tree_model,
                 GtkTreeIter       *iter,
                 gpointer           user_data)
{
  g_auto (GValue) value = G_VALUE_INIT, uint64_value = G_VALUE_INIT;
  gint column_index;
  guint64 uint64;
  g_autofree gchar *hex_string = NULL;

  g_assert (GTK_IS_CELL_RENDERER_TEXT (cell));

  column_index = GPOINTER_TO_INT (user_data);

  /* Grab the original numeric value and render it as a hex string. */
  gtk_tree_model_get_value (tree_model, iter, column_index, &value);

  g_value_init (&uint64_value, G_TYPE_UINT64);
  g_assert (g_value_transform (&value, &uint64_value));
  uint64 = g_value_get_uint64 (&uint64_value);
  hex_string = g_strdup_printf ("0x%llx", (long long unsigned int) uint64);

  g_object_set (G_OBJECT (cell),
                "text", hex_string,
                "family", "Monospace",
                "xalign", 1.0,
                NULL);
}

static void
number_renderer_cb (GtkTreeViewColumn *tree_column,
                    GtkCellRenderer   *cell,
                    GtkTreeModel      *tree_model,
                    GtkTreeIter       *iter,
                    gpointer           user_data)
{
  g_auto (GValue) value = G_VALUE_INIT, int64_value = G_VALUE_INIT;
  gint column_index;
  gint64 int64;
  g_autofree gchar *spaced_string = NULL;

  g_assert (GTK_IS_CELL_RENDERER_TEXT (cell));

  column_index = GPOINTER_TO_INT (user_data);

  /* Grab the original numeric value and render it with grouped digits. */
  gtk_tree_model_get_value (tree_model, iter, column_index, &value);

  g_value_init (&int64_value, G_TYPE_INT64);
  g_assert (g_value_transform (&value, &int64_value));
  int64 = g_value_get_int64 (&int64_value);
  spaced_string = g_strdup_printf ("%'" G_GINT64_FORMAT, int64);

  g_object_set (G_OBJECT (cell),
                "text", spaced_string,
                "xalign", 1.0,
                NULL);
}

static void
empty_string_renderer_cb (GtkTreeViewColumn *tree_column,
                          GtkCellRenderer   *cell,
                          GtkTreeModel      *tree_model,
                          GtkTreeIter       *iter,
                          gpointer           user_data)
{
  g_auto (GValue) value = G_VALUE_INIT;
  EmptyStringRendererData *data = user_data;
  const gchar *str, *foreground;
  PangoStyle style;

  g_assert (GTK_IS_CELL_RENDERER_TEXT (cell));

  /* Grab the original string value. If it is NULL or empty, substitute a
   * replacement and format it differently. */
  gtk_tree_model_get_value (tree_model, iter, data->column, &value);
  str = g_value_get_string (&value);

  if (str == NULL || *str == '\0')
    {
      str = data->text;
      foreground = "gray";
      style = PANGO_STYLE_ITALIC;
    }
  else
    {
      foreground = NULL;
      style = PANGO_STYLE_NORMAL;
    }

  g_object_set (G_OBJECT (cell),
                "text", str,
                "foreground", foreground,
                "style", style,
                NULL);
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

static void
main_stack_notify_visible_child_name (GObject    *object,
                                      GParamSpec *pspec,
                                      gpointer    user_data)
{
  DfvViewerWindow *self = DFV_VIEWER_WINDOW (user_data);
  gboolean is_file_pane;

  is_file_pane = g_str_equal (gtk_stack_get_visible_child_name (self->main_stack),
                              "file");

  gtk_widget_set_visible (GTK_WIDGET (self->file_stack_switcher),
                          is_file_pane);
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
  gtk_header_bar_set_title (self->header_bar,
                            gtk_window_get_title (GTK_WINDOW (self)));
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
  g_autoptr (DwlSourceModel) source_model = NULL;
  g_autoptr (DwlTaskModel) task_model = NULL;
  g_autoptr (GPtrArray) sources = NULL;  /* (element-type DflSource) */
  g_autoptr (GPtrArray) tasks = NULL;  /* (element-type DflTask) */
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

  sources = dfl_model_dup_sources (model);
  source_model = dwl_source_model_new (sources);
  gtk_tree_view_set_model (self->sources_tree_view,
                           GTK_TREE_MODEL (source_model));

  tasks = dfl_model_dup_tasks (model);
  task_model = dwl_task_model_new (tasks);
  gtk_tree_view_set_model (self->tasks_tree_view,
                           GTK_TREE_MODEL (task_model));

  gtk_stack_set_visible_child_name (self->file_stack, "timeline");
  gtk_stack_set_visible_child_name (self->main_stack, "file");
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
      const gchar *filename;

      filename = g_file_info_get_display_name (file_info);
      gtk_window_set_title (GTK_WINDOW (self), filename);
      gtk_header_bar_set_title (self->header_bar, filename);
    }
}
