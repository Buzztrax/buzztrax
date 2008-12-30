/* $Id$
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/**
 * SECTION:btmainmenu
 * @short_description: class for the editor main menu
 *
 * Provides the main application menu.
 */

/* @todo main-menu tasks
 *  - enable/disable edit menu entries based on selection and focus
 */

#define BT_EDIT
#define BT_MAIN_MENU_C

#include "bt-edit.h"

enum {
  MAIN_MENU_APP=1,
};


struct _BtMainMenuPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  G_POINTER_ALIAS(BtEditApplication *,app);

  /* MenuItems */
  GtkWidget *save_item;
  
  /* state */
  gboolean fullscreen;
};

static GtkMenuBarClass *parent_class=NULL;

//-- event handler

static void on_menu_new_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;

  g_assert(user_data);

  GST_INFO("menu new event occurred");
  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  bt_main_window_new_song(main_window);
  g_object_unref(main_window);
}

static void on_menu_open_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;

  g_assert(user_data);

  GST_INFO("menu open event occurred");
  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  bt_main_window_open_song(main_window);
  g_object_unref(main_window);
}

#if GTK_CHECK_VERSION(2,10,0)
static void on_menu_open_recent_activate(GtkRecentChooser *chooser,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  GtkRecentInfo *info;
  gchar *file_name;
  const gchar *uri;
  
  if(!(info=gtk_recent_chooser_get_current_item(chooser))) {
    GST_WARNING ("Unable to retrieve the current recent-item, aborting...");
    return;
  }
  uri=gtk_recent_info_get_uri(info);
  file_name=g_filename_from_uri(uri,NULL,NULL);
  
  GST_INFO("menu open event occurred : %s",file_name);

  if(!bt_edit_application_load_song(self->priv->app,file_name)) {
    BtMainWindow *main_window;
    gchar *msg=g_strdup_printf(_("An error occurred while loading the song from file '%s'"),file_name);
    g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
    
    bt_dialog_message(main_window,_("Can't load song"),_("Can't load song"),msg);
    g_free(msg);
    g_object_unref(main_window);
  }
  g_free(file_name);
}
#endif

static void on_menu_save_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;

  g_assert(user_data);

  GST_INFO("menu save event occurred");
  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  bt_main_window_save_song(main_window);
  g_object_unref(main_window);
}

static void on_menu_saveas_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;

  g_assert(user_data);

  GST_INFO("menu saveas event occurred");
  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  bt_main_window_save_song_as(main_window);
  g_object_unref(main_window);
}

static void on_menu_render_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  GtkWidget *settings,*progress;

  g_assert(user_data);

  GST_INFO("menu render event occurred");
  if((settings=GTK_WIDGET(bt_render_dialog_new(self->priv->app)))) {
    BtMainWindow *main_window;
    gint answer;

    g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
    gtk_window_set_transient_for(GTK_WINDOW(settings),GTK_WINDOW(main_window));
    gtk_widget_show_all(settings);
    answer=gtk_dialog_run(GTK_DIALOG(settings));
    if(answer==GTK_RESPONSE_ACCEPT) {
      gtk_widget_hide_all(settings);
      if((progress=GTK_WIDGET(bt_render_progress_new(self->priv->app,BT_RENDER_DIALOG(settings))))) {
        gtk_window_set_transient_for(GTK_WINDOW(progress),GTK_WINDOW(main_window));
        gtk_widget_show_all(progress);
        // run song rendering
        bt_render_progress_run(BT_RENDER_PROGRESS(progress));
        gtk_widget_destroy(progress);
      }
    }
    gtk_widget_destroy(settings);
    g_object_unref(main_window);
  }
}

static void on_menu_quit_activate(GtkMenuItem *menuitem,gpointer user_data) {
  gboolean cont;
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;
  
  // @todo: this should have the code from main-window.c:on_window_delete_event
  // and on_window_delete_event() should trigger this

  g_assert(user_data);

  GST_INFO("menu quit event occurred");
  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  g_signal_emit_by_name(G_OBJECT(main_window),"delete_event",(gpointer)main_window,&cont);
  g_object_unref(main_window);
  GST_INFO("  result = %d",cont);
  if(!cont) {
    gtk_widget_destroy(GTK_WIDGET(main_window));
  }
}

