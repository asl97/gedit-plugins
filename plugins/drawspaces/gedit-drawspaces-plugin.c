/*
 * Copyright (C) 2008 Ignacio Casal Quinteiro <nacho.resa@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-drawspaces-plugin.h"

#include <glib/gi18n-lib.h>
#include <gedit/gedit-debug.h>
#include <gedit/gedit-view.h>
#include <gedit/gedit-tab.h>
#include <gedit/gedit-window.h>
#include <gedit/gedit-window-activatable.h>
#include <gedit/gedit-utils.h>
#include <libpeasui/peas-ui-configurable.h>

#define DRAWSPACES_SETTINGS_BASE   "org.gnome.gedit.plugins.drawspaces"
#define SETTINGS_KEY_ENABLE        "enable"
#define SETTINGS_KEY_DRAW_TABS     "draw-tabs"
#define SETTINGS_KEY_DRAW_SPACES   "draw-spaces"
#define SETTINGS_KEY_DRAW_NEWLINE  "draw-newline"
#define SETTINGS_KEY_DRAW_NBSP     "draw-nbsp"
#define SETTINGS_KEY_DRAW_LEADING  "draw-leading"
#define SETTINGS_KEY_DRAW_TEXT     "draw-text"
#define SETTINGS_KEY_DRAW_TRAILING "draw-trailing"

#define UI_FILE "gedit-drawspaces-plugin.ui"

#define GEDIT_DRAWSPACES_PLUGIN_GET_PRIVATE(object) \
				(G_TYPE_INSTANCE_GET_PRIVATE ((object),	\
				GEDIT_TYPE_DRAWSPACES_PLUGIN,		\
				GeditDrawspacesPluginPrivate))

static void gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface);
static void peas_ui_configurable_iface_init (PeasUIConfigurableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (GeditDrawspacesPlugin,
				gedit_drawspaces_plugin,
				PEAS_TYPE_EXTENSION_BASE,
				0,
				G_IMPLEMENT_INTERFACE_DYNAMIC (GEDIT_TYPE_WINDOW_ACTIVATABLE,
							       gedit_window_activatable_iface_init)
				G_IMPLEMENT_INTERFACE_DYNAMIC (PEAS_UI_TYPE_CONFIGURABLE,
							       peas_ui_configurable_iface_init))

struct _GeditDrawspacesPluginPrivate
{
	GSettings *settings;

	GeditWindow *window;

	GtkSourceDrawSpacesFlags flags;

	GtkActionGroup *action_group;
	guint           ui_id;

	guint enable : 1;
};

typedef struct _DrawspacesConfigureWidget DrawspacesConfigureWidget;

struct _DrawspacesConfigureWidget
{
	GSettings *settings;

	GtkWidget *content;

	GtkWidget *draw_tabs;
	GtkWidget *draw_spaces;
	GtkWidget *draw_newline;
	GtkWidget *draw_nbsp;
	GtkWidget *draw_leading;
	GtkWidget *draw_text;
	GtkWidget *draw_trailing;
};

enum
{
	COLUMN_LABEL,
	COLUMN_LOCATION
};

static const gchar submenu [] = {
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='ViewMenu' action='View'>"
"      <separator />"
"      <menuitem name='DrawSpacesMenu' action='DrawSpaces'/>"
"    </menu>"
"  </menubar>"
"</ui>"
};

static void draw_spaces (GeditDrawspacesPlugin *plugin);

static void
on_active_toggled (GtkToggleAction       *action,
		   GeditDrawspacesPlugin *plugin)
{
	GeditDrawspacesPluginPrivate *priv;
	gboolean value;

	priv = plugin->priv;

	value = gtk_toggle_action_get_active (action);
	priv->enable = value;

	g_settings_set_boolean (priv->settings,
				SETTINGS_KEY_ENABLE, value);

	draw_spaces (plugin);
}

static const GtkToggleActionEntry action_entries[] =
{
	{ "DrawSpaces", NULL, N_("Show _White Space"), NULL,
	 N_("Show spaces and tabs"),
	 G_CALLBACK (on_active_toggled)},
};

static void
on_settings_changed (GSettings             *settings,
		     const gchar           *key,
		     GeditDrawspacesPlugin *plugin)
{
	gboolean value;

	value = g_settings_get_boolean (settings, key);

	if (strcmp (key, SETTINGS_KEY_DRAW_TABS) == 0)
	{
		if (value)
		{
			plugin->priv->flags |= GTK_SOURCE_DRAW_SPACES_TAB;
		}
		else
		{
			plugin->priv->flags &= ~GTK_SOURCE_DRAW_SPACES_TAB;
		}
	}
	else if (strcmp (key, SETTINGS_KEY_DRAW_SPACES) == 0)
	{
		if (value)
		{
			plugin->priv->flags |= GTK_SOURCE_DRAW_SPACES_SPACE;
		}
		else
		{
			plugin->priv->flags &= ~GTK_SOURCE_DRAW_SPACES_SPACE;
		}
	}
	else if (strcmp (key, SETTINGS_KEY_DRAW_NEWLINE) == 0)
	{
		if (value)
		{
			plugin->priv->flags |= GTK_SOURCE_DRAW_SPACES_NEWLINE;
		}
		else
		{
			plugin->priv->flags &= ~GTK_SOURCE_DRAW_SPACES_NEWLINE;
		}
	}
	else if (strcmp (key, SETTINGS_KEY_DRAW_NBSP) == 0)
	{
		if (value)
		{
			plugin->priv->flags |= GTK_SOURCE_DRAW_SPACES_NBSP;
		}
		else
		{
			plugin->priv->flags &= ~GTK_SOURCE_DRAW_SPACES_NBSP;
		}
	}
	else if (strcmp (key, SETTINGS_KEY_DRAW_LEADING) == 0)
	{
		if (value)
		{
			plugin->priv->flags |= GTK_SOURCE_DRAW_SPACES_LEADING;
		}
		else
		{
			plugin->priv->flags &= ~GTK_SOURCE_DRAW_SPACES_LEADING;
		}
	}
	else if (strcmp (key, SETTINGS_KEY_DRAW_TEXT) == 0)
	{
		if (value)
		{
			plugin->priv->flags |= GTK_SOURCE_DRAW_SPACES_TEXT;
		}
		else
		{
			plugin->priv->flags &= ~GTK_SOURCE_DRAW_SPACES_TEXT;
		}
	}
	else if (strcmp (key, SETTINGS_KEY_DRAW_TRAILING) == 0)
	{
		if (value)
		{
			plugin->priv->flags |= GTK_SOURCE_DRAW_SPACES_TRAILING;
		}
		else
		{
			plugin->priv->flags &= ~GTK_SOURCE_DRAW_SPACES_TRAILING;
		}
	}

	draw_spaces (plugin);
}

static void
gedit_drawspaces_plugin_init (GeditDrawspacesPlugin *plugin)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditDrawspacesPlugin initializing");

	plugin->priv = GEDIT_DRAWSPACES_PLUGIN_GET_PRIVATE (plugin);

	plugin->priv->settings = g_settings_new (DRAWSPACES_SETTINGS_BASE);

	g_signal_connect (plugin->priv->settings,
			  "changed",
			  G_CALLBACK (on_settings_changed),
			  plugin);
}

static void
gedit_drawspaces_plugin_dispose (GObject *object)
{
	GeditDrawspacesPlugin *plugin = GEDIT_DRAWSPACES_PLUGIN (object);

	gedit_debug_message (DEBUG_PLUGINS, "GeditDrawspacesPlugin disposing");

	if (plugin->priv->settings != NULL)
	{
		g_object_unref (plugin->priv->settings);
		plugin->priv->settings = NULL;
	}

	G_OBJECT_CLASS (gedit_drawspaces_plugin_parent_class)->dispose (object);
}

static void
draw_spaces (GeditDrawspacesPlugin *plugin)
{
	GeditDrawspacesPluginPrivate *priv;
	GList *views, *l;

	priv = plugin->priv;

	views = gedit_window_get_views (priv->window);
	for (l = views; l != NULL; l = g_list_next (l))
	{
		gtk_source_view_set_draw_spaces (GTK_SOURCE_VIEW (l->data),
						 priv->enable ? priv->flags : 0);
	}

	g_list_free (views);
}

static void
tab_added_cb (GeditWindow *window,
	      GeditTab *tab,
	      GeditDrawspacesPlugin *plugin)
{
	GeditView *view;

	if (plugin->priv->enable)
	{
		view = gedit_tab_get_view (tab);

		gtk_source_view_set_draw_spaces (GTK_SOURCE_VIEW (view),
						 plugin->priv->flags);
	}
}

static void
get_config_options (GeditDrawspacesPlugin *plugin)
{
	gboolean tabs, spaces, newline, nbsp, leading, text, trailing;
	GeditDrawspacesPluginPrivate *priv = plugin->priv;

	priv->enable = g_settings_get_boolean (priv->settings,
					       SETTINGS_KEY_ENABLE);

	tabs = g_settings_get_boolean (priv->settings,
				       SETTINGS_KEY_DRAW_TABS);

	spaces = g_settings_get_boolean (priv->settings,
					 SETTINGS_KEY_DRAW_SPACES);

	newline = g_settings_get_boolean (priv->settings,
					  SETTINGS_KEY_DRAW_NEWLINE);

	nbsp = g_settings_get_boolean (priv->settings,
				       SETTINGS_KEY_DRAW_NBSP);

	leading = g_settings_get_boolean (priv->settings,
					  SETTINGS_KEY_DRAW_LEADING);

	text = g_settings_get_boolean (priv->settings,
				       SETTINGS_KEY_DRAW_TEXT);

	trailing = g_settings_get_boolean (priv->settings,
					   SETTINGS_KEY_DRAW_TRAILING);

	if (tabs)
	{
		priv->flags |= GTK_SOURCE_DRAW_SPACES_TAB;
	}

	if (spaces)
	{
		priv->flags |= GTK_SOURCE_DRAW_SPACES_SPACE;
	}

	if (newline)
	{
		priv->flags |= GTK_SOURCE_DRAW_SPACES_NEWLINE;
	}

	if (nbsp)
	{
		priv->flags |= GTK_SOURCE_DRAW_SPACES_NBSP;
	}

	if (leading)
	{
		priv->flags |= GTK_SOURCE_DRAW_SPACES_LEADING;
	}

	if (text)
	{
		priv->flags |= GTK_SOURCE_DRAW_SPACES_TEXT;
	}

	if (trailing)
	{
		priv->flags |= GTK_SOURCE_DRAW_SPACES_TRAILING;
	}
}

static void
gedit_drawspaces_plugin_activate (GeditWindowActivatable *activatable,
				  GeditWindow            *window)
{
	GeditDrawspacesPluginPrivate *priv;
	GtkUIManager *manager;
	GtkAction *action;
	GError *error = NULL;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_DRAWSPACES_PLUGIN (activatable)->priv;
	priv->window = window;

	get_config_options (GEDIT_DRAWSPACES_PLUGIN (activatable));

	manager = gedit_window_get_ui_manager (window);

	priv->action_group = gtk_action_group_new ("GeditDrawspacesPluginActions");
	gtk_action_group_set_translation_domain (priv->action_group,
						 GETTEXT_PACKAGE);
	gtk_action_group_add_toggle_actions (priv->action_group,
					     action_entries,
					     G_N_ELEMENTS (action_entries),
					     activatable);

	/* Lets set the default value */
	action = gtk_action_group_get_action (priv->action_group,
					      "DrawSpaces");
	g_signal_handlers_block_by_func (action, on_active_toggled, activatable);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				      priv->enable);
	g_signal_handlers_unblock_by_func (action, on_active_toggled, activatable);

	gtk_ui_manager_insert_action_group (manager, priv->action_group, -1);

	priv->ui_id = gtk_ui_manager_add_ui_from_string (manager,
							 submenu,
							 -1,
							 &error);
	if (error)
	{
		g_warning ("%s", error->message);
		g_error_free (error);
	}

	if (priv->enable)
	{
		draw_spaces (GEDIT_DRAWSPACES_PLUGIN (activatable));
	}

	g_signal_connect (window, "tab-added",
			  G_CALLBACK (tab_added_cb), activatable);
}

