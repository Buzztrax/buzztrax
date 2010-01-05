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
 * SECTION:btmissingframeworkelementsdialog
 * @short_description: missing core and application elements
 *
 * A dialog to inform about missing core and application elements.
 */
/*
 * @todo: support gst-codec-install
 * Since gst-plugin-base-0.10.15 there is gst_install_plugins_supported().
 * If is supported we could do:
 *
 * GstInstallPluginsReturn result;
 * gchar *details[] = {
 *   gst_missing_element_installer_detail_new(factory-name1),
 *   NULL
 * };
 * result = gst_install_plugins_sync (details, NULL);
 * if (result == GST_INSTALL_PLUGINS_SUCCESS ||
 *     result == GST_INSTALL_PLUGINS_PARTIAL_SUCCESS) {
 *   gst_update_registry ();
 * }
*/
#define BT_EDIT
#define BT_MISSING_FRAMEWORK_ELEMENTS_DIALOG_C

#include "bt-edit.h"

//-- property ids

enum {
  MISSING_FRAMEWORK_ELEMENTS_DIALOG_APP=1,
  MISSING_FRAMEWORK_ELEMENTS_DIALOG_CORE_ELEMENTS,
  MISSING_FRAMEWORK_ELEMENTS_DIALOG_EDIT_ELEMENTS,
};

struct _BtMissingFrameworkElementsDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* list of missing elements */
  GList *core_elements, *edit_elements;

  GtkWidget *ignore_button;
};

static GtkDialogClass *parent_class=NULL;

//-- event handler

//-- helper methods

static void make_listview(GtkWidget *vbox,GList *missing_elements,const gchar *msg) {
  GtkWidget *label,*missing_list,*missing_list_view;
  GList *node;
  gchar *missing_text,*ptr;
  gint length=0;

  label=gtk_label_new(msg);
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,0);

  for(node=missing_elements;node;node=g_list_next(node)) {
    length+=2+strlen((gchar *)(node->data));
  }
  ptr=missing_text=g_malloc(length);
  for(node=missing_elements;node;node=g_list_next(node)) {
    length=g_sprintf(ptr,"%s\n",(gchar *)(node->data));
    ptr=&ptr[length];
  }
  ptr[-1]='\0'; // remove last '\n'

  missing_list = gtk_text_view_new();
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(missing_list), FALSE);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(missing_list), FALSE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(missing_list), GTK_WRAP_WORD);
  gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(missing_list)),missing_text,-1);
  gtk_widget_show(missing_list);
  g_free(missing_text);

  missing_list_view = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (missing_list_view), GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (missing_list_view), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(missing_list_view), missing_list);
  gtk_widget_show(missing_list_view);
  gtk_box_pack_start(GTK_BOX(vbox),missing_list_view,TRUE,TRUE,0);
}

