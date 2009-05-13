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
 * SECTION:btmainwindow
 * @short_description: root buzztard editor window
 *
 * The main window class is a container for the #BtMainMenu, the #BtMainToolbar,
 * the #BtMainStatusbar and the #BtMainPages tabbed notebook.
 */

#define BT_EDIT
#define BT_MAIN_WINDOW_C

#include "bt-edit.h"

enum {
  MAIN_WINDOW_APP=1,
  MAIN_WINDOW_TOOLBAR,
  MAIN_WINDOW_STATUSBAR,
  MAIN_WINDOW_PAGES
};


struct _BtMainWindowPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  G_POINTER_ALIAS(BtEditApplication *,app);

  /* the menu of the window */
  BtMainMenu *menu;
  /* the toolbar of the window */
  BtMainToolbar *toolbar;
  /* the content pages of the window */
  BtMainPages *pages;
  /* the statusbar of the window */
  BtMainStatusbar *statusbar;
};

static GtkWindowClass *parent_class=NULL;

enum { TARGET_URI_LIST };
static GtkTargetEntry drop_types[] = {
   { "text/uri-list", 0, TARGET_URI_LIST }
};
static gint n_drop_types = sizeof(drop_types) / sizeof(GtkTargetEntry);

//-- helper methods

/* update the filename to have an extension, mathching the selected filter */ 
static gchar* update_filename_ext(const BtMainWindow *self,GtkFileChooser *dialog,gchar *file_name,GList *filters) {
  GtkFileFilter *this_filter,*that_filter;
  gchar *new_file_name=NULL;
  gchar *ext;
  const GList *plugins, *pnode, *fnode;
  BtSongIOModuleInfo *info;
  guint ix;
  
  plugins=bt_song_io_get_module_info_list();
  this_filter=gtk_file_chooser_get_filter(dialog);
  
  GST_WARNING("old song name = '%s', filter %p",file_name, this_filter);
  
  ext=strrchr(file_name,'.');

  if(ext) {
    ext++;
    // cut off known extensions
    for(pnode=plugins,fnode=filters;pnode;pnode=g_list_next(pnode)) {
      info=(BtSongIOModuleInfo *)pnode->data;
      ix=0;
      while(info->formats[ix].name) {
        that_filter=fnode->data;
        
        if((this_filter!=that_filter) && !strcmp(ext,info->formats[ix].extension)) {
          file_name[strlen(file_name)-strlen(info->formats[ix].extension)]='\0';
          GST_INFO("cut fn to: %s",file_name);
          pnode=NULL;
          break;
        }
        fnode=g_list_next(fnode);ix++;
      }
    }
  }

  // append new extension
  for(pnode=plugins,fnode=filters;pnode;pnode=g_list_next(pnode)) {
    info=(BtSongIOModuleInfo *)pnode->data;
    ix=0;
    while(info->formats[ix].name) {
      that_filter=fnode->data;
  
      if((this_filter==that_filter) && (!ext || strcmp(ext,info->formats[ix].extension))) {
        new_file_name=g_strdup_printf("%s.%s",file_name,info->formats[ix].extension);
        pnode=NULL;
        break;
      }
      fnode=g_list_next(fnode);ix++;
    }
  }
  
  if(new_file_name && strcmp(file_name,new_file_name)) {
    GST_WARNING("new song name = '%s'",new_file_name);
    return (new_file_name);
  }
  g_free (new_file_name);
  return(NULL);
}

//-- event handler

static gboolean on_window_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
  BtMainWindow *self=BT_MAIN_WINDOW(user_data);
  gboolean res=TRUE;

  g_assert(user_data);

  GST_INFO("delete event occurred");
  // returning TRUE means, we don't want the window to be destroyed
  if(bt_main_window_check_quit(self)) {
    /* @todo: remember window size
    BtSettings *settings;
    int x, y, w, h;

    g_object_get(G_OBJECT(self->priv->app),"settings",&settings,NULL);
    gtk_window_get_position (GTK_WINDOW (self), &x, &y);
    gtk_window_get_size (GTK_WINDOW (self), &w, &h);

    g_object_set(G_OBJECT(settings),"window-xpos",x,"window-ypos",y,"window-width",w,"window-height",h,NULL);

    g_object_unref(settings);
    */
    // @todo: if we do this the refcount goes from 1 to 3
    //gtk_widget_hide_all(GTK_WIDGET(self));
    res=FALSE;
  }
  return(res);
}

