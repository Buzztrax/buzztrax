/* $Id: tools.c,v 1.5 2005-01-15 22:02:53 ensonic Exp $
 * gui helper
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

/* This is an internally used function to find pixmap files. */
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
//GtkWidget *create_pixmap(const gchar *filename) {
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
//GdkPixbuf *create_pixbuf(const gchar *filename) {
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
	
	dialog = gtk_dialog_new_with_buttons(title,
                                        GTK_WINDOW(self),
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                        NULL);

  box=gtk_hbox_new(FALSE,12);
  gtk_container_set_border_width(GTK_CONTAINER(box),6);

  icon=gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO,GTK_ICON_SIZE_DIALOG);
  gtk_container_add(GTK_CONTAINER(box),icon);
  
	// @todo if headline is NULL use title ?
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
  
	// @todo if headline is NULL use title ?
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