static void
gedit_drawspaces_plugin_deactivate (GeditWindowActivatable *activatable,
				    GeditWindow            *window)
{
	GeditDrawspacesPluginPrivate *priv;
	GtkUIManager *manager;

	gedit_debug (DEBUG_PLUGINS);

	priv = GEDIT_DRAWSPACES_PLUGIN (activatable)->priv;

	manager = gedit_window_get_ui_manager (window);

	priv->enable = FALSE;
	draw_spaces (GEDIT_DRAWSPACES_PLUGIN (activatable));

	g_signal_handlers_disconnect_by_func (window, tab_added_cb, activatable);

	gtk_ui_manager_remove_ui (manager, priv->ui_id);
	gtk_ui_manager_remove_action_group (manager, priv->action_group);
}

static void
widget_destroyed (GtkObject *obj, gpointer widget_pointer)
{
	DrawspacesConfigureWidget *widget = (DrawspacesConfigureWidget *)widget_pointer;

	gedit_debug (DEBUG_PLUGINS);

	g_slice_free (DrawspacesConfigureWidget, widget_pointer);

	gedit_debug_message (DEBUG_PLUGINS, "END");
}

static DrawspacesConfigureWidget *
get_configuration_widget (GeditDrawspacesPlugin *plugin)
{
	DrawspacesConfigureWidget *widget = NULL;
	gboolean ret;
	GtkWidget *error_widget;
	gchar *datadir;
	gchar *filename;

	gchar *root_objects[] = {
		"content",
		NULL
	};

	widget = g_slice_new (DrawspacesConfigureWidget);

	datadir = peas_extension_base_get_data_dir (PEAS_EXTENSION_BASE (plugin));
	filename = g_build_filename (datadir, UI_FILE, NULL);

	ret = gedit_utils_get_ui_objects (filename,
					  root_objects,
					  &error_widget,
					  "content", &widget->content,
					  "check_button_draw_tabs", &widget->draw_tabs,
					  "check_button_draw_spaces", &widget->draw_spaces,
					  "check_button_draw_new_lines", &widget->draw_newline,
					  "check_button_draw_nbsp", &widget->draw_nbsp,
					  "check_button_draw_leading", &widget->draw_leading,
					  "check_button_draw_text", &widget->draw_text,
					  "check_button_draw_trailing", &widget->draw_trailing,
					  NULL);

	g_free (datadir);
	g_free (filename);

	if (!ret)
	{
		widget->content = error_widget;
		return widget;
	}

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget->draw_tabs),
				      plugin->priv->flags & GTK_SOURCE_DRAW_SPACES_TAB);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget->draw_spaces),
				      plugin->priv->flags & GTK_SOURCE_DRAW_SPACES_SPACE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget->draw_newline),
				      plugin->priv->flags & GTK_SOURCE_DRAW_SPACES_NEWLINE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget->draw_nbsp),
				      plugin->priv->flags & GTK_SOURCE_DRAW_SPACES_NBSP);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget->draw_leading),
				      plugin->priv->flags & GTK_SOURCE_DRAW_SPACES_LEADING);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget->draw_text),
				      plugin->priv->flags & GTK_SOURCE_DRAW_SPACES_TEXT);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget->draw_trailing),
				      plugin->priv->flags & GTK_SOURCE_DRAW_SPACES_TRAILING);

	g_settings_bind (plugin->priv->settings,
			 SETTINGS_KEY_DRAW_TABS,
			 widget->draw_tabs,
			 "active",
			 (G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET));
	g_settings_bind (plugin->priv->settings,
			 SETTINGS_KEY_DRAW_SPACES,
			 widget->draw_spaces,
			 "active",
			 (G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET));
	g_settings_bind (plugin->priv->settings,
			 SETTINGS_KEY_DRAW_NEWLINE,
			 widget->draw_newline,
			 "active",
			 (G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET));
	g_settings_bind (plugin->priv->settings,
			 SETTINGS_KEY_DRAW_NBSP,
			 widget->draw_nbsp,
			 "active",
			 (G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET));
	g_settings_bind (plugin->priv->settings,
			 SETTINGS_KEY_DRAW_LEADING,
			 widget->draw_leading,
			 "active",
			 (G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET));
	g_settings_bind (plugin->priv->settings,
			 SETTINGS_KEY_DRAW_TEXT,
			 widget->draw_text,
			 "active",
			 (G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET));
	g_settings_bind (plugin->priv->settings,
			 SETTINGS_KEY_DRAW_TRAILING,
			 widget->draw_trailing,
			 "active",
			 (G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET));

	g_signal_connect (widget->content, "destroy",
			  G_CALLBACK (widget_destroyed), widget);

	return widget;
}