static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
  GST_INFO("destroy occurred");
  if(gtk_main_level()) {
    GST_INFO("  leaving main-loop");
    gtk_main_quit();
  }
}

static void on_song_unsaved_changed(const BtSong *song,GParamSpec *arg,gpointer user_data) {
  BtMainWindow *self=BT_MAIN_WINDOW(user_data);
  gchar *title,*name;
  BtSongInfo *song_info;
  gboolean unsaved;

  g_assert(user_data);

  g_object_get(G_OBJECT(song),"song-info",&song_info,"unsaved",&unsaved,NULL);
  // compose title
  g_object_get(G_OBJECT(song_info),"name",&name,NULL);
  // we don't use PACKAGE_NAME = 'buzztard' for the window title
  title=g_strdup_printf("%s (%s) - Buzztard",name,(unsaved?_("unsaved"):_("saved")));g_free(name);
  gtk_window_set_title(GTK_WINDOW(self), title);
  g_free(title);
  //-- release the references
  g_object_unref(song_info);

  GST_INFO("song.unsaved has changed : song=%p, menu=%p, unsaved=%d",song,user_data,unsaved);
}

static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtMainWindow *self=BT_MAIN_WINDOW(user_data);
  BtSong *song;

  g_assert(user_data);

  GST_INFO("song has changed : app=%p, window=%p",app,user_data);

  // get song from app
  g_object_get(G_OBJECT(app),"song",&song,NULL);
  if(!song) return;

  on_song_unsaved_changed(song,arg,self);
  g_signal_connect(G_OBJECT(song), "notify::unsaved", G_CALLBACK(on_song_unsaved_changed), (gpointer)self);
  //-- release the references
  g_object_unref(song);
}

static void on_window_dnd_drop(GtkWidget *widget, GdkDragContext *dc, gint x, gint y, GtkSelectionData *selection_data, guint info, guint t, gpointer user_data) {
  BtMainWindow *self=BT_MAIN_WINDOW(user_data);
  glong i=0;
  gchar *ptr=(gchar *)selection_data->data;

  g_assert(user_data);

  GST_INFO("something has been dropped on our app: window=%p data='%s'",user_data,selection_data->data);
  // find first \0 or \n or \r
  while((*ptr) && (*ptr!='\n') && (*ptr!='\r')) {
    ptr++;i++;
  }
  if(i) {
    gchar *file_name=g_strndup((gchar *)selection_data->data,i);
    gboolean res=TRUE;

    if(!bt_edit_application_load_song(self->priv->app,file_name)) {
      gchar *msg=g_strdup_printf(_("An error occurred while trying to load the song from file '%s'"),file_name);
      bt_dialog_message(self,_("Can't load song"),_("Can't load song"),msg);
      g_free(msg);
      res=FALSE;
    }
    gtk_drag_finish(dc,res,FALSE,t);
    g_free(file_name);
  }
}

/* just for testing
static gboolean on_window_event(GtkWidget *widget, GdkEvent  *event, gpointer user_data) {
  if(event->type==GDK_BUTTON_PRESS) {
    GdkEventButton *e=(GdkEventButton*)event;

    GST_INFO("type=%4d, window=%p, send_event=%3d, time=%8d",e->type,e->window,e->send_event,e->time);
    GST_INFO("x=%6.4lf, y=%6.4lf, axes=%p, state=%4d",e->x,e->y,e->axes,e->state);
    GST_INFO("button=%4d, device=%p, x_root=%6.4lf, y_root=%6.4lf\n",e->button,e->device,e->x_root,e->y_root);
  }
  return(FALSE);
}
*/

//-- helper methods

static gboolean bt_main_window_init_ui(const BtMainWindow *self) {
  GtkWidget *box;
  GdkPixbuf *window_icon;

  gtk_widget_set_name(GTK_WIDGET(self),"main window");
  gtk_window_set_role(GTK_WINDOW(self),"buzztard-edit::main");

  // create and set window icon
  if((window_icon=gdk_pixbuf_new_from_theme("buzztard",16))) {
    gtk_window_set_icon(GTK_WINDOW(self),window_icon);
    g_object_unref(window_icon);
  }

  // create main layout container
  box=gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(self),box);

  GST_INFO("before creating content, app->ref_ct=%d",G_OBJECT(self->priv->app)->ref_count);

  // add the menu-bar
  self->priv->menu=bt_main_menu_new(self->priv->app);