static void on_menu_cut_activate(GtkMenuItem *menuitem,gpointer user_data) {
  //BtMainMenu *self=BT_MAIN_MENU(user_data);

  g_assert(user_data);

  GST_INFO("menu cut event occurred");
  /* @todo implement me */
}

static void on_menu_copy_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;
  BtMainPages *pages;

  g_assert(user_data);

  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);

  switch(gtk_notebook_get_current_page(GTK_NOTEBOOK(pages))) {
    case BT_MAIN_PAGES_MACHINES_PAGE: {
      GST_INFO("menu copy event occurred for machine page");
    } break;
    case BT_MAIN_PAGES_PATTERNS_PAGE: {
      BtMainPagePatterns *page;
      GST_INFO("menu copy event occurred for pattern page");
      g_object_get(G_OBJECT(pages),"patterns-page",&page,NULL);
      bt_main_page_pattern_copy_selection(page);
      g_object_unref(page);
    } break;
    case BT_MAIN_PAGES_SEQUENCE_PAGE: {
      BtMainPageSequence *page;
      GST_INFO("menu copy event occurred for sequence page");
      g_object_get(G_OBJECT(pages),"sequence-page",&page,NULL);
      bt_main_page_sequence_copy_selection(page);
      g_object_unref(page);
    } break;
    case BT_MAIN_PAGES_WAVES_PAGE: {
      GST_INFO("menu copy event occurred for waves page");
    } break;
    case BT_MAIN_PAGES_INFO_PAGE: {
      GST_INFO("menu copy event occurred for info page");
    } break;
  }

  g_object_unref(pages);
  g_object_unref(main_window);
}

static void on_menu_paste_activate(GtkMenuItem *menuitem,gpointer user_data) {
  //BtMainMenu *self=BT_MAIN_MENU(user_data);

  g_assert(user_data);

  GST_INFO("menu paste event occurred");
  /* @todo implement me */
}

static void on_menu_delete_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;
  BtMainPages *pages;

  g_assert(user_data);

  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);

  switch(gtk_notebook_get_current_page(GTK_NOTEBOOK(pages))) {
    case BT_MAIN_PAGES_MACHINES_PAGE: {
      GST_INFO("menu delete event occurred for machine page");
    } break;
    case BT_MAIN_PAGES_PATTERNS_PAGE: {
      GST_INFO("menu delete event occurred for pattern page");
    } break;
    case BT_MAIN_PAGES_SEQUENCE_PAGE: {
      BtMainPageSequence *sequence_page;
      GST_INFO("menu delete event occurred for sequence page");
      g_object_get(G_OBJECT(pages),"sequence-page",&sequence_page,NULL);
      bt_main_page_sequence_delete_selection(sequence_page);
      g_object_unref(sequence_page);
    } break;
    case BT_MAIN_PAGES_WAVES_PAGE: {
      GST_INFO("menu delete event occurred for waves page");
    } break;
    case BT_MAIN_PAGES_INFO_PAGE: {
      GST_INFO("menu delete event occurred for info page");
    } break;
  }

  g_object_unref(pages);
  g_object_unref(main_window);
}

static void on_menu_settings_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  GtkWidget *dialog;

  g_assert(user_data);

  GST_INFO("menu settings event occurred");
  if((dialog=GTK_WIDGET(bt_settings_dialog_new(self->priv->app)))) {
    BtMainWindow *main_window;

    g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
    gtk_window_set_transient_for(GTK_WINDOW(dialog),GTK_WINDOW(main_window));
    g_object_unref(main_window);
    gtk_widget_show_all(dialog);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
  }
}

static void on_menu_view_toolbar_toggled(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;
  BtMainToolbar *toolbar;
  BtSettings *settings;
  gboolean shown;

  g_assert(user_data);

  GST_INFO("menu 'view toolbar' event occurred");
  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,"settings",&settings,NULL);
  g_object_get(G_OBJECT(main_window),"toolbar",&toolbar,NULL);

  shown=gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem));
  g_object_set(G_OBJECT(toolbar),"visible",shown,NULL);
  g_object_set(G_OBJECT(settings),"toolbar-hide",!shown,NULL);

  g_object_unref(toolbar);
  g_object_unref(settings);
  g_object_unref(main_window);
}

