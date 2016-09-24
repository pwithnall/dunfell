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
 * SECTION:application
 * @short_description: #GtkApplication subclass for the viewer application
 * @stability: Unstable
 * @include: viewer/application.h
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

#include "application.h"
#include "viewer-window.h"


static void dfv_application_activate (GApplication *application);
static void dfv_application_open (GApplication  *application,
                                  GFile        **files,
                                  gint           n_files,
                                  const gchar   *hint);
static void open_action_cb (GSimpleAction *action,
                            GVariant      *parameter,
                            gpointer       user_data);
static void record_action_cb (GSimpleAction *action,
                              GVariant      *parameter,
                              gpointer       user_data);
static void about_action_cb (GSimpleAction *action,
                             GVariant      *parameter,
                             gpointer       user_data);
static void quit_action_cb (GSimpleAction *action,
                            GVariant      *parameter,
                            gpointer       user_data);

struct _DfvApplication
{
  GtkApplication parent;
};

G_DEFINE_TYPE (DfvApplication, dfv_application, GTK_TYPE_APPLICATION)

static void
dfv_application_class_init (DfvApplicationClass *klass)
{
  GApplicationClass *application_class = G_APPLICATION_CLASS (klass);

  application_class->activate = dfv_application_activate;
  application_class->open = dfv_application_open;
}

static void
dfv_application_init (DfvApplication *self)
{
  const GActionEntry actions[] = {
    { "open", open_action_cb, NULL, NULL, NULL },
    { "record", record_action_cb, NULL, NULL, NULL },
    { "about", about_action_cb, NULL, NULL, NULL },
    { "quit", quit_action_cb, NULL, NULL, NULL },
  };

  g_set_application_name (_("Dunfell Viewer"));

  /* Set up actions. */
  g_action_map_add_action_entries (G_ACTION_MAP (self), actions,
                                   G_N_ELEMENTS (actions), self);
}

static void
dfv_application_activate (GApplication *application)
{
  GtkWindow *window = NULL;

  /* Create a new window. */
  window = GTK_WINDOW (dfv_viewer_window_new (GTK_APPLICATION (application)));
  gtk_widget_show (GTK_WIDGET (window));
}

static void
dfv_application_open (GApplication  *application,
                      GFile        **files,
                      gint           n_files,
                      const gchar   *hint)
{
  guint i;

  for (i = 0; i < (guint) n_files; i++)
    {
      GtkWindow *window = NULL;

      window = GTK_WINDOW (dfv_viewer_window_new_for_file (GTK_APPLICATION (application),
                                                           files[i]));
      gtk_widget_show (GTK_WIDGET (window));
    }
}

static DfvViewerWindow *
find_intro_window (DfvApplication *self)
{
  DfvViewerWindow *intro_window = NULL;
  GList/*<unowned GtkWindow>*/ *l;  /* unowned */

  /* Find an existing window which is showing the ‘intro’ pane, or create a
   * new one. */
  for (l = gtk_application_get_windows (GTK_APPLICATION (self));
       l != NULL && intro_window == NULL;
       l = l->next)
    {
      GtkWindow *window = GTK_WINDOW (l->data);

      if (DFV_IS_VIEWER_WINDOW (window) &&
          strcmp (dfv_viewer_window_get_pane_name (DFV_VIEWER_WINDOW (window)),
                  "intro") == 0)
        {
          intro_window = DFV_VIEWER_WINDOW (window);
        }
    }

  if (intro_window == NULL)
    {
      intro_window = dfv_viewer_window_new (GTK_APPLICATION (self));
      gtk_widget_show_all (GTK_WIDGET (intro_window));
    }

  return intro_window;
}

static void
open_action_cb (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  DfvApplication *self = DFV_APPLICATION (user_data);
  DfvViewerWindow *window = NULL;

  window = find_intro_window (self);
  gtk_window_present (GTK_WINDOW (window));
  dfv_viewer_window_open (window, NULL);
}

static void
record_action_cb (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer       user_data)
{
  DfvApplication *self = DFV_APPLICATION (user_data);
  DfvViewerWindow *window = NULL;

  window = find_intro_window (self);
  gtk_window_present (GTK_WINDOW (window));
  dfv_viewer_window_record (window);
}

static void
about_action_cb (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  DfvApplication *self = DFV_APPLICATION (user_data);
  const gchar *authors[] = {
    "Philip Withnall <philip@tecnocode.co.uk>",
    NULL
  };

  gtk_show_about_dialog (gtk_application_get_active_window (GTK_APPLICATION (self)),
                         "version", VERSION,
                         "copyright", _("Copyright © 2015 Philip Withnall"),
                         "comments", _("A program for extracting events from "
                                       "GLib main contexts and visualising "
                                       "them for the purposes of debugging and "
                                       "optimisation."),
                         "authors", authors,
                         "translator-credits", _("translator-credits"),
                         "license-type", GTK_LICENSE_LGPL_2_1,
                         "wrap-license", TRUE,
                         "website-label", _("Dunfell Website"),
                         "website", PACKAGE_URL,
                         NULL);
}

static void
quit_action_cb (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  DfvApplication *self = DFV_APPLICATION (user_data);

  g_application_quit (G_APPLICATION (self));
}

/**
 * dfv_application_new:
 *
 * TODO
 *
 * Returns: (transfer full): a new #DfvApplication
 * Since: 0.1.0
 */
DfvApplication *
dfv_application_new (void)
{
  return g_object_new (DFV_TYPE_APPLICATION,
                       "application-id", "org.gnome.Dunfell.Viewer",
                       "flags", G_APPLICATION_HANDLES_OPEN,
                       "register-session", TRUE,
                       NULL);
}
