/* $Id: tools.h,v 1.7 2005-04-20 17:37:08 ensonic Exp $
 * gui helper
 */

#ifndef BT_EDIT_TOOLS_H
#define BT_EDIT_TOOLS_H
 
/* pixmap/buf helpers */
extern void add_pixmap_directory(const gchar *directory);
extern GtkWidget *gtk_image_new_from_filename(const gchar *filename);
extern GdkPixbuf *gdk_pixbuf_new_from_filename(const gchar *filename);

/* helper for simple message/question dialogs */
extern void bt_dialog_message(const BtMainWindow *self,const gchar *title,const gchar *headline,const gchar *message);
extern gboolean bt_dialog_question(const BtMainWindow *self,const gchar *title,const gchar *headline,const gchar *message);

/* gdk thread locking helpers */
extern void bt_threads_init(void);
extern void gdk_threads_try_enter(void);
extern void gdk_threads_try_leave(void);

/* gtk toolbar helper */
extern GtkToolbarStyle gtk_toolbar_get_style_from_string(const gchar *style_name);

#endif // BT_EDIT_TOOLS_H
