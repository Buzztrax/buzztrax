/* $Id: tools.h,v 1.1 2004-08-13 20:44:07 ensonic Exp $
 * gui helper
 */

#ifndef TOOL_H
#define TOOL_H
 
/* Use this function to set the directory containing installed pixmaps. */
extern void add_pixmap_directory(const gchar     *directory);

/*
 * Private Functions.
 */

/* This is used to create the pixmaps used in the interface. */
extern GtkWidget *create_pixmap(const gchar *filename);

/* This is used to create the pixbufs used in the interface. */
extern GdkPixbuf *create_pixbuf(const gchar *filename);

#endif // TOOL_H
