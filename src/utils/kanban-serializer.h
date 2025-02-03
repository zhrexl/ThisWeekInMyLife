/* kanban-serializer.h
 *
 * Copyright 2023 zhrexl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk-4.0/gtk/gtk.h>

typedef struct {
  GByteArray *text;
  GList *anchors;
} KanbanUnserializedContent;

/*
 * Use g_free(KanbanAnchor->title) to release the string
 * */
typedef struct {
  guint offset;
  gchar *title;
  gboolean active;
} KanbanAnchor;

GBytes *get_serialized_buffer(GtkTextBuffer *buffer);

KanbanUnserializedContent *get_unserialized_buffer(const gchar *description);
