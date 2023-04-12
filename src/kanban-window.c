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

#include "kanban-window.h"
#include "kanban-column.h"
#include "json-glib/json-glib.h"

const gchar FileName[] = ".thisweekinmylife\0";

struct _KanbanWindow
{
    AdwApplicationWindow  parent_instance;

    AdwToastOverlay* toast_overlay;

    /* Template widgets */
    GtkHeaderBar        *header_bar;
    GtkBox              *mainBox;
    GtkButton           *save;
    GList               *ListOfColumns;
};

G_DEFINE_FINAL_TYPE (KanbanWindow, kanban_window, ADW_TYPE_APPLICATION_WINDOW)


gboolean
save_cards(gpointer user_data)
{
  JsonGenerator *generator;
  JsonNode *root;
  JsonObject *CardObject;

  KanbanColumn *item;
  KanbanWindow* wnd = KANBAN_WINDOW (user_data);

  gchar *data;
  gsize len;

  generator    = json_generator_new ();
  root         = json_node_new (JSON_NODE_OBJECT);
  CardObject   = json_object_new ();

  for(GList* elem = wnd->ListOfColumns; elem; elem = elem->next) {
    item = elem->data;
    kanban_column_get_json(item, CardObject);
  }

  json_node_take_object (root, CardObject);
  json_generator_set_root (generator, root);

  g_object_set (generator, "pretty", false, NULL);
  data = json_generator_to_data (generator, &len);

  //g_print("checking nested object %s",data);
  const gchar* home_dir = g_get_home_dir ();

  // TODO: file_path should be a global file
  gchar* file_path = g_strdup_printf ("%s/%s", home_dir, FileName);

  GError* error = NULL;
  g_file_set_contents (file_path, data, -1, &error);
  if (error != NULL) {
    gchar* msg = g_strdup_printf ("Error saving file: %s\n", error->message);
    g_printerr ("%s", msg);
    adw_toast_overlay_add_toast (wnd->toast_overlay, adw_toast_new (msg));
    g_free(msg);
    g_error_free (error);
    return 1;
  }

  adw_toast_overlay_add_toast (wnd->toast_overlay, adw_toast_new ("Saved"));

  gtk_widget_set_sensitive (GTK_WIDGET (wnd->save), false);
  g_free(file_path);
  g_free (data);

  json_node_free (root);
  g_object_unref (generator);

  return TRUE;
}
static void
response (AdwMessageDialog* self, gchar* response, gpointer user_data)
{
  if (strstr(response, "cancel"))
    return;

  if (strstr (response, "save"))
    save_cards (user_data);

  gtk_window_destroy (GTK_WINDOW (user_data));
}
static gboolean
save_before_quit(KanbanWindow* self)
{
  gboolean sensitive = gtk_widget_get_sensitive(GTK_WIDGET(self->save));

  if(!sensitive)
    return false;

  GtkWidget *dialog;

  dialog = adw_message_dialog_new (GTK_WINDOW(self), ("Save Changes?"), NULL);

  adw_message_dialog_format_body (ADW_MESSAGE_DIALOG (dialog),
                                ("Open document contains unsaved changes. Changes which are not saved will be permanently lost."));

  adw_message_dialog_add_responses (ADW_MESSAGE_DIALOG (dialog),
                                  "cancel",  ("_Cancel"),
                                  "discard",  ("_Discard"),
                                  "save", ("_Save & Quit"),
                                  NULL);

  adw_message_dialog_set_response_appearance (ADW_MESSAGE_DIALOG (dialog), "discard", ADW_RESPONSE_DESTRUCTIVE);
  adw_message_dialog_set_response_appearance (ADW_MESSAGE_DIALOG (dialog), "save", ADW_RESPONSE_SUGGESTED);

  adw_message_dialog_set_default_response (ADW_MESSAGE_DIALOG (dialog), "cancel");
  adw_message_dialog_set_close_response (ADW_MESSAGE_DIALOG (dialog), "cancel");

  g_signal_connect (dialog, "response", G_CALLBACK (response), self);

  gtk_window_present (GTK_WINDOW (dialog));

  return TRUE;
  //save_cards (self);
}
static gboolean
item_drag_drop (GtkDropTarget *dest,
                const GValue  *value,
                double         x,
                double         y){

  if (G_VALUE_TYPE (value) != G_TYPE_POINTER)
    return FALSE;

  GtkWidget* card = (GtkWidget*)g_value_get_pointer(value);

  g_object_ref(card);

  GtkWidget* old_box        = gtk_widget_get_parent (card);
  GtkWidget* old_view       = gtk_widget_get_parent(old_box);
  GtkWidget* old_scrollWnd  = gtk_widget_get_parent(old_view);
  KanbanColumn* old_col     = KANBAN_COLUMN (gtk_widget_get_parent (old_scrollWnd));

  kanban_column_remove_card (old_col, card);

  KanbanColumn* col = KANBAN_COLUMN (gtk_event_controller_get_widget (
                                     GTK_EVENT_CONTROLLER (dest)));
  kanban_column_add_card (col,card);
  g_object_unref (card);

  return TRUE;
}