static GtkWidget *
gedit_drawspaces_plugin_create_configure_widget (PeasUIConfigurable *configurable)
{
	DrawspacesConfigureWidget *widget;

	widget = get_configuration_widget (GEDIT_DRAWSPACES_PLUGIN (configurable));

	return widget->content;
}

static void
gedit_drawspaces_plugin_class_init (GeditDrawspacesPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (object_class, sizeof (GeditDrawspacesPluginPrivate));

	object_class->dispose = gedit_drawspaces_plugin_dispose;
}

static void
gedit_drawspaces_plugin_class_finalize (GeditDrawspacesPluginClass *klass)
{
}

static void
peas_ui_configurable_iface_init (PeasUIConfigurableInterface *iface)
{
	iface->create_configure_widget = gedit_drawspaces_plugin_create_configure_widget;
}

static void
gedit_window_activatable_iface_init (GeditWindowActivatableInterface *iface)
{
	iface->activate = gedit_drawspaces_plugin_activate;
	iface->deactivate = gedit_drawspaces_plugin_deactivate;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	gedit_drawspaces_plugin_register_type (G_TYPE_MODULE (module));

	peas_object_module_register_extension_type (module,
						    GEDIT_TYPE_WINDOW_ACTIVATABLE,
						    GEDIT_TYPE_DRAWSPACES_PLUGIN);
	peas_object_module_register_extension_type (module,
						    PEAS_UI_TYPE_CONFIGURABLE,
						    GEDIT_TYPE_DRAWSPACES_PLUGIN);
}

/* ex:set ts=8 noet: */
