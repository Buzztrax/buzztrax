/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:btmainpages
 * @short_description: class for the editor main pages
 *
 * The user interface of the buzztrax editor is divided into several pages.
 * This class implements the notebook widgets to manage the sub-pages:
 * #BtMainPageMachines, #BtMainPagePatterns, #BtMainPageSequence,
 * #BtMainPageWaves and #BtMainPageInfo.
 */

#define BT_EDIT
#define BT_MAIN_PAGES_C

#include "bt-edit.h"

enum
{
  MAIN_PAGES_MACHINES_PAGE = 1,
  MAIN_PAGES_PATTERNS_PAGE,
  MAIN_PAGES_SEQUENCE_PAGE,
  MAIN_PAGES_WAVES_PAGE,
  MAIN_PAGES_INFO_PAGE
};


struct _BtMainPagesPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the machines tab */
  BtMainPageMachines *machines_page;
  /* the patterns tab */
  BtMainPagePatterns *patterns_page;
  /* the sequence tab */
  BtMainPageSequence *sequence_page;
  /* the waves tab */
  BtMainPageWaves *waves_page;
  /* the information tab */
  BtMainPageInfo *info_page;
};

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtMainPages, bt_main_pages, GTK_TYPE_NOTEBOOK, 
    G_ADD_PRIVATE(BtMainPages));


//-- event handler

static void
on_song_changed (const BtEditApplication * app, GParamSpec * arg,
    gpointer user_data)
{
  BtMainPages *self = BT_MAIN_PAGES (user_data);
  BtSong *song;
  GHashTable *properties;
  gchar *prop;

  GST_INFO ("song has changed : app=%p, self=%p", app, self);
  // get song from app
  g_object_get (self->priv->app, "song", &song, NULL);
  if (!song) {
    return;
  }
  // restore page
  bt_child_proxy_get (song, "setup::properties", &properties, NULL);
  if ((prop = (gchar *) g_hash_table_lookup (properties, "active-page"))) {
    guint page = atoi (prop);

    gtk_notebook_set_current_page (GTK_NOTEBOOK (self), page);
    GST_INFO ("reactivate page %d", page);
  }
  // release the reference
  g_object_unref (song);
  GST_INFO ("song has changed done");
}

static void
on_page_switched (GtkNotebook * notebook, GParamSpec * arg, gpointer user_data)
{
  BtMainPages *self = BT_MAIN_PAGES (user_data);
  BtSong *song;
  GHashTable *properties;
  GtkWidget *page;
  gchar *prop;
  guint page_num;

  // get objects
  g_object_get (self->priv->app, "song", &song, NULL);
  if (!song) {
    return;
  }

  g_object_get (notebook, "page", &page_num, NULL);
  GST_INFO ("page has switched : self=%p, page=%d", self, page_num);

  // ensure the new page gets focused, this sounds like a hack though
  page = gtk_notebook_get_nth_page (notebook, page_num);
  GTK_WIDGET_GET_CLASS (page)->focus (page, GTK_DIR_TAB_FORWARD);

  // remember page
  bt_child_proxy_get (song, "setup::properties", &properties, NULL);
  prop = g_strdup_printf ("%u", page_num);
  g_hash_table_insert (properties, g_strdup ("active-page"), prop);

  // release the reference
  g_object_unref (song);
  GST_INFO ("page-switched done");
}

//-- helper methods

static void
bt_main_pages_add_tab (const BtMainPages * self, GtkWidget * content,
    gchar * str, gchar * icon, gchar * tip)
{
  GtkWidget *label, *event_box, *box, *image;

  label = gtk_label_new (str);
  //gtk_label_set_ellipsize(GTK_LABEL(label),PANGO_ELLIPSIZE_END);
  gtk_widget_set_name (label, str);
  gtk_widget_show (label);

  image = gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_MENU);
  gtk_widget_show (image);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BOX_BORDER);
  gtk_widget_show (box);
  //gtk_box_pack_start(GTK_BOX(box),image,FALSE,FALSE,0);
  //gtk_box_pack_start(GTK_BOX(box),label,TRUE,FALSE,0);
  gtk_container_add (GTK_CONTAINER (box), image);
  gtk_container_add (GTK_CONTAINER (box), label);

  event_box = gtk_event_box_new ();
  g_object_set (event_box, "visible-window", FALSE, NULL);
  gtk_container_add (GTK_CONTAINER (event_box), box);

  gtk_widget_set_tooltip_text (event_box, tip);

  gtk_notebook_insert_page (GTK_NOTEBOOK (self), content, event_box, -1);
}