static KanbanColumn*
create_column(KanbanWindow* Window, const gchar* title)
{
  KanbanColumn*   column      = kanban_column_new ();
  GtkDropTarget*  target      = gtk_drop_target_new (G_TYPE_POINTER,
                                                       GDK_ACTION_COPY);

  kanban_column_set_title (column, title);

  g_signal_connect (target, "drop", G_CALLBACK (item_drag_drop), NULL);
  g_object_bind_property (column, "needs-saving", Window->save, "sensitive", G_BINDING_BIDIRECTIONAL);
  g_object_set(column, "needs-saving", 0, NULL);

  gtk_widget_add_controller (GTK_WIDGET (column),
                               GTK_EVENT_CONTROLLER (target));

  return column;
}

static void
kanban_window_class_init (KanbanWindowClass *klass)
{
GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/zhrexl/kanban/kanban-window.ui");
  gtk_widget_class_bind_template_child (widget_class, KanbanWindow, header_bar);
  gtk_widget_class_bind_template_child (widget_class, KanbanWindow, mainBox);
  gtk_widget_class_bind_template_child (widget_class, KanbanWindow, save);
  gtk_widget_class_bind_template_child (widget_class, KanbanWindow, toast_overlay);

  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), save_cards);
}


static
void create_cards_from_json(KanbanColumn* Column, JsonNode* node)
{
  JsonObject* object = json_node_get_object (node);
  GList* list = json_object_get_members (object);

  for (GList* child = list; child != NULL; child = child->next)
  {
    const gchar*  objectname  = child->data;
    JsonNode*     member      = json_object_get_member (object, objectname);
    JsonObject*   objmember   = json_node_get_object (member);

    const gchar*  description = json_object_get_string_member (objmember,
                                                              "description");
    gboolean revealed = json_object_get_int_member (objmember, "revealed");

    kanban_column_add_new_card (Column, objectname, description, revealed);
  }

  g_list_free(list);
}


static
int loadjson (KanbanWindow* self, gchar* file_path)
{
  JsonParser* parser = json_parser_new ();
  GError* error = NULL;

  if(!g_file_test (file_path, G_FILE_TEST_EXISTS))
  {
    g_print("No Json was found! Creating a new one...\n");
    return 1;
  }
  if (!json_parser_load_from_file(parser, file_path, &error)) {
    g_printerr ("Error parsing JSON: %s\n", error->message);
    g_error_free (error);
    g_object_unref (parser);
    return 1;
  }

  JsonNode* root = json_parser_get_root (parser);
  if (json_node_get_node_type (root) != JSON_NODE_OBJECT) {
    g_printerr ("JSON root is not an object\n");
    g_object_unref (parser);
    return 1;
  }

  JsonObject* object = json_node_get_object (root);

  GList* objects = json_object_get_members (object);

  if(!g_list_length (objects))
    return 1;

  for (GList* child = objects; child != NULL; child = child->next)
  {
    const char*     object_name = child->data;
    KanbanColumn*   column      = create_column (self, object_name);

    gtk_box_append (self->mainBox, GTK_WIDGET (column));

    self->ListOfColumns = g_list_append (self->ListOfColumns, column);

    JsonNode* node = json_object_get_member (object, object_name);
    create_cards_from_json (column, node);
  }

  g_list_free (objects);
  g_object_unref (parser);
  return 0;
}

static gboolean
load_ui(KanbanWindow* self)
{
  const char* Weekdays[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};

  /* Restore json */
  const gchar* home_dir = g_get_home_dir ();
  gchar* file_path      = g_strdup_printf ("%s/%s",
                                           home_dir, FileName);

  if (loadjson (self, file_path))
  {
    for (int i = 0; i < 5; i++)
    {
      KanbanColumn* column  = create_column(self, Weekdays[i]);

      gtk_box_append (self->mainBox, GTK_WIDGET (column));

      self->ListOfColumns = g_list_append (self->ListOfColumns, column);
    }
  }

  g_free(file_path);

  return FALSE;
}

static void
kanban_window_init (KanbanWindow *self)
{

  gtk_widget_init_template (GTK_WIDGET (self));

  g_idle_add ((GSourceFunc)load_ui, self);

  g_signal_connect(self, "close-request", G_CALLBACK(save_before_quit), NULL);

}