static void on_menu_view_statusbar_toggled(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;
  BtMainStatusbar *statusbar;
  BtSettings *settings;
  gboolean shown;

  g_assert(user_data);

  GST_INFO("menu 'view toolbar' event occurred");
  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,"settings",&settings,NULL);
  g_object_get(G_OBJECT(main_window),"statusbar",&statusbar,NULL);

  shown=gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem));
  g_object_set(G_OBJECT(statusbar),"visible",shown,NULL);
  g_object_set(G_OBJECT(settings),"statusbar-hide",!shown,NULL);

  g_object_unref(statusbar);
  g_object_unref(settings);
  g_object_unref(main_window);
}

static void on_menu_view_tabs_toggled(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;
  BtMainPages *pages;
  BtSettings *settings;
  gboolean shown;

  g_assert(user_data);

  GST_INFO("menu 'view tabs' event occurred");
  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,"settings",&settings,NULL);
  g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);

  shown=gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem));
  g_object_set(G_OBJECT(pages),"show-tabs",shown,NULL);
  g_object_set(G_OBJECT(settings),"tabs-hide",!shown,NULL);

  g_object_unref(pages);
  g_object_unref(settings);
  g_object_unref(main_window);
}

#if GTK_CHECK_VERSION(2,8,0)
static void on_menu_fullscreen_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;

  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  /* @idea: reflow things a bit for full-screen:
   * - hide menu bar and have a menu-button on toolbar
   * - have a right justified label on toolbar to show window title
   */
  if(!self->priv->fullscreen) {
    gtk_window_fullscreen(GTK_WINDOW(main_window));
    self->priv->fullscreen=TRUE;
  }
  else {
    gtk_window_unfullscreen(GTK_WINDOW(main_window));
    self->priv->fullscreen=FALSE;
  }
  g_object_unref(main_window);
}
#endif

static void on_menu_goto_machine_view_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;
  BtMainPages *pages;

  g_assert(user_data);

  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);

  gtk_notebook_set_current_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_MACHINES_PAGE);

  g_object_unref(pages);
  g_object_unref(main_window);
}

static void on_menu_goto_pattern_view_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;
  BtMainPages *pages;

  g_assert(user_data);

  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);

  gtk_notebook_set_current_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_PATTERNS_PAGE);

  g_object_unref(pages);
  g_object_unref(main_window);
}

static void on_menu_goto_sequence_view_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;
  BtMainPages *pages;

  g_assert(user_data);

  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);

  gtk_notebook_set_current_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_SEQUENCE_PAGE);

  g_object_unref(pages);
  g_object_unref(main_window);
}

static void on_menu_goto_waves_view_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;
  BtMainPages *pages;

  g_assert(user_data);

  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);

  gtk_notebook_set_current_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_WAVES_PAGE);

  g_object_unref(pages);
  g_object_unref(main_window);
}

static void on_menu_goto_info_view_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;
  BtMainPages *pages;

  g_assert(user_data);

  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);

  gtk_notebook_set_current_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_INFO_PAGE);

  g_object_unref(pages);
  g_object_unref(main_window);
}

static void on_menu_play_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtSong *song;

  // get song from app and start playback
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  if(!bt_song_play(song)) {
    GST_WARNING("failed to play");
  }
  // release the reference
  g_object_unref(song);
}

static void on_menu_stop_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtSong *song;

  // get song from app and stop playback
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  if(!bt_song_stop(song)) {
    GST_WARNING("failed to stop");
  }
  // release the reference
  g_object_unref(song);
}

static void on_menu_help_activate(GtkMenuItem *menuitem,gpointer user_data) {
  //BtMainMenu *self=BT_MAIN_MENU(user_data);
#if GTK_CHECK_VERSION(2,14,0)
  GError *error=NULL;
#endif

  //g_assert(user_data);

  GST_INFO("menu help event occurred");
  
  // use "ghelp:buzztard-edit?topic" for context specific help
#if GTK_CHECK_VERSION(2,14,0)
  if(!gtk_show_uri(gtk_widget_get_screen(GTK_WIDGET(menuitem)),"ghelp:buzztard-edit",gtk_get_current_event_time(),&error)) {
    GST_WARNING("Failed to display help: %s\n",error->message);
    g_error_free(error);
  }
#else
  gnome_vfs_url_show("ghelp:buzztard-edit");
#endif
}

