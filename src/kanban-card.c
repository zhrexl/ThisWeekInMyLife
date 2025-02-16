/* kanban-card.c
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

// TODO: Implement a way to colorize cards, and have the color be saved in JSON
// format too

#include "config.h"

#include "kanban-card.h"
#include "glib-object.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "kanban-application.h"
#include "kanban-column.h"
#include "kanban-window.h"
#include "utils/kanban-serializer.h"

static GParamSpec *needs_saving = NULL;

struct _KanbanCard
{
  GtkListBoxRow      parent_instance;

  GtkBox            *handlerDrag;
  GtkEditableLabel  *LblCardName;
  GtkRevealer       *revealercard;
  GtkRevealer       *drop_revealer;
  GtkTextView       *description;
  AdwButtonContent  *BtnContent;
  guint              description_changed;
  gboolean           needs_saving;
};

const gchar checktemplate[] = "<task status=";
const gchar donexml[]       = "done";
const gchar progress[]      = "progress";
const gchar titlexml[]      = " title=\"";
const gchar endtitle[]      = "\"/>";
const gchar nullbyte[]      = "\0";

G_DEFINE_FINAL_TYPE (KanbanCard, kanban_card, GTK_TYPE_LIST_BOX_ROW)

KanbanCard* kanban_card_new (void)
{
	return g_object_new (KANBAN_TYPE_CARD,
                      NULL);
}

void kanban_card_set_title(KanbanCard *Card, const char *title) {
  gtk_editable_set_text(GTK_EDITABLE(Card->LblCardName), title);
}

const gchar*
kanban_card_get_title(KanbanCard* Card)
{
  return gtk_editable_get_text (GTK_EDITABLE (Card->LblCardName));
}

gboolean
kanban_card_get_reveal(KanbanCard* Card)
{
  return gtk_revealer_get_reveal_child (Card->revealercard);
}

void
kanban_card_set_reveal(KanbanCard* card, gboolean revealed)
{
  if (!revealed)
  {
    adw_button_content_set_icon_name (card->BtnContent, 
                                      "go-down-symbolic");

    gtk_widget_set_can_target (GTK_WIDGET(card->LblCardName), 
                                                      false);
                                                  
    gtk_widget_set_can_focus (GTK_WIDGET(card->LblCardName), 
                                                      false);
  }
  else
  {
    adw_button_content_set_icon_name (card->BtnContent, "go-up-symbolic");
    gtk_widget_set_can_target (GTK_WIDGET(card->LblCardName), true);
    gtk_widget_set_can_focus (GTK_WIDGET(card->LblCardName), true);
  }

  gtk_revealer_set_reveal_child (card->revealercard, revealed);
}

/* the user must unref the returned pointer with g_bytes_unref() */
GBytes*
kanban_card_get_description(KanbanCard* Card)
{
  GtkTextBuffer* Buffer = gtk_text_view_get_buffer(Card->description);
  return get_serialized_buffer (Buffer);
}

void kanban_card_content_dropped(KanbanCard* self) {
  gtk_revealer_set_reveal_child(self->drop_revealer, false);
}

static void
create_task(GtkTextView* text_view, GtkTextIter* iter, const gchar* title, gboolean active)
{

  GtkTextChildAnchor* anchor = gtk_text_buffer_create_child_anchor (gtk_text_view_get_buffer (text_view), iter);
  GtkWidget* box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_widget_set_margin_start (box, 5);

  GtkWidget* label  = gtk_editable_label_new (title);
  GtkWidget * child = gtk_check_button_new ();

  gtk_check_button_set_active (GTK_CHECK_BUTTON (child), active);

  gtk_box_append (GTK_BOX(box), child);
  gtk_box_append (GTK_BOX(box), label);

  //gtk_widget_set_size_request (child, 10, -1);
  gtk_text_view_add_child_at_anchor (text_view, box, anchor);
  //gtk_text_buffer_insert_at_cursor (buffer, "\n", 1);"
}

