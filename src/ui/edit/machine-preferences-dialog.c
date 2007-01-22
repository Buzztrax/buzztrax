/* $Id: machine-preferences-dialog.c,v 1.29 2007-01-22 21:00:58 ensonic Exp $
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
 */ 

/* @todo: filter certain properties (tempo iface, ...)
 */

#define BT_EDIT
#define BT_MACHINE_PREFERENCES_DIALOG_C

#include "bt-edit.h"

//-- property ids

enum {
  MACHINE_PREFERENCES_DIALOG_APP=1,
  MACHINE_PREFERENCES_DIALOG_MACHINE
};

struct _BtMachinePreferencesDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtEditApplication *app;

  /* the underlying machine */
  BtMachine *machine;
};

static GtkDialogClass *parent_class=NULL;

//-- event handler

static void on_range_property_notify(const GstElement *machine,GParamSpec *property,gpointer user_data) {
  GtkWidget *widget=GTK_WIDGET(user_data);
  gdouble value;
  
  g_assert(user_data);

  //GST_INFO("preferences value notify received for: '%s'",property->name);
  
  g_object_get(G_OBJECT(machine),property->name,&value,NULL);
  //gdk_threads_enter();
  gtk_range_set_value(GTK_RANGE(widget),value);
  //gdk_threads_leave();
}

static void on_double_entry_property_notify(const GstElement *machine,GParamSpec *property,gpointer user_data) {
  GtkWidget *widget=GTK_WIDGET(user_data);
  gdouble value;
  gchar *str_value;
  
  g_assert(user_data);

  //GST_INFO("preferences value notify received for: '%s'",property->name);
  
  g_object_get(G_OBJECT(machine),property->name,&value,NULL);
  str_value=g_strdup_printf("%f",value);
  //gdk_threads_enter();
  gtk_entry_set_text(GTK_ENTRY(widget),str_value);
  //gdk_threads_leave();
  g_free(str_value);
}

static void on_combobox_property_notify(const GstElement *machine,GParamSpec *property,gpointer user_data) {
  GtkWidget *widget=GTK_WIDGET(user_data);
  gint value;

  g_assert(user_data);
  
  g_object_get(G_OBJECT(machine),property->name,&value,NULL);
  //gdk_threads_enter();
  gtk_combo_box_set_active(GTK_COMBO_BOX(widget),value);
  //gdk_threads_leave();
}

static void on_range_property_changed(GtkRange *range,gpointer user_data) {
  GstElement *machine=GST_ELEMENT(user_data);
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(range));
  
  g_assert(user_data);

  //GST_INFO("preferences value change received for: '%s'",name);
  g_object_set(machine,name,gtk_range_get_value(range),NULL);
}

static void on_double_entry_property_changed(GtkEditable *editable,gpointer user_data) {
  GstElement *machine=GST_ELEMENT(user_data);
  gdouble value;
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(editable));
  
  g_assert(user_data);

  //GST_INFO("preferences value change received for: '%s'",name);
  value=g_strtod(gtk_entry_get_text(GTK_ENTRY(editable)),NULL);
  g_object_set(machine,name,value,NULL);
}

static void on_spinbutton_property_changed(GtkSpinButton *spinbutton,gpointer user_data) {
  GstElement *machine=GST_ELEMENT(user_data);
  gint value;
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(spinbutton));
  
  g_assert(user_data);

  GST_INFO("preferences value change received for: '%s'",name);
  value=gtk_spin_button_get_value_as_int(spinbutton);
  g_object_set(machine,name,value,NULL);
}

static void on_combobox_property_changed(GtkComboBox *combobox, gpointer user_data) {
  GstElement *machine=GST_ELEMENT(user_data);
  gint value;
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(combobox));
  
  g_assert(user_data);

  GST_INFO("preferences value change received for: '%s'",name);
  value=gtk_combo_box_get_active(combobox);
  g_object_set(machine,name,value,NULL);
}

/*
 * on_table_size_request:
 *
 * we adjust the scrollable-window size to contain the whole area
 */