#ifndef USE_HILDON
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->menu),FALSE,FALSE,0);
#else
  hildon_window_set_menu(HILDON_WINDOW(self), GTK_MENU(self->priv->menu));
#endif
  // add the tool-bar
  self->priv->toolbar=bt_main_toolbar_new(self->priv->app);
#ifndef USE_HILDON
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->toolbar),FALSE,FALSE,0);
#else
  hildon_window_add_toolbar(HILDON_WINDOW(self), GTK_TOOLBAR(self->priv->toolbar));
#endif
  // add the window content pages
  self->priv->pages=bt_main_pages_new(self->priv->app);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->pages),TRUE,TRUE,0);
  // add the status bar
  self->priv->statusbar=bt_main_statusbar_new(self->priv->app);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->statusbar),FALSE,FALSE,0);

  gtk_window_add_accel_group(GTK_WINDOW(self),bt_ui_resources_get_accel_group());

  gtk_drag_dest_set(GTK_WIDGET(self),
    (GtkDestDefaults) (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT | GTK_DEST_DEFAULT_DROP),
    drop_types, n_drop_types, GDK_ACTION_COPY);
  g_signal_connect(G_OBJECT(self), "drag-data-received", G_CALLBACK(on_window_dnd_drop),(gpointer)self);

  GST_INFO("content created, app->ref_ct=%d",G_OBJECT(self->priv->app)->ref_count);

  g_signal_connect(G_OBJECT(self->priv->app), "notify::song", G_CALLBACK(on_song_changed), (gpointer)self);
  g_signal_connect(G_OBJECT(self),"delete-event", G_CALLBACK(on_window_delete_event),(gpointer)self);
  g_signal_connect(G_OBJECT(self),"destroy",      G_CALLBACK(on_window_destroy),(gpointer)self);
  /* just for testing
  g_signal_connect(G_OBJECT(self),"event",G_CALLBACK(on_window_event),(gpointer)self);
  */

  GST_INFO("signal connected, app->ref_ct=%d",G_OBJECT(self->priv->app)->ref_count);

  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_window_new:
 * @app: the application the window belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtMainWindow *bt_main_window_new(const BtEditApplication *app) {
  BtMainWindow *self;
  BtSettings *settings;
  gboolean toolbar_hide,statusbar_hide,tabs_hide;
  // int x, y, w, h;

  GST_INFO("creating a new window, app->ref_ct=%d",G_OBJECT(app)->ref_count);
  // eventualy hide the toolbar
  g_object_get(G_OBJECT(app),"settings",&settings,NULL);
  g_object_get(G_OBJECT(settings),
    "toolbar-hide",&toolbar_hide,
    "statusbar-hide",&statusbar_hide,
    "tabs-hide",&tabs_hide,
    //"window-xpos",&x,"window-ypos",&y,"window-width",&w,"window-height",&h,
    NULL);
  g_object_unref(settings);

  if(!(self=BT_MAIN_WINDOW(g_object_new(BT_TYPE_MAIN_WINDOW,"app",app,"type",GTK_WINDOW_TOPLEVEL,NULL)))) {
    goto Error;
  }
  GST_INFO("new main_window created, app->ref_ct=%d",G_OBJECT(app)->ref_count);
  // generate UI
  if(!bt_main_window_init_ui(self)) {
    goto Error;
  }
  GST_INFO("new main_window layouted, app->ref_ct=%d",G_OBJECT(app)->ref_count);

  // this enforces a minimum size
  //gtk_widget_set_size_request(GTK_WIDGET(self),800,600);
  // this causes a problem with resizing the sequence-view
  //gtk_window_set_default_size(GTK_WINDOW(self),800,600);
  // this is deprecated
  //gtk_widget_set_usize(GTK_WIDGET(self), 800,600);

  gtk_window_resize(GTK_WINDOW(self),800,600);

  /* @todo: use position from settings
  if(w>0 && h>0) {
    gtk_window_move (window, x, y);
    gtk_window_set_default_size (window, w, h);
  }
  */

  gtk_widget_show_all(GTK_WIDGET(self));
  if(toolbar_hide) {
    gtk_widget_hide(GTK_WIDGET(self->priv->toolbar));
  }
  if(statusbar_hide) {
    gtk_widget_hide(GTK_WIDGET(self->priv->statusbar));
  }
  if(tabs_hide) {
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(self->priv->pages),FALSE);
  }

  GST_INFO("new main_window shown");
  return(self);
