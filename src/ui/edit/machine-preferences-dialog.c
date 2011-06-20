/* $Id$
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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
 * SECTION:btmachinepreferencesdialog
 * @short_description: machine non-realtime parameters
 * @see_also: #BtMachine
 *
 * A dialog to configure static settings of a #BtMachine.
 */

/* @todo: filter certain properties (tempo iface, ...)
 *
 * @todo: we have a few notifies, but not for all types
 * - do we need them at all? who else could change things?
 *
 * @todo: we should only be able to have one preferences for each machien type
 *
 * @todo: save the chosen settings somewhere
 * - gconf would need some sort of schema
 */

#define BT_EDIT
#define BT_MACHINE_PREFERENCES_DIALOG_C

#include "bt-edit.h"

//-- property ids

enum {
  MACHINE_PREFERENCES_DIALOG_MACHINE=1
};

struct _BtMachinePreferencesDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the underlying machine */
  BtMachine *machine;
};

static GQuark widget_parent_quark=0;

//-- the class

G_DEFINE_TYPE (BtMachinePreferencesDialog, bt_machine_preferences_dialog, GTK_TYPE_WINDOW);


//-- event handler helper

static void mark_song_as_changed(const BtMachinePreferencesDialog *self) {
  BtSong *song;

  g_object_get(self->priv->app,"song",&song,NULL);
  bt_song_set_unsaved(song,TRUE);
  g_object_unref(song);
}

//-- event handler

static void on_range_property_notify(const GstElement *machine,GParamSpec *property,gpointer user_data) {
  GtkWidget *widget=GTK_WIDGET(user_data);
  gdouble value;

  //GST_INFO("preferences value notify received for: '%s'",property->name);

  g_object_get((gpointer)machine,property->name,&value,NULL);
  gtk_range_set_value(GTK_RANGE(widget),value);
}

static void on_double_entry_property_notify(const GstElement *machine,GParamSpec *property,gpointer user_data) {
  GtkWidget *widget=GTK_WIDGET(user_data);
  gdouble value;
  gchar *str_value;

  //GST_INFO("preferences value notify received for: '%s'",property->name);

  g_object_get((gpointer)machine,property->name,&value,NULL);
  str_value=g_strdup_printf("%7.2lf",value);
  gtk_entry_set_text(GTK_ENTRY(widget),str_value);
  g_free(str_value);
}

static void on_combobox_property_notify(const GstElement *machine,GParamSpec *property,gpointer user_data) {
  GtkWidget *widget=GTK_WIDGET(user_data);
  gint ivalue,nvalue;
  GtkTreeModel *store;
  GtkTreeIter iter;

  g_object_get((gpointer)machine,property->name,&nvalue,NULL);
  store=gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
  gtk_tree_model_get_iter_first(store,&iter);
  do {
    gtk_tree_model_get(store,&iter,0,&ivalue,-1);
    if(ivalue==nvalue) break;
  } while(gtk_tree_model_iter_next(store,&iter));

  gtk_combo_box_set_active_iter(GTK_COMBO_BOX(widget),&iter);
}


static void on_entry_property_changed(GtkEditable *editable,gpointer user_data) {
  GstElement *machine=GST_ELEMENT(user_data);
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(editable));
  BtMachinePreferencesDialog *self=BT_MACHINE_PREFERENCES_DIALOG(g_object_get_qdata(G_OBJECT(editable),widget_parent_quark));

  //GST_INFO("preferences value change received for: '%s'",name);
  g_object_set(machine,name,gtk_entry_get_text(GTK_ENTRY(editable)),NULL);
  mark_song_as_changed(self);
}

static void on_checkbox_property_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
  GstElement *machine=GST_ELEMENT(user_data);
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(togglebutton));
  BtMachinePreferencesDialog *self=BT_MACHINE_PREFERENCES_DIALOG(g_object_get_qdata(G_OBJECT(togglebutton),widget_parent_quark));

  //GST_INFO("preferences value change received for: '%s'",name);
  g_object_set(machine,name,gtk_toggle_button_get_active(togglebutton),NULL);
  mark_song_as_changed(self);
}