static void on_table_size_request(GtkWidget *widget,GtkRequisition *requisition,gpointer user_data) {
  GtkWidget *parent=GTK_WIDGET(user_data);
  gint height=requisition->height,width=-1;
  gint max_height=gdk_screen_get_height(gdk_screen_get_default());

  GST_INFO("#### table  size req %d x %d (max-height=%d)", requisition->width,requisition->height,max_height);
  // have a minimum width
  if(requisition->width<250) {
    width=250;
  }
  // constrain the height by screen height
  if(height>max_height) {
    // lets hope that 32 gives enough space for window-decoration + panels
    height=max_height-32;
  }
  // @todo: is the '2' some border or padding
  gtk_widget_set_size_request(parent,width,height + 2);
}

//-- helper methods

static gboolean bt_machine_preferences_dialog_init_ui(const BtMachinePreferencesDialog *self) {
  BtMainWindow *main_window;
  GtkWidget *label,*widget1,*widget2,*table,*scrolled_window;
  GtkAdjustment *spin_adjustment;
  GtkTooltips *tips=gtk_tooltips_new();
  gchar *id,*title;
  GdkPixbuf *window_icon=NULL;
  GstElement *machine;
  GParamSpec **properties,*property;
  guint i,k,props,number_of_properties;

  g_object_get(self->priv->app,"main-window",&main_window,NULL);
  gtk_window_set_transient_for(GTK_WINDOW(self),GTK_WINDOW(main_window));
  
  // create and set window icon
  if((window_icon=bt_ui_ressources_get_pixbuf_by_machine(self->priv->machine))) {
    gtk_window_set_icon(GTK_WINDOW(self),window_icon);
  }
  
  // leave the choice of width to gtk
  //gtk_window_set_default_size(GTK_WINDOW(self),-1,200);
  ////gtk_widget_set_size_request(GTK_WIDGET(self),300,200);
  //gtk_window_set_default_size(GTK_WINDOW(self),300,-1);
  
  // set a title
  g_object_get(self->priv->machine,"id",&id,"machine",&machine,NULL);
  title=g_strdup_printf(_("%s preferences"),id);
  gtk_window_set_title(GTK_WINDOW(self),title);
  g_free(id);g_free(title);
  
  // get machine properties
  if((properties=g_object_class_list_properties(G_OBJECT_CLASS(GST_ELEMENT_GET_CLASS(machine)),&number_of_properties))) {
    gchar *signal_name;
    GType param_type,base_type;
    
    GST_INFO("machine has %d properties",number_of_properties);
    
    props=number_of_properties;
    for(i=0;i<number_of_properties;i++) {
      property=properties[i];
      if(property->flags&GST_PARAM_CONTROLLABLE) props--;
      else if(GST_IS_OBJECT(machine) && !strncmp(property->name,"name\0",5)) props--;        
      // @todo: skip more baseclass properties
      // @todo: skip know interface properties (tempo, childbin)
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
      g_signal_connect(G_OBJECT(table),"size-request",G_CALLBACK(on_table_size_request),(gpointer)scrolled_window);
      
      for(i=0,k=0;i<number_of_properties;i++) {
        property=properties[i];
        // skip some properties
        if(property->flags&GST_PARAM_CONTROLLABLE) continue;
        if(GST_IS_OBJECT(machine) && !strncmp(property->name,"name\0",5)) continue;
        // @todo: skip more baseclass properties
        // @todo: skip know interface properties (tempo, childbin)
          
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
            //GParamSpecString *string_property=G_PARAM_SPEC_STRING(property);
            gchar *value;
            
            g_object_get(machine,property->name,&value,NULL);
            widget1=gtk_entry_new();
            gtk_entry_set_text(GTK_ENTRY(widget1),safe_string(value));g_free(value);
            widget2=NULL;
          } break;
          case G_TYPE_BOOLEAN: {
            gboolean value;
            
            g_object_get(machine,property->name,&value,NULL);
            widget1=gtk_check_button_new();
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget1),value);
            widget2=NULL;
          } break;
          case G_TYPE_INT: {
            GParamSpecInt *int_property=G_PARAM_SPEC_INT(property);
            gint value;
            gdouble step;
            
            g_object_get(machine,property->name,&value,NULL);
            step=(gdouble)(int_property->maximum-int_property->minimum)/1024.0;
            GST_INFO("  int : %d...%d, step=%f",int_property->minimum,int_property->maximum,step);
            spin_adjustment=GTK_ADJUSTMENT(gtk_adjustment_new((gdouble)value,(gdouble)int_property->minimum, (gdouble)int_property->maximum,1.0,step,step));
            widget1=gtk_spin_button_new(spin_adjustment,1.0,0);
            gtk_widget_set_name(GTK_WIDGET(widget1),property->name);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget1),(gdouble)value);
            widget2=NULL;
            // connect handlers
            g_signal_connect(G_OBJECT(widget1), "value-changed", (GCallback)on_spinbutton_property_changed, (gpointer)machine);
          } break;
          case G_TYPE_UINT: {
            GParamSpecUInt *uint_property=G_PARAM_SPEC_UINT(property);
            guint value;
            gdouble step;
            
            g_object_get(machine,property->name,&value,NULL);
            step=(gdouble)(uint_property->maximum-uint_property->minimum)/1024.0;
            GST_INFO("  uint : %u...%u, step=%f",uint_property->minimum,uint_property->maximum,step);
            spin_adjustment=GTK_ADJUSTMENT(gtk_adjustment_new((gdouble)value,(gdouble)uint_property->minimum, (gdouble)uint_property->maximum,1.0,step,step));
            widget1=gtk_spin_button_new(spin_adjustment,1.0,0);
            gtk_widget_set_name(GTK_WIDGET(widget1),property->name);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget1),(gdouble)value);
            widget2=NULL;
            // connect handlers
            g_signal_connect(G_OBJECT(widget1), "value-changed", (GCallback)on_spinbutton_property_changed, (gpointer)machine);
          } break;
          case G_TYPE_LONG: {
            GParamSpecLong *long_property=G_PARAM_SPEC_LONG(property);
            glong value;
            gdouble step;
            
            g_object_get(machine,property->name,&value,NULL);
            step=(gdouble)(long_property->maximum-long_property->minimum)/1024.0;
            GST_INFO("  long : %ld...%ld, step=%f",long_property->minimum,long_property->maximum,step);
            spin_adjustment=GTK_ADJUSTMENT(gtk_adjustment_new((gdouble)value,(gdouble)long_property->minimum, (gdouble)long_property->maximum,1.0,step,step));
            widget1=gtk_spin_button_new(spin_adjustment,1.0,0);
            gtk_widget_set_name(GTK_WIDGET(widget1),property->name);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget1),(gdouble)value);
            widget2=NULL;
            // connect handlers
            g_signal_connect(G_OBJECT(widget1), "value-changed", (GCallback)on_spinbutton_property_changed, (gpointer)machine);
          } break;
          case G_TYPE_ULONG: {
            GParamSpecULong *ulong_property=G_PARAM_SPEC_ULONG(property);
            gulong value;
            gdouble step;
            
            g_object_get(machine,property->name,&value,NULL);
            step=(gdouble)(ulong_property->maximum-ulong_property->minimum)/1024.0;
            GST_INFO("  ulong : %lu...%lu, step=%f",ulong_property->minimum,ulong_property->maximum,step);
            spin_adjustment=GTK_ADJUSTMENT(gtk_adjustment_new((gdouble)value,(gdouble)ulong_property->minimum, (gdouble)ulong_property->maximum,1.0,step,step));
            widget1=gtk_spin_button_new(spin_adjustment,1.0,0);
            gtk_widget_set_name(GTK_WIDGET(widget1),property->name);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget1),(gdouble)value);
            widget2=NULL;
            // connect handlers
            g_signal_connect(G_OBJECT(widget1), "value-changed", (GCallback)on_spinbutton_property_changed, (gpointer)machine);
          } break;
          case G_TYPE_INT64: {
            GParamSpecInt64 *int64_property=G_PARAM_SPEC_INT64(property);
            gint64 value;
            gdouble step;
            
            g_object_get(machine,property->name,&value,NULL);
            step=(gdouble)(int64_property->maximum-int64_property->minimum)/1024.0;
            GST_INFO("  int : %"G_GINT64_FORMAT"...%"G_GINT64_FORMAT", step=%f",int64_property->minimum,int64_property->maximum,step);
            spin_adjustment=GTK_ADJUSTMENT(gtk_adjustment_new((gdouble)value,(gdouble)int64_property->minimum, (gdouble)int64_property->maximum,1.0,step,step));
            widget1=gtk_spin_button_new(spin_adjustment,1.0,0);
            gtk_widget_set_name(GTK_WIDGET(widget1),property->name);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget1),(gdouble)value);
            widget2=NULL;
            // connect handlers
            g_signal_connect(G_OBJECT(widget1), "value-changed", (GCallback)on_spinbutton_property_changed, (gpointer)machine);
          } break;
          case G_TYPE_UINT64: {
            GParamSpecUInt64 *uint64_property=G_PARAM_SPEC_UINT64(property);
            guint64 value;
            gdouble step;
            
            g_object_get(machine,property->name,&value,NULL);
            step=(gdouble)(uint64_property->maximum-uint64_property->minimum)/1024.0;
            GST_INFO("  uint : %"G_GUINT64_FORMAT"...%"G_GUINT64_FORMAT", step=%f",uint64_property->minimum,uint64_property->maximum,step);
            spin_adjustment=GTK_ADJUSTMENT(gtk_adjustment_new((gdouble)value,(gdouble)uint64_property->minimum, (gdouble)uint64_property->maximum,1.0,step,step));
            widget1=gtk_spin_button_new(spin_adjustment,1.0,0);
            gtk_widget_set_name(GTK_WIDGET(widget1),property->name);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget1),(gdouble)value);
            widget2=NULL;
            // connect handlers
            g_signal_connect(G_OBJECT(widget1), "value-changed", (GCallback)on_spinbutton_property_changed, (gpointer)machine);
          } break;
          case G_TYPE_DOUBLE: {
            GParamSpecDouble *double_property=G_PARAM_SPEC_DOUBLE(property);
            gdouble step,value;
            gchar *str_value;
    
            g_object_get(machine,property->name,&value,NULL);
            // get max(max,-min), count digits -> to determine needed length of field
            str_value=g_strdup_printf("%7.2f",value);
            step=(double_property->maximum-double_property->minimum)/1024.0;
            widget1=gtk_hscale_new_with_range(double_property->minimum,double_property->maximum,step);
            gtk_widget_set_name(GTK_WIDGET(widget1),property->name);
            gtk_scale_set_draw_value(GTK_SCALE(widget1),FALSE);
            gtk_range_set_value(GTK_RANGE(widget1),value);
            widget2=gtk_entry_new();
            gtk_widget_set_name(GTK_WIDGET(widget2),property->name);
            gtk_entry_set_text(GTK_ENTRY(widget2),str_value);
            g_object_set(widget2,"max-length",9,"width-chars",9,NULL);
            g_free(str_value);
            signal_name=g_strdup_printf("notify::%s",property->name);
            g_signal_connect(G_OBJECT(machine), signal_name, (GCallback)on_range_property_notify, (gpointer)widget1);
            g_signal_connect(G_OBJECT(machine), signal_name, (GCallback)on_double_entry_property_notify, (gpointer)widget2);
            g_signal_connect(G_OBJECT(widget1), "value-changed", (GCallback)on_range_property_changed, (gpointer)machine);
            g_signal_connect(G_OBJECT(widget2), "changed", (GCallback)on_double_entry_property_changed, (gpointer)machine);
            g_free(signal_name);
          } break;
          case G_TYPE_ENUM: {
            GParamSpecEnum *enum_property=G_PARAM_SPEC_ENUM(property);
            GEnumClass *enum_class=enum_property->enum_class;
            GEnumValue *enum_value;
            gint value;
            
            widget1=gtk_combo_box_new_text();        
            for(value=enum_class->minimum;value<=enum_class->maximum;value++) {
              enum_value=g_enum_get_value(enum_class, value);
              gtk_combo_box_append_text(GTK_COMBO_BOX(widget1),enum_value->value_nick);
            }
            g_object_get(machine,property->name,&value,NULL);
            gtk_combo_box_set_active(GTK_COMBO_BOX(widget1),value);
            signal_name=g_strdup_printf("notify::%s",property->name);
            g_signal_connect(G_OBJECT(machine), signal_name, (GCallback)on_combobox_property_notify, (gpointer)widget1);
            g_signal_connect(G_OBJECT(widget1), "changed", (GCallback)on_combobox_property_changed, (gpointer)machine);
            g_free(signal_name);
            widget2=NULL;
          } break;
          default: {
            gchar *str=g_strdup_printf("unhandled type \"%s\"",G_PARAM_SPEC_TYPE_NAME(property));
            widget1=gtk_label_new(str);g_free(str);
            widget2=NULL;
          }
        }
        gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),widget1,g_param_spec_get_blurb(property),NULL);
        if(!widget2) {
          gtk_table_attach(GTK_TABLE(table),widget1, 1, 3, k, k+1, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
        }
        else {
          gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),widget2,g_param_spec_get_blurb(property),NULL);
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
    }
    g_free(properties);
  }
  else {
    gtk_container_add(GTK_CONTAINER(self),gtk_label_new(_("machine has no preferences")));
  }
  
  g_object_try_unref(machine);
  g_object_try_unref(main_window);
  return(TRUE);
}

