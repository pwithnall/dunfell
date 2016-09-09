/* vim:set et sw=2 cin cino=t0,f0,(0,{s,>2s,n-s,^-s,e2s: */
/*
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
 * SECTION:statistics-pane
 * @short_description: pane widget to display statistics on selected objects
 * @stability: Unstable
 * @include: libdunfell-ui/statistics-pane.h
 *
 * A pane which displays pertinent statistics from an underlying #DflModel, and
 * can display more detailed statistics for a selected object, if one is set as
 * #DwlStatisticsPane:selected-object.
 *
 * This is intended to be used as a side pane in an application rather than the
 * main display of the model’s data.
 *
 * Since: UNRELEASED
 */

#include "config.h"

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "libdunfell-ui/statistics-pane.h"


static void dwl_statistics_pane_get_property (GObject      *object,
                                              guint         property_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);
static void dwl_statistics_pane_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void dwl_statistics_pane_constructed  (GObject      *object);
static void dwl_statistics_pane_dispose      (GObject      *object);

static void add_default_css                  (GtkWidget *widget);

static void dwl_statistics_pane_update_overall_statistics (DwlStatisticsPane *self);

struct _DwlStatisticsPane
{
  GtkBin parent;

  DflModel *model;  /* (ownership full) */
  GObject *selected_object;  /* (ownership full) (nullable); NULL iff no object is selected */

  GtkStack *stack;

  /* Overall statistics. */
  GtkLabel *n_sources;
  GtkLabel *n_tasks;
};

G_DEFINE_TYPE (DwlStatisticsPane, dwl_statistics_pane, GTK_TYPE_BIN)

typedef enum
{
  PROP_MODEL = 1,
  PROP_SELECTED_OBJECT,
} DwlStatisticsPaneProperty;

