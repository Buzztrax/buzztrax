/* $Id: tools.h,v 1.3 2004-12-08 18:17:32 ensonic Exp $
 * gui helper
 */

#ifndef BT_TOOLS_H
#define BT_TOOLS_H
 
/* pixmap/buf helpers */
extern void add_pixmap_directory(const gchar *directory);
extern GtkWidget *create_pixmap(const gchar *filename);
extern GdkPixbuf *create_pixbuf(const gchar *filename);

/* helper for simple message/question dialogs */
void bt_dialog_message(const BtMainWindow *self,const gchar *title,const gchar *headline,const gchar *message);
gboolean bt_dialog_question(const BtMainWindow *self,const gchar *title,const gchar *headline,const gchar *message);

#endif // BT_TOOLS_H
