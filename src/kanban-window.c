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

#include "kanban-window.h"

#include <glib/gi18n.h>

#include "kanban-application.h"
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
    GtkToggleButton     *EditBtn;
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
  KanbanWindow* wnd;
  gchar *data = NULL;
  gchar *file_path = NULL;
  gsize len;
  gboolean success = FALSE;

  g_return_val_if_fail(KANBAN_IS_WINDOW(user_data), FALSE);
  
  wnd = KANBAN_WINDOW(user_data);
  generator = json_generator_new();
  root = json_node_new(JSON_NODE_OBJECT);
  CardObject = json_object_new();

  for(GList* elem = wnd->ListOfColumns; elem; elem = elem->next) {
    item = elem->data;
    kanban_column_get_json(item, CardObject);
  }

  json_node_take_object(root, CardObject);
  json_generator_set_root(generator, root);
  g_object_set(generator, "pretty", FALSE, NULL);
  
  data = json_generator_to_data(generator, &len);
  file_path = g_build_filename(g_get_home_dir(), FileName, NULL);

  GError* error = NULL;
  if (!g_file_set_contents(file_path, data, -1, &error)) {
    gchar* msg = g_strdup_printf("Error saving file: %s\n", error->message);
    g_printerr("%s", msg);
    adw_toast_overlay_add_toast(wnd->toast_overlay, adw_toast_new(msg));
    g_free(msg);
    g_error_free(error);
    goto cleanup;
  }

  adw_toast_overlay_add_toast(wnd->toast_overlay, adw_toast_new("Saved"));
  gtk_widget_set_sensitive(GTK_WIDGET(wnd->save), FALSE);
  SaveNeeded = FALSE;
  success = TRUE;

cleanup:
  g_free(file_path);
  g_free(data);
  json_node_free(root);
  g_object_unref(generator);

  return success; 
}


static void
response(AdwDialog* self, const char* response, gpointer user_data)
{
  g_return_if_fail(ADW_IS_DIALOG(self));
  g_return_if_fail(response != NULL);
  g_return_if_fail(KANBAN_IS_WINDOW(user_data));

  if (g_strcmp0(response, "cancel") == 0)
    return;

  if (g_strcmp0(response, "save") == 0)
  {
    save_cards(user_data);
    SaveNeeded = FALSE;
  }

  GApplication* app = G_APPLICATION(gtk_window_get_application(GTK_WINDOW(user_data)));
  g_application_quit(app);
}



static gboolean
save_before_quit(KanbanWindow* self)
{
  g_return_val_if_fail(KANBAN_IS_WINDOW(self), FALSE);
  
  gboolean sensitive = gtk_widget_get_sensitive(GTK_WIDGET(self->save));

  if (!sensitive)
    return FALSE;

  AdwDialog *dialog;

  dialog = ADW_DIALOG(adw_alert_dialog_new(_("Save Changes?"),
                                         _("Open document contains unsaved changes. Changes which are not saved will be permanently lost.")));

  adw_alert_dialog_add_responses(ADW_ALERT_DIALOG(dialog),
                               "cancel", _("_Cancel"),
                               "discard", _("_Discard"),
                               "save", _("_Save & Quit"),
                               NULL);

  adw_alert_dialog_set_response_appearance(ADW_ALERT_DIALOG(dialog), "discard", ADW_RESPONSE_DESTRUCTIVE);
  adw_alert_dialog_set_response_appearance(ADW_ALERT_DIALOG(dialog), "save", ADW_RESPONSE_SUGGESTED);

  adw_alert_dialog_set_default_response(ADW_ALERT_DIALOG(dialog), "cancel");
  adw_alert_dialog_set_close_response(ADW_ALERT_DIALOG(dialog), "cancel");

  g_signal_connect(dialog, "response", G_CALLBACK(response), self);

  adw_dialog_present(dialog, GTK_WIDGET(self));

  return TRUE;
}

static void
remove_column(KanbanColumn* Column, gpointer user_data)
{
  KanbanWindow* Window = KANBAN_WINDOW (user_data);
  Window->ListOfColumns = g_list_remove (Window->ListOfColumns, Column);
  gtk_box_remove (Window->mainBox, GTK_WIDGET(Column));
  gtk_widget_set_sensitive (GTK_WIDGET (Window->save), true);
}

static gboolean
item_drag_drop (GtkDropTarget *dest,
                const GValue  *value,
                double         x,
                double         y){

  if (G_VALUE_TYPE (value) != G_TYPE_POINTER)
    return FALSE;

  GtkWidget* card = (GtkWidget*)g_value_get_pointer(value);

  KanbanColumn* col = KANBAN_COLUMN (gtk_event_controller_get_widget (
                                     GTK_EVENT_CONTROLLER (dest)));

  kanban_column_content_dropped(col);
  kanban_column_insert_card(col,y,card);
  g_object_unref (card);

  return TRUE;
}

