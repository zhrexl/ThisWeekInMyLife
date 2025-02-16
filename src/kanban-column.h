/* kanban-column.h
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

#include <adwaita.h>

G_BEGIN_DECLS

#define KANBAN_COLUMN_TYPE (kanban_column_get_type())

G_DECLARE_FINAL_TYPE (KanbanColumn, kanban_column, KANBAN, COLUMN, GtkBox)

KanbanColumn
*kanban_column_new(void);

void
kanban_column_set_title(KanbanColumn* Card, const char *title);

GtkListBox*
kanban_column_get_cards_box(KanbanColumn* Column);

void
kanban_column_add_card(KanbanColumn* Column, gpointer card);

void
kanban_column_remove_card(KanbanColumn* Column, gpointer card);

void kanban_column_get_json(KanbanColumn* Column, gpointer CardObject);

void
kanban_column_add_new_card(KanbanColumn* Column, const gchar* title, const gchar* description, gboolean revealed);

void kanban_column_insert_card(KanbanColumn *Column, double y, gpointer card);

void kanban_column_content_dropped(KanbanColumn *self);
G_END_DECLS

