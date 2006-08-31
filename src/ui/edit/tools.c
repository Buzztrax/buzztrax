/* $Id: tools.c,v 1.14 2006-08-31 19:57:57 ensonic Exp $
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

#define BT_EDIT
#define BT_TOOLS_C
 
#include "bt-edit.h"

static GList *pixmaps_directories = NULL;

/**
 * add_pixmap_directory:
 * @directory: register another directory to search for pixmaps
 *
 * Use this function to set the directory containing installed pixmaps.
 */
void add_pixmap_directory(const gchar *directory) {
  pixmaps_directories = g_list_prepend(pixmaps_directories, g_strdup(directory));
}

/*
 * find_pixmap_file:
 * @filename: name of the file to look for
 *
 * This is an internally used function to find pixmap files.
 * Use add_pixmap_directory() to extend the serarch path.
 *
 * Returns: the path that has this file
 */
static gchar *find_pixmap_file(const gchar *filename) {
  GList *elem;

  /* We step through each of the pixmaps directory to find it. */
  elem = pixmaps_directories;
  while(elem) {
    gchar *pathname = g_strdup_printf("%s%s%s", (gchar*)elem->data, G_DIR_SEPARATOR_S, filename);
    if (g_file_test(pathname, G_FILE_TEST_EXISTS)) return pathname;
    g_free(pathname);
    elem = elem->next;
  }
  return NULL;
}

/**
 * gtk_image_new_from_filename:
 * @filename: the filename of the image file
 *
 * Creates a new pixmap image widget for the image file.
 *
 * Returns: a new pixmap widget
 */
GtkWidget *gtk_image_new_from_filename(const gchar *filename) {
  gchar *pathname = NULL;
  GtkWidget *pixmap;

  if(!filename || !filename[0]) return gtk_image_new();

  pathname = find_pixmap_file(filename);

  if(!pathname) {
    g_warning (_("Couldn't find pixmap file: %s"), filename);
    return gtk_image_new ();
  }

  pixmap = gtk_image_new_from_file(pathname);
  g_free(pathname);
  return pixmap;
}

/**
 * gdk_pixbuf_new_from_filename:
 * @filename: the filename of the image file
 *
 * Creates a new pixbuf image for the image file.
 *
 * Returns: a new pixbuf
 */
GdkPixbuf *gdk_pixbuf_new_from_filename(const gchar *filename) {
  gchar *pathname = NULL;
  GdkPixbuf *pixbuf;
  GError *error = NULL;

  if(!filename || !filename[0]) return NULL;

  pathname = find_pixmap_file(filename);

  if(!pathname) {
    g_warning (_("Couldn't find pixmap file: %s"), filename);
    return NULL;
  }

  pixbuf = gdk_pixbuf_new_from_file(pathname, &error);
  if(!pixbuf) {
    fprintf(stderr, "Failed to load pixbuf file: %s: %s\n",pathname, error->message);
    g_error_free(error);
  }
  g_free(pathname);
  return pixbuf;
}


// @todo use GtkMessageDialog for the next two

/**
 * bt_dialog_message:
 * @self: the applications main window
 * @title: the title of the message
 * @headline: the bold headline of the message
 * @message: the message itself
 *
 * Displays a modal message dialog, that needs to be confirmed with "Okay".
 */
void bt_dialog_message(const BtMainWindow *self,const gchar *title,const gchar *headline,const gchar *message) {
  GtkWidget *label,*icon,*box;
  gchar *str; 
  GtkWidget *dialog;
  
  g_return_if_fail(BT_IS_MAIN_WINDOW(self));
  g_return_if_fail(BT_IS_STRING(title));
  g_return_if_fail(BT_IS_STRING(headline));
  g_return_if_fail(BT_IS_STRING(message));

  dialog = gtk_dialog_new_with_buttons(title,
                                        GTK_WINDOW(self),
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                        NULL);

  box=gtk_hbox_new(FALSE,12);
  gtk_container_set_border_width(GTK_CONTAINER(box),6);

  // when to use GTK_STOCK_DIALOG_WARNING
  icon=gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO,GTK_ICON_SIZE_DIALOG);
  gtk_container_add(GTK_CONTAINER(box),icon);
  
  // @idea if headline is NULL use title ?
  label=gtk_label_new(NULL);
  str=g_strdup_printf("<big><b>%s</b></big>\n\n%s",headline,message);
  gtk_label_set_markup(GTK_LABEL(label),str);
  g_free(str);
  gtk_container_add(GTK_CONTAINER(box),label);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),box);
  gtk_widget_show_all(dialog);
                                                  
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