static gboolean bt_missing_framework_elements_dialog_init_ui(const BtMissingFrameworkElementsDialog *self) {
  GtkWidget *label,*icon,*hbox,*vbox;
  gchar *str;
  gboolean res=TRUE;
  //GdkPixbuf *window_icon=NULL;

  gtk_widget_set_name(GTK_WIDGET(self),"Missing GStreamer elements");

  // create and set window icon
  /*
  if((window_icon=bt_ui_resources_get_icon_pixbuf_by_machine(self->priv->machine))) {
    gtk_window_set_icon(GTK_WINDOW(self),window_icon);
    g_object_unref(window_icon);
  }
  */

  // set a title
  gtk_window_set_title(GTK_WINDOW(self),_("Missing GStreamer elements"));

    // add dialog commision widgets (okay, cancel)
  gtk_dialog_add_buttons(GTK_DIALOG(self),
                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                          NULL);

  gtk_dialog_set_default_response(GTK_DIALOG(self),GTK_RESPONSE_ACCEPT);

  hbox=gtk_hbox_new(FALSE,12);
  gtk_container_set_border_width(GTK_CONTAINER(hbox),6);

  icon=gtk_image_new_from_stock(self->priv->core_elements?GTK_STOCK_DIALOG_ERROR:GTK_STOCK_DIALOG_WARNING,GTK_ICON_SIZE_DIALOG);
  gtk_container_add(GTK_CONTAINER(hbox),icon);

  vbox=gtk_vbox_new(FALSE,6);
  label=gtk_label_new(NULL);
  str=g_strdup_printf("<big><b>%s</b></big>",_("Missing GStreamer elements"));
  gtk_label_set_markup(GTK_LABEL(label),str);
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
  g_free(str);
  gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,0);
  if(self->priv->core_elements) {
    GST_DEBUG("%d missing core elements",g_list_length(self->priv->core_elements));
    make_listview(vbox,self->priv->core_elements,_("The elements listed below are missing from your installation, but are required."));
  }
  if(self->priv->edit_elements) {
    BtSettings *settings;
    gchar *machine_ignore_list;
    GList *edit_elements=NULL;

    GST_DEBUG("%d missing edit elements",g_list_length(self->priv->edit_elements));

    g_object_get(G_OBJECT(self->priv->app),"settings",&settings,NULL);
    g_object_get(G_OBJECT(settings),"missing-machines",&machine_ignore_list,NULL);
    g_object_unref(settings);

    if(machine_ignore_list) {
      GList *node;
      gchar *name;
      gboolean have_elements=FALSE;

      for(node=self->priv->edit_elements;node;node=g_list_next(node)) {
        name=(gchar *)(node->data);
        // if this is the message ("starts with "->") or is not in the ignored list, append
        if(name[0]=='-') {
          // if all elements between two messages are ignored, drop the message too
          if(have_elements) {
            edit_elements=g_list_append(edit_elements,node->data);
            have_elements=FALSE;
          }
        }
        else if(!strstr(machine_ignore_list,name)) {
          edit_elements=g_list_append(edit_elements,node->data);
          have_elements=TRUE;
        }
      }
      GST_DEBUG("filtered to %d missing edit elements",g_list_length(edit_elements));
    }
    else {
      edit_elements=self->priv->edit_elements;
    }

    if(edit_elements) {
      make_listview(vbox,edit_elements,_("The elements listed below are missing from your installation, but are recommended for full functionality."));

      self->priv->ignore_button=gtk_check_button_new_with_label(_("don't warn again"));
      gtk_box_pack_start(GTK_BOX(vbox),self->priv->ignore_button,FALSE,FALSE,0);

      if(machine_ignore_list) {
        g_list_free(edit_elements);
      }
    }
    else {
      // if we have only non-critical elements and ignore them already, don't show the dialog.
      if(!self->priv->core_elements) {
        GST_INFO("no new missing elements to show");
        res=FALSE;
      }
    }
  }
  gtk_container_add(GTK_CONTAINER(hbox),vbox);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(self)->vbox),hbox);

  return(res);
}

//-- constructor methods

/**
 * bt_missing_framework_elements_dialog_new:
 * @app: the application the dialog belongs to
 * @core_elements: list of missing core elements
 * @edit_elements: list of missing edit elements
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtMissingFrameworkElementsDialog *bt_missing_framework_elements_dialog_new(const BtEditApplication *app,GList *core_elements,GList *edit_elements) {
  BtMissingFrameworkElementsDialog *self;

  if(!(self=BT_MISSING_FRAMEWORK_ELEMENTS_DIALOG(g_object_new(BT_TYPE_MISSING_FRAMEWORK_ELEMENTS_DIALOG,"app",app,"core-elements",core_elements,"edit-elements",edit_elements,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_missing_framework_elements_dialog_init_ui(self)) {
    goto Error;
  }
  return(self);
Error:
  gtk_widget_destroy(GTK_WIDGET(self));
  return(NULL);
}

//-- methods

/**
 * bt_missing_framework_elements_dialog_apply:
 * @self: the dialog which settings to apply
 *
 * Makes the dialog settings effective.
 */
