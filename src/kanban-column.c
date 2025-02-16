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

#include "kanban-column.h"
#include "config.h"
#include "gtk/gtk.h"
#include "kanban-application.h"
#include "kanban-card.h"
#include <json-glib/json-glib.h>

static GParamSpec *needs_saving = NULL;
static GParamSpec *edit_mode = NULL;
static guint SIGNAL_DELETE_COLUMN = 0;
static guint SIGNAL_CONTENT_DROPPED = 1;

struct _KanbanColumn {
  GtkBox parent_instance;

  GtkEditableLabel *title;
  GtkRevealer *Revealer;
  GtkButton *RemoveBtn;
  GtkListBox *CardsBox;
  GList *Cards;

  gboolean needs_saving;
  gboolean edit_mode;
};

G_DEFINE_FINAL_TYPE(KanbanColumn, kanban_column, GTK_TYPE_BOX)

KanbanColumn *kanban_column_new(void) {
  return g_object_new(KANBAN_COLUMN_TYPE, NULL);
}

static void remove_column(GtkButton *btn, gpointer user_data) {
  g_signal_emit(user_data, SIGNAL_DELETE_COLUMN, 0);
}
static void kanban_column_set_needs_saving(KanbanColumn *Column, bool needs) {
  GValue val = G_VALUE_INIT;
  g_value_init(&val, G_TYPE_BOOLEAN);
  g_value_set_boolean(&val, needs);
  g_object_set_property(G_OBJECT(Column),"needs_saving", &val);
}
void kanban_column_set_title(KanbanColumn *Column, const char *title) {
  gtk_editable_set_text(GTK_EDITABLE(Column->title), title);
}

void kanban_column_content_dropped(KanbanColumn *self) {
  g_signal_emit(self, SIGNAL_CONTENT_DROPPED, 0);
}

// This is related to issue #31
static void kanban_column_content_dropped_callback(KanbanColumn *self) {
  KanbanCard *item;
  for (GList *elem = self->Cards; elem; elem = elem->next) {
    item = elem->data;
    kanban_card_content_dropped(item);
  }

}

GtkListBox *kanban_column_get_cards_box(KanbanColumn *Column) {
  return Column->CardsBox;
}

void kanban_column_get_json(KanbanColumn *Column, gpointer CardObject) {
  JsonObject *object, *nested;
  object = json_object_new();

  {
    KanbanCard *item;
    for (GList *elem = Column->Cards; elem; elem = elem->next) {
      nested = json_object_new();

      item = elem->data;
      GBytes *byteDescription = kanban_card_get_description(item);
      gsize size = 0;
      const gchar *description = g_bytes_get_data(byteDescription, &size);

      json_object_set_string_member(nested, "description", description);
      json_object_set_int_member(nested, "revealed",
                                 kanban_card_get_reveal(item));
      json_object_set_object_member(object, kanban_card_get_title(item),
                                    nested);

      g_bytes_unref(byteDescription);
    }
  }

  const gchar *title = gtk_editable_get_text(GTK_EDITABLE(Column->title));
  json_object_set_object_member((JsonObject *)CardObject, title, object);
}

static void add_card(KanbanColumn *Column, KanbanCard *card){
  gtk_list_box_append(Column->CardsBox, GTK_WIDGET(card));
  Column->Cards = g_list_append(Column->Cards, card);
}

void kanban_column_add_card(KanbanColumn *Column, gpointer card) {
  add_card(Column, KANBAN_CARD(card));
  kanban_column_set_needs_saving(Column, true);
}

void kanban_column_insert_card(KanbanColumn *Column, double y, gpointer card){
  GtkListBoxRow* row = gtk_list_box_get_row_at_y(Column->CardsBox,y);

  if (row == NULL) {
    add_card(Column, KANBAN_CARD(card));
    return;
  }

  int index = gtk_list_box_row_get_index(row);
  gtk_list_box_insert(Column->CardsBox, card, index );
  Column->Cards = g_list_insert(Column->Cards, card, index);

  kanban_column_set_needs_saving(Column, true);
}

