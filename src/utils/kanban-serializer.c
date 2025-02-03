/* kanban-serializer.c
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

#include "kanban-serializer.h"

static gchar checktemplate[] = "<task status=";
static gchar donexml[] = "done";
static gchar progress[] = "progress";
static gchar titlexml[] = " title=\"";
static gchar endtitle[] = "\"/>";
static gchar nullbyte[] = "\0";

#define lenstr(X) sizeof(X) / sizeof(X[0]) - 1

/*
 * get_serialized_buffer returns a GByteArray with the serialized
 * content of GtkTextBuffer according to predefined template
 *
 * the user must unref the returned pointer with g_bytes_unref() */

GBytes *get_serialized_buffer(GtkTextBuffer *buffer) {
  GtkTextIter start, end;

  GByteArray *ret = g_byte_array_new();
  gtk_text_buffer_get_bounds(buffer, &start, &end);

  GtkTextIter pos0, pos1;
  GtkTextChildAnchor *anch;

  pos0 = start;
  do {
    pos1 = pos0;
    gboolean not_end = TRUE;
    while (anch = gtk_text_iter_get_child_anchor(&pos1), anch == NULL) {
      not_end = gtk_text_iter_forward_char(&pos1);
      if (!not_end)
        break;
    }

    gchar *text = gtk_text_iter_get_text(&pos0, &pos1);
    ret = g_byte_array_append(ret, (guint8 *)text, strlen(text));
    g_free(text);

    pos0 = pos1;

    if (!not_end && !anch)
      continue;

    guint widgets_len = 0;
    GtkWidget **widgets = gtk_text_child_anchor_get_widgets(anch, &widgets_len);

    if ((!widgets_len) || (!widgets))
      continue;

    for (int i = 0; i < widgets_len; i++) {
      gboolean isChecked = false;
      const gchar *tasklbl = NULL;

      for (GtkWidget *child = gtk_widget_get_first_child(widgets[i]);
           child != NULL; child = gtk_widget_get_next_sibling(child)) {
        if (GTK_IS_CHECK_BUTTON(child))
          isChecked = gtk_check_button_get_active(GTK_CHECK_BUTTON(child));
        else if (GTK_IS_EDITABLE_LABEL(child))
          tasklbl = gtk_editable_get_text(GTK_EDITABLE(child));
      }

      ret = g_byte_array_append(ret, (guint8 *)checktemplate,
                                lenstr(checktemplate));

      if (isChecked)
        ret = g_byte_array_append(ret, (guint8 *)donexml, lenstr(donexml));
      else
        ret = g_byte_array_append(ret, (guint8 *)progress, lenstr(progress));

      ret = g_byte_array_append(ret, (guint8 *)titlexml, lenstr(titlexml));

      int len = strlen(tasklbl);
      if (len)
        ret = g_byte_array_append(ret, (guint8 *)(tasklbl), len);

      ret = g_byte_array_append(ret, (guint8 *)endtitle, lenstr(endtitle));
    }

  } while (gtk_text_iter_forward_char(&pos0));

  ret = g_byte_array_append(ret, (guint8 *)nullbyte, 1);

  return g_byte_array_free_to_bytes(ret);
}

KanbanUnserializedContent *get_unserialized_buffer(const gchar *description) {
  int dsc_len = strlen(description);

  if (!dsc_len)
    return NULL;

  KanbanUnserializedContent *KUnContent =
      malloc(sizeof(KanbanUnserializedContent));

  GByteArray *ret = g_byte_array_new();
  GList *list = NULL;

  gchar *dataptr = (gchar *)description;

  while (dsc_len > 0) {
    gchar *n_dataptr = (gchar *)(g_strstr_len(dataptr, dsc_len, checktemplate));

    /* In case there's no task just copy everything and returns */
    if (n_dataptr == NULL) {
      ret = g_byte_array_append(ret, (guint8 *)dataptr, dsc_len);
      break;
    } else
      ret = g_byte_array_append(ret, (guint8 *)dataptr, n_dataptr - dataptr);

    // Create Anchor
    KanbanAnchor *anchor = malloc(sizeof(KanbanAnchor));
    anchor->offset = ret->len;

    n_dataptr += lenstr(checktemplate);
    gchar *title = g_strstr_len(n_dataptr, strlen(n_dataptr), titlexml);

    if (title == NULL) {
      free(anchor);
      g_print("Corrupted Filed :(\n");
      break;
    }

    gchar *done = g_strstr_len(n_dataptr, title - n_dataptr, "done");
    anchor->active = done ? true : false;

    title += lenstr(titlexml);
    gchar *next = g_strstr_len(title, strlen(title), endtitle);

    if (!next) {
      g_print("Corrupted json :(\n");
      break;
    }

    anchor->title = g_strndup(title, next - title);
    next += lenstr(endtitle);
    dsc_len -= (next - dataptr);
    dataptr = next;

    list = g_list_append(list, anchor);
  }
  KUnContent->text = ret;
  KUnContent->anchors = list;

  return KUnContent;
}
