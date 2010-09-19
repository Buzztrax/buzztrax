/* $Id$
 *
 * Buzztard
 * Copyright (C) 2007 Buzztard team <buzztard-devel@lists.sf.net>
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
 * SECTION:btaboutdialog
 * @short_description: class for the editor about dialog
 *
 * Info about the project, people involved, license and so on.
 */

#define BT_EDIT
#define BT_ABOUT_DIALOG_C

#include "bt-edit.h"

struct _BtAboutDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;
};

//-- the class

G_DEFINE_TYPE (BtAboutDialog, bt_about_dialog, GTK_TYPE_ABOUT_DIALOG);

//-- event handler

static void on_about_dialog_url_clicked(GtkAboutDialog *about, const gchar *link, gpointer user_data) {
  gtk_show_uri_simple(GTK_WIDGET(about),link);
}

//-- helper methods

static void bt_about_dialog_init_ui(const BtAboutDialog *self) {
  GtkWidget *news,*news_view;
  const gchar *authors[] = {
    "Stefan 'ensonic' Kost <ensonic@users.sf.net>",
    "Krzysztof Foltman <wdev@foltman.com>",
    "Thomas 'waffel' Wabner <waffel@users.sf.net>",
    "Patric Schmitz  <berzerka@users.sf.net>",
    NULL
  };
  const gchar *documenters[] = {
    "Stefan 'ensonic' Kost <ensonic@users.sf.net>",
    NULL
  };
  const gchar *artists[] = {
    "Marc 'marcbroe' Broekhuis <marcbroe@gmail.com>",
    "Jakub Steiner <jimmac@ximian.com>",
    NULL
  };
  /* Note to translators: put here your name and email so it will show up in
   * the "about" box. Example: Stefan 'ensonic' Kost <ensonic@users.sf.net> */
  gchar *translators =_("translator-credits");
  
  gchar *copyright = g_alloca(strlen(_("Copyright \xc2\xa9 2003-%d Buzztard developer team"))+3);
  sprintf(copyright,_("Copyright \xc2\xa9 2003-%d Buzztard developer team"),2009);

  /* we can get logo via icon name, so this here is just for educational purpose
  GdkPixbuf *logo;
  GError *error = NULL;
  logo=gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),"buzztard",48,0,&error);
  //logo = gdk_pixbuf_new_from_file(DATADIR""G_DIR_SEPARATOR_S"icons"G_DIR_SEPARATOR_S"hicolor"G_DIR_SEPARATOR_S"48x48"G_DIR_SEPARATOR_S"apps"G_DIR_SEPARATOR_S""PACKAGE".png",&error);
  if(!logo) {
    GST_WARNING("Couldn't load icon: %s", error->message);
    g_error_free(error);
  }
  */

  g_object_set((gpointer)self,
    "artists",artists,
    "authors",authors,
    "comments",_("Music production environment"),
    "copyright",copyright,
    "documenters", documenters,
    /* http://www.gnu.org/licenses/lgpl.html, http://www.gnu.de/lgpl-ger.html */
    "license", _(
      "This package is free software; you can redistribute it and/or "
      "modify it under the terms of the GNU Library General Public "
      "License as published by the Free Software Foundation; either "
      "version 2 of the License, or (at your option) any later version."
      "\n\n"
      "This package is distributed in the hope that it will be useful, "
      "but WITHOUT ANY WARRANTY; without even the implied warranty of "
      "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU "
      "Library General Public License for more details."
      "\n\n"
      "You should have received a copy of the GNU Library General Public "
      "License along with this package; if not, write to the "
      "Free Software Foundation, Inc., 59 Temple Place - Suite 330, "
      "Boston, MA 02111-1307, USA."
    ),
    "logo-icon-name",PACKAGE_NAME,
    "translator-credits",(!strcmp(translators,"translator-credits"))?translators:NULL,
    "version", PACKAGE_VERSION,
    "website","http://www.buzztard.org",
    "wrap-license",TRUE,
    NULL);

  // install hooks for hyper-links
  gtk_about_dialog_set_email_hook(on_about_dialog_url_clicked, (gpointer)self, NULL);
  gtk_about_dialog_set_url_hook(on_about_dialog_url_clicked, (gpointer)self, NULL);

  // add the NEWS directly below copyright
  news = gtk_text_view_new();
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(news), FALSE);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(news), FALSE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(news), GTK_WRAP_WORD);
  gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(news)),
    _("Development version (do not translate this)"
    //_(""
    ),-1);

  news_view = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (news_view), GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (news_view), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(news_view), news);

  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(self))), news_view, TRUE, TRUE, 0);

  gtk_dialog_set_has_separator(GTK_DIALOG(self), TRUE);
}

//-- constructor methods

/**
 * bt_about_dialog_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtAboutDialog *bt_about_dialog_new(void) {
  BtAboutDialog *self;

  self=BT_ABOUT_DIALOG(g_object_new(BT_TYPE_ABOUT_DIALOG,NULL));
  bt_about_dialog_init_ui(self);
  return(self);
}

//-- methods

//-- wrapper

//-- class internals

static void bt_about_dialog_dispose(GObject *object) {
  BtAboutDialog *self = BT_ABOUT_DIALOG(object);
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  g_object_unref(self->priv->app);

  G_OBJECT_CLASS(bt_about_dialog_parent_class)->dispose(object);
}

static void bt_about_dialog_init(BtAboutDialog *self) {
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_ABOUT_DIALOG, BtAboutDialogPrivate);
  GST_DEBUG("!!!! self=%p",self);
  self->priv->app = bt_edit_application_new();
}

static void bt_about_dialog_class_init(BtAboutDialogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  g_type_class_add_private(klass,sizeof(BtAboutDialogPrivate));

  gobject_class->dispose      = bt_about_dialog_dispose;
}

