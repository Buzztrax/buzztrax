/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __GTK_SCROLLED_SYNC_WINDOW_H__
#define __GTK_SCROLLED_SYNC_WINDOW_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define GTK_TYPE_SCROLLED_SYNC_WINDOW            (gtk_scrolled_sync_window_get_type ())
#define GTK_SCROLLED_SYNC_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_SCROLLED_SYNC_WINDOW, GtkScrolledSyncWindow))
#define GTK_SCROLLED_SYNC_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_SCROLLED_SYNC_WINDOW, GtkScrolledSyncWindowClass))
#define GTK_IS_SCROLLED_SYNC_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_SCROLLED_SYNC_WINDOW))
#define GTK_IS_SCROLLED_SYNC_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_SCROLLED_SYNC_WINDOW))
#define GTK_SCROLLED_SYNC_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_SCROLLED_SYNC_WINDOW, GtkScrolledSyncWindowClass))


typedef struct _GtkScrolledSyncWindow              GtkScrolledSyncWindow;
typedef struct _GtkScrolledSyncWindowPrivate       GtkScrolledSyncWindowPrivate;
typedef struct _GtkScrolledSyncWindowClass         GtkScrolledSyncWindowClass;

struct _GtkScrolledSyncWindow
{
  GtkBin container;

  GtkScrolledSyncWindowPrivate *priv;
};

struct _GtkScrolledSyncWindowClass
{
  GtkBinClass parent_class;

  /* Action signals for keybindings. Do not connect to these signals
   */

  /* Unfortunately, GtkScrollType is deficient in that there is
   * no horizontal/vertical variants for GTK_SCROLL_START/END,
   * so we have to add an additional boolean flag.
   */
  gboolean (*scroll_child) (GtkScrolledSyncWindow *scrolled_sync_window,
	  		    GtkScrollType      scroll,
			    gboolean           horizontal);

  void (* move_focus_out) (GtkScrolledSyncWindow *scrolled_sync_window,
			   GtkDirectionType   direction);

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};


GType          gtk_scrolled_sync_window_get_type          (void) G_GNUC_CONST;
GtkWidget*     gtk_scrolled_sync_window_new               (GtkAdjustment     *hadjustment,
						      GtkAdjustment     *vadjustment);
void           gtk_scrolled_sync_window_set_hadjustment   (GtkScrolledSyncWindow *scrolled_sync_window,
						      GtkAdjustment     *hadjustment);
void           gtk_scrolled_sync_window_set_vadjustment   (GtkScrolledSyncWindow *scrolled_sync_window,
						      GtkAdjustment     *vadjustment);
GtkAdjustment* gtk_scrolled_sync_window_get_hadjustment   (GtkScrolledSyncWindow *scrolled_sync_window);
GtkAdjustment* gtk_scrolled_sync_window_get_vadjustment   (GtkScrolledSyncWindow *scrolled_sync_window);
//GDK_DEPRECATED_IN_3_8_FOR(gtk_container_add)
void	       gtk_scrolled_sync_window_add_with_viewport (GtkScrolledSyncWindow *scrolled_sync_window,
						      GtkWidget		*child);
void           gtk_scrolled_sync_window_set_kinetic_scrolling  (GtkScrolledSyncWindow        *scrolled_sync_window,
                                                           gboolean                  kinetic_scrolling);
gboolean       gtk_scrolled_sync_window_get_kinetic_scrolling  (GtkScrolledSyncWindow        *scrolled_sync_window);

void           gtk_scrolled_sync_window_set_capture_button_press (GtkScrolledSyncWindow      *scrolled_sync_window,
                                                             gboolean                capture_button_press);
gboolean       gtk_scrolled_sync_window_get_capture_button_press (GtkScrolledSyncWindow      *scrolled_sync_window);


G_END_DECLS


#endif /* __GTK_SCROLLED_SYNC_WINDOW_H__ */
