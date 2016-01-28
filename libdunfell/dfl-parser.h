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

#ifndef DFL_PARSER_H
#define DFL_PARSER_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include "dfl-event-sequence.h"

G_BEGIN_DECLS

/**
 * DflParser:
 *
 * All the fields in this structure are private.
 *
 * Since: 0.1.0
 */
#define DFL_TYPE_PARSER dfl_parser_get_type ()
G_DECLARE_FINAL_TYPE (DflParser, dfl_parser, DFL, PARSER, GObject)

DflParser *dfl_parser_new (void);

void dfl_parser_load_from_data (DflParser *self,
                                const guint8 *data,
                                gssize length,
                                GError **error);

void dfl_parser_load_from_file (DflParser *self,
                                const gchar *filename,
                                GError **error);

void dfl_parser_load_from_stream (DflParser *self,
                                  GInputStream *stream,
                                  GCancellable *cancellable,
                                  GError **error);
void dfl_parser_load_from_stream_async (DflParser *self,
                                        GInputStream *stream,
                                        GCancellable *cancellable,
                                        GAsyncReadyCallback callback,
                                        gpointer user_data);
void dfl_parser_load_from_stream_finish (DflParser *self,
                                         GAsyncResult *result,
                                         GError **error);

DflEventSequence *dfl_parser_get_event_sequence (DflParser *self);

G_END_DECLS

#endif /* !DFL_PARSER_H */