static void on_range_property_changed(GtkRange *range,gpointer user_data) {
  GstElement *machine=GST_ELEMENT(user_data);
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(range));
  BtMachinePreferencesDialog *self=BT_MACHINE_PREFERENCES_DIALOG(g_object_get_qdata(G_OBJECT(range),widget_parent_quark));

  //GST_INFO("preferences value change received for: '%s'",name);
  g_object_set(machine,name,gtk_range_get_value(range),NULL);
  mark_song_as_changed(self);
}

static void on_double_entry_property_changed(GtkEditable *editable,gpointer user_data) {
  GstElement *machine=GST_ELEMENT(user_data);
  gdouble value;
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(editable));
  BtMachinePreferencesDialog *self=BT_MACHINE_PREFERENCES_DIALOG(g_object_get_qdata(G_OBJECT(editable),widget_parent_quark));

  //GST_INFO("preferences value change received for: '%s'",name);
  value=g_ascii_strtod(gtk_entry_get_text(GTK_ENTRY(editable)),NULL);
  g_object_set(machine,name,value,NULL);
  mark_song_as_changed(self);
}

static void on_spinbutton_property_changed(GtkSpinButton *spinbutton,gpointer user_data) {
  GstElement *machine=GST_ELEMENT(user_data);
  gint value;
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(spinbutton));
  BtMachinePreferencesDialog *self=BT_MACHINE_PREFERENCES_DIALOG(g_object_get_qdata(G_OBJECT(spinbutton),widget_parent_quark));

  GST_INFO("preferences value change received for: '%s'",name);
  value=gtk_spin_button_get_value_as_int(spinbutton);
  g_object_set(machine,name,value,NULL);
  mark_song_as_changed(self);
}

static void on_combobox_property_changed(GtkComboBox *combobox, gpointer user_data) {
  GstElement *machine=GST_ELEMENT(user_data);
  gint value;
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(combobox));
  BtMachinePreferencesDialog *self=BT_MACHINE_PREFERENCES_DIALOG(g_object_get_qdata(G_OBJECT(combobox),widget_parent_quark));
  GtkTreeModel *store;
  GtkTreeIter iter;

  GST_INFO("preferences value change received for: '%s'",name);
  //value=gtk_combo_box_get_active(combobox);
  store=gtk_combo_box_get_model(combobox);
  if(gtk_combo_box_get_active_iter(combobox,&iter)) {
    gtk_tree_model_get(store,&iter,0,&value,-1);
    g_object_set(machine,name,value,NULL);
  }
  mark_song_as_changed(self);
}

/*
 * on_table_size_request:
 *
 * we adjust the scrollable-window size to contain the whole area
 */
static void on_table_size_request(GtkWidget *widget,GtkRequisition *requisition,gpointer user_data) {
  //BtMachinePreferencesDialog *self=BT_MACHINE_PREFERENCES_DIALOG(user_data);
  GtkWidget *parent=gtk_widget_get_parent(gtk_widget_get_parent(widget));
  gint height=requisition->height,width=-1;
  gint max_height=gdk_screen_get_height(gdk_screen_get_default());
  gint available_heigth;

  GST_DEBUG("#### table  size req %d x %d (max-height=%d)", requisition->width,requisition->height,max_height);
  // have a minimum width
  if(requisition->width<250) {
    width=250;
  }
  // constrain the height by screen height minus some space for panels and deco
  available_heigth=max_height-SCREEN_BORDER_HEIGHT;
  if(height>available_heigth) {
    height=available_heigth;
  }
  // @todo: is the '2' some border or padding
  gtk_widget_set_size_request(parent,width,height + 2);
}

static void on_machine_id_changed(const BtMachine *machine,GParamSpec *arg,gpointer user_data) {
  BtMachinePreferencesDialog *self=BT_MACHINE_PREFERENCES_DIALOG(user_data);
  gchar *id,*title;

  g_object_get((GObject *)machine,"id",&id,NULL);
  title=g_strdup_printf(_("%s preferences"),id);
  gtk_window_set_title(GTK_WINDOW(self),title);
  g_free(id);g_free(title);
}