static void
dwl_statistics_pane_class_init (DwlStatisticsPaneClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  /* Set up the composite widget. */
  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/gnome/libdunfell-ui/ui/statistics-pane.ui");
  gtk_widget_class_bind_template_child (widget_class,
                                        DwlStatisticsPane, stack);

  gtk_widget_class_bind_template_child (widget_class,
                                        DwlStatisticsPane, n_sources);
  gtk_widget_class_bind_template_child (widget_class,
                                        DwlStatisticsPane, n_tasks);

  object_class->get_property = dwl_statistics_pane_get_property;
  object_class->set_property = dwl_statistics_pane_set_property;
  object_class->constructed = dwl_statistics_pane_constructed;
  object_class->dispose = dwl_statistics_pane_dispose;

  /**
   * DwlStatisticsPane:model:
   *
   * Model containing the underlying data to extract statistics from.
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_MODEL,
                                   g_param_spec_object ("model",
                                                        "Model",
                                                        "Model containing the "
                                                        "underlying data to "
                                                        "extract statistics "
                                                        "from.",
                                                        DFL_TYPE_MODEL,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DwlStatisticsPane:selected-object:
   *
   * An object to display more detailed statistics for. If the object does not
   * exist in #DwlStatisticsPane:model, or if its type is unrecognised, it will
   * be ignored (without any error).
   *
   * If it is set to %NULL or to an unrecognised object, overall statistics for
   * the whole model will be displayed.
   *
   * Supported types include #DflMainContext, #DflSource, #DflTask and
   * #DflThread.
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_SELECTED_OBJECT,
                                   g_param_spec_object ("selected-object",
                                                        "Selected Object",
                                                        "An object to display "
                                                        "more detailed "
                                                        "statistics for.",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));
}

static void
dwl_statistics_pane_init (DwlStatisticsPane *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  add_default_css (GTK_WIDGET (self));
}

static void
dwl_statistics_pane_get_property (GObject     *object,
                                  guint        property_id,
                                  GValue      *value,
                                  GParamSpec  *pspec)
{
  DwlStatisticsPane *self = DWL_STATISTICS_PANE (object);

  switch ((DwlStatisticsPaneProperty) property_id)
    {
    case PROP_MODEL:
      g_value_set_object (value, self->model);
      break;
    case PROP_SELECTED_OBJECT:
      g_value_set_object (value, self->selected_object);
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
dwl_statistics_pane_set_property (GObject           *object,
                                  guint              property_id,
                                  const GValue      *value,
                                  GParamSpec        *pspec)
{
  DwlStatisticsPane *self = DWL_STATISTICS_PANE (object);

  switch ((DwlStatisticsPaneProperty) property_id)
    {
    case PROP_MODEL:
      /* Construct-only. */
      g_assert (self->model == NULL);
      self->model = g_value_dup_object (value);
      break;
    case PROP_SELECTED_OBJECT:
      dwl_statistics_pane_set_selected_object (self,
                                               g_value_get_object (value));
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
dwl_statistics_pane_constructed (GObject *object)
{
  DwlStatisticsPane *self = DWL_STATISTICS_PANE (object);

  /* Chain up. */
  G_OBJECT_CLASS (dwl_statistics_pane_parent_class)->constructed (object);

  /* We must have a model set. */
  g_assert (self->model != NULL);

  /* Set the initial stack page. */
  dwl_statistics_pane_update_overall_statistics (self);
  gtk_stack_set_visible_child_name (self->stack, "overall");
}

static void
dwl_statistics_pane_dispose (GObject *object)
{
  DwlStatisticsPane *self = DWL_STATISTICS_PANE (object);

  g_clear_object (&self->selected_object);
  g_clear_object (&self->model);

  G_OBJECT_CLASS (dwl_statistics_pane_parent_class)->dispose (object);
}

static void
add_default_css (GtkWidget *widget)
{
  g_autoptr (GtkCssProvider) provider = NULL;
  g_autoptr (GError) error = NULL;
  const gchar *css;

  css = "#statistics_list { min-width: 500px }\n";

  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (provider, css, -1, &error);
  g_assert_no_error (error);

  gtk_style_context_add_provider (gtk_widget_get_style_context (widget),
                                  GTK_STYLE_PROVIDER (provider),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

/**
 * dwl_statistics_pane_new:
 * @model: (transfer none): model to display statistics for
 *
 * Create a new #DwlStatisticsPane to display statistics for the given @model.
 *
 * Returns: (transfer full): a new #DwlStatisticsPane
 * Since: UNRELEASED
 */
DwlStatisticsPane *
dwl_statistics_pane_new (DflModel *model)
{
  g_return_val_if_fail (DFL_IS_MODEL (model), NULL);

  return g_object_new (DWL_TYPE_STATISTICS_PANE,
                       "model", model,
                       NULL);
}

/**
 * dwl_statistics_pane_set_selected_object:
 * @self: a #DwlStatisticsPane
 * @obj: (nullable) (transfer none): a new object to display statistics on, or
 *    %NULL
 *
 * Set #DwlStatisticsPane:selected-object to @obj, which may be %NULL to
 * display overall statistics.
 *
 * Since: UNRELEASED
 */
void
dwl_statistics_pane_set_selected_object (DwlStatisticsPane *self,
                                         GObject           *obj)
{
  gboolean changed;

  g_return_if_fail (DWL_IS_STATISTICS_PANE (self));
  g_return_if_fail (obj == NULL || G_IS_OBJECT (obj));

  changed = g_set_object (&self->selected_object, obj);

  if (obj == NULL)
    gtk_stack_set_visible_child_name (self->stack, "overall");

  if (changed)
    g_object_notify (G_OBJECT (self), "selected-object");
}

static void
dwl_statistics_pane_update_overall_statistics (DwlStatisticsPane *self)
{
  g_autoptr (GPtrArray) sources = NULL;  /* (element-type DflSource) */
  g_autoptr (GPtrArray) tasks = NULL;  /* (element-type DflTask) */
  g_autofree gchar *n_sources = NULL, *n_tasks = NULL;

  sources = dfl_model_dup_sources (self->model);
  tasks = dfl_model_dup_tasks (self->model);

  n_sources = g_strdup_printf ("%u", sources->len);
  n_tasks = g_strdup_printf ("%u", tasks->len);

  gtk_label_set_text (self->n_sources, n_sources);
  gtk_label_set_text (self->n_tasks, n_tasks);
}