static void on_menu_about_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);

  g_assert(user_data);

  bt_edit_application_show_about(self->priv->app);
}

#ifdef USE_DEBUG
static void on_menu_debug_dump_pipeline_graph_and_show(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  const gchar *path;

  if((path=g_getenv("GST_DEBUG_DUMP_DOT_DIR"))) {
    BtSong *song;
    GstBin *bin;
    gchar *cmd;
    GError *error=NULL;

    g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
    g_object_get(G_OBJECT(song),"bin",&bin,NULL);
    
    GST_DEBUG_BIN_TO_DOT_FILE(bin,
      /*GST_DEBUG_GRAPH_SHOW_ALL,*/
      GST_DEBUG_GRAPH_SHOW_CAPS_DETAILS|GST_DEBUG_GRAPH_SHOW_STATES,
      PACKAGE_NAME);
  
    // release the reference
    gst_object_unref(bin);
    g_object_unref(song);
  
    // convert file
    cmd=g_strdup_printf("dot -Tpng -o%s"G_DIR_SEPARATOR_S""PACKAGE_NAME".png %s"G_DIR_SEPARATOR_S""PACKAGE_NAME".dot",path,path); 
    if(!g_spawn_command_line_sync(cmd,NULL,NULL,NULL,&error)) {
        GST_WARNING("Failed to convert dot-graph: %s\n",error->message);
        g_error_free(error);
    }
    else {
      gchar *png_uri;
      
      png_uri=g_strdup_printf("file://%s"G_DIR_SEPARATOR_S""PACKAGE_NAME".png",path);
      // show image
#if GTK_CHECK_VERSION(2,14,0)
      if(!gtk_show_uri(gtk_widget_get_screen(GTK_WIDGET(menuitem)),png_uri,gtk_get_current_event_time(),&error)) {
        GST_WARNING("Failed to display dot-graph: %s\n",error->message);
        g_error_free(error);
      }
#else
      gnome_vfs_url_show(png_uri);
#endif
      g_free(png_uri);
    }    
    g_free(cmd);
  }
  else {
    // the envvar is only checked at gst_init()
    GST_WARNING("You need to start the app with GST_DEBUG_DUMP_DOT_DIR env-var set.");
  }
}
#endif


static void on_song_unsaved_changed(const BtSong *song,GParamSpec *arg,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  gboolean unsaved;

  g_assert(user_data);

  g_object_get(G_OBJECT(song),"unsaved",&unsaved,NULL);
  gtk_widget_set_sensitive(self->priv->save_item,unsaved);

  GST_INFO("song.unsaved has changed : song=%p, menu=%p, unsaved=%d",song,user_data,unsaved);
}

static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtSong *song;

  g_assert(user_data);

  GST_INFO("song has changed : app=%p, toolbar=%p",app,user_data);

  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  if(!song) return;

  on_song_unsaved_changed(song,NULL,self);
  g_signal_connect(G_OBJECT(song), "notify::unsaved", G_CALLBACK(on_song_unsaved_changed), (gpointer)self);
  g_object_try_unref(song);
}

//-- helper methods

static gboolean bt_main_menu_init_ui(const BtMainMenu *self) {
  GtkWidget *item,*menu,*subitem;
  BtSettings *settings;
  gboolean toolbar_hide,statusbar_hide,tabs_hide;
  GtkAccelGroup *accel_group=bt_ui_resources_get_accel_group();

  gtk_widget_set_name(GTK_WIDGET(self),"main menu");
  g_object_get(G_OBJECT(self->priv->app),"settings",&settings,NULL);
  g_object_get(G_OBJECT(settings),
    "toolbar-hide",&toolbar_hide,
    "statusbar-hide",&statusbar_hide,
    "tabs-hide",&tabs_hide,
    NULL);
  g_object_unref(settings);
  //gtk_menu_set_accel_path(GTK_MENU(self),"<Buzztard-Main>/MainMenu");

  //-- file menu
  item=gtk_menu_item_new_with_mnemonic(_("_File"));
  gtk_container_add(GTK_CONTAINER(self),item);

  menu=gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item),menu);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_NEW,accel_group);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_new_activate),(gpointer)self);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN,accel_group);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_open_activate),(gpointer)self);