void
kanban_card_set_description(KanbanCard* Card,const gchar* description)
{
  int dsc_len = strlen(description);

  if (!dsc_len)
    return;

  GtkTextBuffer*  buf = gtk_text_view_get_buffer(Card->description);
  /* Block changed signal to avoid unnecessary unsaved file flag */
  g_signal_handler_block(buf, Card->description_changed);

  KanbanUnserializedContent* KUnContent = get_unserialized_buffer (description);

  if (!KUnContent)
  {
    g_print ("Error loading saved filed\n");
    return;
  }

  /* Set text description to buffer */
  GBytes* bytes = g_byte_array_free_to_bytes(KUnContent->text);
  gsize    size;
  char* text = (char*)g_bytes_get_data (bytes, &size);

  GtkTextIter iter;
  gtk_text_buffer_set_text (buf, text, size);
  gtk_text_buffer_get_end_iter(buf, &iter);
  g_bytes_unref (bytes);

  /* Create and insert the anchors in GtkTextView */
  GList* elem = NULL;
  /* For each inserted anchor offset increments by one */
  int i = 0;
  for(elem = KUnContent->anchors; elem; elem = elem->next)
  {
    KanbanAnchor* anchor = elem->data;
    gtk_text_iter_set_offset (&iter, anchor->offset + i);
    create_task (Card->description, &iter, anchor->title, anchor->active);
    g_free(anchor->title);
    i++;
  }
  g_signal_handler_unblock(buf, Card->description_changed);

  g_list_free (KUnContent->anchors);
  free(KUnContent);
}

static void
insert_checkbox(GtkButton* btn, gpointer data)
{
  GtkTextView* text_view  = GTK_TEXT_VIEW (data);
  GtkTextBuffer* buffer   = gtk_text_view_get_buffer(text_view);
  GtkTextIter iter        = {};

  gtk_text_buffer_insert_at_cursor (buffer, "\n", 1);
  gtk_text_buffer_get_iter_at_mark (buffer, &iter, gtk_text_buffer_get_mark (buffer, "insert"));

  create_task(text_view, &iter, "Task #", false);
}


static void
delete_clicked(GtkButton* btn, gpointer user_data)
{
  GtkWidget *old_box        = gtk_widget_get_parent (GTK_WIDGET(user_data));
  GtkWidget *old_view       = gtk_widget_get_parent(old_box);
  GtkWidget *old_scrollWnd  = gtk_widget_get_parent(old_view);
  KanbanColumn *old_col     = KANBAN_COLUMN (gtk_widget_get_parent (old_scrollWnd));

  g_object_set(user_data, "needs-saving", 1, NULL);
  SaveNeeded = true;
  kanban_column_remove_card(old_col, user_data);
}

static void
reveal_clicked(GtkButton* btn, gpointer user_data)
{
  KanbanCard* card = (KanbanCard*)user_data;
  kanban_card_set_reveal (card, !kanban_card_get_reveal (card));
}
static void 
kanban_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec) 
{
  KanbanCard *self = KANBAN_CARD (object);

  if (property_id == 1)
    g_value_set_boolean (value, self->needs_saving);
  else
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void 
kanban_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec) 
{
  KanbanCard *self = KANBAN_CARD(object);

  if (property_id == 1)
    self->needs_saving = g_value_get_boolean(value);
  else
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
kanban_card_class_init (KanbanCardClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);


  gtk_widget_class_set_template_from_resource (widget_class, 
                "/com/github/zhrexl/kanban/kanban-card.ui");

  gtk_widget_class_bind_template_child (widget_class,
                                        KanbanCard, handlerDrag);
  gtk_widget_class_bind_template_child (widget_class,
                                        KanbanCard, LblCardName);
  gtk_widget_class_bind_template_child (widget_class,
                                        KanbanCard, revealercard);
  gtk_widget_class_bind_template_child (widget_class,
                                        KanbanCard, drop_revealer);
  gtk_widget_class_bind_template_child (widget_class,
                                        KanbanCard, description);
  gtk_widget_class_bind_template_child (widget_class,
                                        KanbanCard, BtnContent);

  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass),
                                           reveal_clicked);

  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass),
                                           delete_clicked);

  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass),
                                           insert_checkbox);

  GObjectClass *GClass = G_OBJECT_CLASS(klass);
  GClass->get_property = kanban_get_property;
  GClass->set_property = kanban_set_property;
  needs_saving = g_param_spec_boolean("needs-saving", "needsave",
                                      "Boolean value", 0,
                                      G_PARAM_READWRITE);
  g_object_class_install_property (GClass, 1, needs_saving);
}

