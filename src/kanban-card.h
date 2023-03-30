/* kanban-cards.h
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

#define KANBAN_TYPE_CARD (kanban_card_get_type())

G_DECLARE_FINAL_TYPE (KanbanCard, kanban_card, KANBAN, CARD, GtkListBoxRow)

KanbanCard *kanban_card_new(void);

void
kanban_card_set_title(KanbanCard* Card, const char *title);

void
kanban_card_set_css_provider(KanbanCard* card, GtkStyleProvider* CssProvider);

const gchar*
kanban_card_get_title(KanbanCard* Card);

gchar*
kanban_card_get_description(KanbanCard* Card);

void
kanban_card_set_description(KanbanCard* Card,const gchar* description);

G_END_DECLS