static void
bt_main_pages_init_ui (const BtMainPages * self)
{
  gtk_widget_set_name (GTK_WIDGET (self), "song views");

  GST_INFO ("before creating pages, app: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self->priv->app));

  // don't emit notify::page for each add
  g_object_freeze_notify ((GObject *) self);

  if (!BT_EDIT_UI_CONFIG ("no-machines-page")) {
    // add wigets for machine view
    self->priv->machines_page = bt_main_page_machines_new (self);
    bt_main_pages_add_tab (self, GTK_WIDGET (self->priv->machines_page),
        _("machines"), "buzztrax_tab_machines",
        _("machines used in the song and their wires"));
  }
  if (!BT_EDIT_UI_CONFIG ("no-patterns-page")) {
    // add wigets for pattern view
    self->priv->patterns_page = bt_main_page_patterns_new (self);
    bt_main_pages_add_tab (self, GTK_WIDGET (self->priv->patterns_page),
        _("patterns"), "buzztrax_tab_patterns", _("event pattern editor"));
  }
  if (!BT_EDIT_UI_CONFIG ("no-sequence-page")) {
    // add wigets for sequence view
    self->priv->sequence_page = bt_main_page_sequence_new (self);
    bt_main_pages_add_tab (self, GTK_WIDGET (self->priv->sequence_page),
        _("sequence"), "buzztrax_tab_sequence", _("song sequence editor"));
  }
  if (!BT_EDIT_UI_CONFIG ("no-wavetable-page")) {
    // add wigets for waves view
    self->priv->waves_page = bt_main_page_waves_new (self);
    bt_main_pages_add_tab (self, GTK_WIDGET (self->priv->waves_page),
        _("wave table"), "buzztrax_tab_waves", _("sample wave table editor"));
  }
  if (!BT_EDIT_UI_CONFIG ("no-info-page")) {
    // add widgets for song info view
    self->priv->info_page = bt_main_page_info_new (self);
    bt_main_pages_add_tab (self, GTK_WIDGET (self->priv->info_page),
        _("information"), "buzztrax_tab_info", _("song meta data editor"));
  }
  // @idea add widgets for machine help view
  // GTK_STOCK_HELP icon
  // embed mozilla/gtk-html/webkit-gtk

  g_object_thaw_notify ((GObject *) self);

  // register event handlers
  g_signal_connect_object (self->priv->app, "notify::song",
      G_CALLBACK (on_song_changed), (gpointer) self, 0);
  g_signal_connect ((gpointer) self, "notify::page",
      G_CALLBACK (on_page_switched), (gpointer) self);

  GST_DEBUG ("  done");
}

//-- constructor methods

/**
 * bt_main_pages_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtMainPages *
bt_main_pages_new (void)
{
  BtMainPages *self;

  self = BT_MAIN_PAGES (g_object_new (BT_TYPE_MAIN_PAGES, NULL));
  bt_main_pages_init_ui (self);
  return self;
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_main_pages_get_property (GObject * object, guint property_id, GValue * value,
    GParamSpec * pspec)
{
  BtMainPages *self = BT_MAIN_PAGES (object);
  return_if_disposed ();
  switch (property_id) {
    case MAIN_PAGES_MACHINES_PAGE:
      g_value_set_object (value, self->priv->machines_page);
      break;
    case MAIN_PAGES_PATTERNS_PAGE:
      g_value_set_object (value, self->priv->patterns_page);
      break;
    case MAIN_PAGES_SEQUENCE_PAGE:
      g_value_set_object (value, self->priv->sequence_page);
      break;
    case MAIN_PAGES_WAVES_PAGE:
      g_value_set_object (value, self->priv->waves_page);
      break;
    case MAIN_PAGES_INFO_PAGE:
      g_value_set_object (value, self->priv->info_page);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_main_pages_dispose (GObject * object)
{
  BtMainPages *self = BT_MAIN_PAGES (object);
  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_object_unref (self->priv->app);
  // this disposes the pages for us
  G_OBJECT_CLASS (bt_main_pages_parent_class)->dispose (object);
}

static void
bt_main_pages_init (BtMainPages * self)
{
  self->priv = bt_main_pages_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
}

static void
bt_main_pages_class_init (BtMainPagesClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = bt_main_pages_get_property;
  gobject_class->dispose = bt_main_pages_dispose;

  g_object_class_install_property (gobject_class, MAIN_PAGES_MACHINES_PAGE,
      g_param_spec_object ("machines-page", "machines page prop",
          "the machines view page", BT_TYPE_MAIN_PAGE_MACHINES,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, MAIN_PAGES_PATTERNS_PAGE,
      g_param_spec_object ("patterns-page", "patterns page prop",
          "the patterns view page", BT_TYPE_MAIN_PAGE_PATTERNS,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, MAIN_PAGES_SEQUENCE_PAGE,
      g_param_spec_object ("sequence-page", "sequence page prop",
          "the sequence view page", BT_TYPE_MAIN_PAGE_SEQUENCE,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, MAIN_PAGES_WAVES_PAGE,
      g_param_spec_object ("waves-page", "waves page prop",
          "the waves view page", BT_TYPE_MAIN_PAGE_WAVES,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, MAIN_PAGES_INFO_PAGE,
      g_param_spec_object ("info-page", "info page prop", "the info view page",
          BT_TYPE_MAIN_PAGE_INFO, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}