void kanban_column_remove_card(KanbanColumn *Column, gpointer card) {
  gtk_list_box_remove(Column->CardsBox, GTK_WIDGET(card));
  Column->Cards = g_list_remove(Column->Cards, card);
  kanban_column_set_needs_saving(Column, true);
}

void kanban_column_add_new_card(KanbanColumn *Column, const gchar *title,
                                const gchar *description, gboolean revealed) {
  KanbanCard *card = kanban_card_new();

  kanban_card_set_title(card, title);
  kanban_card_set_description(card, description);
  kanban_card_set_reveal(card, revealed);

  add_card(Column, card);

  g_object_bind_property(Column, "needs-saving", card, "needs-saving",
                         G_BINDING_BIDIRECTIONAL);
}

static void add_card_clicked(GtkButton *btn, gpointer user_data) {
  KanbanCard *card = kanban_card_new();
  KanbanColumn *Column = (KanbanColumn *)user_data;

  gchar *title;
  title = g_strdup_printf("Activity #%d", g_list_length(Column->Cards));
  kanban_card_set_title(card, title);
  g_free(title);

  g_object_bind_property(Column, "needs-saving", card, "needs-saving",
                         G_BINDING_BIDIRECTIONAL);
  add_card(Column, card);
}

static void kanban_get_property(GObject *object, guint property_id,
                                GValue *value, GParamSpec *pspec) {
  KanbanColumn *self = KANBAN_COLUMN(object);

  if (property_id == 1)
    g_value_set_boolean(value, self->needs_saving);
  else if (property_id == 2)
    g_value_set_boolean(value, self->edit_mode);
  else
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
}

static void kanban_set_property(GObject *object, guint property_id,
                                const GValue *value, GParamSpec *pspec) {
  KanbanColumn *self = KANBAN_COLUMN(object);

  if (property_id == 1)
    self->needs_saving = g_value_get_boolean(value);
  else if (property_id == 2)
    self->edit_mode = g_value_get_boolean(value);
  else
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
}

static void kanban_column_class_init(KanbanColumnClass *klass) {
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

  gtk_widget_class_set_template_from_resource(
      widget_class, "/com/github/zhrexl/kanban/kanban-column.ui");
  gtk_widget_class_bind_template_child(widget_class, KanbanColumn, title);
  gtk_widget_class_bind_template_child(widget_class, KanbanColumn, CardsBox);
  gtk_widget_class_bind_template_child(widget_class, KanbanColumn, Revealer);
  gtk_widget_class_bind_template_child(widget_class, KanbanColumn, RemoveBtn);

  gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(klass),
                                          add_card_clicked);
  gtk_widget_class_bind_template_callback(GTK_WIDGET_CLASS(klass),
                                          remove_column);

  GObjectClass *GClass = G_OBJECT_CLASS(klass);
  GClass->get_property = kanban_get_property;
  GClass->set_property = kanban_set_property;

  needs_saving = g_param_spec_boolean("needs-saving", "needsave",
                                      "Boolean value", 0, G_PARAM_READWRITE);
  edit_mode = g_param_spec_boolean("edit-mode", "editmode", "Boolean value", 0,
                                   G_PARAM_READWRITE);

  g_object_class_install_property(GClass, 1, needs_saving);
  g_object_class_install_property(GClass, 2, edit_mode);

  SIGNAL_DELETE_COLUMN =
      g_signal_new("delete-column", G_TYPE_FROM_CLASS(klass),
                   G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                   0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  SIGNAL_CONTENT_DROPPED =
    g_signal_new("content-dropped", G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}
static void title_changed(GtkEditableLabel *label, gpointer user_data) {
  g_object_set(user_data, "needs-saving", 1, NULL);
  if (IsInitialized) {
    SaveNeeded = true;
  }
}

static void kanban_column_init(KanbanColumn *self) {
  gtk_widget_init_template(GTK_WIDGET(self));

  /* Initialize private variable */
  self->Cards = NULL;
  g_object_bind_property(self, "edit-mode", self->Revealer, "reveal-child",
                         G_BINDING_BIDIRECTIONAL);
  g_signal_connect(self->title, "changed", G_CALLBACK(title_changed), self);

  g_signal_connect(self, "content-dropped",  G_CALLBACK(kanban_column_content_dropped_callback), self);
}
