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
#include "kanban-window.h"

static GParamSpec *needs_saving = NULL;

struct _KanbanCard
{
  GtkListBoxRow      parent_instance;

  GtkBox            *handlerDrag;
  GtkEditableLabel  *LblCardName;
  GtkRevealer       *revealercard;
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
  gtk_revealer_set_reveal_child (card->revealercard,revealed);

}

static GBytes*
kanban_card_get_full_description(GtkTextBuffer *buffer)
{
  GtkTextIter start, end;

  GByteArray* ret = g_byte_array_new();
  gtk_text_buffer_get_bounds(buffer, &start, &end);

  GtkTextIter pos0,pos1;
  GtkTextChildAnchor* anch;

  pos0 = start;
  do {
    pos1 = pos0;
    gboolean not_end = TRUE;
    while (anch = gtk_text_iter_get_child_anchor(&pos1), anch == NULL)
    {
      not_end = gtk_text_iter_forward_char(&pos1);
      if (!not_end)
        break;
    }

    gchar* text = gtk_text_iter_get_text(&pos0, &pos1);
    g_byte_array_append(ret, (guint8*)text, strlen(text));
    g_free(text);

    pos0 = pos1;

    if (!not_end && !anch)
      continue;

    guint widgets_len = 0;
    GtkWidget** widgets = gtk_text_child_anchor_get_widgets(anch, &widgets_len);

    if ((!widgets_len) || (!widgets)){
      continue;
    }

    for (int i = 0; i < widgets_len; i++)
    {
      gboolean isChecked = false;
      const gchar* tasklbl = NULL;

      for (GtkWidget* child = gtk_widget_get_first_child (widgets[i]);
                                                      child != NULL;
                        child = gtk_widget_get_next_sibling (child))
      {
        if (GTK_IS_CHECK_BUTTON (child))
          isChecked = gtk_check_button_get_active (GTK_CHECK_BUTTON (child));
        else if (GTK_IS_EDITABLE_LABEL (child))
          tasklbl = gtk_editable_get_text (GTK_EDITABLE(child));
      }

      g_byte_array_append(ret, (guint8*)checktemplate, strlen(checktemplate));

      if (isChecked)
         g_byte_array_append(ret, (guint8*)donexml, strlen(donexml));
      else
        g_byte_array_append(ret, (guint8*)progress, strlen(progress));

      g_byte_array_append(ret, (guint8*)titlexml, strlen(titlexml));

      int len = strlen(tasklbl);
      if (len)
        g_byte_array_append(ret, (guint8*)(tasklbl), len);

      g_byte_array_append(ret, (guint8*)endtitle, strlen(endtitle));
    }

  } while (gtk_text_iter_forward_char(&pos0));

  g_byte_array_append (ret, (guint8*)nullbyte, 1);

  return g_byte_array_free_to_bytes(ret);
}

