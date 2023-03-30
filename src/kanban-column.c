/* kanban-column.c
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

#include "config.h"
#include "kanban-column.h"
#include "kanban-card.h"
#include <json-glib/json-glib.h>

struct _KanbanColumn
{
  GtkBox parent_instance;

  GtkEditableLabel* title;
  GtkListBox* CardsBox;

  GtkStyleProvider* provider;
  GList*         Cards;
};

G_DEFINE_FINAL_TYPE (KanbanColumn, kanban_column, GTK_TYPE_BOX)

KanbanColumn*
kanban_column_new (void)
{
  return g_object_new (KANBAN_COLUMN_TYPE,
                      NULL);
}

void
kanban_column_set_title(KanbanColumn* Column, const char *title)
{
  gtk_editable_set_text (GTK_EDITABLE (Column->title), title);
}

void
kanban_column_set_provider(KanbanColumn* Column, GtkStyleProvider* provider)
{
  Column->provider = provider;
}

GtkListBox*
kanban_column_get_cards_box(KanbanColumn* Column)
{
  return Column->CardsBox;
}

void kanban_column_get_json(KanbanColumn* Column, gpointer CardObject)
{
  JsonObject *object, *nested;

  object = json_object_new ();

  {
    KanbanCard *item;
    for(GList* elem = Column->Cards; elem; elem = elem->next) {
      nested = json_object_new ();

      item = elem->data;
      gchar* description = kanban_card_get_description (item);

      json_object_set_string_member (nested, "description", description);
      json_object_set_object_member (object, kanban_card_get_title (item), nested);

      g_free(description);
    }
  }

  const gchar* title = gtk_editable_get_text (GTK_EDITABLE(Column->title));
  json_object_set_object_member ((JsonObject*)CardObject, title, object);
}

static void
add_card(KanbanColumn* Column, KanbanCard* card)
{
  gtk_list_box_append(Column->CardsBox, GTK_WIDGET(card));
  Column->Cards = g_list_append (Column->Cards, card);
}
void
kanban_column_add_card(KanbanColumn* Column, gpointer card)
{
  add_card(Column, KANBAN_CARD (card));
}
void
kanban_column_remove_card(KanbanColumn* Column, gpointer card)
{
  gtk_list_box_remove(Column->CardsBox, GTK_WIDGET(card));
  Column->Cards = g_list_remove (Column->Cards, card);
}

void
kanban_column_add_new_card(KanbanColumn* Column, const gchar* title, const gchar* description)
{
  KanbanCard*   card   = kanban_card_new();

  kanban_card_set_title (card, title);
  kanban_card_set_description (card, description);
  gtk_style_context_add_provider (gtk_widget_get_style_context (GTK_WIDGET (card)), Column->provider, G_MAXUINT);
  kanban_card_set_css_provider(card, Column->provider);
  add_card(Column,card);
}

static void
add_card_clicked(GtkButton* btn, gpointer data)
{
  KanbanCard*   card   = kanban_card_new();
  KanbanColumn* Column = (KanbanColumn*)data;

  gchar *title;
  title = g_strdup_printf("Activity #%d", g_list_length (Column->Cards));
  kanban_card_set_title (card, title);
  g_free(title);

  gtk_style_context_add_provider (gtk_widget_get_style_context (GTK_WIDGET (card)), Column->provider, G_MAXUINT);
  kanban_card_set_css_provider(card, Column->provider);
  add_card(Column,card);
}
static void
kanban_column_class_init (KanbanColumnClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/zhrexl/kanban/kanban-column.ui");
  gtk_widget_class_bind_template_child (widget_class, KanbanColumn, title);
  gtk_widget_class_bind_template_child (widget_class, KanbanColumn, CardsBox);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), add_card_clicked);
}

static void
kanban_column_init (KanbanColumn *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  /* Initialize private variable */
  self->Cards = NULL;

}