#if GTK_CHECK_VERSION(2,10,0)
  subitem = gtk_menu_item_new_with_mnemonic (_("_Recently used"));
  gtk_container_add(GTK_CONTAINER(menu),subitem);

  item=gtk_recent_chooser_menu_new_for_manager(gtk_recent_manager_get_default());
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(subitem),item);
  g_signal_connect (item, "item-activated", G_CALLBACK (on_menu_open_recent_activate), (gpointer)self);
  
  {
    GtkRecentFilter *filter=gtk_recent_filter_new();
    const GList *plugins, *node;
    BtSongIOModuleInfo *info;
    guint ix;

    //gtk_recent_filter_add_application (filter, "buzztard-edit");

    // set filters
    plugins=bt_song_io_get_module_info_list();
    for(node=plugins;node;node=g_list_next(node)) {
      info=(BtSongIOModuleInfo *)node->data;
      ix=0;
      while(info->formats[ix].name) {
        gtk_recent_filter_add_mime_type(filter,info->formats[ix].mime_type);
        ix++;
      }
    }
    /* workaround for http://bugzilla.gnome.org/show_bug.cgi?id=541236 */
    gtk_recent_filter_add_pattern(filter,"*.xml");
    gtk_recent_chooser_add_filter(GTK_RECENT_CHOOSER(item),filter);
    gtk_recent_chooser_set_filter(GTK_RECENT_CHOOSER(item),filter);
  }
#endif
  
  subitem=gtk_separator_menu_item_new();
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  gtk_widget_set_sensitive(subitem,FALSE);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_SAVE,accel_group);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_save_activate),(gpointer)self);
  self->priv->save_item=subitem;

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_SAVE_AS,accel_group);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_saveas_activate),(gpointer)self);

  subitem=gtk_separator_menu_item_new();
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  gtk_widget_set_sensitive(subitem,FALSE);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_RECORD,accel_group);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_render_activate),(gpointer)self);

  subitem=gtk_separator_menu_item_new();
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  gtk_widget_set_sensitive(subitem,FALSE);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT,accel_group);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_quit_activate),(gpointer)self);

  // edit menu
  item=gtk_menu_item_new_with_mnemonic(_("_Edit"));
  gtk_container_add(GTK_CONTAINER(self),item);

  menu=gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item),menu);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_CUT,accel_group);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_cut_activate),(gpointer)self);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_COPY,accel_group);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_copy_activate),(gpointer)self);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_PASTE,accel_group);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_paste_activate),(gpointer)self);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE,accel_group);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_delete_activate),(gpointer)self);

  subitem=gtk_separator_menu_item_new();
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  gtk_widget_set_sensitive(subitem,FALSE);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES,accel_group);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_settings_activate),(gpointer)self);

  // view menu
  item=gtk_menu_item_new_with_mnemonic(_("_View"));
  gtk_container_add(GTK_CONTAINER(self),item);

  menu=gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item),menu);
  gtk_menu_set_accel_group(GTK_MENU(menu), accel_group);
  gtk_menu_set_accel_path(GTK_MENU(menu),"<Buzztard-Main>/MainMenu/View");

  subitem=gtk_check_menu_item_new_with_mnemonic(_("Toolbar"));
  // from here we can't hide the toolbar as it is not yet created and shown
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(subitem),!toolbar_hide);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"toggled",G_CALLBACK(on_menu_view_toolbar_toggled),(gpointer)self);

  subitem=gtk_check_menu_item_new_with_mnemonic(_("Statusbar"));
  // from here we can't hide the statusbar as it is not yet created and shown
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(subitem),!statusbar_hide);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"toggled",G_CALLBACK(on_menu_view_statusbar_toggled),(gpointer)self);

  subitem=gtk_check_menu_item_new_with_mnemonic(_("Tabs"));
  // from here we can't hide the tabs as they are not yet created and shown
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(subitem),!tabs_hide);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"toggled",G_CALLBACK(on_menu_view_tabs_toggled),(gpointer)self);

  /* @todo 'Machine properties' show/hide toggle */
  /* @todo 'Analyzer windows' show/hide toggle */
  