//-- constructor methods

/**
 * bt_machine_preferences_dialog_new:
 * @app: the application the dialog belongs to
 * @machine: the machine to create the dialog for
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtMachinePreferencesDialog *bt_machine_preferences_dialog_new(const BtEditApplication *app,const BtMachine *machine) {
  BtMachinePreferencesDialog *self;

  if(!(self=BT_MACHINE_PREFERENCES_DIALOG(g_object_new(BT_TYPE_MACHINE_PREFERENCES_DIALOG,"app",app,"machine",machine,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_machine_preferences_dialog_init_ui(self)) {
    goto Error;
  }
  gtk_widget_show_all(GTK_WIDGET(self));
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_machine_preferences_dialog_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMachinePreferencesDialog *self = BT_MACHINE_PREFERENCES_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case MACHINE_PREFERENCES_DIALOG_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    case MACHINE_PREFERENCES_DIALOG_MACHINE: {
      g_value_set_object(value, self->priv->machine);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given preferences for this object */
static void bt_machine_preferences_dialog_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMachinePreferencesDialog *self = BT_MACHINE_PREFERENCES_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case MACHINE_PREFERENCES_DIALOG_APP: {
      g_object_try_unref(self->priv->app);
      self->priv->app = g_object_try_ref(g_value_get_object(value));
      //GST_DEBUG("set the app for settings_dialog: %p",self->priv->app);
    } break;
    case MACHINE_PREFERENCES_DIALOG_MACHINE: {
      g_object_try_unref(self->priv->machine);
      self->priv->machine = g_object_try_ref(g_value_get_object(value));
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
  
  // disconnect handlers connected to machine properties
  g_object_get(self->priv->machine,"machine",&machine,NULL);
  g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_range_property_notify,NULL);
  g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_double_entry_property_notify,NULL);
  g_object_unref(machine);
  
  g_object_try_unref(self->priv->app);
  g_object_try_unref(self->priv->machine);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_machine_preferences_dialog_finalize(GObject *object) {
  //BtMachinePreferencesDialog *self = BT_MACHINE_PREFERENCES_DIALOG(object);

  //GST_DEBUG("!!!! self=%p",self);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_machine_preferences_dialog_init(GTypeInstance *instance, gpointer g_class) {
  BtMachinePreferencesDialog *self = BT_MACHINE_PREFERENCES_DIALOG(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_MACHINE_PREFERENCES_DIALOG, BtMachinePreferencesDialogPrivate);
}