void bt_missing_framework_elements_dialog_apply(const BtMissingFrameworkElementsDialog *self) {

  if(self->priv->ignore_button && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->priv->ignore_button))) {
    BtSettings *settings;
    gchar *machine_ignore_list;
    GList *node,*edit_elements=NULL;
    gchar *ptr,*name;
    gint length=0,offset=0;

    g_object_get(G_OBJECT(self->priv->app),"settings",&settings,NULL);
    g_object_get(G_OBJECT(settings),"missing-machines",&machine_ignore_list,NULL);
    GST_INFO("was ignoring : [%s]",machine_ignore_list);

    if(machine_ignore_list) {
      offset=length=strlen(machine_ignore_list);
    }
    for(node=self->priv->edit_elements;node;node=g_list_next(node)) {
      name=(gchar *)(node->data);
      if(name[0]!='-') {
        if(!offset || !strstr(machine_ignore_list,name)) {
          edit_elements=g_list_append(edit_elements,node->data);
          length+=2+strlen((gchar *)(node->data));
        }
      }
    }
    GST_INFO("enlarging to %d bytes",length);
    machine_ignore_list=g_realloc(machine_ignore_list,length);
    ptr=&machine_ignore_list[offset];
    for(node=edit_elements;node;node=g_list_next(node)) {
      length=g_sprintf(ptr,"%s,",(gchar *)(node->data));
      ptr=&ptr[length];
    }
    g_list_free(edit_elements);
    GST_INFO("now ignoring : [%s]",machine_ignore_list);

    g_object_set(G_OBJECT(settings),"missing-machines",machine_ignore_list,NULL);
    g_object_unref(settings);
  }
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_missing_framework_elements_dialog_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMissingFrameworkElementsDialog *self = BT_MISSING_FRAMEWORK_ELEMENTS_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case MISSING_FRAMEWORK_ELEMENTS_DIALOG_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    case MISSING_FRAMEWORK_ELEMENTS_DIALOG_CORE_ELEMENTS: {
      g_value_set_pointer(value, self->priv->core_elements);
    } break;
    case MISSING_FRAMEWORK_ELEMENTS_DIALOG_EDIT_ELEMENTS: {
      g_value_set_pointer(value, self->priv->edit_elements);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_missing_framework_elements_dialog_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMissingFrameworkElementsDialog *self = BT_MISSING_FRAMEWORK_ELEMENTS_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case MISSING_FRAMEWORK_ELEMENTS_DIALOG_APP: {
      g_object_try_unref(self->priv->app);
      self->priv->app = g_object_try_ref(g_value_get_object(value));
      GST_DEBUG("set the app for missing_framework_elements_dialog: %p",self->priv->app);
    } break;
    case MISSING_FRAMEWORK_ELEMENTS_DIALOG_CORE_ELEMENTS: {
      self->priv->core_elements = g_value_get_pointer(value);
    } break;
    case MISSING_FRAMEWORK_ELEMENTS_DIALOG_EDIT_ELEMENTS: {
      self->priv->edit_elements = g_value_get_pointer(value);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_missing_framework_elements_dialog_dispose(GObject *object) {
  BtMissingFrameworkElementsDialog *self = BT_MISSING_FRAMEWORK_ELEMENTS_DIALOG(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  g_object_try_unref(self->priv->app);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_missing_framework_elements_dialog_finalize(GObject *object) {
  BtMissingFrameworkElementsDialog *self = BT_MISSING_FRAMEWORK_ELEMENTS_DIALOG(object);

  GST_DEBUG("!!!! self=%p",self);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_missing_framework_elements_dialog_init(GTypeInstance *instance, gpointer g_class) {
  BtMissingFrameworkElementsDialog *self = BT_MISSING_FRAMEWORK_ELEMENTS_DIALOG(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_MISSING_FRAMEWORK_ELEMENTS_DIALOG, BtMissingFrameworkElementsDialogPrivate);
}

static void bt_missing_framework_elements_dialog_class_init(BtMissingFrameworkElementsDialogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtMissingFrameworkElementsDialogPrivate));

  gobject_class->set_property = bt_missing_framework_elements_dialog_set_property;
  gobject_class->get_property = bt_missing_framework_elements_dialog_get_property;
  gobject_class->dispose      = bt_missing_framework_elements_dialog_dispose;
  gobject_class->finalize     = bt_missing_framework_elements_dialog_finalize;

  g_object_class_install_property(gobject_class,MISSING_FRAMEWORK_ELEMENTS_DIALOG_APP,
                                  g_param_spec_object("app",
                                     "app construct prop",
                                     "Set application object, the dialog belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,MISSING_FRAMEWORK_ELEMENTS_DIALOG_CORE_ELEMENTS,
                                  g_param_spec_pointer("core-elements",
                                     "core-elements construct prop",
                                     "Set missing core-elements list, the dialog handles",
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,MISSING_FRAMEWORK_ELEMENTS_DIALOG_EDIT_ELEMENTS,
                                  g_param_spec_pointer("edit-elements",
                                     "edit-elements construct prop",
                                     "Set missing edit-elements list, the dialog handles",
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
}

GType bt_missing_framework_elements_dialog_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof (BtMissingFrameworkElementsDialogClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_missing_framework_elements_dialog_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtMissingFrameworkElementsDialog),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_missing_framework_elements_dialog_init, // instance_init
    };
    type = g_type_register_static(GTK_TYPE_DIALOG,"BtMissingFrameworkElementsDialog",&info,0);
  }
  return type;
}
