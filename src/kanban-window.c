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

struct _KanbanWindow
{
    AdwApplicationWindow  parent_instance;

    /* Template widgets */
    GtkHeaderBar        *header_bar;
    GtkBox              *mainBox;
    GList*              ListOfColumns;

    GtkStyleProvider    *provider;
};

G_DEFINE_FINAL_TYPE (KanbanWindow, kanban_window, ADW_TYPE_APPLICATION_WINDOW)

static void
apply_css (GtkWidget *widget, GtkStyleProvider *provider)
{
  GtkWidget *child;
  gtk_style_context_add_provider (gtk_widget_get_style_context (widget), provider, G_MAXUINT);
  for (child = gtk_widget_get_first_child (widget);
       child != NULL;
       child = gtk_widget_get_next_sibling (child))
    apply_css (child, provider);
}

static gboolean
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

  g_object_set (generator, "pretty", true, NULL);
  data = json_generator_to_data (generator, &len);

  //g_print("checking nested object %s",data);
  const gchar* home_dir = g_get_home_dir ();
  gchar* file_path = g_strdup_printf ("%s/.thisweekinmylife.json", home_dir);

  GError* error = NULL;
  g_file_set_contents (file_path, data, -1, &error);
  if (error != NULL) {
    g_printerr ("Error saving file: %s\n", error->message);
    g_error_free (error);
    return 1;
  }

  g_free(file_path);
  g_free (data);
  //g_free(home_dir);
  json_node_free (root);
  g_object_unref (generator);

  return TRUE;
}

static void
kanban_window_class_init (KanbanWindowClass *klass)
{
GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/zhrexl/kanban/kanban-window.ui");
  gtk_widget_class_bind_template_child (widget_class, KanbanWindow, header_bar);
  gtk_widget_class_bind_template_child (widget_class, KanbanWindow, mainBox);
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

  KanbanColumn* col = KANBAN_COLUMN (gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (dest)));
  kanban_column_add_card (col,card);
  g_object_unref (card);

  return TRUE;
}
static
void create_cards_from_json(KanbanColumn* Column, JsonNode* node)
{
  JsonObject* object = json_node_get_object (node);
  GList* list = json_object_get_members (object);

  for (GList* child = list; child != NULL; child = child->next)
  {
    const gchar* objectname = child->data;

    //g_print("%s\n", objectname);

    JsonNode* member = json_object_get_member (object, objectname);
    JsonObject* objmember = json_node_get_object (member);

    const gchar* description = json_object_get_string_member (objmember, "description");
    //g_print("%s\n", description);
    kanban_column_add_new_card (Column, objectname, description);
  }

  g_list_free(list);
}
static
int loadjson (KanbanWindow* self, gchar* file_path)
{
  JsonParser* parser = json_parser_new ();
  GError* error = NULL;
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


  for (GList* child = objects; child != NULL; child = child->next)
  {
    const char* object_name = child->data;
    KanbanColumn* column  = kanban_column_new ();
    GtkDropTarget* target = gtk_drop_target_new (G_TYPE_POINTER, GDK_ACTION_COPY);
    g_signal_connect (target, "drop", G_CALLBACK (item_drag_drop), NULL);

    kanban_column_set_title (column, object_name);
    kanban_column_set_provider(column, self->provider);
    gtk_box_append (self->mainBox, GTK_WIDGET (column));
    gtk_widget_add_controller (GTK_WIDGET (column), GTK_EVENT_CONTROLLER (target));

    self->ListOfColumns = g_list_append (self->ListOfColumns, column);

    JsonNode* node = json_object_get_member (object, object_name);
    create_cards_from_json (column, node);
  }

  g_list_free (objects);
  g_object_unref (parser);
  return 0;
}
static void
kanban_window_init (KanbanWindow *self)
{
  const char* Weekdays[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};

  gtk_widget_init_template (GTK_WIDGET (self));

  /* Load CSS */
  self->provider = GTK_STYLE_PROVIDER (gtk_css_provider_new ());
  gtk_css_provider_load_from_resource (GTK_CSS_PROVIDER (self->provider), "/com/github/zhrexl/kanban/stylesheet.css");

  /* Restore json */
  const gchar* home_dir = g_get_home_dir ();
  gchar* file_path = g_strdup_printf ("%s/.thisweekinmylife.json", home_dir);


  if (g_file_test (file_path, G_FILE_TEST_EXISTS)) {
    loadjson (self, file_path);
  }
  else
  {
    for (int i = 0; i < 5; i++)
    {
      KanbanColumn* column  = kanban_column_new ();
      GtkDropTarget* target = gtk_drop_target_new (G_TYPE_POINTER, GDK_ACTION_COPY);
      g_signal_connect (target, "drop", G_CALLBACK (item_drag_drop), NULL);

      kanban_column_set_title (column, Weekdays[i]);
      kanban_column_set_provider(column, self->provider);
      gtk_box_append (self->mainBox, GTK_WIDGET (column));
      gtk_widget_add_controller (GTK_WIDGET (column), GTK_EVENT_CONTROLLER (target));

      self->ListOfColumns = g_list_append (self->ListOfColumns, column);
    }
  }

  apply_css (GTK_WIDGET(self), self->provider);

  g_timeout_add (1000, (GSourceFunc)save_cards, self);

  g_free(file_path);
}
