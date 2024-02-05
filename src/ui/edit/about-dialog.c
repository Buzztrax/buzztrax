/* Buzztrax
 * Copyright (C) 2007 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
/**
 * SECTION:btaboutdialog
 * @short_description: class for the editor about dialog
 *
 * Info about the project, people involved, license and so on.
 */

#define BT_EDIT
#define BT_ABOUT_DIALOG_C

#include "bt-edit.h"

/**
 * bt_about_dialog_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
GtkAboutDialog *
bt_about_dialog_new (BtEditApplication* app)
{
  GtkWidget *self;

  self = gtk_about_dialog_new ();
  
  GtkWidget *news, *news_view;
  const gchar *authors[] = {
#include "authors.h"
    NULL
  };
  const gchar *documenters[] = {
    "Stefan Sauer <ensonic@users.sf.net>",
    NULL
  };
  const gchar *artists[] = {
    "Marc 'marcbroe' Broekhuis <marcbroe@gmail.com>",
    "Jakub Steiner <jimmac@ximian.com>",
    NULL
  };
  /* Note to translators: put here your name and email so it will show up in
   * the "about" box. Example: Stefan 'ensonic' Sauer <ensonic@users.sf.net> */
  gchar *translators = _("translator-credits");

  gint len = strlen (_("Copyright \xc2\xa9 2003-%d Buzztrax developer team")) + 3;
  gchar *copyright = g_alloca (len);
  snprintf (copyright, len, _("Copyright \xc2\xa9 2003-%d Buzztrax developer team"),
      BT_RELEASE_YEAR);

  /* we can get logo via icon name, so this here is just for educational purpose
     GdkPixbuf *logo;
     GError *error = NULL;
     logo=gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),"buzztrax",48,0,&error);
     //logo = gdk_pixbuf_new_from_file(DATADIR""G_DIR_SEPARATOR_S"icons"G_DIR_SEPARATOR_S"hicolor"G_DIR_SEPARATOR_S"48x48"G_DIR_SEPARATOR_S"apps"G_DIR_SEPARATOR_S""PACKAGE".png",&error);
     if(!logo) {
     GST_WARNING("Couldn't load icon: %s", error->message);
     g_error_free(error);
     }
   */

  g_object_set ((gpointer) self,
      "artists", artists,
      "authors", authors,
      "comments", _("Music production environment"),
      "copyright", copyright, "documenters", documenters,
      /* http://www.gnu.org/licenses/lgpl.html, http://www.gnu.de/lgpl-ger.html */
      "license",
      _("This package is free software; you can redistribute it and/or "
          "modify it under the terms of the GNU Library General Public "
          "License as published by the Free Software Foundation; either "
          "version 2.1 of the License, or (at your option) any later version."
          "\n\n"
          "This package is distributed in the hope that it will be useful, "
          "but WITHOUT ANY WARRANTY; without even the implied warranty of "
          "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU "
          "Library General Public License for more details." "\n\n"
          "You should have received a copy of the GNU Library General Public "
          "License along with this package; if not, write to the "
          "Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, "
          "Boston, MA  02110-1301  USA."), "logo-icon-name", PACKAGE_NAME,
      "translator-credits", (!strcmp (translators,
              "translator-credits")) ? translators : NULL, "version",
      PACKAGE_VERSION, "website", "http://www.buzztrax.org", "wrap-license",
      TRUE, NULL);

  // add the NEWS directly below copyright
  news = gtk_text_view_new ();
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (news), FALSE);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (news), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (news), GTK_WRAP_WORD);
  gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (news)),
      /* Note to translators: We will send announcements of pre-release tarballs
       * roughly one month before the final release. We collect feedback from
       * beta testers during this period and do smaller changes, but take care
       * to not change translated strings. */
      "Development version (do not translate this)"
      /*_
      ("")
      */
      , -1);

  news_view = gtk_scrolled_window_new ();
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (news_view),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (news_view), GTK_WIDGET (news));
  
  return GTK_ABOUT_DIALOG (self);
}