Error:
  GST_WARNING("new main_window failed");
  if(self) gtk_object_destroy(GTK_OBJECT(self));
  return(NULL);
}

//-- methods

/**
 * bt_main_window_run:
 * @self: the window instance to setup and run
 *
 * build, show and run the main window
 *
 * Returns: true for success
 */
gboolean bt_main_window_run(const BtMainWindow *self) {
  gboolean res=TRUE;
  GST_INFO("before running the UI");
  gtk_main();
  GST_INFO("after running the UI");
  return(res);
}

/**
 * bt_main_window_check_quit:
 * @self: the main window instance
 *
 * Displays a dialog box, that asks the user to confirm exiting the application.
 *
 * Returns: %TRUE if the user has confirmed to exit
 */
gboolean bt_main_window_check_quit(const BtMainWindow *self) {
  gboolean res=TRUE;
  gboolean unsaved=FALSE;
  BtSong *song;

  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  if(song) {
    g_object_get(song,"unsaved",&unsaved,NULL);
    g_object_unref(song);
  }
  if(unsaved) {
    res=bt_dialog_question(self,_("Really quit?"),_("Really quit?"),_("All unsaved changes will be lost then."));
  }

  return(res);
}

/**
 * bt_main_window_new_song:
 * @self: the main window instance
 *
 * Prepares a new song. Triggers cleaning up the old song and refreshes the ui.
 */
void bt_main_window_new_song(const BtMainWindow *self) {
  gboolean res=TRUE;
  gboolean unsaved=FALSE;
  BtSong *song;

  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  if(song) {
    g_object_get(song,"unsaved",&unsaved,NULL);
    g_object_unref(song);
  }
  if(unsaved) {
    // @todo check http://developer.gnome.org/projects/gup/hig/2.0/windows-alert.html#alerts-confirmation
    res=bt_dialog_question(self,_("New song?"),_("New song?"),_("All unsaved changes will be lost then."));
  }
  if(res) {
    if(!bt_edit_application_new_song(self->priv->app)) {
      // @todo show error message (from where? and which error?)
    }
  }
}

/**
 * bt_main_window_open_song:
 * @self: the main window instance
 *
 * Opens a dialog box, where the user can choose a song to load.
 * If the dialog is not canceld, the old song will be freed, the new song will
 * be loaded and the ui will be refreshed upon success.
 */
void bt_main_window_open_song(const BtMainWindow *self) {
  BtSettings *settings;
  GtkWidget *dialog=gtk_file_chooser_dialog_new(_("Open a song"),GTK_WINDOW(self),GTK_FILE_CHOOSER_ACTION_OPEN,
				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				      NULL);
  gint result;
  gchar *folder_name,*file_name=NULL;
  GtkFileFilter *filter,*filter_all;
  const GList *plugins, *node;
  BtSongIOModuleInfo *info;
  guint ix;

  // set filters
  filter_all=gtk_file_filter_new();
  gtk_file_filter_set_name(filter_all,"all supported files");
  plugins=bt_song_io_get_module_info_list();
  for(node=plugins;node;node=g_list_next(node)) {
    info=(BtSongIOModuleInfo *)node->data;
    ix=0;
    while(info->formats[ix].name) {
      filter=gtk_file_filter_new();
      gtk_file_filter_set_name(filter,info->formats[ix].name);
      gtk_file_filter_add_mime_type(filter,info->formats[ix].mime_type);
      gtk_file_filter_add_mime_type(filter_all,info->formats[ix].mime_type);
//#if !GLIB_CHECK_VERSION(2,18,0)
      /* workaround for http://bugzilla.gnome.org/show_bug.cgi?id=541236
       * should be fixed, but is not :/
       */
      if(!strcmp(info->formats[ix].mime_type,"audio/x-bzt-xml")) {
        gtk_file_filter_add_pattern(filter,"*.xml");
        gtk_file_filter_add_pattern(filter_all,"*.xml");
      }
//#endif
      gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog),filter);
      ix++;
    }
  }
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog),filter_all);
  filter=gtk_file_filter_new();
  gtk_file_filter_set_name(filter,"all files");
  gtk_file_filter_add_pattern(filter,"*");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog),filter);
  // set default filter
  gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog),filter_all);

  // set a default songs folder
  //gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),DATADIR""G_DIR_SEPARATOR_S""PACKAGE""G_DIR_SEPARATOR_S"songs"G_DIR_SEPARATOR_S);
  g_object_get(G_OBJECT(self->priv->app),"settings",&settings,NULL);
  g_object_get(settings,"song-folder",&folder_name,NULL);
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),folder_name);
  g_free(folder_name);
  g_object_unref(settings);
 
  result=gtk_dialog_run(GTK_DIALOG(dialog));
  switch(result) {
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      file_name=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
      break;
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_CLOSE:
      break;
    default:
      GST_WARNING("unhandled response code = %d",result);
  }
  gtk_widget_destroy(dialog);
  // load after destoying the dialog, otherwise it stays open all time
  if(file_name) {
    if(!bt_edit_application_load_song(self->priv->app,file_name)) {
      gchar *msg=g_strdup_printf(_("An error occurred while loading the song from file '%s'"),file_name);
      bt_dialog_message(self,_("Can't load song"),_("Can't load song"),msg);
      g_free(msg);
    }
#if GTK_CHECK_VERSION(2,10,0)
    else {
      // store recent file
      GtkRecentManager *manager=gtk_recent_manager_get_default();
      gchar *uri=g_filename_to_uri(file_name,NULL,NULL);
      
      if(!gtk_recent_manager_add_item (manager, uri)) {
	    GST_WARNING("Can't store recent file");
      }
      g_free(uri);
    }
#endif
    g_free(file_name);
  }
}