static void bt_machine_preferences_dialog_class_init(BtMachinePreferencesDialogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtMachinePreferencesDialogPrivate));
  
  gobject_class->set_property = bt_machine_preferences_dialog_set_property;
  gobject_class->get_property = bt_machine_preferences_dialog_get_property;
  gobject_class->dispose      = bt_machine_preferences_dialog_dispose;
  gobject_class->finalize     = bt_machine_preferences_dialog_finalize;

  g_object_class_install_property(gobject_class,MACHINE_PREFERENCES_DIALOG_APP,
                                  g_param_spec_object("app",
                                     "app construct prop",
                                     "Set application object, the dialog belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,MACHINE_PREFERENCES_DIALOG_MACHINE,
                                  g_param_spec_object("machine",
                                     "machine construct prop",
                                     "Set machine object, the dialog handles",
                                     BT_TYPE_MACHINE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

}

GType bt_machine_preferences_dialog_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    const GTypeInfo info = {
      sizeof (BtMachinePreferencesDialogClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_machine_preferences_dialog_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtMachinePreferencesDialog),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_machine_preferences_dialog_init, // instance_init
    };
    type = g_type_register_static(GTK_TYPE_WINDOW,"BtMachinePreferencesDialog",&info,0);
  }
  return type;
}
