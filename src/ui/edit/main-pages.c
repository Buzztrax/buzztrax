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
#include "main-page-sequence.h"
#include "sequence-view.h"

enum
{
  MAIN_PAGES_MACHINES_PAGE = 1,
  MAIN_PAGES_PATTERNS_PAGE,
  MAIN_PAGES_SEQUENCE_PAGE,
  MAIN_PAGES_WAVES_PAGE,
  MAIN_PAGES_INFO_PAGE
};


struct _BtMainPages
{
  GtkWidget parent;
  
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

  GtkWidget *toolbar_view;
};

//-- the class

G_DEFINE_TYPE (BtMainPages, bt_main_pages, GTK_TYPE_WIDGET);


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
  g_object_get (self->app, "song", &song, NULL);
  if (!song) {
    return;
  }
  // restore page
  bt_child_proxy_get (song, "setup::properties", &properties, NULL);
  if ((prop = (gchar *) g_hash_table_lookup (properties, "active-page"))) {
    guint page = atoi (prop);

    /// GTK4 gtk_notebook_set_current_page (self->notebook, page);
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
  g_object_get (self->app, "song", &song, NULL);
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
bt_main_pages_init_ui (const BtMainPages * self)
{
  // register event handlers
  g_signal_connect_object (self->app, "notify::song",
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
  return_if_disposed_self ();
  switch (property_id) {
    case MAIN_PAGES_MACHINES_PAGE:
      g_value_set_object (value, self->machines_page);
      break;
    case MAIN_PAGES_PATTERNS_PAGE:
      g_value_set_object (value, self->patterns_page);
      break;
    case MAIN_PAGES_SEQUENCE_PAGE:
      g_value_set_object (value, self->sequence_page);
      break;
    case MAIN_PAGES_WAVES_PAGE:
      g_value_set_object (value, self->waves_page);
      break;
    case MAIN_PAGES_INFO_PAGE:
      g_value_set_object (value, self->info_page);
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

  gtk_widget_dispose_template (GTK_WIDGET (self), BT_TYPE_MAIN_PAGES);
  
  return_if_disposed_self ();
  self->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_object_unref (self->app);

  // this disposes the pages for us
  G_OBJECT_CLASS (bt_main_pages_parent_class)->dispose (object);
}

static void
bt_main_pages_init (BtMainPages * self)
{
  /* Refer to
   * https://developer.gnome.org/documentation/tutorials/widget-templates.html
   * regarding need for g_type_ensure when using custom widget types in UI
   * builder template files.
   */
  g_type_ensure (BT_TYPE_MAIN_PAGE_INFO);
  g_type_ensure (BT_TYPE_MAIN_PAGE_MACHINES);
  g_type_ensure (BT_TYPE_MAIN_PAGE_PATTERNS);
  g_type_ensure (BT_TYPE_MAIN_PAGE_SEQUENCE);
  g_type_ensure (BT_TYPE_MAIN_PAGE_WAVES);
  g_type_ensure (BT_TYPE_SEQUENCE_VIEW);
  
  gtk_widget_init_template (GTK_WIDGET (self));
  
  self = bt_main_pages_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->app = bt_edit_application_new ();

  bt_main_page_machines_set_pages_parent (self->machines_page, self);
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

  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class,
      "/org/buzztrax/ui/main-pages.ui");
  
  gtk_widget_class_bind_template_child (widget_class, BtMainPages, machines_page);
  gtk_widget_class_bind_template_child (widget_class, BtMainPages, patterns_page);
  gtk_widget_class_bind_template_child (widget_class, BtMainPages, sequence_page);
  gtk_widget_class_bind_template_child (widget_class, BtMainPages, waves_page);
  gtk_widget_class_bind_template_child (widget_class, BtMainPages, info_page);
}