/**
 * bt_main_window_save_song:
 * @self: the main window instance
 *
 * Save the song to disk.
 * If it is a new song it will ask for a file_name and location.
 */
void bt_main_window_save_song(const BtMainWindow *self) {
  BtSong *song;
  BtSongInfo *song_info;
  gchar *file_name=NULL;

  // get songs file-name
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"song-info",&song_info,NULL);
  g_object_get(G_OBJECT(song_info),"file-name",&file_name,NULL);

  // check the file_name of the song
  if(file_name) {
    if(!bt_edit_application_save_song(self->priv->app,file_name)) {
      gchar *msg=g_strdup_printf(_("An error occurred while saving the song to file '%s'."),file_name);
      bt_dialog_message(self,_("Can't save song"),_("Can't save song"),msg);
      g_free(msg);
    }
  }
  else {
    // it is a new song
    bt_main_window_save_song_as(self);
  }
  g_free(file_name);
  g_object_unref(song_info);
  g_object_unref(song);
}

/**
 * bt_main_window_save_song_as:
 * @self: the main window instance
 *
 * Opens a dialog box, where the user can choose a file_name and location to save
 * the song under.
 */
void bt_main_window_save_song_as(const BtMainWindow *self) {
  BtSettings *settings;
  BtSong *song;
  BtSongInfo *song_info;
  gchar *name,*folder_name,*file_name=NULL;
  gchar *old_file_name=NULL;
  gint result;
  GtkWidget *dialog=gtk_file_chooser_dialog_new(_("Save a song"),GTK_WINDOW(self),GTK_FILE_CHOOSER_ACTION_SAVE,
				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
				      NULL);
  GtkFileFilter *filter;
  GList *filters=NULL;
  const GList *plugins, *node;
  BtSongIOModuleInfo *info;
  guint ix;

  // set filters
  plugins=bt_song_io_get_module_info_list();
  for(node=plugins;node;node=g_list_next(node)) {
    info=(BtSongIOModuleInfo *)node->data;
    ix=0;
    while(info->formats[ix].name) {
      filter=gtk_file_filter_new();
      gtk_file_filter_set_name(filter,info->formats[ix].name);
      //gtk_file_filter_add_mime_type(filter,info->formats[ix].mime_type);
      //gtk_file_filter_add_pattern(filter,info->formats[ix].extension);
      gtk_file_filter_add_pattern(filter,g_strconcat("*.",info->formats[ix].extension,NULL));
      gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog),filter);
      GST_DEBUG("add filter for %s/%s/%s",
        info->formats[ix].name,
        info->formats[ix].mime_type,
        info->formats[ix].extension);
      ix++;
      filters=g_list_append(filters,filter);
    }
  }

  // get songs file-name
  g_object_get(G_OBJECT(self->priv->app),"song",&song,"settings",&settings,NULL);
  g_object_get(G_OBJECT(song),"song-info",&song_info,NULL);
  g_object_get(G_OBJECT(song_info),"name",&name,"file-name",&file_name,NULL);
  g_object_get(settings,"song-folder",&folder_name,NULL);
  if(!file_name) {
    GST_DEBUG("use defaults %s/%s",folder_name,name);
    /* the user just created a new document */
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(dialog), folder_name);
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(dialog), name);
  }
  else {
    gboolean found=FALSE;
    GtkFileFilterInfo ffi = { 
      GTK_FILE_FILTER_FILENAME|GTK_FILE_FILTER_DISPLAY_NAME, 
      file_name, 
      NULL, // uri
      file_name,
      NULL // mime-type
    };
    /* the user edited an existing document */
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog),file_name);
    GST_DEBUG("use existing %s",file_name);
    /* select a filter that would show this file */
    for(node=filters;node;node=g_list_next(node)) {
      filter=node->data;
      if(gtk_file_filter_filter(filter,&ffi)) {
        GST_DEBUG("use last path %s, format is '%s'",file_name,gtk_file_filter_get_name(filter));
        gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog),filter);
        found=TRUE;
        break;
      }
    }
    if(!found) {
      gchar *ext=strrchr(file_name,'.');
      if(ext && ext[1]) {
        const GList *pnode,*fnode=filters;
        /* gtk_file_filter_filter() seems to be buggy :/
         * try matching the extension */
        ext++;
        GST_DEBUG("file_filter matching failed, match extension '%s'",ext);
        for(pnode=plugins;(pnode && !found);pnode=g_list_next(pnode)) {
          info=(BtSongIOModuleInfo *)pnode->data;
          ix=0;
          while(info->formats[ix].name && !found) {
            if(!strcmp(ext,info->formats[ix].extension)) {
              filter=fnode->data;
              /* @todo: it matches, but this does not update the dialog */
              gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog),filter);
              GST_DEBUG("format is '%s'",gtk_file_filter_get_name(filter));
              found=TRUE;
            }
            else {
              ix++;
              fnode=g_list_next(fnode);
            }
          }
        }
      }
    }
  }
  old_file_name=file_name;file_name=NULL;
  g_free(folder_name);
  g_free(name);
    
  g_object_unref(settings);
  g_object_unref(song_info);
  g_object_unref(song);

  result=gtk_dialog_run(GTK_DIALOG(dialog));
  switch(result) {
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK: {
      gchar *new_file_name=NULL;

      // make sure it has the extension matching the filter
      file_name=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
      if((new_file_name=update_filename_ext(self,GTK_FILE_CHOOSER(dialog),file_name,filters))) {
        g_free(file_name);
        file_name=new_file_name;      
      }
    } break;
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_CLOSE:
      break;
    default:
      GST_WARNING("unhandled response code = %d",result);
  }
  gtk_widget_destroy(dialog);
  g_list_free(filters);
  // save after destoying the dialog, otherwise it stays open all time
  if(file_name) {
    FILE *file;
    gboolean cont=TRUE;
    
    GST_WARNING("song name = '%s'",file_name);

    if((file=fopen(file_name,"rb"))) {
      GST_INFO("file already exists");
      // it already exists, ask the user what to do (do not save, choose new name, overwrite song)
      cont=bt_dialog_question(self,
        _("File already exists"),
        _("File already exists"),
        _("Choose 'Okay' to overwrite or 'Cancel' to abort saving the song."));
      fclose(file);
    }
    else {
      const gchar *reason=(const gchar *)strerror(errno);
      gchar *msg;

      GST_INFO("file can not be opened : %d : %s",errno,reason);
      switch(errno) {
        case EACCES:  // Permission denied.
          cont=FALSE;
          msg=g_strdup_printf(_("An error occurred while trying to open the file '%s' for writing: %s"),file_name,reason);
          bt_dialog_message(self,_("Can't save song"),_("Can't save song"),msg);
          g_free(msg);
          break;
        default:
          // ENOENT A component of the path file_name does not exist, or the path is an empty string.
          // -> just save
          break;
      }
    }
    if(cont) {
      if(!bt_edit_application_save_song(self->priv->app,file_name)) {
        gchar *msg=g_strdup_printf(_("An error occurred while trying to save the song to file '%s'."),file_name);
        bt_dialog_message(self,_("Can't save song"),_("Can't save song"),msg);
        g_free(msg);
      }
#if GTK_CHECK_VERSION(2,10,0)
      else {
        // store recent file
        GtkRecentManager *manager=gtk_recent_manager_get_default();
        gchar *uri;
        
        if(old_file_name) {
          uri=g_filename_to_uri(old_file_name,NULL,NULL);
          if(!gtk_recent_manager_remove_item (manager, uri, NULL)) {
            GST_WARNING("Can't store recent file");
          }
          g_free(uri);         
        }
        uri=g_filename_to_uri(file_name,NULL,NULL);
        if(!gtk_recent_manager_add_item (manager, uri)) {
          GST_WARNING("Can't store recent file");
        }
        g_free(uri);
      }
#endif
    }
    g_free(file_name);
  }
  g_free(old_file_name);
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_main_window_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMainWindow *self = BT_MAIN_WINDOW(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_WINDOW_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    case MAIN_WINDOW_TOOLBAR: {
      g_value_set_object(value, self->priv->toolbar);
    } break;
    case MAIN_WINDOW_STATUSBAR: {
      g_value_set_object(value, self->priv->statusbar);
    } break;
    case MAIN_WINDOW_PAGES: {
      g_value_set_object(value, self->priv->pages);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_main_window_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMainWindow *self = BT_MAIN_WINDOW(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_WINDOW_APP: {
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->app);
      GST_DEBUG("set the app for main_window: %p, app->ref_ct=%d",self->priv->app,G_OBJECT(self->priv->app)->ref_count);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_main_window_dispose(GObject *object) {
  BtMainWindow *self = BT_MAIN_WINDOW(object);
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  g_signal_handlers_disconnect_matched(self->priv->app,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_song_changed,NULL);
  //g_signal_handlers_disconnect_matched(self,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_window_delete_event,NULL);
  //g_signal_handlers_disconnect_matched(self,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_window_destroy,NULL);
  g_object_try_weak_unref(self->priv->app);

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->dispose(object);
  GST_DEBUG("  done");
}

static void bt_main_window_finalize(GObject *object) {
  BtMainWindow *self = BT_MAIN_WINDOW(object);

  GST_DEBUG("!!!! self=%p, ref_ct=%d",self,G_OBJECT(self)->ref_count);

  G_OBJECT_CLASS(parent_class)->finalize(object);
  GST_DEBUG("  done");
}

static void bt_main_window_init(GTypeInstance *instance, gpointer g_class) {
  BtMainWindow *self = BT_MAIN_WINDOW(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_MAIN_WINDOW, BtMainWindowPrivate);
}

static void bt_main_window_class_init(BtMainWindowClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtMainWindowPrivate));

  gobject_class->set_property = bt_main_window_set_property;
  gobject_class->get_property = bt_main_window_get_property;
  gobject_class->dispose      = bt_main_window_dispose;
  gobject_class->finalize     = bt_main_window_finalize;

  g_object_class_install_property(gobject_class,MAIN_WINDOW_APP,
                                  g_param_spec_object("app",
                                     "app construct prop",
                                     "Set application object, the window belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,MAIN_WINDOW_TOOLBAR,
                                  g_param_spec_object("toolbar",
                                     "toolbar prop",
                                     "Get the toolbar",
                                     BT_TYPE_MAIN_TOOLBAR, /* object type */
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,MAIN_WINDOW_STATUSBAR,
                                  g_param_spec_object("statusbar",
                                     "statusbar prop",
                                     "Get the status bar",
                                     BT_TYPE_MAIN_STATUSBAR, /* object type */
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,MAIN_WINDOW_PAGES,
                                  g_param_spec_object("pages",
                                     "pages prop",
                                     "Get the pages widget",
                                     BT_TYPE_MAIN_PAGES, /* object type */
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));
}

GType bt_main_window_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtMainWindowClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_window_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtMainWindow),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_main_window_init, // instance_init
      NULL // value_table
    };
#ifndef USE_HILDON
    type = g_type_register_static(GTK_TYPE_WINDOW,"BtMainWindow",&info,0);
#else
    type = g_type_register_static(HILDON_TYPE_WINDOW,"BtMainWindow",&info,0);
#endif
  }
  return type;
}
