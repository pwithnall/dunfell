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

#include "dwl-timeline.h"


static void dwl_timeline_dispose (GObject *object);

struct _DwlTimeline
{
  GtkWidget parent;
};

G_DEFINE_TYPE (DwlTimeline, dwl_timeline, GTK_TYPE_WIDGET)

static void
dwl_timeline_class_init (DwlTimelineClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = dwl_timeline_dispose;
}

static void
dwl_timeline_init (DwlTimeline *self)
{
  /* Nothing to see here. */
}

static void
dwl_timeline_dispose (GObject *object)
{
  /* Chain up to the parent class */
  G_OBJECT_CLASS (dwl_timeline_parent_class)->dispose (object);
}

/**
 * dwl_timeline_new:
 *
 * TODO
 *
 * Returns: (transfer full): a new #DwlTimeline
 * Since: UNRELEASED
 */
DwlTimeline *
dwl_timeline_new (void)
{
  return g_object_new (DWL_TYPE_TIMELINE, NULL);
}