GtkWidget*
create_column(KanbanWindow* Window, const gchar* title)
{
  g_return_val_if_fail(KANBAN_IS_WINDOW(Window), NULL);
  g_return_val_if_fail(title != NULL, NULL);

  KanbanColumn* column = kanban_column_new();
  if (!column) {
    g_warning("Failed to create new KanbanColumn");
    return NULL;
  }

  GtkDropTarget* target = gtk_drop_target_new(G_TYPE_POINTER, GDK_ACTION_COPY);
  if (!target) {
    g_warning("Failed to create drop target");
    g_object_unref(column);
    return NULL;
  }

  kanban_column_set_title(column, title);
  
  if (!g_signal_connect(target, "drop", G_CALLBACK(item_drag_drop), NULL)) {
    g_warning("Failed to connect drop signal");
    g_object_unref(target);
    g_object_unref(column);
    return NULL;
  }

  if (!g_signal_connect(column, "delete-column", G_CALLBACK(remove_column), Window)) {
    g_warning("Failed to connect delete-column signal");
    g_object_unref(target);
    g_object_unref(column);
    return NULL;
  }

  g_object_bind_property(column, "needs-saving", Window->save, "sensitive", G_BINDING_BIDIRECTIONAL);
  g_object_bind_property(column, "edit-mode", Window->EditBtn, "active", G_BINDING_BIDIRECTIONAL);
  g_object_set(column, "needs-saving", FALSE, NULL);

  gtk_widget_add_controller(GTK_WIDGET(column), GTK_EVENT_CONTROLLER(target));
  gtk_box_append(Window->mainBox, GTK_WIDGET(column));
  Window->ListOfColumns = g_list_append(Window->ListOfColumns, column);

  return GTK_WIDGET(column);
}

static void
kanban_window_class_init (KanbanWindowClass *klass)
{
GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/zhrexl/kanban/kanban-window.ui");
  gtk_widget_class_bind_template_child (widget_class, KanbanWindow, header_bar);
  gtk_widget_class_bind_template_child (widget_class, KanbanWindow, mainBox);
  gtk_widget_class_bind_template_child (widget_class, KanbanWindow, save);
  gtk_widget_class_bind_template_child (widget_class, KanbanWindow, EditBtn);
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
    gboolean revealed = json_object_get_boolean_member (objmember, "revealed");

    kanban_column_add_new_card (Column, objectname, description, revealed);
  }

  g_list_free(list);
}


static
int loadjson(KanbanWindow* self, const gchar* file_path)
{
  g_return_val_if_fail(KANBAN_IS_WINDOW(self), 1);
  g_return_val_if_fail(file_path != NULL, 1);

  JsonParser* parser = json_parser_new();
  if (!parser) {
    g_warning("Failed to create JSON parser");
    return 1;
  }

  if (!g_file_test(file_path, G_FILE_TEST_EXISTS)) {
    g_message("No JSON file was found! Creating a new one...");
    g_object_unref(parser);
    return 1;
  }

  GError* error = NULL;
  if (!json_parser_load_from_file(parser, file_path, &error)) {
    g_warning("Error parsing JSON: %s", error->message);
    g_error_free(error);
    g_object_unref(parser);
    return 1;
  }

  JsonNode* root = json_parser_get_root(parser);
  if (!root || json_node_get_node_type(root) != JSON_NODE_OBJECT) {
    g_warning("JSON root is not an object");
    g_object_unref(parser);
    return 1;
  }

  JsonObject* object = json_node_get_object(root);
  if (!object) {
    g_warning("Failed to get JSON object from root node");
    g_object_unref(parser);
    return 1;
  }

  GList* objects = json_object_get_members(object);
  if (!objects) {
    g_message("No columns found in JSON");
    g_object_unref(parser);
    return 1;
  }

  for (GList* child = objects; child != NULL; child = child->next) {
    const char* object_name = child->data;
    if (!object_name) {
      g_warning("Invalid column name in JSON");
      continue;
    }

    KanbanColumn* column = KANBAN_COLUMN(create_column(self, object_name));
    if (!column) {
      g_warning("Failed to create column: %s", object_name);
      continue;
    }

    JsonNode* node = json_object_get_member(object, object_name);
    if (node) {
      create_cards_from_json(column, node);
    }
  }

  g_list_free(objects);
  g_object_unref(parser);
  return 0;
}

static gboolean
load_ui(KanbanWindow* self)
{
  g_return_val_if_fail(KANBAN_IS_WINDOW(self), TRUE);

  static const char* Weekdays[] = {
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    NULL
  };

  /* Restore json */
  const gchar* home_dir = g_get_home_dir();
  if (!home_dir) {
    g_warning("Failed to get home directory");
    return TRUE;
  }

  gchar* file_path = g_build_filename(home_dir, FileName, NULL);
  if (!file_path) {
    g_warning("Failed to build file path");
    return TRUE;
  }

  if (loadjson(self, file_path)) {
    for (const char** day = Weekdays; *day != NULL; day++) {
      if (!create_column(self, *day)) {
        g_warning("Failed to create column for %s", *day);
      }
    }
  }

  g_free(file_path);
  IsInitialized = TRUE;
  return FALSE; /* Don't call again */
}

static void
kanban_window_init(KanbanWindow *self)
{
  g_return_if_fail(KANBAN_IS_WINDOW(self));

  gtk_widget_init_template(GTK_WIDGET(self));

  if (!g_idle_add((GSourceFunc)load_ui, self)) {
    g_warning("Failed to add load_ui to idle queue");
  }

  if (!g_signal_connect(self, "close-request", G_CALLBACK(save_before_quit), NULL)) {
    g_warning("Failed to connect close-request signal");
  }

  GSettings *settings = g_settings_new("io.github.zhrexl.thisweekinmylife");
  if (!settings) {
    g_warning("Failed to create settings");
    return;
  }

  g_settings_bind(settings, "width",
                  self, "default-width",
                  G_SETTINGS_BIND_DEFAULT);
  g_settings_bind(settings, "height",
                  self, "default-height",
                  G_SETTINGS_BIND_DEFAULT);
  g_settings_bind(settings, "is-maximized",
                  self, "maximized",
                  G_SETTINGS_BIND_DEFAULT);
  g_settings_bind(settings, "is-fullscreen",
                  self, "fullscreened",
                  G_SETTINGS_BIND_DEFAULT);

  g_object_unref(settings);
}