//-- helper methods

static gboolean skip_property(GstElement *element,GParamSpec *pspec) {
  // skip controlable properties
  if(pspec->flags&GST_PARAM_CONTROLLABLE) return(TRUE);
  // skip uneditable gobject propertes properties
  else if(G_TYPE_IS_CLASSED(pspec->value_type)) return(TRUE);
  else if(pspec->value_type==G_TYPE_POINTER) return(TRUE);
  // skip baseclass properties
  else if(!strncmp(pspec->name,"name\0",5)) return(TRUE);
  else if(pspec->owner_type==GST_TYPE_BASE_SRC) return(TRUE);
  else if(pspec->owner_type==GST_TYPE_BASE_TRANSFORM) return(TRUE);
  else if(pspec->owner_type==GST_TYPE_BASE_SINK) return(TRUE);
  // skip know interface properties (tempo, childbin)
  else if(pspec->owner_type==GSTBT_TYPE_CHILD_BIN) return(TRUE);
  else if(pspec->owner_type==GSTBT_TYPE_HELP) return(TRUE);
  else if(pspec->owner_type==GSTBT_TYPE_TEMPO) return(TRUE);

  GST_INFO("property: %s, owner-type: %s",pspec->name,g_type_name(pspec->owner_type));

  return(FALSE);
}

#define _MAKE_SPIN_BUTTON(t,T,p)                                               \
	case G_TYPE_ ## T: {                                                         \
		GParamSpec ## p *p=G_PARAM_SPEC_ ## T(property);                           \
		g ## t value;                                                              \
		gdouble step;                                                              \
                                                                               \
		g_object_get(machine,property->name,&value,NULL);                          \
		step=(gdouble)(p->maximum-p->minimum)/1024.0;                              \
		spin_adjustment=GTK_ADJUSTMENT(gtk_adjustment_new(                         \
			  (gdouble)value,(gdouble)p->minimum, (gdouble)p->maximum,1.0,step,0.0));\
		widget1=gtk_spin_button_new(spin_adjustment,1.0,0);                        \
		gtk_widget_set_name(GTK_WIDGET(widget1),property->name);                   \
		g_object_set_qdata(G_OBJECT(widget1),widget_parent_quark,(gpointer)self);  \
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget1),(gdouble)value);        \
		widget2=NULL;                                                              \
		g_signal_connect(widget1,"value-changed",G_CALLBACK(on_spinbutton_property_changed),(gpointer)machine); \
	} break;


