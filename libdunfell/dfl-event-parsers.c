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

/* This file is included into dfl-parser.c. All its contents must be static. */

#include <glib.h>

#include "dfl-event.h"

static DflEvent *
parse_main_context_acquire (const gchar *event_type,
                            guint64 timestamp,
                            guint64 tid,
                            const gchar * const *parameters,
                            GError **error)
{
  /* TODO */
  return g_object_new (DFL_TYPE_EVENT, NULL);
}