#if GTK_CHECK_VERSION(2,8,0)
  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_FULLSCREEN,accel_group);
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (subitem), "<Buzztard-Main>/MainMenu/View/FullScreen");
  gtk_accel_map_add_entry ("<Buzztard-Main>/MainMenu/View/FullScreen", GDK_F11, 0);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_fullscreen_activate),(gpointer)self);
#endif
  
  subitem=gtk_separator_menu_item_new();
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  gtk_widget_set_sensitive(subitem,FALSE);

  subitem=gtk_image_menu_item_new_with_mnemonic(_("Go to machine view"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(subitem),gtk_image_new_from_icon_name("tab_machines",GTK_ICON_SIZE_MENU));
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (subitem), "<Buzztard-Main>/MainMenu/View/MachineView");
  gtk_accel_map_add_entry ("<Buzztard-Main>/MainMenu/View/MachineView", GDK_F3, 0);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_goto_machine_view_activate),(gpointer)self);

  subitem=gtk_image_menu_item_new_with_mnemonic(_("Go to pattern view"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(subitem),gtk_image_new_from_icon_name("tab_patterns",GTK_ICON_SIZE_MENU));
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (subitem), "<Buzztard-Main>/MainMenu/View/PatternView");
  gtk_accel_map_add_entry ("<Buzztard-Main>/MainMenu/View/PatternView", GDK_F2, 0);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_goto_pattern_view_activate),(gpointer)self);

  subitem=gtk_image_menu_item_new_with_mnemonic(_("Go to sequence view"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(subitem),gtk_image_new_from_icon_name("tab_sequence",GTK_ICON_SIZE_MENU));
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (subitem), "<Buzztard-Main>/MainMenu/View/SequenceView");
  gtk_accel_map_add_entry ("<Buzztard-Main>/MainMenu/View/SequenceView", GDK_F4, 0);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_goto_sequence_view_activate),(gpointer)self);

  subitem=gtk_image_menu_item_new_with_mnemonic(_("Go to wave table view"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(subitem),gtk_image_new_from_icon_name("tab_waves",GTK_ICON_SIZE_MENU));
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (subitem), "<Buzztard-Main>/MainMenu/View/WaveteableView");
  gtk_accel_map_add_entry ("<Buzztard-Main>/MainMenu/View/WaveteableView", GDK_F9, 0);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_goto_waves_view_activate),(gpointer)self);

  subitem=gtk_image_menu_item_new_with_mnemonic(_("Go to song information"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(subitem),gtk_image_new_from_icon_name("tab_info",GTK_ICON_SIZE_MENU));
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (subitem), "<Buzztard-Main>/MainMenu/View/InfoView");
  gtk_accel_map_add_entry ("<Buzztard-Main>/MainMenu/View/InfoView", GDK_F10, 0);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_goto_info_view_activate),(gpointer)self);

  /* @todo zoom menu items
   *  machine view:  zoom-in/zoom-out/zoom-fit
   *  sequence vide: zoom-in/zoom-out
   *
   
  subitem=gtk_separator_menu_item_new();
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  gtk_widget_set_sensitive(subitem,FALSE);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_ZOOM_FIT,accel_group);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_zoom_fit_activate),(gpointer)self);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_ZOOM_IN,accel_group);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_zoom_in_activate),(gpointer)self);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_ZOOM_OUT,accel_group);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_zoom_out_activate),(gpointer)self);
  */
  
  // playback menu
  item=gtk_menu_item_new_with_mnemonic(_("_Playback"));
  gtk_container_add(GTK_CONTAINER(self),item);

  menu=gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item),menu);
  gtk_menu_set_accel_group(GTK_MENU(menu), accel_group);
  gtk_menu_set_accel_path(GTK_MENU(menu),"<Buzztard-Main>/MainMenu/Playback");

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PLAY,accel_group);
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (subitem), "<Buzztard-Main>/MainMenu/Playback/Play");
  gtk_accel_map_add_entry ("<Buzztard-Main>/MainMenu/Playback/Play", GDK_F5, 0);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_play_activate),(gpointer)self);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_STOP,accel_group);
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (subitem), "<Buzztard-Main>/MainMenu/Playback/Stop");
  gtk_accel_map_add_entry ("<Buzztard-Main>/MainMenu/Playback/Stop", GDK_F8, 0);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_stop_activate),(gpointer)self);

  /* @todo toggle loop item */
  
  // help menu
  item=gtk_menu_item_new_with_mnemonic(_("_Help"));
  gtk_container_add(GTK_CONTAINER(self),item);

  menu=gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item),menu);
  gtk_menu_set_accel_group(GTK_MENU(menu), accel_group);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_HELP,accel_group);
  gtk_widget_remove_accelerator(subitem,accel_group,'h',GDK_CONTROL_MASK);
  gtk_widget_add_accelerator(subitem,"activate",accel_group,GDK_F1,0,GTK_ACCEL_VISIBLE);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_help_activate),(gpointer)self);

  /* @todo 'tip of the day' */
  /* @todo 'translate application' -> link to translator project
   * liblaunchpad-integration1:/usr/share/icons/hicolor/16x16/apps/lpi-translate.png
   */
  /* @todo 'report a problem' -> link to sf.net bug tracker
   * liblaunchpad-integration1:/usr/share/icons/hicolor/16x16/apps/lpi-bug.png
   */

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT,accel_group);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_about_activate),(gpointer)self);

