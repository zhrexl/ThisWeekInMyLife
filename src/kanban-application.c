/* kanban-application.c
 *
 * Copyright 2023 Douglas
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

#include "kanban-application.h"
#include "kanban-window.h"

struct _KanbanApplication
{
	AdwApplication parent_instance;
};

G_DEFINE_TYPE (KanbanApplication, kanban_application, ADW_TYPE_APPLICATION)

KanbanApplication *
kanban_application_new (const char        *application_id,
                        GApplicationFlags  flags)
{
	g_return_val_if_fail (application_id != NULL, NULL);

	return g_object_new (KANBAN_TYPE_APPLICATION,
	                     "application-id", application_id,
	                     "flags", flags,
	                     NULL);
}

static void
kanban_application_activate (GApplication *app)
{
	GtkWindow *window;

	g_assert (KANBAN_IS_APPLICATION (app));

	window = gtk_application_get_active_window (GTK_APPLICATION (app));

	if (window == NULL)
		window = g_object_new (KANBAN_TYPE_WINDOW,
		                       "application", app,
		                       NULL);

        GtkCssProvider* provider = gtk_css_provider_new ();
        gtk_css_provider_load_from_resource (provider,
                                       "/com/github/zhrexl/kanban/stylesheet.css");

        GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (window));
        gtk_style_context_add_provider_for_display (display, GTK_STYLE_PROVIDER (provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

        g_object_unref (provider);


	gtk_window_present (window);
}

static void
kanban_application_class_init (KanbanApplicationClass *klass)
{
	GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

	app_class->activate = kanban_application_activate;
}

static void
kanban_application_about_action (GSimpleAction *action,
                                 GVariant      *parameter,
                                 gpointer       user_data)
{
	static const char *developers[] = {"zhrexl https://github.com/zhrexl/", NULL};
        static const char *artists[] = {"Brage Fuglseth", NULL};

	KanbanApplication *self = user_data;
	GtkWindow *window = NULL;

	g_assert (KANBAN_IS_APPLICATION (self));

	window = gtk_application_get_active_window (GTK_APPLICATION (self));

	adw_show_about_window (window,
	                       "application-name", "This Week in My Life",
	                       "application-icon", "io.github.zhrexl.thisweekinmylife",
	                       "developer-name", "zhrexl",
	                       "version", "0.0.3",
	                       "developers", developers,
                               "artists", artists,
	                       "copyright", "© 2023 zhrexl",
	                       NULL);
}

static void
kanban_application_quit_action (GSimpleAction *action,
                                GVariant      *parameter,
                                gpointer       user_data)
{
	KanbanApplication *self = user_data;

	g_assert (KANBAN_IS_APPLICATION (self));

	g_application_quit (G_APPLICATION (self));
}
static void
kanban_application_save_action (GSimpleAction *action,
                                GVariant      *parameter,
                                gpointer       user_data)
{
	KanbanApplication *self = user_data;

	g_assert (KANBAN_IS_APPLICATION (self));

        KanbanWindow* Window = KANBAN_WINDOW (gtk_application_get_active_window (GTK_APPLICATION (self)));

        save_cards (Window);
}
static void
kanban_application_new_action (GSimpleAction *action,
                                GVariant      *parameter,
                                gpointer       user_data)
{
	KanbanApplication *self = user_data;

	g_assert (KANBAN_IS_APPLICATION (self));

        KanbanWindow* Window = KANBAN_WINDOW (gtk_application_get_active_window (GTK_APPLICATION (self)));

        create_column (Window, "New Column");
}

static const GActionEntry app_actions[] = {
        { "new", kanban_application_new_action },
        { "save", kanban_application_save_action },
	{ "quit", kanban_application_quit_action },
	{ "about", kanban_application_about_action },
};

static void
kanban_application_init (KanbanApplication *self)
{
	g_action_map_add_action_entries (G_ACTION_MAP (self),
	                                 app_actions,
	                                 G_N_ELEMENTS (app_actions),
	                                 self);
	gtk_application_set_accels_for_action (GTK_APPLICATION (self),
	                                       "app.quit",
	                                       (const char *[]) { "<primary>q", NULL });
        gtk_application_set_accels_for_action (GTK_APPLICATION (self),
	                                       "app.save",
	                                       (const char *[]) { "<primary>s", NULL });
        gtk_application_set_accels_for_action (GTK_APPLICATION (self),
	                                       "app.new",
	                                       (const char *[]) { "<primary>n", NULL });
}
