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

enum {
  ABOUT_DIALOG_APP=1
};

struct _BtAboutDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  G_POINTER_ALIAS(BtEditApplication *,app);
};

static GtkDialogClass *parent_class=NULL;

//-- event handler

static void on_about_dialog_url_clicked(GtkAboutDialog *about, const gchar *link, gpointer user_data) {
#if GTK_CHECK_VERSION(2,14,0)
  GError *error=NULL;

  if(!gtk_show_uri(gtk_widget_get_screen (GTK_WIDGET(about)),link,gtk_get_current_event_time(),&error)) {
    GST_WARNING("Failed to display url: %s\n",error->message);
    g_error_free(error);
  }
#else
  gnome_vfs_url_show(link);
#endif
}

//-- helper methods

static gboolean bt_about_dialog_init_ui(const BtAboutDialog *self) {
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

  g_object_set(G_OBJECT(self),
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

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(self)->vbox), news_view, TRUE, TRUE, 0);

  gtk_dialog_set_has_separator(GTK_DIALOG(self), TRUE);

  return(TRUE);
}

//-- constructor methods

/**
 * bt_about_dialog_new:
 * @app: the application the dialog belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtAboutDialog *bt_about_dialog_new(const BtEditApplication *app) {
  BtAboutDialog *self;

  if(!(self=BT_ABOUT_DIALOG(g_object_new(BT_TYPE_ABOUT_DIALOG,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_about_dialog_init_ui(self)) {
    goto Error;
  }
  return(self);
Error:
  gtk_widget_destroy(GTK_WIDGET(self));
  return(NULL);
}

//-- methods

//-- wrapper

//-- class internals

/* sets the given properties for this object */
static void bt_about_dialog_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtAboutDialog *self = BT_ABOUT_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case ABOUT_DIALOG_APP: {
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->app);
      //GST_DEBUG("set the app for about_dialog: %p",self->priv->app);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_about_dialog_dispose(GObject *object) {
  BtAboutDialog *self = BT_ABOUT_DIALOG(object);
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  g_object_try_weak_unref(self->priv->app);

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_about_dialog_finalize(GObject *object) {
  //BtAboutDialog *self = BT_ABOUT_DIALOG(object);

  //GST_DEBUG("!!!! self=%p",self);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_about_dialog_init(GTypeInstance *instance, gpointer g_class) {
  BtAboutDialog *self = BT_ABOUT_DIALOG(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_ABOUT_DIALOG, BtAboutDialogPrivate);
}

static void bt_about_dialog_class_init(BtAboutDialogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtAboutDialogPrivate));

  gobject_class->set_property = bt_about_dialog_set_property;
  gobject_class->dispose      = bt_about_dialog_dispose;
  gobject_class->finalize     = bt_about_dialog_finalize;

  g_object_class_install_property(gobject_class,ABOUT_DIALOG_APP,
                                  g_param_spec_object("app",
                                     "app construct prop",
                                     "Set application object, the dialog belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_WRITABLE|G_PARAM_STATIC_STRINGS));
}

GType bt_about_dialog_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtAboutDialogClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_about_dialog_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtAboutDialog),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_about_dialog_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(GTK_TYPE_ABOUT_DIALOG,"BtAboutDialog",&info,0);
  }
  return type;
}