#ifdef USE_DEBUG
  // debug menu
  item=gtk_menu_item_new_with_mnemonic(("_Debug"));
  gtk_menu_item_set_right_justified(GTK_MENU_ITEM(item),TRUE);
  gtk_container_add(GTK_CONTAINER(self),item);

  menu=gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item),menu);
  
  /* change to "pipeline graph" and have a submenu with
   * - toggles for the details
   *   GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE - show caps-name on edges
   *   GST_DEBUG_GRAPH_SHOW_CAPS_DETAILS	- show caps-details on edges
   *   GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS - show modified parameters on elements
   *   GST_DEBUG_GRAPH_SHOW_STATES - show element states 
   * - "show graph" action item
   */
  subitem=gtk_image_menu_item_new_with_mnemonic(_("Dump pipeline graph and show"));
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_debug_dump_pipeline_graph_and_show),(gpointer)self);
#endif
  
  // register event handlers
  g_signal_connect(G_OBJECT(self->priv->app), "notify::song", G_CALLBACK(on_song_changed), (gpointer)self);

  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_menu_new:
 * @app: the application the menu belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtMainMenu *bt_main_menu_new(const BtEditApplication *app) {
  BtMainMenu *self;

  if(!(self=BT_MAIN_MENU(g_object_new(BT_TYPE_MAIN_MENU,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_main_menu_init_ui(self)) {
    goto Error;
  }
  return(self);
Error:
  if(self) gtk_object_destroy(GTK_OBJECT(self));
  return(NULL);
}

//-- methods


//-- class internals

/* returns a property for the given property_id for this object */
static void bt_main_menu_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMainMenu *self = BT_MAIN_MENU(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_MENU_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_main_menu_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMainMenu *self = BT_MAIN_MENU(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_MENU_APP: {
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->app);
      //GST_DEBUG("set the app for main_menu: %p",self->priv->app);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_main_menu_dispose(GObject *object) {
  BtMainMenu *self = BT_MAIN_MENU(object);
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  g_object_try_weak_unref(self->priv->app);

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_main_menu_finalize(GObject *object) {
  //BtMainMenu *self = BT_MAIN_MENU(object);

  //GST_DEBUG("!!!! self=%p",self);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_main_menu_init(GTypeInstance *instance, gpointer g_class) {
  BtMainMenu *self = BT_MAIN_MENU(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_MAIN_MENU, BtMainMenuPrivate);
}

static void bt_main_menu_class_init(BtMainMenuClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtMainMenuPrivate));

  gobject_class->set_property = bt_main_menu_set_property;
  gobject_class->get_property = bt_main_menu_get_property;
  gobject_class->dispose      = bt_main_menu_dispose;
  gobject_class->finalize     = bt_main_menu_finalize;

  g_object_class_install_property(gobject_class,MAIN_MENU_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "Set application object, the menu belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
}

GType bt_main_menu_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtMainMenuClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_menu_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtMainMenu),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_main_menu_init, // instance_init
      NULL // value_table
    };
#ifndef USE_HILDON
    type = g_type_register_static(GTK_TYPE_MENU_BAR,"BtMainMenu",&info,0);
#else
    type = g_type_register_static(GTK_TYPE_MENU,"BtMainMenu",&info,0);
#endif
  }
  return type;
}
