/* kanban-window.c
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

#include "kanban-card.h"
#include "kanban-column.h"


struct _KanbanCard
{
  GtkListBoxRow parent_instance;

  GtkEditableLabel* LblCardName;
  GtkRevealer* revealercard;
  GtkTextView   *description;
  AdwButtonContent* BtnContent;
};

G_DEFINE_FINAL_TYPE (KanbanCard, kanban_card, GTK_TYPE_LIST_BOX_ROW)

KanbanCard* kanban_card_new (void)
{
	return g_object_new (KANBAN_TYPE_CARD,
	                     NULL);
}

void
kanban_card_set_title(KanbanCard* Card, const char *title)
{
  gtk_editable_set_text (GTK_EDITABLE (Card->LblCardName), title);
}

const gchar*
kanban_card_get_title(KanbanCard* Card)
{
  return gtk_editable_get_text (GTK_EDITABLE (Card->LblCardName));
}

gchar*
kanban_card_get_description(KanbanCard* Card)
{
  GtkTextBuffer* Buffer = gtk_text_view_get_buffer(Card->description);
  GtkTextIter start, end;

  gtk_text_buffer_get_bounds (Buffer, &start, &end);

  return gtk_text_buffer_get_text (Buffer, &start, &end, FALSE);
}

void
kanban_card_set_description(KanbanCard* Card,const gchar* description)
{
  GtkTextBuffer* Buffer = gtk_text_view_get_buffer(Card->description);
  gtk_text_buffer_set_text (Buffer, description, strlen(description));
}



void
kanban_card_set_css_provider(KanbanCard* card, GtkStyleProvider* CssProvider)
{
  gtk_style_context_add_provider (gtk_widget_get_style_context (GTK_WIDGET (card->description)), CssProvider, G_MAXUINT);
  gtk_style_context_add_provider (gtk_widget_get_style_context (GTK_WIDGET (card->LblCardName)), CssProvider, G_MAXUINT);
}

static void
delete_clicked(GtkButton* btn, gpointer user_data)
{
  GtkWidget* old_box        = gtk_widget_get_parent (GTK_WIDGET(user_data));
  GtkWidget* old_view       = gtk_widget_get_parent(old_box);
  GtkWidget* old_scrollWnd  = gtk_widget_get_parent(old_view);
  KanbanColumn* old_col     = KANBAN_COLUMN (gtk_widget_get_parent (old_scrollWnd));

  kanban_column_remove_card (old_col, user_data);
}
static void
reveal_clicked(GtkButton* btn, gpointer user_data)
{
  KanbanCard* card = (KanbanCard*)user_data;

  bool revealed = gtk_revealer_get_reveal_child (card->revealercard);
  if (revealed){
    adw_button_content_set_icon_name (card->BtnContent, "go-down-symbolic");
    gtk_widget_set_can_target (GTK_WIDGET(card->LblCardName), false);
    gtk_widget_set_can_focus (GTK_WIDGET(card->LblCardName), false);
  }
  else{
    adw_button_content_set_icon_name (card->BtnContent, "go-up-symbolic");
    gtk_widget_set_can_target (GTK_WIDGET(card->LblCardName), true);
    gtk_widget_set_can_focus (GTK_WIDGET(card->LblCardName), true);
  }
  gtk_revealer_set_reveal_child (card->revealercard,!revealed);
}
static void
kanban_card_class_init (KanbanCardClass *klass)
{
GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/zhrexl/kanban/kanban-card.ui");
  gtk_widget_class_bind_template_child (widget_class, KanbanCard, LblCardName);
  gtk_widget_class_bind_template_child (widget_class, KanbanCard, revealercard);
  gtk_widget_class_bind_template_child (widget_class, KanbanCard, description);
  gtk_widget_class_bind_template_child (widget_class, KanbanCard, BtnContent);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), reveal_clicked);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), delete_clicked);
}
static GdkContentProvider *
css_drag_prepare (GtkDragSource *source,
                  double         x,
                  double         y,
                  GtkWidget     *button)
{
  GdkPaintable *paintable;

  paintable = gtk_widget_paintable_new (button);
  gtk_drag_source_set_icon (source, paintable, 0, 0);
  g_object_unref (paintable);

  return gdk_content_provider_new_typed (G_TYPE_POINTER, button);
}

static void
kanban_card_init (KanbanCard *self)
{
  GtkDragSource *source;

  gtk_widget_init_template (GTK_WIDGET (self));
  source = gtk_drag_source_new ();
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (source));
  g_signal_connect (source, "prepare", G_CALLBACK (css_drag_prepare), self);
}