/* the user must unref the returned pointer with g_bytes_unref() */
GBytes*
kanban_card_get_description(KanbanCard* Card)
{
  GtkTextBuffer* Buffer = gtk_text_view_get_buffer(Card->description);

  return kanban_card_get_full_description(Buffer);
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
  GtkTextBuffer*  buf = gtk_text_view_get_buffer(Card->description);
  GtkTextIter     end;
  
  int dsc_len = strlen(description);

  if (!dsc_len)
    return;

  gchar* dataptr = (gchar*)description;

  /* Block changed signal to avoid unecessary unsaved file flag */
  g_signal_handler_block(buf, Card->description_changed);

  /* TODO: it might not be necessary to use gtk_text_buffer_get_end_iter
  *  instead perhaps I should set the iter to offset 0 */

  gtk_text_buffer_set_text(buf, "", 0);
  gtk_text_buffer_get_end_iter(buf, &end);

  while (dsc_len > 0)
  {
    gchar* n_dataptr = (gchar*)(g_strstr_len (dataptr, dsc_len, checktemplate));

    if (n_dataptr == NULL)
    {
      gtk_text_buffer_insert(buf, &end, dataptr, dsc_len);
      break;
    }
    else
      gtk_text_buffer_insert(buf, &end, dataptr, n_dataptr-dataptr);

    // Create Anchor Child
    gtk_text_buffer_get_iter_at_offset (buf, &end, n_dataptr-description);

    n_dataptr   += sizeof(checktemplate)/sizeof(checktemplate[0]) -1;

    gchar* title = g_strstr_len(n_dataptr, strlen(n_dataptr), titlexml);

    if (title == NULL)
    {
      g_print("Corrupted json :(\n");
      break;
    }

    gchar* done     = g_strstr_len(n_dataptr,title-n_dataptr,"done");

    gboolean active = false;

    if (done)
      active  = true;
    else
      active  = false;

    title += sizeof(titlexml)/sizeof(titlexml[0]) - 1;
    gchar* next = g_strstr_len(title, strlen(title), endtitle);

    if (!next)
    {
      g_print("Corrupted json :(\n");
      break;
    }

    gchar* taskname = g_strndup (title, next-title);
    next    += sizeof(endtitle)/sizeof(endtitle[0]) - 1;
    dsc_len -= (next - dataptr);
    dataptr  = next;

    create_task (Card->description, &end, taskname, active);

    g_free(taskname);
  }
  g_signal_handler_unblock(buf, Card->description_changed);
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

void
kanban_card_set_css_provider(KanbanCard* card, GtkStyleProvider* CssProvider)
{
  gtk_style_context_add_provider (gtk_widget_get_style_context (GTK_WIDGET (card->description)), CssProvider, G_MAXUINT);
  gtk_style_context_add_provider (gtk_widget_get_style_context (GTK_WIDGET (card->LblCardName)), CssProvider, G_MAXUINT);
}

static void
delete_clicked(GtkButton* btn, gpointer user_data)
{
  GtkWidget *old_box        = gtk_widget_get_parent (GTK_WIDGET(user_data));
  GtkWidget *old_view       = gtk_widget_get_parent(old_box);
  GtkWidget *old_scrollWnd  = gtk_widget_get_parent(old_view);
  KanbanColumn *old_col     = KANBAN_COLUMN (gtk_widget_get_parent (old_scrollWnd));

  kanban_column_remove_card (old_col, user_data);
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

  gtk_widget_class_bind_template_child (widget_class, KanbanCard, handlerDrag);
  gtk_widget_class_bind_template_child (widget_class, KanbanCard, LblCardName);
  gtk_widget_class_bind_template_child (widget_class, KanbanCard, revealercard);
  gtk_widget_class_bind_template_child (widget_class, KanbanCard, description);
  gtk_widget_class_bind_template_child (widget_class, KanbanCard, BtnContent);

  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), reveal_clicked);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), delete_clicked);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), insert_checkbox);

  GObjectClass *GClass = G_OBJECT_CLASS(klass);
  GClass->get_property = kanban_get_property;
  GClass->set_property = kanban_set_property;
  needs_saving = g_param_spec_boolean("needs-saving", "needsave", "Boolean value", 0, G_PARAM_READWRITE);
  g_object_class_install_property (GClass, 1, needs_saving);
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
kanban_card_changed(GtkTextBuffer* buf, gpointer user_data)
{
  KanbanCard *self = KANBAN_CARD(user_data);
  
  if (!self->needs_saving)
    g_object_set (G_OBJECT(user_data), "needs-saving", 1, NULL);

}
static void
kanban_card_init (KanbanCard *self)
{
  GtkDragSource *source;
  GtkTextBuffer* buf;

  gtk_widget_init_template (GTK_WIDGET (self));
  source = gtk_drag_source_new ();
  gtk_widget_add_controller (GTK_WIDGET (self->handlerDrag), GTK_EVENT_CONTROLLER (source));

  g_signal_connect (source, "prepare", G_CALLBACK (css_drag_prepare), self);

  buf = gtk_text_view_get_buffer(self->description);
  self->description_changed = g_signal_connect (buf, "changed", 
                                                G_CALLBACK(kanban_card_changed), self);
}
