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

#ifndef DFL_TYPES_H
#define DFL_TYPES_H

/**
 * SECTION:types
 * @short_description: common data types
 * @stability: Unstable
 * @include: libdunfell/types.h
 *
 * TODO
 *
 * Since: 0.1.0
 */

#include <glib.h>

G_BEGIN_DECLS

/**
 * DflTimestamp:
 *
 * TODO
 *
 * In microseconds.
 *
 * Since: 0.1.0
 */
typedef guint64 DflTimestamp;

/**
 * DflThreadId:
 *
 * TODO
 *
 * Since: 0.1.0
 */
typedef guint64 DflThreadId;

/**
 * DflDuration:
 *
 * TODO
 *
 * In microseconds.
 *
 * Since: 0.1.0
 */
typedef gint64 DflDuration;

/**
 * DflId:
 *
 * TODO
 *
 * Since: 0.1.0
 */
typedef guintptr DflId;

/**
 * DFL_ID_INVALID:
 *
 * TODO
 *
 * Since: 0.1.0
 */
#define DFL_ID_INVALID ((guintptr) 0)

G_END_DECLS

#endif /* !DFL_TYPES_H */