static void bt_machine_preferences_dialog_init_ui(const BtMachinePreferencesDialog *self) {
  BtMainWindow *main_window;
  GtkWidget *label,*widget1,*widget2,*table,*scrolled_window;
  GtkAdjustment *spin_adjustment;
  GdkPixbuf *window_icon=NULL;
  GstElement *machine;
  GParamSpec **properties,*property;
  guint i,k,props,number_of_properties;

  gtk_widget_set_name(GTK_WIDGET(self),"machine preferences");

  g_object_get(self->priv->app,"main-window",&main_window,NULL);
  gtk_window_set_transient_for(GTK_WINDOW(self),GTK_WINDOW(main_window));

  // create and set window icon
  if((window_icon=bt_ui_resources_get_icon_pixbuf_by_machine(self->priv->machine))) {
    gtk_window_set_icon(GTK_WINDOW(self),window_icon);
    g_object_unref(window_icon);
  }

  // leave the choice of width to gtk
  //gtk_window_set_default_size(GTK_WINDOW(self),-1,200);
  ////gtk_widget_set_size_request(GTK_WIDGET(self),300,200);
  //gtk_window_set_default_size(GTK_WINDOW(self),300,-1);

  g_object_get(self->priv->machine,"machine",&machine,NULL);
  // set dialog title
  on_machine_id_changed(self->priv->machine,NULL,(gpointer)self);

  // get machine properties
  if((properties=g_object_class_list_properties(G_OBJECT_CLASS(GST_ELEMENT_GET_CLASS(machine)),&number_of_properties))) {
    gchar *signal_name;
    GType param_type,base_type;

    GST_INFO("machine has %d properties",number_of_properties);

    props=number_of_properties;
    for(i=0;i<number_of_properties;i++) {
      if(skip_property(machine,properties[i])) props--;
    }
    GST_INFO("machine has %d preferences",props);
    if(props) {
      // machine preferences inside a scrolled window
      scrolled_window=gtk_scrolled_window_new(NULL,NULL);
      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
      gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_NONE);

      // add machine preferences into the table
      table=gtk_table_new(/*rows=*/props/*+1 (space eater)*/,/*columns=*/3,/*homogenous=*/FALSE);
      gtk_container_set_border_width(GTK_CONTAINER(table),6);
      g_signal_connect(table,"size-request",G_CALLBACK(on_table_size_request),(gpointer)self);

      for(i=0,k=0;i<number_of_properties;i++) {
        property=properties[i];
        // skip some properties
        if(skip_property(machine,property)) continue;

        GST_INFO("property %p has name '%s'",property,property->name);
        // get name
        label=gtk_label_new(property->name);
        gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
        gtk_table_attach(GTK_TABLE(table),label, 0, 1, k, k+1, GTK_FILL,GTK_SHRINK, 2,1);

        param_type=property->value_type;
        while((base_type=g_type_parent(param_type))) param_type=base_type;
        GST_INFO("... base type is : %s",g_type_name(param_type));

        switch(param_type) {
          case G_TYPE_STRING: {
            gchar *value;

            g_object_get(machine,property->name,&value,NULL);
            widget1=gtk_entry_new();
            gtk_widget_set_name(GTK_WIDGET(widget1),property->name);
            g_object_set_qdata(G_OBJECT(widget1),widget_parent_quark,(gpointer)self);
            gtk_entry_set_text(GTK_ENTRY(widget1),safe_string(value));g_free(value);
            widget2=NULL;
            // connect handlers
            g_signal_connect(widget1, "changed", G_CALLBACK(on_entry_property_changed), (gpointer)machine);
          } break;
          case G_TYPE_BOOLEAN: {
            gboolean value;

            g_object_get(machine,property->name,&value,NULL);
            widget1=gtk_check_button_new();
            gtk_widget_set_name(GTK_WIDGET(widget1),property->name);
            g_object_set_qdata(G_OBJECT(widget1),widget_parent_quark,(gpointer)self);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget1),value);
            widget2=NULL;
            // connect handlers
            g_signal_connect(widget1, "toggled", G_CALLBACK(on_checkbox_property_toggled), (gpointer)machine);
          } break;
          _MAKE_SPIN_BUTTON(int,INT,Int)
          _MAKE_SPIN_BUTTON(uint,UINT,UInt)
          _MAKE_SPIN_BUTTON(int64,INT64,Int64)
          _MAKE_SPIN_BUTTON(uint64,UINT64,UInt64)
          _MAKE_SPIN_BUTTON(long,LONG,Long)
          _MAKE_SPIN_BUTTON(ulong,ULONG,ULong)
          case G_TYPE_DOUBLE: {
            GParamSpecDouble *p=G_PARAM_SPEC_DOUBLE(property);
            gdouble step,value;
            gchar *str_value;

            g_object_get(machine,property->name,&value,NULL);
            // get max(max,-min), count digits -> to determine needed length of field
            str_value=g_strdup_printf("%7.2lf",value);
            step=(p->maximum-p->minimum)/1024.0;
            widget1=gtk_hscale_new_with_range(p->minimum,p->maximum,step);
            gtk_widget_set_name(GTK_WIDGET(widget1),property->name);
            g_object_set_qdata(G_OBJECT(widget1),widget_parent_quark,(gpointer)self);
            gtk_scale_set_draw_value(GTK_SCALE(widget1),FALSE);
            gtk_range_set_value(GTK_RANGE(widget1),value);
            widget2=gtk_entry_new();
            gtk_widget_set_name(GTK_WIDGET(widget2),property->name);
            g_object_set_qdata(G_OBJECT(widget1),widget_parent_quark,(gpointer)self);
            gtk_entry_set_text(GTK_ENTRY(widget2),str_value);
            g_object_set(widget2,"max-length",9,"width-chars",9,NULL);
            g_free(str_value);
            signal_name=g_strdup_printf("notify::%s",property->name);
            g_signal_connect(machine, signal_name, G_CALLBACK(on_range_property_notify), (gpointer)widget1);
            g_signal_connect(machine, signal_name, G_CALLBACK(on_double_entry_property_notify), (gpointer)widget2);
            g_signal_connect(widget1, "value-changed", G_CALLBACK(on_range_property_changed), (gpointer)machine);
            g_signal_connect(widget2, "changed", G_CALLBACK(on_double_entry_property_changed), (gpointer)machine);
            g_free(signal_name);
          } break;
          case G_TYPE_ENUM: {
            GParamSpecEnum *enum_property=G_PARAM_SPEC_ENUM(property);
            GEnumClass *enum_class=enum_property->enum_class;
            GEnumValue *enum_value;
            GtkCellRenderer *renderer;
            GtkListStore *store;
            GtkTreeIter iter;
            gint value, ivalue;

            widget1=gtk_combo_box_new();
            GST_INFO("enum range: %d, %d",enum_class->minimum,enum_class->maximum);
            // need a real model because of sparse enums
            store=gtk_list_store_new(2,G_TYPE_ULONG,G_TYPE_STRING);
            for(value=enum_class->minimum;value<=enum_class->maximum;value++) {
              if((enum_value=g_enum_get_value(enum_class, value))) {
                //GST_INFO("enum value: %d, '%s', '%s'",enum_value->value,enum_value->value_name,enum_value->value_nick);
                //gtk_combo_box_append_text(GTK_COMBO_BOX(widget1),enum_value->value_nick);
                gtk_list_store_append(store,&iter);
                gtk_list_store_set(store,&iter,
                  0,enum_value->value,
                  1,enum_value->value_nick,
                  -1);
              }
            }
            renderer=gtk_cell_renderer_text_new();
            gtk_cell_renderer_set_fixed_size(renderer, 1, -1);
            gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
            gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget1),renderer,TRUE);
            gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(widget1),renderer,"text",1,NULL);
            gtk_combo_box_set_model(GTK_COMBO_BOX(widget1),GTK_TREE_MODEL(store));

            g_object_get(machine,property->name,&value,NULL);
            gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store),&iter);
            do {
              gtk_tree_model_get((GTK_TREE_MODEL(store)),&iter,0,&ivalue,-1);
              if(ivalue==value) break;
            } while(gtk_tree_model_iter_next(GTK_TREE_MODEL(store),&iter));

            gtk_combo_box_set_active_iter(GTK_COMBO_BOX(widget1),&iter);
            gtk_widget_set_name(GTK_WIDGET(widget1),property->name);
            g_object_set_qdata(G_OBJECT(widget1),widget_parent_quark,(gpointer)self);

            signal_name=g_strdup_printf("notify::%s",property->name);
            g_signal_connect(machine, signal_name, G_CALLBACK(on_combobox_property_notify), (gpointer)widget1);
            g_signal_connect(widget1, "changed", G_CALLBACK(on_combobox_property_changed), (gpointer)machine);
            g_free(signal_name);
            widget2=NULL;
          } break;
          default: {
            gchar *str=g_strdup_printf("unhandled type \"%s\"",G_PARAM_SPEC_TYPE_NAME(property));
            widget1=gtk_label_new(str);g_free(str);
            widget2=NULL;
          }
        }
        gtk_widget_set_tooltip_text(widget1,g_param_spec_get_blurb(property));
        if(!widget2) {
          gtk_table_attach(GTK_TABLE(table),widget1, 1, 3, k, k+1, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
        }
        else {
          gtk_widget_set_tooltip_text(widget2,g_param_spec_get_blurb(property));
          gtk_table_attach(GTK_TABLE(table),widget1, 1, 2, k, k+1, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
          gtk_table_attach(GTK_TABLE(table),widget2, 2, 3, k, k+1, GTK_FILL,GTK_SHRINK, 2,1);
        }
        k++;
      }
      // eat remaning space
      //gtk_table_attach(GTK_TABLE(table),gtk_label_new(" "), 0, 3, k, k+1, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);
      gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),table);
      gtk_container_add(GTK_CONTAINER(self),scrolled_window);
    }
    else {
      label=gtk_label_new(_("no settings"));
      gtk_container_add(GTK_CONTAINER(self),label);
      gtk_widget_set_size_request(GTK_WIDGET(self),250,-1);
    }
    g_free(properties);
  }
  else {
    gtk_container_add(GTK_CONTAINER(self),gtk_label_new(_("machine has no preferences")));
  }

  // track machine name (keep window title up-to-date)
  g_signal_connect(self->priv->machine,"notify::id",G_CALLBACK(on_machine_id_changed),(gpointer)self);

  g_object_unref(machine);
  g_object_unref(main_window);
}