/**
 * bt_dialog_question:
 * @self: the applications main window
 * @title: the title of the message
 * @headline: the bold headline of the message
 * @message: the message itself
 *
 * Displays a modal question dialog, that needs to be confirmed with "Okay" or aborted with "Cancel".
 * Returns: %TRUE for Okay, %FALSE otherwise
 */
gboolean bt_dialog_question(const BtMainWindow *self,const gchar *title,const gchar *headline,const gchar *message) {
  gboolean result=FALSE;
  gint answer;
  GtkWidget *label,*icon,*box;
  gchar *str; 
  GtkWidget *dialog;

  g_return_val_if_fail(BT_IS_MAIN_WINDOW(self),FALSE);
  g_return_val_if_fail(BT_IS_STRING(title),FALSE);
  g_return_val_if_fail(BT_IS_STRING(headline),FALSE);
  g_return_val_if_fail(BT_IS_STRING(message),FALSE);
  
  dialog = gtk_dialog_new_with_buttons(title,
                                        GTK_WINDOW(self),
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                                        NULL);

  box=gtk_hbox_new(FALSE,12);
  gtk_container_set_border_width(GTK_CONTAINER(box),6);

  icon=gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION,GTK_ICON_SIZE_DIALOG);
  gtk_container_add(GTK_CONTAINER(box),icon);
  
  // @idea if headline is NULL use title ?
  label=gtk_label_new(NULL);
  str=g_strdup_printf("<big><b>%s</b></big>\n\n%s",headline,message);
  gtk_label_set_markup(GTK_LABEL(label),str);
  g_free(str);
  gtk_container_add(GTK_CONTAINER(box),label);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),box);
  gtk_widget_show_all(dialog);
                                                  
  answer=gtk_dialog_run(GTK_DIALOG(dialog));
  switch(answer) {
    case GTK_RESPONSE_ACCEPT:
      result=TRUE;
      break;
    case GTK_RESPONSE_REJECT:
      result=FALSE;
      break;
    default:
      GST_WARNING("unhandled response code = %d",answer);
  }
  gtk_widget_destroy(dialog);
  GST_INFO("bt_dialog_question(\"%s\") = %d",title,result);
  return(result);
}

// gdk thread locking helpers
// idea is from rythmbox sources

static GThread *main_thread = NULL;

/**
 * bt_threads_init:
 *
 * Initialises gdk thread locking helpers. Do call this together like in:
 * <informalexample><programlisting language="c">
 * if(!g_thread_supported()) {  // are g_threads() already initialized
 *    g_thread_init(NULL);
 *    gdk_threads_init();
 *    bt_threads_init();
 *  }</programlisting>
 * </informalexample>
 */
void bt_threads_init(void) {
  main_thread=g_thread_self();
}

static gboolean bt_threads_in_main_thread(void) {
  return(main_thread==g_thread_self());
}

/**
 * gdk_threads_try_enter:
 *
 * Use this instead of gdk_threads_enter(). This methods avoids lockups that
 * happen when calling gdk_threads_enter() twice from the same thread.
 * To unlock use that matching gdk_threads_try_leave().
 */
void gdk_threads_try_enter(void) {
  if(!bt_threads_in_main_thread())  gdk_threads_enter();
}

/**
 * gdk_threads_try_leave:
 *
 * Use this instead of gdk_threads_leave(). This methods matches
 * gdk_threads_try_enter().
 */
void gdk_threads_try_leave(void) {
  if (!bt_threads_in_main_thread()) gdk_threads_leave();
}


// gtk toolbar helper

/**
 * gtk_toolbar_get_style_from_string:
 * @style_name: the style name returned from gconf settings
 *
 * toolbar gconf to style conversion
 *
 * Returns: the style id
 */
GtkToolbarStyle gtk_toolbar_get_style_from_string(const gchar *style_name) {

  g_return_val_if_fail(style_name,GTK_TOOLBAR_BOTH);
  
  if (!strcmp(style_name,"icons")) {
    return(GTK_TOOLBAR_ICONS);
  }
  else if (!strcmp(style_name,"both")) {
    return(GTK_TOOLBAR_BOTH);
  }
  else if (!strcmp(style_name,"both-horiz")) {
    return(GTK_TOOLBAR_BOTH_HORIZ);
  }
  else if (!strcmp(style_name,"text")) {
    return(GTK_TOOLBAR_TEXT);
  }
  return(GTK_TOOLBAR_BOTH);
}