static void
kanban_card_changed(GtkTextBuffer* buf, gpointer user_data)
{
    g_object_set (G_OBJECT(user_data), "needs-saving", 1, NULL);
    if (IsInitialized) {
      SaveNeeded = true;
    }
}

static void
kanban_card_title_changed(GtkEditableLabel* label, gpointer user_data)
{
  g_object_set(user_data, "needs-saving", 1, NULL);
  if (IsInitialized) {
    SaveNeeded = true;
  }
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
  drop_motion_enter (
    GtkDropControllerMotion* self,
    gdouble x,
    gdouble y,
    gpointer user_data
  )
{
  KanbanCard *card = KANBAN_CARD (user_data);
  gtk_revealer_set_reveal_child(card->drop_revealer, true);
}

static void
  drop_motion_leave (
    GtkDropControllerMotion* self,
    gdouble x,
    gdouble y,
    gpointer user_data
  )
{
  KanbanCard *card = KANBAN_CARD (user_data);
  gtk_revealer_set_reveal_child(card->drop_revealer, false);
}

static void
on_drag_begin (GtkDragSource *source,
               GdkDrag       *drag,
               GtkWidget      *self) {
  g_object_ref(self);
  GdkPaintable *paintable, *paintable2;

  paintable = gtk_widget_paintable_new (self);
  paintable2 = gdk_paintable_get_current_image(paintable);

  gtk_drag_source_set_icon (source, paintable2, 0, 0);
  g_object_unref (paintable);
  g_object_unref (paintable2);

  GtkWidget* old_box        = gtk_widget_get_parent (self);
  GtkWidget* old_view       = gtk_widget_get_parent(old_box);
  GtkWidget* old_scrollWnd  = gtk_widget_get_parent(old_view);
  KanbanColumn* old_col     = KANBAN_COLUMN (gtk_widget_get_parent (old_scrollWnd));

  kanban_column_remove_card (old_col, self);

}


static void
kanban_card_init (KanbanCard *self)
{
  GtkDragSource *source;
  GtkDropControllerMotion *motion = NULL;
  GtkTextBuffer* buf;

  gtk_widget_init_template (GTK_WIDGET (self));
  source = gtk_drag_source_new ();
  gtk_widget_add_controller (GTK_WIDGET (self->handlerDrag),
                             GTK_EVENT_CONTROLLER (source));

  gtk_drag_source_set_content(source, gdk_content_provider_new_typed (G_TYPE_POINTER, self));

  g_signal_connect (source, "drag-begin", G_CALLBACK (on_drag_begin), self);

  motion = GTK_DROP_CONTROLLER_MOTION(gtk_drop_controller_motion_new());
  gtk_widget_add_controller (GTK_WIDGET (self->handlerDrag),(GtkEventController*)motion);

  g_signal_connect(motion, "enter", G_CALLBACK (drop_motion_enter), self);
  g_signal_connect(motion, "leave", G_CALLBACK (drop_motion_leave), self);

  buf = gtk_text_view_get_buffer(self->description);
  
  self->description_changed = g_signal_connect (buf, "changed", 
                                                G_CALLBACK(kanban_card_changed), self);

  g_signal_connect(self->LblCardName, "changed", G_CALLBACK(kanban_card_title_changed), self);
  // Both the description and the title are separate, so we had to implement them independently
}