//-- constructor methods

/**
 * bt_machine_preferences_dialog_new:
 * @machine: the machine to create the dialog for
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtMachinePreferencesDialog *bt_machine_preferences_dialog_new(const BtMachine *machine) {
  BtMachinePreferencesDialog *self;

  self=BT_MACHINE_PREFERENCES_DIALOG(g_object_new(BT_TYPE_MACHINE_PREFERENCES_DIALOG,"machine",machine,NULL));
  bt_machine_preferences_dialog_init_ui(self);
  gtk_widget_show_all(GTK_WIDGET(self));
  return(self);
}

//-- methods

//-- wrapper

//-- class internals

static void bt_machine_preferences_dialog_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec) {
  BtMachinePreferencesDialog *self = BT_MACHINE_PREFERENCES_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case MACHINE_PREFERENCES_DIALOG_MACHINE: {
      g_object_try_unref(self->priv->machine);
      self->priv->machine = g_value_dup_object(value);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_machine_preferences_dialog_dispose(GObject *object) {
  BtMachinePreferencesDialog *self = BT_MACHINE_PREFERENCES_DIALOG(object);
  GstElement *machine;

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  // disconnect handlers
  g_signal_handlers_disconnect_matched(self->priv->machine,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_machine_id_changed,self);
  // disconnect handlers connected to machine properties
  g_object_get(self->priv->machine,"machine",&machine,NULL);
  g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_range_property_notify,NULL);
  g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_double_entry_property_notify,NULL);
  g_object_unref(machine);

  g_object_try_unref(self->priv->machine);
  g_object_unref(self->priv->app);

  G_OBJECT_CLASS(bt_machine_preferences_dialog_parent_class)->dispose(object);
}

static void bt_machine_preferences_dialog_init(BtMachinePreferencesDialog *self) {
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_MACHINE_PREFERENCES_DIALOG, BtMachinePreferencesDialogPrivate);
  GST_DEBUG("!!!! self=%p",self);
  self->priv->app = bt_edit_application_new();
}

static void bt_machine_preferences_dialog_class_init(BtMachinePreferencesDialogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  widget_parent_quark=g_quark_from_static_string("BtMachinePreferencesDialog::widget-parent");

  g_type_class_add_private(klass,sizeof(BtMachinePreferencesDialogPrivate));

  gobject_class->set_property = bt_machine_preferences_dialog_set_property;
  gobject_class->dispose      = bt_machine_preferences_dialog_dispose;

  g_object_class_install_property(gobject_class,MACHINE_PREFERENCES_DIALOG_MACHINE,
                                  g_param_spec_object("machine",
                                     "machine construct prop",
                                     "Set machine object, the dialog handles",
                                     BT_TYPE_MACHINE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_WRITABLE|G_PARAM_STATIC_STRINGS));

}

