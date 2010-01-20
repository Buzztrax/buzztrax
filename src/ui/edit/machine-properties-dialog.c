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
 * SECTION:btmachinepropertiesdialog
 * @short_description: machine realtime parameters
 * @see_also: #BtMachine
 *
 * A dialog to configure dynamic settings of a #BtMachine. The dialog also
 * allows to editing and manage presets for machines that support them.
 */

/* @todo: play machines
 * - we want to assign a note-controller to a machines note-trigger property
 *   and boolean-trigger controller to machines trigger properties
 *   - right now we don't show widgets for these
 */
#define BT_EDIT
#define BT_MACHINE_PROPERTIES_DIALOG_C

#include "bt-edit.h"

#define DEFAULT_LABEL_WIDTH 70

//-- property ids

enum {
  MACHINE_PROPERTIES_DIALOG_MACHINE=1
};

struct _BtMachinePropertiesDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the underlying machine */
  BtMachine *machine;
  gulong voices;

  GtkWidget *main_toolbar,*preset_toolbar;
  GtkWidget *preset_box;
  GtkWidget *preset_list;
#if !GTK_CHECK_VERSION(2,12,0)
  GtkTooltips *preset_tips;
#endif

  GtkWidget *param_group_box;
  /* need this to remove right expander when wire is removed */
  GHashTable *group_to_object;
  guint num_global, num_wires;

  //guint double_notify_source_id;
};

static GtkDialogClass *parent_class=NULL;

static GQuark widget_label_quark=0;
static GQuark widget_parent_quark=0;
static GQuark control_object_quark=0;
static GQuark control_property_quark=0;

enum {
  PRESET_LIST_LABEL=0,
  PRESET_LIST_COMMENT
};

typedef struct {
  const GstElement *machine;
  GParamSpec *property;
  gpointer user_data;
} BtNotifyIdleData;

#define MAKE_NOTIFY_IDLE_DATA(data,machine,property,user_data) \
  data=g_new(BtNotifyIdleData,1); \
  data->machine=machine; \
  data->property=property; \
  data->user_data=user_data; \
  g_object_add_weak_pointer(data->user_data,&data->user_data)
  
#define FREE_NOTIFY_IDLE_DATA(data) \
  g_object_remove_weak_pointer(data->user_data,&data->user_data); \
  g_free(data)

//-- event handler helper

static gboolean preset_list_edit_preset_meta(const BtMachinePropertiesDialog *self,GstElement *machine,gchar **name,gchar **comment) {
  gboolean result=FALSE;
  GtkWidget *dialog;

  GST_INFO("create preset edit dialog");

  dialog=GTK_WIDGET(bt_machine_preset_properties_dialog_new(machine,name,comment));
  GST_INFO("run preset edit dialog");
  gtk_window_set_transient_for(GTK_WINDOW(dialog),GTK_WINDOW(self));
  gtk_widget_show_all(GTK_WIDGET(dialog));

  if(gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT) {
    bt_machine_preset_properties_dialog_apply(BT_MACHINE_PRESET_PROPERTIES_DIALOG(dialog));
    result=TRUE;
  }

  gtk_widget_destroy(dialog);
  return(result);
}

static void preset_list_refresh(const BtMachinePropertiesDialog *self) {
  GstElement *machine;
  GtkListStore *store;
  GtkTreeIter tree_iter;
  gchar **presets,**preset;
  gchar *comment;

  GST_INFO("rebuilding preset list");

  g_object_get(self->priv->machine,"machine",&machine,NULL);
  presets=gst_preset_get_preset_names(GST_PRESET(machine));

  // we store the string twice, as we use the pointer as the key in the hashmap
  // and the string gets copied
  store=gtk_list_store_new(2,G_TYPE_STRING,G_TYPE_STRING);
  if(presets) {
    for(preset=presets;*preset;preset++) {
      gst_preset_get_meta(GST_PRESET(machine),*preset,"comment",&comment);
      GST_INFO(" adding item : '%s', '%s'",*preset,safe_string(comment));

      gtk_list_store_append(store, &tree_iter);
      gtk_list_store_set(store,&tree_iter,PRESET_LIST_LABEL,*preset,PRESET_LIST_COMMENT,comment,-1);
      g_free(comment);
    }
  }
  gtk_tree_view_set_model(GTK_TREE_VIEW(self->priv->preset_list),GTK_TREE_MODEL(store));
  g_object_unref(store); // drop with treeview
  gst_object_unref(machine);
  g_strfreev(presets);
  GST_INFO("rebuilt preset list");
}

static void mark_song_as_changed(const BtMachinePropertiesDialog *self) {
  BtSong *song;
  
  g_object_get(self->priv->app,"song",&song,NULL);
  bt_song_set_unsaved(song,TRUE);
  g_object_unref(song);
}

// interaction control helper

static void on_control_bind(const BtInteractionControllerMenu *menu,GParamSpec *arg,gpointer user_data) {
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);
  BtIcControl *control;
  GstObject *object;
  gchar *property_name;

  g_object_get(G_OBJECT(menu),"selected-control",&control,NULL);
  //GST_INFO("control selected: %p",control);
  object=g_object_get_qdata(G_OBJECT(menu),control_object_quark);
  property_name=g_object_get_qdata(G_OBJECT(menu),control_property_quark);

  bt_machine_bind_parameter_control(self->priv->machine,object,property_name,control);
  g_object_unref(control);
}

static void on_control_unbind(GtkMenuItem *menuitem,gpointer user_data) {
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);
  GtkWidget *menu;
  GstObject *object;
  gchar *property_name;

  menu=gtk_widget_get_parent(GTK_WIDGET(menuitem));

  object=g_object_get_qdata(G_OBJECT(menu),control_object_quark);
  property_name=g_object_get_qdata(G_OBJECT(menu),control_property_quark);

  bt_machine_unbind_parameter_control(self->priv->machine,object,property_name);
  //g_object_unref(menu);
}

static void on_control_unbind_all(GtkMenuItem *menuitem,gpointer user_data) {
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);

  bt_machine_unbind_parameter_controls(self->priv->machine);
}

static void on_parameter_reset(GtkMenuItem *menuitem,gpointer user_data) {
  //BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);
  GtkWidget *menu;
  GObject *object;
  gchar *property_name;
  GParamSpec *pspec;

  menu=gtk_widget_get_parent(GTK_WIDGET(menuitem));

  object=g_object_get_qdata(G_OBJECT(menu),control_object_quark);
  property_name=g_object_get_qdata(G_OBJECT(menu),control_property_quark);
  
  if((pspec=g_object_class_find_property(G_OBJECT_GET_CLASS(object),property_name))) {
    GValue gvalue={0,};

    g_value_init(&gvalue, pspec->value_type);
    g_param_value_set_default (pspec, &gvalue);
    g_object_set_property (object, property_name, &gvalue);
    g_value_unset(&gvalue);
  }
}

static void on_parameter_reset_all(GtkMenuItem *menuitem,gpointer user_data) {
  const BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);

  bt_machine_reset_parameters(self->priv->machine);
}


//-- event handler

static void on_double_range_property_changed(GtkRange *range,gpointer user_data);
static void on_float_range_property_changed(GtkRange *range,gpointer user_data);
static void on_int_range_property_changed(GtkRange *range,gpointer user_data);
static void on_uint_range_property_changed(GtkRange *range,gpointer user_data);
static void on_checkbox_property_toggled(GtkToggleButton *togglebutton, gpointer user_data);
static void on_combobox_property_changed(GtkComboBox *combobox, gpointer user_data);

static gchar* on_int_range_global_property_format_value(GtkScale *scale, gdouble value, gpointer user_data) {
  BtMachine *machine=BT_MACHINE(user_data);
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(scale));
  glong index=bt_machine_get_global_param_index(machine,name,NULL);
  GValue int_value={0,};
  gchar *str=NULL;
  static gchar _str[20];

  g_value_init(&int_value,G_TYPE_INT);
  g_value_set_int(&int_value,(gint)value);
  if(!(str=bt_machine_describe_global_param_value(machine,index,&int_value))) {
    g_sprintf(_str,"%d",(gint)value);
    str=_str;
  }
  else {
    strncpy(_str,str,20);
    _str[19]='\0';
    g_free(str);
    str=_str;
  }
  return(str);
}

static gchar* on_int_range_voice_property_format_value(GtkScale *scale, gdouble value, gpointer user_data) {
  BtMachine *machine=BT_MACHINE(user_data);
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(scale));
  glong index=bt_machine_get_voice_param_index(machine,name,NULL);
  GValue int_value={0,};
  gchar *str=NULL;
  static gchar _str[20];

  g_value_init(&int_value,G_TYPE_INT);
  g_value_set_int(&int_value,(gint)value);
  if(!(str=bt_machine_describe_voice_param_value(machine,index,&int_value))) {
    g_sprintf(_str,"%d",(gint)value);
    str=_str;
  }
  else {
    strncpy(_str,str,20);
    _str[19]='\0';
    g_free(str);
    str=_str;
  }
  return(str);
}

static gchar* on_uint_range_global_property_format_value(GtkScale *scale, gdouble value, gpointer user_data) {
  BtMachine *machine=BT_MACHINE(user_data);
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(scale));
  glong index=bt_machine_get_global_param_index(machine,name,NULL);
  GValue uint_value={0,};
  gchar *str=NULL;
  static gchar _str[20];

  g_value_init(&uint_value,G_TYPE_UINT);
  g_value_set_uint(&uint_value,(guint)value);
  if(!(str=bt_machine_describe_global_param_value(machine,index,&uint_value))) {
    g_sprintf(_str,"%u",(guint)value);
    str=_str;
  }
  else {
    strncpy(_str,str,20);
    _str[19]='\0';
    g_free(str);
    str=_str;
  }
  return(str);
}

static gchar* on_uint_range_voice_property_format_value(GtkScale *scale, gdouble value, gpointer user_data) {
  BtMachine *machine=BT_MACHINE(user_data);
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(scale));
  glong index=bt_machine_get_voice_param_index(machine,name,NULL);
  GValue uint_value={0,};
  gchar *str=NULL;
  static gchar _str[20];

  g_value_init(&uint_value,G_TYPE_UINT);
  g_value_set_uint(&uint_value,(guint)value);
  if(!(str=bt_machine_describe_voice_param_value(machine,index,&uint_value))) {
    g_sprintf(_str,"%u",(guint)value);
    str=_str;
  }
  else {
    strncpy(_str,str,20);
    _str[19]='\0';
    g_free(str);
    str=_str;
  }
  return(str);
}

// @todo: should we have this in btmachine.c?
static void bt_machine_update_default_param_value(BtMachine *self,GstObject *param_parent, const gchar *property_name) {
  GstController *ctrl;

  if((ctrl=gst_object_get_controller(G_OBJECT(param_parent)))) {
    GstElement *element;
    glong voice=-1;
   
    g_object_get(self,"machine",&element,NULL);
    
    // check if we update a voice parameter
    if((param_parent!=GST_OBJECT(element)) && GST_IS_CHILD_PROXY(element)) { 
      gulong i,voices=gst_child_proxy_get_children_count(GST_CHILD_PROXY(element));
      GstObject *voice_elem;
      
      for(i=0;i<voices;i++) {
        voice_elem=gst_child_proxy_get_child_by_index(GST_CHILD_PROXY(element),i);
        if(voice_elem==param_parent) {
          gst_object_unref(voice_elem);
          voice=i;
          break;
        }
        gst_object_unref(voice_elem);
      }
    }
    
    // update the default value at ts=0
    if(voice==-1) {
      bt_machine_set_global_param_default(self,
        bt_machine_get_global_param_index(self,property_name,NULL));
    }
    else {
      bt_machine_set_voice_param_default(self,voice,
        bt_machine_get_voice_param_index(self,property_name,NULL));
    }
    g_object_unref(element);

    /* @todo: it should actualy postpone the disable to the next timestamp
     * (not possible right now).
     *
     * @idea: can we have a livecontrolsource that subclasses interpolationcs
     * - when enabling, if would need to delay the enabled to the next control-point
     * - it would need to peek at the control-point list :/
     */
    gst_controller_set_property_disabled(ctrl,(gchar *)property_name,FALSE);
  }
  else {
    GST_DEBUG("object not controlled, type=%s, instance=%s",
      G_OBJECT_TYPE_NAME(param_parent),
      GST_IS_OBJECT(param_parent)?GST_OBJECT_NAME(param_parent):"");
  }
}

static void update_param_after_interaction(GtkWidget *widget,gpointer user_data) {
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(g_object_get_qdata(G_OBJECT(widget),widget_parent_quark));

  bt_machine_update_default_param_value(self->priv->machine, GST_OBJECT(user_data), gtk_widget_get_name(GTK_WIDGET(widget)));
}

static gboolean on_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data, BtInteractionControllerMenuType type) {
  GstObject *param_parent=GST_OBJECT(user_data);
  const gchar *property_name=gtk_widget_get_name(GTK_WIDGET(widget));
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(g_object_get_qdata(G_OBJECT(widget),widget_parent_quark));
  gboolean res=FALSE;

  GST_INFO("button_press : button 0x%x, type 0x%d",event->button,event->type);
  if(event->type == GDK_BUTTON_PRESS) {
    if(event->button == 3) {
      GtkMenu *menu;
      GtkWidget *menu_item;
      GtkWidget *item_unbind,*item_unbind_all;

      // create context menu
      // @todo: do we leak that menu here?
      menu=GTK_MENU(bt_interaction_controller_menu_new(type));
      g_object_get(menu,"item-unbind",&item_unbind,"item-unbind-all",&item_unbind_all,NULL);
      g_object_set_qdata(G_OBJECT(menu),control_object_quark,(gpointer)param_parent);
      g_object_set_qdata(G_OBJECT(menu),control_property_quark,(gpointer)property_name);
      g_signal_connect(G_OBJECT(menu),"notify::selected-control",G_CALLBACK(on_control_bind),(gpointer)self);
      g_signal_connect(G_OBJECT(item_unbind),"activate",G_CALLBACK(on_control_unbind),(gpointer)self);
      g_signal_connect(G_OBJECT(item_unbind_all),"activate",G_CALLBACK(on_control_unbind_all),(gpointer)self);

      // add extra items
      menu_item=gtk_separator_menu_item_new();
      gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
      gtk_widget_set_sensitive(menu_item,FALSE);
      gtk_widget_show(menu_item);
    
      menu_item=gtk_image_menu_item_new_with_label(_("Reset parameter"));
      g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_parameter_reset),(gpointer)self);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
      gtk_widget_show(menu_item);

      menu_item=gtk_image_menu_item_new_with_label(_("Reset all parameters"));
      g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_parameter_reset_all),(gpointer)self);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
      gtk_widget_show(menu_item);

      gtk_menu_popup(menu,NULL,NULL,NULL,NULL,3,gtk_get_current_event_time());
      res=TRUE;
    }
    else if(event->button == 1) {
      GstController *ctrl;
      if((ctrl=gst_object_get_controller(G_OBJECT(param_parent)))) {
        gst_controller_set_property_disabled(ctrl,(gchar *)property_name,TRUE);
      }
    }
  }
  return(res);
}

static gboolean on_button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
  if(event->button == 1 && event->type == GDK_BUTTON_RELEASE) {
    update_param_after_interaction(widget,user_data);
  }
  return(FALSE);
}

static gboolean on_trigger_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
  return(on_button_press_event(widget,event,user_data,BT_INTERACTION_CONTROLLER_TRIGGER_MENU));
}

static gboolean on_range_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
  return(on_button_press_event(widget,event,user_data,BT_INTERACTION_CONTROLLER_RANGE_MENU));
}


static void update_double_range_label(GtkLabel *label,gdouble value) {
  gchar str[100];
 
  g_sprintf(str,"%lf",value);
  gtk_label_set_text(label,str);
}

static gboolean on_double_range_property_notify_idle(gpointer _data) {
  BtNotifyIdleData *data=(BtNotifyIdleData *)_data;

  if(data->user_data) {
    const GstElement *machine=data->machine;
    GParamSpec *property=data->property;
    GtkWidget *widget=GTK_WIDGET(data->user_data);
    GtkLabel *label=GTK_LABEL(g_object_get_qdata(G_OBJECT(widget),widget_label_quark));
    gdouble value;
  
    GST_INFO("property value notify received : %s ",property->name);
  
    g_object_get(G_OBJECT(machine),property->name,&value,NULL);
    g_signal_handlers_block_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_double_range_property_changed,(gpointer)machine);
    gtk_range_set_value(GTK_RANGE(widget),value);
    g_signal_handlers_unblock_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_double_range_property_changed,(gpointer)machine);
    update_double_range_label(label,value);
  }
  FREE_NOTIFY_IDLE_DATA(data);
  return(FALSE);
}

static void on_double_range_property_notify(const GstElement *machine,GParamSpec *property,gpointer user_data) {
  BtNotifyIdleData *data;
  //BtMachinePropertiesDialog *self=g_object_get_qdata(G_OBJECT(user_data),widget_parent_quark);

  MAKE_NOTIFY_IDLE_DATA(data,machine,property,user_data);
  //if (!self->priv->double_notify_source_id)
  //  self->priv->double_notify_source_id=
  g_idle_add(on_double_range_property_notify_idle,(gpointer)data);
}

static void on_double_range_property_changed(GtkRange *range,gpointer user_data) {
  GstObject *param_parent=GST_OBJECT(user_data);
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(range));
  GtkLabel *label=GTK_LABEL(g_object_get_qdata(G_OBJECT(range),widget_label_quark));
  const BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(g_object_get_qdata(G_OBJECT(range),widget_parent_quark));
  gdouble value=gtk_range_get_value(range);

  //GST_INFO("property value change received : %lf",value);

  g_signal_handlers_block_matched(param_parent,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_double_range_property_notify,(gpointer)range);
  g_object_set(param_parent,name,value,NULL);
  g_signal_handlers_unblock_matched(param_parent,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_double_range_property_notify,(gpointer)range);
  update_double_range_label(label,value);
  mark_song_as_changed(self);
}


static void update_float_range_label(GtkLabel *label,gfloat value) {
  gchar str[100];
 
  g_sprintf(str,"%f",value);
  gtk_label_set_text(label,str);
}
 
static gboolean on_float_range_property_notify_idle(gpointer _data) {
  BtNotifyIdleData *data=(BtNotifyIdleData *)_data;

  if(data->user_data) {
    const GstElement *machine=data->machine;
    GParamSpec *property=data->property;
    GtkWidget *widget=GTK_WIDGET(data->user_data);
    GtkLabel *label=GTK_LABEL(g_object_get_qdata(G_OBJECT(widget),widget_label_quark));
    gfloat value;
  
    //GST_INFO("property value notify received : %s ",property->name);
  
    g_object_get(G_OBJECT(machine),property->name,&value,NULL);
    g_signal_handlers_block_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_float_range_property_changed,(gpointer)machine);
    gtk_range_set_value(GTK_RANGE(widget),value);
    g_signal_handlers_unblock_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_float_range_property_changed,(gpointer)machine);
    update_float_range_label(label,value);
  }
  FREE_NOTIFY_IDLE_DATA(data);
  return(FALSE);
}

static void on_float_range_property_notify(const GstElement *machine,GParamSpec *property,gpointer user_data) {
  BtNotifyIdleData *data;

  MAKE_NOTIFY_IDLE_DATA(data,machine,property,user_data);
  g_idle_add(on_float_range_property_notify_idle,(gpointer)data);
}

static void on_float_range_property_changed(GtkRange *range,gpointer user_data) {
  GstObject *param_parent=GST_OBJECT(user_data);
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(range));
  GtkLabel *label=GTK_LABEL(g_object_get_qdata(G_OBJECT(range),widget_label_quark));
  const BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(g_object_get_qdata(G_OBJECT(range),widget_parent_quark));
  gfloat value=gtk_range_get_value(range);

  //GST_INFO("property value change received : %f",value);

  g_signal_handlers_block_matched(param_parent,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_float_range_property_notify,(gpointer)range);
  g_object_set(param_parent,name,value,NULL);
  g_signal_handlers_unblock_matched(param_parent,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_float_range_property_notify,(gpointer)range);
  update_float_range_label(label,value);
  mark_song_as_changed(self);
}


static void update_int_range_label(const BtMachinePropertiesDialog *self,GtkRange *range,GstObject *param_parent,GtkLabel *label,gdouble value) {
  if(GST_IS_ELEMENT(param_parent)) {
    gtk_label_set_text(label,on_int_range_global_property_format_value(GTK_SCALE(range),value,(gpointer)self->priv->machine));
  }
  else {
    gtk_label_set_text(label,on_int_range_voice_property_format_value(GTK_SCALE(range),value,(gpointer)self->priv->machine));
  }
}

static gboolean on_int_range_property_notify_idle(gpointer _data) {
  BtNotifyIdleData *data=(BtNotifyIdleData *)_data;

  if(data->user_data) {
    const GstElement *machine=data->machine;
    GParamSpec *property=data->property;
    GtkWidget *widget=GTK_WIDGET(data->user_data);
    GtkLabel *label=GTK_LABEL(g_object_get_qdata(G_OBJECT(widget),widget_label_quark));
    BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(g_object_get_qdata(G_OBJECT(widget),widget_parent_quark));
    gint value;
  
    g_object_get(G_OBJECT(machine),property->name,&value,NULL);
    //GST_INFO("property value notify received : %s => : %d",property->name,value);
  
    g_signal_handlers_block_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_int_range_property_changed,(gpointer)machine);
    gtk_range_set_value(GTK_RANGE(widget),(gdouble)value);
    g_signal_handlers_unblock_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_int_range_property_changed,(gpointer)machine);
    update_int_range_label(self,GTK_RANGE(widget),GST_OBJECT(machine),label,(gdouble)value);
  }
  FREE_NOTIFY_IDLE_DATA(data);
  return(FALSE);
}

static void on_int_range_property_notify(const GstElement *machine,GParamSpec *property,gpointer user_data) {
  BtNotifyIdleData *data;

  MAKE_NOTIFY_IDLE_DATA(data,machine,property,user_data);
  g_idle_add(on_int_range_property_notify_idle,(gpointer)data);
}

static void on_int_range_property_changed(GtkRange *range,gpointer user_data) {
  GstObject *param_parent=GST_OBJECT(user_data);
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(range));
  GtkLabel *label=GTK_LABEL(g_object_get_qdata(G_OBJECT(range),widget_label_quark));
  const BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(g_object_get_qdata(G_OBJECT(range),widget_parent_quark));
  gdouble value=gtk_range_get_value(range);

  //GST_INFO("property value change received");

  g_signal_handlers_block_matched(param_parent,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_int_range_property_notify,(gpointer)range);
  g_object_set(param_parent,name,(gint)value,NULL);
  g_signal_handlers_unblock_matched(param_parent,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_int_range_property_notify,(gpointer)range);
  update_int_range_label(self,range,param_parent,label,value);
  mark_song_as_changed(self);
}


static void update_uint_range_label(const BtMachinePropertiesDialog *self,GtkRange *range,GstObject *param_parent,GtkLabel *label,gdouble value) {
  if(GST_IS_ELEMENT(param_parent)) {
    gtk_label_set_text(label,on_uint_range_global_property_format_value(GTK_SCALE(range),value,(gpointer)self->priv->machine));
  }
  else {
    gtk_label_set_text(label,on_uint_range_voice_property_format_value(GTK_SCALE(range),value,(gpointer)self->priv->machine));
  }
}

static gboolean on_uint_range_property_notify_idle(gpointer _data) {
  BtNotifyIdleData *data=(BtNotifyIdleData *)_data;

  if(data->user_data) {
    const GstElement *machine=data->machine;
    GParamSpec *property=data->property;
    GtkWidget *widget=GTK_WIDGET(data->user_data);
    GtkLabel *label=GTK_LABEL(g_object_get_qdata(G_OBJECT(widget),widget_label_quark));
    BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(g_object_get_qdata(G_OBJECT(widget),widget_parent_quark));
    guint value;
  
    //GST_INFO("property value notify received : %s ",property->name);
  
    g_object_get(G_OBJECT(machine),property->name,&value,NULL);
    g_signal_handlers_block_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_uint_range_property_changed,(gpointer)machine);
    gtk_range_set_value(GTK_RANGE(widget),(gdouble)value);
    g_signal_handlers_unblock_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_uint_range_property_changed,(gpointer)machine);
    update_uint_range_label(self,GTK_RANGE(widget),GST_OBJECT(machine),label,(gdouble)value);
  }
  FREE_NOTIFY_IDLE_DATA(data);
  return(FALSE);
}

static void on_uint_range_property_notify(const GstElement *machine,GParamSpec *property,gpointer user_data) {
  BtNotifyIdleData *data;

  MAKE_NOTIFY_IDLE_DATA(data,machine,property,user_data);
  g_idle_add(on_uint_range_property_notify_idle,(gpointer)data);
}

static void on_uint_range_property_changed(GtkRange *range,gpointer user_data) {
  GstObject *param_parent=GST_OBJECT(user_data);
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(range));
  GtkLabel *label=GTK_LABEL(g_object_get_qdata(G_OBJECT(range),widget_label_quark));
  const BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(g_object_get_qdata(G_OBJECT(range),widget_parent_quark));
  gdouble value=gtk_range_get_value(range);

  GST_INFO("property value change received");

  g_signal_handlers_block_matched(param_parent,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_uint_range_property_notify,(gpointer)range);
  g_object_set(param_parent,name,(guint)value,NULL);
  g_signal_handlers_unblock_matched(param_parent,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_uint_range_property_notify,(gpointer)range);
  update_uint_range_label(self,range,param_parent,label,value);
  mark_song_as_changed(self);
}


static gboolean on_combobox_property_notify_idle(gpointer _data) {
  BtNotifyIdleData *data=(BtNotifyIdleData *)_data;

  if(data->user_data) {
    const GstElement *machine=data->machine;
    GParamSpec *property=data->property;
    GtkWidget *widget=GTK_WIDGET(data->user_data);
    gint ivalue,nvalue;
    GtkTreeModel *store;
    GtkTreeIter iter;
  
    GST_DEBUG("property value notify received : %s ",property->name);
  
    g_object_get(G_OBJECT(machine),property->name,&nvalue,NULL);
    store=gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
    gtk_tree_model_get_iter_first(store,&iter);
    do {
      gtk_tree_model_get(store,&iter,0,&ivalue,-1);
      if(ivalue==nvalue) break;
    } while(gtk_tree_model_iter_next(store,&iter));
  
    g_signal_handlers_block_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_combobox_property_changed,(gpointer)machine);
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(widget),&iter);
    g_signal_handlers_unblock_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_combobox_property_changed,(gpointer)machine);
  }
  FREE_NOTIFY_IDLE_DATA(data);
  return(FALSE);
}

static void on_combobox_property_notify(const GstElement *machine,GParamSpec *property,gpointer user_data) {
  BtNotifyIdleData *data;

  MAKE_NOTIFY_IDLE_DATA(data,machine,property,user_data);
  g_idle_add(on_combobox_property_notify_idle,(gpointer)data);
}

static void on_combobox_property_changed(GtkComboBox *combobox, gpointer user_data) {
  GstObject *param_parent=GST_OBJECT(user_data);
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(combobox));
  const BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(g_object_get_qdata(G_OBJECT(combobox),widget_parent_quark));
  GtkTreeModel *store;
  GtkTreeIter iter;
  gint value;

  //GST_INFO("property value change received");

  //value=gtk_combo_box_get_active(combobox);
  store=gtk_combo_box_get_model(combobox);
  if(gtk_combo_box_get_active_iter(combobox,&iter)) {
    gtk_tree_model_get(store,&iter,0,&value,-1);
    g_signal_handlers_block_matched(param_parent,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_combobox_property_notify,(gpointer)combobox);
    g_object_set(param_parent,name,value,NULL);
    g_signal_handlers_unblock_matched(param_parent,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_combobox_property_notify,(gpointer)combobox);
    //GST_WARNING("property value change received: %d",value);
    update_param_after_interaction(GTK_WIDGET(combobox),user_data);
    mark_song_as_changed(self);
  }
}


static gboolean on_checkbox_property_notify_idle(gpointer _data) {
  BtNotifyIdleData *data=(BtNotifyIdleData *)_data;

  if(data->user_data) {
    const GstElement *machine=data->machine;
    GParamSpec *property=data->property;
    GtkWidget *widget=GTK_WIDGET(data->user_data);
    gboolean value;
  
    //GST_INFO("property value notify received : %s ",property->name);
  
    g_object_get(G_OBJECT(machine),property->name,&value,NULL);
    g_signal_handlers_block_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_checkbox_property_toggled,(gpointer)machine);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),value);
    g_signal_handlers_unblock_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_checkbox_property_toggled,(gpointer)machine);
  }
  FREE_NOTIFY_IDLE_DATA(data);
  return(FALSE);
}

static void on_checkbox_property_notify(const GstElement *machine,GParamSpec *property,gpointer user_data) {
  BtNotifyIdleData *data;

  MAKE_NOTIFY_IDLE_DATA(data,machine,property,user_data);
  g_idle_add(on_checkbox_property_notify_idle,(gpointer)data);
}

static void on_checkbox_property_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
  GstObject *param_parent=GST_OBJECT(user_data);
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(togglebutton));
  const BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(g_object_get_qdata(G_OBJECT(togglebutton),widget_parent_quark));
  gboolean value;

  //GST_INFO("property value change received");

  value=gtk_toggle_button_get_active(togglebutton);
  g_signal_handlers_block_matched(param_parent,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_checkbox_property_notify,(gpointer)togglebutton);
  g_object_set(param_parent,name,value,NULL);
  g_signal_handlers_unblock_matched(param_parent,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_checkbox_property_notify,(gpointer)togglebutton);
  //update_param_after_interaction(GTK_WIDGET(togglebutton),user_data);
  mark_song_as_changed(self);
}


static void on_toolbar_help_clicked(GtkButton *button,gpointer user_data) {
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);
  GstElement *machine;

  // show help for machine
  g_object_get(self->priv->machine,"machine",&machine,NULL);
  bt_machine_action_help(GTK_WIDGET(self),machine);
  gst_object_unref(machine);
}

static void on_toolbar_about_clicked(GtkButton *button,gpointer user_data) {
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);
  BtMainWindow *main_window;
  GstElement *machine;

  GST_INFO("context_menu about event occurred");
  // show info about machine
  g_object_get(self->priv->machine,"machine",&machine,NULL);
  g_object_get(self->priv->app,"main-window",&main_window,NULL);
  bt_machine_action_about(machine,main_window);
  g_object_unref(main_window);
  gst_object_unref(machine);
}

static void on_toolbar_random_clicked(GtkButton *button,gpointer user_data) {
  const BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);

  bt_machine_randomize_parameters(self->priv->machine);
}

static void on_toolbar_reset_clicked(GtkButton *button,gpointer user_data) {
  const BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);

  bt_machine_reset_parameters(self->priv->machine);
}

static void on_toolbar_show_hide_clicked(GtkButton *button,gpointer user_data) {
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);

  if(gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(button))) {
    GST_DEBUG("win: %d,%d, box: %d,%d",
      GTK_WIDGET(self)->allocation.width,GTK_WIDGET(self)->allocation.height,
      GTK_WIDGET(self->priv->preset_box)->allocation.width,GTK_WIDGET(self->priv->preset_box)->allocation.height);
    // expand window
    gtk_window_resize(GTK_WINDOW(self),
      GTK_WIDGET(self)->allocation.width+(GTK_WIDGET(self->priv->preset_box)->allocation.width+12),
      GTK_WIDGET(self)->allocation.height);

    gtk_widget_show_all(self->priv->preset_box);
  }
  else {
    /*
    GtkRequisition wsize,bsize;

    gtk_widget_size_request(GTK_WIDGET(self),&wsize);
    gtk_widget_size_request(GTK_WIDGET(self->priv->preset_box),&bsize);
    GST_WARNING("win: %d,%d, box: %d,%d",wsize.width,wsize.height,bsize.width,bsize.height);
    */
    GST_DEBUG("win: %d,%d, box: %d,%d",
      GTK_WIDGET(self)->allocation.width,GTK_WIDGET(self)->allocation.height,
      GTK_WIDGET(self->priv->preset_box)->allocation.width,GTK_WIDGET(self->priv->preset_box)->allocation.height);

    gtk_widget_hide(self->priv->preset_box);
    // shrink window
    //gtk_widget_set_size_request(GTK_WIDGET(self),wsize.width-bsize.width,wsize.height);
    //gtk_window_resize(GTK_WINDOW(self),wsize.width-bsize.width,wsize.height);
    gtk_window_resize(GTK_WINDOW(self),
      GTK_WIDGET(self)->allocation.width-(GTK_WIDGET(self->priv->preset_box)->allocation.width+12),
      GTK_WIDGET(self)->allocation.height);
  }
}

static void on_toolbar_preset_add_clicked(GtkButton *button,gpointer user_data) {
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);
  GstElement *machine;
  gchar *name=NULL,*comment=NULL;

  g_object_get(self->priv->machine,"machine",&machine,NULL);

  // ask for name & comment
  if(preset_list_edit_preset_meta(self,machine,&name,&comment)) {
    GST_INFO("about to add a new preset : '%s'",name);

    gst_preset_save_preset(GST_PRESET(machine),name);
    gst_preset_set_meta(GST_PRESET(machine),name,"comment",comment);
    preset_list_refresh(self);
  }
  gst_object_unref(machine);
}

static void on_toolbar_preset_remove_clicked(GtkButton *button,gpointer user_data) {
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;

  // get current preset from list
  selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(self->priv->preset_list));
  if(gtk_tree_selection_get_selected(selection, &model, &iter)) {
    gchar *name;
    GstElement *machine;

    gtk_tree_model_get(model,&iter,PRESET_LIST_LABEL,&name,-1);
    g_object_get(self->priv->machine,"machine",&machine,NULL);

    GST_INFO("about to delete preset : '%s'",name);
    gst_preset_delete_preset(GST_PRESET(machine),name);
    preset_list_refresh(self);
    gst_object_unref(machine);
  }
}

static void on_toolbar_preset_edit_clicked(GtkButton *button,gpointer user_data) {
  const BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;

  // get current preset from list
  selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(self->priv->preset_list));
  if(gtk_tree_selection_get_selected(selection, &model, &iter)) {
    gchar *old_name,*new_name,*comment;
    GstElement *machine;

    gtk_tree_model_get(model,&iter,PRESET_LIST_LABEL,&old_name,-1);
    g_object_get(self->priv->machine,"machine",&machine,NULL);

    GST_INFO("about to edit preset : '%s'",old_name);
    gst_preset_get_meta(GST_PRESET(machine),old_name,"comment",&comment);
    new_name=g_strdup(old_name);
    // change for name & comment
    if(preset_list_edit_preset_meta(self,machine,&new_name,&comment)) {
      gst_preset_rename_preset(GST_PRESET(machine),old_name,new_name);
      gst_preset_set_meta(GST_PRESET(machine),new_name,"comment",comment);
      preset_list_refresh(self);
    }
    g_free(old_name);
    g_free(comment);
    gst_object_unref(machine);
  }
}

static void on_preset_list_row_activated(GtkTreeView *tree_view,GtkTreePath *path,GtkTreeViewColumn *column,gpointer user_data) {
  const BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);
  GtkTreeModel *model;
  GtkTreeIter iter;

  // get current preset from list
  model=gtk_tree_view_get_model(tree_view);
  if(gtk_tree_model_get_iter(model,&iter,path)) {
    gchar *name;
    GstElement *machine;

    gtk_tree_model_get(model,&iter,PRESET_LIST_LABEL,&name,-1);

    g_object_get(self->priv->machine,"machine",&machine,NULL);

    GST_INFO("about to load preset : '%s'",name);
    if(gst_preset_load_preset(GST_PRESET(machine),name)) {
      mark_song_as_changed(self);
      bt_machine_set_param_defaults(self->priv->machine);
    }
    gst_object_unref(machine);
  }
}

#if !GTK_CHECK_VERSION(2,12,0)
static void on_preset_list_motion_notify(GtkTreeView *tree_view,GdkEventMotion *event,gpointer user_data) {
  const BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);
  GdkWindow *bin_window;
  GtkTreePath *path;
  GtkTreeViewColumn *column;

  bin_window=gtk_tree_view_get_bin_window(tree_view);
  if(event->window!=bin_window) return;

  if(gtk_tree_view_get_path_at_pos(tree_view,(gint)event->x,(gint)event->y,&path,&column,NULL,NULL)) {
    GtkTreeModel *model;
    GtkTreeIter iter;

    model=gtk_tree_view_get_model(tree_view);
    if(gtk_tree_model_get_iter(model,&iter,path)) {
      static gchar *old_comment=NULL;
      gchar *comment;
      GtkWindow *tip_window=GTK_WINDOW(self->priv->preset_tips->tip_window);

      gtk_tree_model_get(model,&iter,PRESET_LIST_COMMENT,&comment,-1);
      if(!comment || !old_comment || (comment && old_comment && strcmp(comment,old_comment))) {
        GST_LOG("tip is '%s'",comment);
        //gtk_tooltips_set_tip(self->priv->preset_tips,GTK_WIDGET(tree_view),(comment?comment:""),NULL);
        gtk_tooltips_set_tip(self->priv->preset_tips,GTK_WIDGET(tree_view),comment,NULL);
        if(tip_window && GTK_WIDGET_VISIBLE(tip_window)) {
          GdkRectangle vr,cr;
          gint ox,oy,tx,ty;

          gtk_tree_view_get_visible_rect(tree_view,&vr);
          gtk_tree_view_get_background_area(tree_view,path,column,&cr);
          gdk_window_get_origin(bin_window,&tx,&ty);
          gtk_tree_view_tree_to_widget_coords(tree_view,vr.x+cr.x,vr.y+cr.y,&ox,&oy);
          GST_INFO("tx=%4d,ty=%4d  ox=%4d,oy=%4d",tx,ty,ox,oy);
          //tx += GTK_WIDGET(tree_view)->allocation.x + ox + cr.width / 2 - (GTK_WIDGET(tip_window)->allocation.width / 2 + 4);
          //ty += GTK_WIDGET(tree_view)->allocation.y + oy + cr.height;
          tx += ox + cr.width / 2 - (GTK_WIDGET(tip_window)->allocation.width / 2 + 4);
          ty += oy + cr.height;
          GST_INFO("tx=%4d,ty=%4d",tx,ty);
          gtk_window_move(tip_window,tx,ty);
        }
        old_comment=comment;
      }
    }
    gtk_tree_path_free(path);
  }
}
#else
static gboolean on_preset_list_query_tooltip(GtkWidget *widget,gint x,gint y,gboolean keyboard_mode,GtkTooltip *tooltip,gpointer user_data) {
  GtkTreeView *tree_view=GTK_TREE_VIEW(widget);
  GtkTreePath *path;
  GtkTreeViewColumn *column;
  gboolean res=FALSE;

  if(gtk_tree_view_get_path_at_pos(tree_view,x,y,&path,&column,NULL,NULL)) {
    GtkTreeModel *model;
    GtkTreeIter iter;

    model=gtk_tree_view_get_model(tree_view);
    if(gtk_tree_model_get_iter(model,&iter,path)) {
      gchar *comment;
      
      gtk_tree_model_get(model,&iter,PRESET_LIST_COMMENT,&comment,-1);
      if(comment) {
        GST_LOG("tip is '%s'",comment);
        gtk_tooltip_set_text(tooltip,comment);
        res=TRUE;
      }
    }
    gtk_tree_path_free(path);
  }
  return(res);
}
#endif

static void on_preset_list_selection_changed(GtkTreeSelection *treeselection,gpointer user_data) {
  gtk_widget_set_sensitive(GTK_WIDGET(user_data),(gtk_tree_selection_count_selected_rows(treeselection)!=0));
}

static void on_toolbar_style_changed(const BtSettings *settings,GParamSpec *arg,gpointer user_data) {
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);
  gchar *toolbar_style;

  g_object_get(G_OBJECT(settings),"toolbar-style",&toolbar_style,NULL);
  if(!BT_IS_STRING(toolbar_style)) return;

  GST_INFO("!!!  toolbar style has changed '%s'",toolbar_style);
  gtk_toolbar_set_style(GTK_TOOLBAR(self->priv->main_toolbar),gtk_toolbar_get_style_from_string(toolbar_style));
  if(self->priv->preset_toolbar) {
    gtk_toolbar_set_style(GTK_TOOLBAR(self->priv->preset_toolbar),gtk_toolbar_get_style_from_string(toolbar_style));
  }
  g_free(toolbar_style);
}

/*
 * on_box_size_request:
 *
 * we adjust the scrollable-window size to contain the whole area
 */
static void on_box_size_request(GtkWidget *widget,GtkRequisition *requisition,gpointer user_data) {
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);
  GtkWidget *parent=widget->parent->parent;
  gint height=requisition->height,width=-1;
  gint max_height=gdk_screen_get_height(gdk_screen_get_default());
  gint available_heigth;

  GST_DEBUG("#### box size req %d x %d (max-height=%d, toolbar-height=%d)",
    requisition->width,requisition->height,
    max_height,self->priv->main_toolbar->allocation.height);
  // have a minimum width
  if(requisition->width<300) {
    width=300;
  }
  // constrain the height by screen height minus some space for panels, deco and
  // our toolbar
  available_heigth=max_height-SCREEN_BORDER_HEIGHT-self->priv->main_toolbar->allocation.height;
  if(height>available_heigth) {
    height=available_heigth;
  }
  // @todo: is the '2' some border or padding
  gtk_widget_set_size_request(parent,width,height + 2);
}

//-- helper methods

static GtkWidget *make_int_range_widget(const BtMachinePropertiesDialog *self,GstObject *machine,GParamSpec *property,GValue *range_min,GValue *range_max,GtkWidget *label) {
  GtkWidget *widget;
  gchar *signal_name;
  gint value;

  g_object_get(machine,property->name,&value,NULL);
  //step=(int_property->maximum-int_property->minimum)/1024.0;
  widget=gtk_hscale_new_with_range(g_value_get_int(range_min),g_value_get_int(range_max),1.0);
  gtk_scale_set_draw_value(GTK_SCALE(widget),/*TRUE*/FALSE);
  //gtk_scale_set_value_pos(GTK_SCALE(widget),GTK_POS_RIGHT);
  gtk_range_set_value(GTK_RANGE(widget),value);
  gtk_widget_set_name(GTK_WIDGET(widget),property->name);
  g_object_set_qdata(G_OBJECT(widget),widget_label_quark,(gpointer)label);
  g_object_set_qdata(G_OBJECT(widget),widget_parent_quark,(gpointer)self);
  // @todo add numerical entry as well ?

  signal_name=g_alloca(9+strlen(property->name));
  g_sprintf(signal_name,"notify::%s",property->name);
  g_signal_connect(G_OBJECT(machine), signal_name, G_CALLBACK(on_int_range_property_notify), (gpointer)widget);
  g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(on_int_range_property_changed), (gpointer)machine);
  /* we have draw_value=FALSE
  if(GST_IS_ELEMENT(machine)) {
    g_signal_connect(G_OBJECT(widget), "format-value", G_CALLBACK(on_int_range_global_property_format_value), (gpointer)self->priv->machine);
  }
  else {
    g_signal_connect(G_OBJECT(widget), "format-value", G_CALLBACK(on_int_range_voice_property_format_value), (gpointer)self->priv->machine);
  }
  */
  g_signal_connect(G_OBJECT(widget),"button-press-event",G_CALLBACK(on_range_button_press_event), (gpointer)machine);
  g_signal_connect(G_OBJECT(widget),"button-release-event",G_CALLBACK(on_button_release_event), (gpointer)machine);

  // update formatted text on label
  update_int_range_label(self,GTK_RANGE(widget),machine,GTK_LABEL(label),(gdouble)value);
  return(widget);
}

static GtkWidget *make_uint_range_widget(const BtMachinePropertiesDialog *self,GstObject *machine,GParamSpec *property,GValue *range_min,GValue *range_max,GtkWidget *label) {
  GtkWidget *widget;
  gchar *signal_name;
  gint value;

  g_object_get(machine,property->name,&value,NULL);
  //step=(int_property->maximum-int_property->minimum)/1024.0;
  widget=gtk_hscale_new_with_range(g_value_get_uint(range_min),g_value_get_uint(range_max),1.0);
  gtk_scale_set_draw_value(GTK_SCALE(widget),/*TRUE*/FALSE);
  //gtk_scale_set_value_pos(GTK_SCALE(widget),GTK_POS_RIGHT);
  gtk_range_set_value(GTK_RANGE(widget),value);
  gtk_widget_set_name(GTK_WIDGET(widget),property->name);
  g_object_set_qdata(G_OBJECT(widget),widget_label_quark,(gpointer)label);
  g_object_set_qdata(G_OBJECT(widget),widget_parent_quark,(gpointer)self);
  // @todo add numerical entry as well ?

  signal_name=g_alloca(9+strlen(property->name));
  g_sprintf(signal_name,"notify::%s",property->name);
  g_signal_connect(G_OBJECT(machine),signal_name,G_CALLBACK(on_uint_range_property_notify), (gpointer)widget);
  g_signal_connect(G_OBJECT(widget),"value-changed",G_CALLBACK(on_uint_range_property_changed), (gpointer)machine);
  /* we have draw_value=FALSE
  if(GST_IS_ELEMENT(machine)) {
    g_signal_connect(G_OBJECT(widget),"format-value",G_CALLBACK(on_uint_range_global_property_format_value), (gpointer)self->priv->machine);
  }
  else {
    g_signal_connect(G_OBJECT(widget),"format-value",G_CALLBACK(on_uint_range_voice_property_format_value), (gpointer)self->priv->machine);
  }
  */
  g_signal_connect(G_OBJECT(widget),"button-press-event",G_CALLBACK(on_range_button_press_event), (gpointer)machine);
  g_signal_connect(G_OBJECT(widget),"button-release-event",G_CALLBACK(on_button_release_event), (gpointer)machine);

  // update formatted text on label
  update_uint_range_label(self,GTK_RANGE(widget),machine,GTK_LABEL(label),(gdouble)value);
  return(widget);
}

static GtkWidget *make_float_range_widget(const BtMachinePropertiesDialog *self,GstObject *machine,GParamSpec *property,GValue *range_min,GValue *range_max,GtkWidget *label) {
  GtkWidget *widget;
  gchar *signal_name;
  gfloat step,value;
  gfloat value_min=g_value_get_float(range_min);
  gfloat value_max=g_value_get_float(range_max);

  g_object_get(machine,property->name,&value,NULL);
  step=((gdouble)value_max-(gdouble)value_min)/1024.0;
  //GST_WARNING("step = %f", step);

  widget=gtk_hscale_new_with_range(value_min,value_max,step);
  gtk_scale_set_draw_value(GTK_SCALE(widget),/*TRUE*/FALSE);
  //gtk_scale_set_value_pos(GTK_SCALE(widget),GTK_POS_RIGHT);
  gtk_range_set_value(GTK_RANGE(widget),value);
  gtk_widget_set_name(GTK_WIDGET(widget),property->name);
  g_object_set_qdata(G_OBJECT(widget),widget_label_quark,(gpointer)label);
  g_object_set_qdata(G_OBJECT(widget),widget_parent_quark,(gpointer)self);
  // @todo add numerical entry as well ?

  signal_name=g_alloca(9+strlen(property->name));
  g_sprintf(signal_name,"notify::%s",property->name);
  g_signal_connect(G_OBJECT(machine), signal_name, G_CALLBACK(on_float_range_property_notify), (gpointer)widget);
  g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(on_float_range_property_changed), (gpointer)machine);
  //g_signal_connect(G_OBJECT(widget), "format-value", G_CALLBACK(on_float_range_global_property_format_value), (gpointer)machine);

  g_signal_connect(G_OBJECT(widget),"button-press-event",G_CALLBACK(on_range_button_press_event), (gpointer)machine);
  g_signal_connect(G_OBJECT(widget),"button-release-event",G_CALLBACK(on_button_release_event), (gpointer)machine);

  // update formatted text on label
  update_float_range_label(GTK_LABEL(label),value);
  return(widget);
}

static GtkWidget *make_double_range_widget(const BtMachinePropertiesDialog *self,GstObject *machine,GParamSpec *property,GValue *range_min,GValue *range_max,GtkWidget *label) {
  GtkWidget *widget;
  gchar *signal_name;
  gdouble step,value;
  gdouble value_min=g_value_get_double(range_min);
  gdouble value_max=g_value_get_double(range_max);

  g_object_get(machine,property->name,&value,NULL);
  step=(value_max-value_min)/1024.0;

  widget=gtk_hscale_new_with_range(value_min,value_max,step);
  gtk_scale_set_draw_value(GTK_SCALE(widget),/*TRUE*/FALSE);
  //gtk_scale_set_value_pos(GTK_SCALE(widget),GTK_POS_RIGHT);
  gtk_range_set_value(GTK_RANGE(widget),value);
  gtk_widget_set_name(GTK_WIDGET(widget),property->name);
  g_object_set_qdata(G_OBJECT(widget),widget_label_quark,(gpointer)label);
  g_object_set_qdata(G_OBJECT(widget),widget_parent_quark,(gpointer)self);
  // @todo add numerical entry as well ?

  signal_name=g_alloca(9+strlen(property->name));
  g_sprintf(signal_name,"notify::%s",property->name);
  g_signal_connect(G_OBJECT(machine), signal_name, G_CALLBACK(on_double_range_property_notify), (gpointer)widget);
  g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(on_double_range_property_changed), (gpointer)machine);
  //g_signal_connect(G_OBJECT(widget), "format-value", G_CALLBACK(on_double_range_global_property_format_value), (gpointer)machine);

  g_signal_connect(G_OBJECT(widget),"button-press-event",G_CALLBACK(on_range_button_press_event), (gpointer)machine);
  g_signal_connect(G_OBJECT(widget),"button-release-event",G_CALLBACK(on_button_release_event), (gpointer)machine);

  // update formatted text on label
  update_double_range_label(GTK_LABEL(label),value);
  return(widget);
}

static GtkWidget *make_combobox_widget(const BtMachinePropertiesDialog *self,GstObject *machine,GParamSpec *property,GValue *range_min,GValue *range_max) {
  GtkWidget *widget;
  gchar *signal_name;
  GParamSpecEnum *enum_property=G_PARAM_SPEC_ENUM(property);
  GEnumClass *enum_class=enum_property->enum_class;
  GEnumValue *enum_value;
  GtkCellRenderer *renderer;
  GtkListStore *store;
  GtkTreeIter iter;
  gint value, ivalue;

  widget=gtk_combo_box_new();

  // need a real model because of sparse enums
  store=gtk_list_store_new(2,G_TYPE_INT,G_TYPE_STRING);
  for(value=enum_class->minimum;value<=enum_class->maximum;value++) {
    if((enum_value=g_enum_get_value(enum_class, value))) {
      //gtk_combo_box_append_text(GTK_COMBO_BOX(widget),enum_value->value_nick);
      if(BT_IS_STRING(enum_value->value_nick)) {
        gtk_list_store_append(store,&iter);
        gtk_list_store_set(store,&iter,
          0,enum_value->value,
          1,enum_value->value_nick,
          -1);
      }
    }
  }
  renderer=gtk_cell_renderer_text_new();
  gtk_cell_renderer_set_fixed_size(renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget),renderer,TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(widget),renderer,"text",1,NULL);
  gtk_combo_box_set_model(GTK_COMBO_BOX(widget),GTK_TREE_MODEL(store));

  g_object_get(machine,property->name,&value,NULL);
  gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store),&iter);
  do {
    gtk_tree_model_get((GTK_TREE_MODEL(store)),&iter,0,&ivalue,-1);
    if(ivalue==value) break;
  } while(gtk_tree_model_iter_next(GTK_TREE_MODEL(store),&iter));
  
  gtk_combo_box_set_active_iter(GTK_COMBO_BOX(widget),&iter);
  gtk_widget_set_name(GTK_WIDGET(widget),property->name);
  g_object_set_qdata(G_OBJECT(widget),widget_parent_quark,(gpointer)self);

  signal_name=g_alloca(9+strlen(property->name));
  g_sprintf(signal_name,"notify::%s",property->name);
  g_signal_connect(G_OBJECT(machine), signal_name, G_CALLBACK(on_combobox_property_notify), (gpointer)widget);
  g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(on_combobox_property_changed), (gpointer)machine);

  //g_signal_connect(G_OBJECT(widget),"button-press-event",G_CALLBACK(on_range_button_press_event), (gpointer)machine);
  //g_signal_connect(G_OBJECT(widget),"button-release-event",G_CALLBACK(on_button_release_event), (gpointer)machine);

  return(widget);
}

static GtkWidget *make_checkbox_widget(const BtMachinePropertiesDialog *self,GstObject *machine,GParamSpec *property) {
  GtkWidget *widget;
  gchar *signal_name;
  gboolean value;

  g_object_get(machine,property->name,&value,NULL);

  widget=gtk_check_button_new();
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),value);
  gtk_widget_set_name(GTK_WIDGET(widget),property->name);
  g_object_set_qdata(G_OBJECT(widget),widget_parent_quark,(gpointer)self);

  signal_name=g_alloca(9+strlen(property->name));
  g_sprintf(signal_name,"notify::%s",property->name);
  g_signal_connect(G_OBJECT(machine), signal_name, G_CALLBACK(on_checkbox_property_notify), (gpointer)widget);
  g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(on_checkbox_property_toggled), (gpointer)machine);

  g_signal_connect(G_OBJECT(widget),"button-press-event",G_CALLBACK(on_trigger_button_press_event), (gpointer)machine);
  g_signal_connect(G_OBJECT(widget),"button-release-event",G_CALLBACK(on_button_release_event), (gpointer)machine);

  return(widget);
}

static GtkWidget *make_global_param_box(const BtMachinePropertiesDialog *self,gulong global_params,gulong voice_params,GstElement *machine) {
  GtkWidget *expander=NULL;
  GtkWidget *label,*table;
  GtkWidget *widget1,*widget2;
  GParamSpec *property;
  GValue *range_min,*range_max;
  GType base_type;
  gulong i,k,params;
#if !GTK_CHECK_VERSION(2,12,0)
  GtkTooltips *tips=gtk_tooltips_new();
#endif

  // determine params to be skipped
  params=global_params;
  for(i=0;i<global_params;i++) {
    if(bt_machine_is_global_param_trigger(self->priv->machine,i)) params--;
    else {
      if(voice_params && bt_machine_get_voice_param_index(self->priv->machine,bt_machine_get_global_param_name(self->priv->machine,i),NULL)>-1) params--;
    }
  }
  if(params) {
    expander=gtk_expander_new(_("global properties"));
    gtk_expander_set_expanded(GTK_EXPANDER(expander),TRUE);

    // add global machine controls into the table
    table=gtk_table_new(/*rows=*/params+1,/*columns=*/3,/*homogenous=*/FALSE);
    self->priv->num_global=1;

    for(i=0,k=0;i<global_params;i++) {
      if(bt_machine_is_global_param_trigger(self->priv->machine,i)) continue;
      if(voice_params && bt_machine_get_voice_param_index(self->priv->machine,bt_machine_get_global_param_name(self->priv->machine,i),NULL)>-1) continue;

      bt_machine_get_global_param_details(self->priv->machine,i,&property,&range_min,&range_max);
      GST_INFO("global property %p has name '%s','%s'",property,property->name,bt_machine_get_global_param_name(self->priv->machine,i));
      // get name
      label=gtk_label_new((gchar *)bt_machine_get_global_param_name(self->priv->machine,i));
      gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
      gtk_table_attach(GTK_TABLE(table),label, 0, 1, k, k+1, GTK_FILL,GTK_SHRINK, 2,1);
     
      base_type=bt_g_type_get_base_type(property->value_type);
      GST_INFO("... base type is : %s",g_type_name(base_type));

      // DEBUG
      if(range_min && range_max) {
        gchar *str_min=g_strdup_value_contents(range_min);
        gchar *str_max=g_strdup_value_contents(range_max);
        GST_INFO("... has range : %s ... %s",str_min,str_max);
        g_free(str_min);g_free(str_max);
      }
      // DEBUG

      // implement widget types
      switch(base_type) {
        case G_TYPE_STRING:
          widget1=gtk_label_new("string");
          widget2=NULL;
          break;
        case G_TYPE_BOOLEAN:
          widget1=make_checkbox_widget(self,GST_OBJECT(machine),property);
          widget2=NULL;
          break;
        case G_TYPE_INT:
          widget2=gtk_label_new(NULL);
          widget1=make_int_range_widget(self,GST_OBJECT(machine),property,range_min,range_max,widget2);
         break;
        case G_TYPE_UINT:
          widget2=gtk_label_new(NULL);
          widget1=make_uint_range_widget(self,GST_OBJECT(machine),property,range_min,range_max,widget2);
          break;
        case G_TYPE_FLOAT:
          widget2=gtk_label_new(NULL);
          widget1=make_float_range_widget(self,GST_OBJECT(machine),property,range_min,range_max,widget2);
          break;
        case G_TYPE_DOUBLE:
          widget2=gtk_label_new(NULL);
          widget1=make_double_range_widget(self,GST_OBJECT(machine),property,range_min,range_max,widget2);
          break;
        case G_TYPE_ENUM: {
          if(property->value_type==GSTBT_TYPE_TRIGGER_SWITCH)
            widget1=make_checkbox_widget(self,GST_OBJECT(machine),property);
          else
            widget1=make_combobox_widget(self,GST_OBJECT(machine),property,range_min,range_max);
          widget2=NULL;
        } break;
        default: {
          gchar *str=g_strdup_printf("unhandled type \"%s\"",G_PARAM_SPEC_TYPE_NAME(property));
          widget1=gtk_label_new(str);g_free(str);
          widget2=NULL;
        }
      }
      if(range_min) { g_free(range_min);range_min=NULL; }
      if(range_max) { g_free(range_max);range_max=NULL; }

      gtk_widget_set_tooltip_text(widget1,g_param_spec_get_blurb(property));
      if(!widget2) {
        gtk_table_attach(GTK_TABLE(table),widget1, 1, 3, k, k+1, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
      }
      else {
        gtk_widget_set_tooltip_text(widget2,g_param_spec_get_blurb(property));
        gtk_table_attach(GTK_TABLE(table),widget1, 1, 2, k, k+1, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
        /* @todo how can we avoid the wobble here?
         * hack would be to set some 'good' default size
         * if we use GTK_FILL|GTK_EXPAND than it uses too much space (same as widget1)
         */
        gtk_widget_set_size_request(widget2,DEFAULT_LABEL_WIDTH,-1);
        if(GTK_IS_LABEL(widget2)) {
          gtk_label_set_ellipsize(GTK_LABEL(widget2),PANGO_ELLIPSIZE_END);
          gtk_label_set_single_line_mode(GTK_LABEL(widget2),TRUE);
          gtk_misc_set_alignment(GTK_MISC(widget2),0.0,0.5);
        }
        gtk_table_attach(GTK_TABLE(table),widget2, 2, 3, k, k+1, GTK_FILL,GTK_SHRINK, 2,1);
      }
      k++;
    }
    // eat remaning space
    //gtk_table_attach(GTK_TABLE(table),gtk_label_new(" "), 0, 3, k, k+1, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
    gtk_container_add(GTK_CONTAINER(expander),table);
  }
  else {
    GST_INFO("all global params skipped");
  }
  return(expander);
}

static GtkWidget *make_voice_param_box(const BtMachinePropertiesDialog *self,gulong voice_params,gulong voice,GstElement *machine) {
  GtkWidget *expander=NULL;
  GtkWidget *label,*table;
  GtkWidget *widget1,*widget2;
  GParamSpec *property;
  GValue *range_min,*range_max;
  GType base_type;
  GstObject *machine_voice;
  gchar *name;
  gulong i,k,params;
#if !GTK_CHECK_VERSION(2,12,0)
  GtkTooltips *tips=gtk_tooltips_new();
#endif

  params=voice_params;
  for(i=0;i<voice_params;i++) {
    if(bt_machine_is_voice_param_trigger(self->priv->machine,i)) {
      GST_INFO("skipping voice param %lu",i);
      params--;
    }
  }
  if(params) {
    name=g_strdup_printf(_("voice %lu properties"),voice+1);
    expander=gtk_expander_new(name);
    gtk_expander_set_expanded(GTK_EXPANDER(expander),TRUE);
    g_free(name);

    machine_voice=gst_child_proxy_get_child_by_index(GST_CHILD_PROXY(machine),voice);
    if(!machine_voice) {
      GST_WARNING("Cannot get voice child for voice %lu",voice);
    }

    // add voice machine controls into the table
    table=gtk_table_new(/*rows=*/params+1,/*columns=*/2,/*homogenous=*/FALSE);

    for(i=0,k=0;i<voice_params;i++) {
      if(bt_machine_is_voice_param_trigger(self->priv->machine,i)) {
        GST_INFO("skipping voice param %lu",i);
        continue;
      }

      bt_machine_get_voice_param_details(self->priv->machine,i,&property,&range_min,&range_max);
      GST_INFO("voice property %p has name '%s','%s'",property,property->name,bt_machine_get_voice_param_name(self->priv->machine,i));
      // get name
      label=gtk_label_new((gchar *)bt_machine_get_voice_param_name(self->priv->machine,i));
      gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
      gtk_table_attach(GTK_TABLE(table),label, 0, 1, k, k+1, GTK_FILL,GTK_SHRINK, 2,1);

      base_type=bt_g_type_get_base_type(property->value_type);
      GST_INFO("... base typoe is : %s",g_type_name(base_type));
      
      // DEBUG
      if(range_min && range_max) {
        gchar *str_min=g_strdup_value_contents(range_min);
        gchar *str_max=g_strdup_value_contents(range_max);
        GST_INFO("... has range : %s ... %s",str_min,str_max);
        g_free(str_min);g_free(str_max);
      }
      // DEBUG

      // implement widget types
      switch(base_type) {
        case G_TYPE_STRING:
          widget1=gtk_label_new("string");
          widget2=NULL;
          break;
        case G_TYPE_BOOLEAN:
          widget1=make_checkbox_widget(self,machine_voice,property);
          widget2=NULL;
          break;
        case G_TYPE_INT:
          widget2=gtk_label_new(NULL);
          widget1=make_int_range_widget(self,machine_voice,property,range_min,range_max,widget2);
          break;
        case G_TYPE_UINT:
          widget2=gtk_label_new(NULL);
          widget1=make_uint_range_widget(self,machine_voice,property,range_min,range_max,widget2);
          break;
        case G_TYPE_FLOAT:
          widget2=gtk_label_new(NULL);
          widget1=make_float_range_widget(self,machine_voice,property,range_min,range_max,widget2);
          break;
        case G_TYPE_DOUBLE:
          widget2=gtk_label_new(NULL);
          widget1=make_double_range_widget(self,machine_voice,property,range_min,range_max,widget2);
          break;
        case G_TYPE_ENUM: {
          if(property->value_type==GSTBT_TYPE_TRIGGER_SWITCH)
            widget1=make_checkbox_widget(self,machine_voice,property);
          else
            widget1=make_combobox_widget(self,machine_voice,property,range_min,range_max);
          widget2=NULL;
        } break;
        default: {
          gchar *str=g_strdup_printf("unhandled type \"%s\"",G_PARAM_SPEC_TYPE_NAME(property));
          widget1=gtk_label_new(str);g_free(str);
          widget2=NULL;
        }
      }
      if(range_min) { g_free(range_min);range_min=NULL; }
      if(range_max) { g_free(range_max);range_max=NULL; }

      gtk_widget_set_tooltip_text(widget1,g_param_spec_get_blurb(property));
      if(!widget2) {
        gtk_table_attach(GTK_TABLE(table),widget1, 1, 3, k, k+1, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
      }
      else {
        gtk_widget_set_tooltip_text(widget2,g_param_spec_get_blurb(property));
        gtk_table_attach(GTK_TABLE(table),widget1, 1, 2, k, k+1, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
        /* @todo how can we avoid the wobble here?
         * hack would be to set some 'good' default size
         * if we use GTK_FILL|GTK_EXPAND than it uses too much space (same as widget1)
         */
        gtk_widget_set_size_request(widget2,DEFAULT_LABEL_WIDTH,-1);
        if(GTK_IS_LABEL(widget2)) {
          gtk_label_set_ellipsize(GTK_LABEL(widget2),PANGO_ELLIPSIZE_END);
          gtk_label_set_single_line_mode(GTK_LABEL(widget2),TRUE);
          gtk_misc_set_alignment(GTK_MISC(widget2),0.0,0.5);
        }
        gtk_table_attach(GTK_TABLE(table),widget2, 2, 3, k, k+1, GTK_FILL,GTK_SHRINK, 2,1);
      }
      k++;
    }
    // eat remaning space
    //gtk_table_attach(GTK_TABLE(table),gtk_label_new(" "), 0, 3, k, k+1, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
    gtk_container_add(GTK_CONTAINER(expander),table);
    g_object_unref(machine_voice);
  }
  return(expander);
}


static void on_machine_voices_notify(const BtMachine *machine,GParamSpec *arg,gpointer user_data) {
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);
  gulong i,new_voices,voice_params;
  GtkWidget *expander;
  GstElement *machine_object;

  g_object_get(G_OBJECT(machine),
    "voices",&new_voices,
    "voice-params",&voice_params,
    "machine",&machine_object,
    NULL);

  GST_INFO("voices changed: %lu -> %lu",self->priv->voices,new_voices);

  if(new_voices>self->priv->voices) {
    for(i=self->priv->voices;i<new_voices;i++) {
      // add ui for voice
      if((expander=make_voice_param_box(self,voice_params,i,machine_object))) {
        gtk_box_pack_start(GTK_BOX(self->priv->param_group_box),expander,TRUE,TRUE,0);
        gtk_box_reorder_child(GTK_BOX(self->priv->param_group_box),expander,self->priv->num_global+i);
        gtk_widget_show_all(expander);
      }
    }
    gst_object_unref(machine_object);
  }
  else {
    GList *children,*node;

    children=gtk_container_get_children(GTK_CONTAINER(self->priv->param_group_box));
    node=g_list_last(children);
    // skip wire param boxes
    for(i=0;i<self->priv->num_wires;i++) node=g_list_previous(node);
    for(i=self->priv->voices;i>new_voices;i--) {
      // remove ui for voice
      gtk_container_remove(GTK_CONTAINER(self->priv->param_group_box),GTK_WIDGET(node->data));
      // no need to disconnect signals as the voice_child is already gone
      node=g_list_previous(node);
    }
    g_list_free(children);
  }

  self->priv->voices=new_voices;
}

static GtkWidget *make_wire_param_box(const BtMachinePropertiesDialog *self,BtWire *wire) {
  GtkWidget *expander=NULL;
  GtkWidget *label,*table;
  GtkWidget *widget1,*widget2;
  GstObject *param_parent;
  GParamSpec *property;
  GValue *range_min,*range_max;
  GType base_type;
  gchar *src_id,*name;
  gulong i,params;
#if !GTK_CHECK_VERSION(2,12,0)
  GtkTooltips *tips=gtk_tooltips_new();
#endif
  BtMachine *src;

  g_object_get(wire,"num-params",&params,"src",&src,NULL);
  if(params) {
    g_object_get(src,"id",&src_id,NULL);
    name=g_strdup_printf(_("%s wire properties"),src_id);
    expander=gtk_expander_new(name);
    gtk_expander_set_expanded(GTK_EXPANDER(expander),TRUE);
    g_free(name);
    g_free(src_id);

    // add wire controls into the table
    table=gtk_table_new(/*rows=*/params+1,/*columns=*/2,/*homogenous=*/FALSE);
    g_hash_table_insert(self->priv->group_to_object,wire,expander);
    self->priv->num_wires++;

    for(i=0;i<params;i++) {
      bt_wire_get_param_details(wire,i,&property,&range_min,&range_max);
      GST_INFO("wire property %p has name '%s','%s'",property,property->name,bt_wire_get_param_name(wire,i));
      
      // get name
      label=gtk_label_new((gchar *)bt_wire_get_param_name(wire,i));
      gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
      gtk_table_attach(GTK_TABLE(table),label, 0, 1, i, i+1, GTK_FILL,GTK_SHRINK, 2,1);

      base_type=bt_g_type_get_base_type(property->value_type);
      GST_INFO("... base typoe is : %s",g_type_name(base_type));

      // DEBUG
      if(range_min && range_max) {
        gchar *str_min=g_strdup_value_contents(range_min);
        gchar *str_max=g_strdup_value_contents(range_max);
        GST_INFO("... has range : %s ... %s",str_min,str_max);
        g_free(str_min);g_free(str_max);
      }
      // DEBUG
      
      // bah, hardcoded hack
      switch(i) {
        case 0:
          g_object_get(wire,"gain",&param_parent,NULL);
          break;
        case 1:
          g_object_get(wire,"pan",&param_parent,NULL);
          break;
        default:
          GST_WARNING("unimplemented wire param");
          break;
      }

      // implement widget types
      switch(base_type) {
        case G_TYPE_FLOAT:
          widget2=gtk_label_new(NULL);
          widget1=make_float_range_widget(self,param_parent,property,range_min,range_max,widget2);
          break;
        case G_TYPE_DOUBLE:
          widget2=gtk_label_new(NULL);
          widget1=make_double_range_widget(self,param_parent,property,range_min,range_max,widget2);
          break;
        default: {
          gchar *str=g_strdup_printf("unhandled type \"%s\"",G_PARAM_SPEC_TYPE_NAME(property));
          widget1=gtk_label_new(str);g_free(str);
          widget2=NULL;
        }
      }
      if(range_min) { g_free(range_min);range_min=NULL; }
      if(range_max) { g_free(range_max);range_max=NULL; }

      gtk_widget_set_tooltip_text(widget1,g_param_spec_get_blurb(property));
      if(!widget2) {
        gtk_table_attach(GTK_TABLE(table),widget1, 1, 3, i, i+1, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
      }
      else {
        gtk_widget_set_tooltip_text(widget2,g_param_spec_get_blurb(property));
        gtk_table_attach(GTK_TABLE(table),widget1, 1, 2, i, i+1, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
        /* @todo how can we avoid the wobble here?
         * hack would be to set some 'good' default size
         * if we use GTK_FILL|GTK_EXPAND than it uses too much space (same as widget1)
         */
        gtk_widget_set_size_request(widget2,DEFAULT_LABEL_WIDTH,-1);
        if(GTK_IS_LABEL(widget2)) {
          gtk_label_set_ellipsize(GTK_LABEL(widget2),PANGO_ELLIPSIZE_END);
          gtk_label_set_single_line_mode(GTK_LABEL(widget2),TRUE);
          gtk_misc_set_alignment(GTK_MISC(widget2),0.0,0.5);
        }
        gtk_table_attach(GTK_TABLE(table),widget2, 2, 3, i, i+1, GTK_FILL,GTK_SHRINK, 2,1);
      }
      
      gst_object_unref(param_parent);
      param_parent=NULL;
    }
    // eat remaning space
    //gtk_table_attach(GTK_TABLE(table),gtk_label_new(" "), 0, 3, k, k+1, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
    gtk_container_add(GTK_CONTAINER(expander),table);
  }
  g_object_unref(src);
  return(expander);
}

static void on_wire_added(const BtSetup *setup,BtWire *wire,gpointer user_data) {
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);
  GtkWidget *expander;
  BtMachine *dst;
  
  g_object_get(wire,"dst",&dst,NULL);
  if(dst==self->priv->machine) {
    if((expander=make_wire_param_box(self,wire))) {
      gtk_box_pack_start(GTK_BOX(self->priv->param_group_box),expander,TRUE,TRUE,0);
      gtk_widget_show_all(expander);
    }
  }
  g_object_unref(dst);
}

static void on_wire_removed(const BtSetup *setup,BtWire *wire,gpointer user_data) {
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);
  GtkWidget *expander;

  // determine the right expander
  if((expander=g_hash_table_lookup(self->priv->group_to_object,wire))) {
    gtk_container_remove(GTK_CONTAINER(self->priv->param_group_box),GTK_WIDGET(expander));
    g_hash_table_remove(self->priv->group_to_object,wire);
    self->priv->num_wires--;
  }
}

static gboolean bt_machine_properties_dialog_init_preset_box(const BtMachinePropertiesDialog *self) {
  GtkWidget *scrolled_window;
  GtkWidget *tool_item,*remove_tool_button,*edit_tool_button;
  GtkTreeSelection *tree_sel;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *tree_col;
#if !GTK_CHECK_VERSION(2,12,0)
  GtkTooltips *tips=gtk_tooltips_new();
#endif

  self->priv->preset_box=gtk_vbox_new(FALSE,0);

  // add preset controls toolbar
  self->priv->preset_toolbar=gtk_toolbar_new();

  tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_ADD));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(tool_item),_("Add new preset"));
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->preset_toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_preset_add_clicked),(gpointer)self);

  remove_tool_button=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_REMOVE));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(tool_item),_("Remove preset"));
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->preset_toolbar),GTK_TOOL_ITEM(remove_tool_button),-1);
  g_signal_connect(G_OBJECT(remove_tool_button),"clicked",G_CALLBACK(on_toolbar_preset_remove_clicked),(gpointer)self);

  edit_tool_button=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_EDIT));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(tool_item),_("Edit preset name and comment"));
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->preset_toolbar),GTK_TOOL_ITEM(edit_tool_button),-1);
  g_signal_connect(G_OBJECT(edit_tool_button),"clicked",G_CALLBACK(on_toolbar_preset_edit_clicked),(gpointer)self);

  gtk_box_pack_start(GTK_BOX(self->priv->preset_box),self->priv->preset_toolbar,FALSE,FALSE,0);

  // add preset list
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_IN);
  self->priv->preset_list=gtk_tree_view_new();
  g_object_set(self->priv->preset_list,"enable-search",FALSE,"rules-hint",TRUE,"fixed-height-mode",TRUE,NULL);
  gtk_widget_set_events(self->priv->preset_list,gtk_widget_get_events(self->priv->preset_list)|GDK_POINTER_MOTION_MASK);
  tree_sel=gtk_tree_view_get_selection(GTK_TREE_VIEW(self->priv->preset_list));
  gtk_tree_selection_set_mode(tree_sel,GTK_SELECTION_SINGLE);
#if !GTK_CHECK_VERSION(2,12,0)
  self->priv->preset_tips=gtk_tooltips_new();
  gtk_tooltips_set_tip(self->priv->preset_tips,self->priv->preset_list,"",NULL);
  g_signal_connect(G_OBJECT(self->priv->preset_list), "motion-notify-event", G_CALLBACK(on_preset_list_motion_notify), (gpointer)self);
#else
  g_object_set(self->priv->preset_list,"has-tooltip",TRUE,NULL);
  g_signal_connect(G_OBJECT(self->priv->preset_list), "query-tooltip", G_CALLBACK(on_preset_list_query_tooltip), (gpointer)self);
  // alternative: gtk_tree_view_set_tooltip_row
#endif
  g_signal_connect(G_OBJECT(self->priv->preset_list), "row-activated", G_CALLBACK(on_preset_list_row_activated), (gpointer)self);
  g_signal_connect(G_OBJECT(tree_sel), "changed", G_CALLBACK(on_preset_list_selection_changed), (gpointer)remove_tool_button);
  g_signal_connect(G_OBJECT(tree_sel), "changed", G_CALLBACK(on_preset_list_selection_changed), (gpointer)edit_tool_button);

  // add cell renderers
  renderer=gtk_cell_renderer_text_new();
  gtk_cell_renderer_set_fixed_size(renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
  g_object_set(renderer,"xalign",0.0,NULL);
  if((tree_col=gtk_tree_view_column_new_with_attributes(_("Preset"),renderer,"text",0,NULL))) {
    g_object_set(tree_col,"sizing",GTK_TREE_VIEW_COLUMN_FIXED,"fixed-width",135,NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(self->priv->preset_list),tree_col,-1);
  }
  else GST_WARNING("can't create treeview column");

  // add list data
  /* @todo: need a presets-changed signal refresh the list
   * - then we can also remove the preset_list_refresh() calls in the callback
   *   that change the presets
   * @todo: need a presets-changed signal on the class level
   * - if we have two instances running, the other wants to reload the list
   */
  preset_list_refresh(self);

  gtk_container_add(GTK_CONTAINER(scrolled_window),self->priv->preset_list);
  gtk_box_pack_start(GTK_BOX(self->priv->preset_box),GTK_WIDGET(scrolled_window),TRUE,TRUE,0);

  // the list is static, don't free
  //g_list_free(presets);
  return(TRUE);
}


static void bt_machine_properties_dialog_init_ui(const BtMachinePropertiesDialog *self) {
  BtMainWindow *main_window;
  BtSong *song;
  BtSetup *setup;
  GtkWidget *param_box,*hbox;
  GtkWidget *expander,*scrolled_window;
  GtkWidget *tool_item;
  gchar *id,*title;
  GdkPixbuf *window_icon=NULL;
  gulong global_params,voice_params;
  GstElement *machine;
  BtSettings *settings;
#if !GTK_CHECK_VERSION(2,12,0)
  GtkTooltips *tips=gtk_tooltips_new();
#endif
  GList *wires;
  
  gtk_widget_set_name(GTK_WIDGET(self),_("machine properties"));

  g_object_get(self->priv->app,"main-window",&main_window,"settings",&settings,"song",&song,NULL);
  g_object_get(song,"setup",&setup,NULL);
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

  // get machine data
  g_object_get(self->priv->machine,
    "id",&id,
    "global-params",&global_params,
    "voice-params",&voice_params,
    "voices",&self->priv->voices,
    "machine",&machine,
    NULL);
  // set dialog title
  title=g_strdup_printf(_("%s properties"),id);
  gtk_window_set_title(GTK_WINDOW(self),title);
  g_free(id);g_free(title);

  GST_INFO("machine has %lu global properties, %lu voice properties and %lu voices",global_params,voice_params,self->priv->voices);
  
  self->priv->group_to_object=g_hash_table_new(NULL,NULL);

  // add widgets to the dialog content area
  // should we use a hpaned or hbox for the presets?
  hbox=gtk_hbox_new(FALSE,12);
  param_box=gtk_vbox_new(FALSE,0);
  //gtk_container_set_border_width(GTK_CONTAINER(param_box),6);
  gtk_box_pack_start(GTK_BOX(hbox),param_box,TRUE,TRUE,0);

  // create preset pane
  if(GST_IS_PRESET(machine)) {
    if(bt_machine_properties_dialog_init_preset_box(self)) {
      gtk_box_pack_end(GTK_BOX(hbox),self->priv->preset_box,FALSE,FALSE,0);
    }
  }

  // create toolbar
  self->priv->main_toolbar=gtk_toolbar_new();

  tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_ABOUT));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(tool_item),_("Info about this machine"));
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->main_toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_about_clicked),(gpointer)self);

  tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_HELP));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(tool_item),_("Help for this machine"));
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->main_toolbar),GTK_TOOL_ITEM(tool_item),-1);
  if(!GSTBT_IS_HELP(machine)) {
    gtk_widget_set_sensitive(tool_item,FALSE);
  }
  else {
    g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_help_clicked),(gpointer)self);
  }

  tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_NEW));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(tool_item),_("Randomize parameters"));
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->main_toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_random_clicked),(gpointer)self);  

  tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_UNDO));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(tool_item),_("Reset parameters to defaults"));
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->main_toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_reset_clicked),(gpointer)self);
  
  // @todo: add copy/paste buttons

  tool_item=GTK_WIDGET(gtk_toggle_tool_button_new_from_stock(GTK_STOCK_INDEX));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(tool_item),_("Show/Hide preset pane"));
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(tool_item),_("Presets"));
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->main_toolbar),GTK_TOOL_ITEM(tool_item),-1);
  if(!GST_IS_PRESET(machine)) {
    gtk_widget_set_sensitive(tool_item,FALSE);
  }
  else {
    /* @todo: add settings for "show presets by default" */
    gtk_widget_set_no_show_all(self->priv->preset_box,TRUE);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(tool_item),FALSE);
    g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_show_hide_clicked),(gpointer)self);
  }

  // let settings control toolbar style
  on_toolbar_style_changed(settings,NULL,(gpointer)self);
  g_signal_connect(G_OBJECT(settings), "notify::toolbar-style", G_CALLBACK(on_toolbar_style_changed), (gpointer)self);

  gtk_box_pack_start(GTK_BOX(param_box),self->priv->main_toolbar,FALSE,FALSE,0);

  // machine controls inside a scrolled window
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_NONE);
  self->priv->param_group_box=gtk_vbox_new(FALSE,0);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),self->priv->param_group_box);
  gtk_box_pack_start(GTK_BOX(param_box),scrolled_window,TRUE,TRUE,0);
  g_signal_connect(G_OBJECT(self->priv->param_group_box),"size-request",G_CALLBACK(on_box_size_request),(gpointer)self);

  /* show widgets for global parameters */
  if(global_params) {
    if((expander=make_global_param_box(self,global_params,voice_params,machine))) {
      gtk_box_pack_start(GTK_BOX(self->priv->param_group_box),expander,TRUE,TRUE,0);
    }
  }
  /* show widgets for voice parameters */
  if(self->priv->voices*voice_params) {
    gulong j;

    for(j=0;j<self->priv->voices;j++) {
      if((expander=make_voice_param_box(self,voice_params,j,machine))) {
        gtk_box_pack_start(GTK_BOX(self->priv->param_group_box),expander,TRUE,TRUE,0);
      }
    }
  }
  /* show volume/panorama widgets for incomming wires */
  if((wires=self->priv->machine->dst_wires)) {
    BtWire *wire;
    GList *node;
    
    for(node=wires;node;node=g_list_next(node)) {
      wire=BT_WIRE(node->data);
      if((expander=make_wire_param_box(self,wire))) {
        gtk_box_pack_start(GTK_BOX(self->priv->param_group_box),expander,TRUE,TRUE,0);
      }
    }
  }
  gtk_container_add(GTK_CONTAINER(self),hbox);

  // dynamically adjust voices
  g_signal_connect(G_OBJECT(self->priv->machine),"notify::voices",G_CALLBACK(on_machine_voices_notify),(gpointer)self);
  // dynamically adjust wire params
  g_signal_connect(G_OBJECT(setup),"wire-added",G_CALLBACK(on_wire_added),(gpointer)self);
  g_signal_connect(G_OBJECT(setup),"wire-removed",G_CALLBACK(on_wire_removed),(gpointer)self);

  g_object_unref(machine);
  g_object_unref(setup);
  g_object_unref(song);
  g_object_unref(main_window);
  g_object_unref(settings);
}

//-- constructor methods

/**
 * bt_machine_properties_dialog_new:
 * @machine: the machine to create the dialog for
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtMachinePropertiesDialog *bt_machine_properties_dialog_new(const BtMachine *machine) {
  BtMachinePropertiesDialog *self;

  self=BT_MACHINE_PROPERTIES_DIALOG(g_object_new(BT_TYPE_MACHINE_PROPERTIES_DIALOG,"machine",machine,NULL));
  bt_machine_properties_dialog_init_ui(self);
  gtk_widget_show_all(GTK_WIDGET(self));
  if(self->priv->preset_box) {
    gtk_widget_set_no_show_all(self->priv->preset_box,FALSE);
  }
  return(self);
}

//-- methods

//-- wrapper

//-- class internals

static void bt_machine_properties_dialog_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec) {
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case MACHINE_PROPERTIES_DIALOG_MACHINE: {
      g_object_try_unref(self->priv->machine);
      self->priv->machine = g_object_try_ref(g_value_get_object(value));
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_machine_properties_dialog_dispose(GObject *object) {
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG(object);
  BtSong *song;
  BtSetup *setup;
  gulong j;
  GstElement *machine;
  GstObject *machine_voice;
  GList *wires;

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  g_object_get(self->priv->app,"song",&song,NULL);
  if(song) {
    g_object_get(song,"setup",&setup,NULL);
  }
  else {
    setup=NULL;
  }
  // disconnect handler for dynamic groups
  g_signal_handlers_disconnect_matched(self->priv->machine,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_machine_voices_notify,self);
  // disconnect wire handlers
  if(setup) {
    g_signal_handlers_disconnect_matched(setup,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_wire_added,self);
    g_signal_handlers_disconnect_matched(setup,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_wire_removed,self);
  }
  // disconnect all handlers that are connected to params
  g_object_get(self->priv->machine,"machine",&machine,NULL);
  g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_float_range_property_notify,NULL);
  g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_double_range_property_notify,NULL);
  g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_int_range_property_notify,NULL);
  g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_uint_range_property_notify,NULL);
  g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_checkbox_property_notify,NULL);
  g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_combobox_property_notify,NULL);
  for(j=0;j<self->priv->voices;j++) {
    machine_voice=gst_child_proxy_get_child_by_index(GST_CHILD_PROXY(machine),j);
    g_signal_handlers_disconnect_matched(machine_voice,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_float_range_property_notify,NULL);
    g_signal_handlers_disconnect_matched(machine_voice,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_double_range_property_notify,NULL);
    g_signal_handlers_disconnect_matched(machine_voice,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_int_range_property_notify,NULL);
    g_signal_handlers_disconnect_matched(machine_voice,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_uint_range_property_notify,NULL);
    g_signal_handlers_disconnect_matched(machine_voice,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_checkbox_property_notify,NULL);
    g_signal_handlers_disconnect_matched(machine_voice,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_combobox_property_notify,NULL);
  }
  // disconnect wire parameters
  if((wires=self->priv->machine->dst_wires)) {
    BtWire *wire;
    GstObject *gain,*pan;
    GList *node;
    
    for(node=wires;node;node=g_list_next(node)) {
      wire=BT_WIRE(node->data);
      g_object_get(wire,"gain",&gain,"pan",&pan,NULL);
      if(gain) {
        g_signal_handlers_disconnect_matched(gain,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_float_range_property_notify,NULL);
        g_signal_handlers_disconnect_matched(gain,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_double_range_property_notify,NULL);
        gst_object_unref(gain);
      }
      if(pan) {
        g_signal_handlers_disconnect_matched(pan,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_float_range_property_notify,NULL);
        g_signal_handlers_disconnect_matched(pan,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_double_range_property_notify,NULL);
        gst_object_unref(pan);
      }
    }
  }
  g_object_unref(machine);
  g_object_try_unref(setup);
  g_object_try_unref(song);

  g_object_try_unref(self->priv->machine);
  g_object_unref(self->priv->app);

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_machine_properties_dialog_finalize(GObject *object) {
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG(object);

  GST_DEBUG("!!!! self=%p",self);
  
  g_hash_table_destroy(self->priv->group_to_object);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_machine_properties_dialog_init(GTypeInstance *instance, gpointer g_class) {
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_MACHINE_PROPERTIES_DIALOG, BtMachinePropertiesDialogPrivate);
  self->priv->app = bt_edit_application_new();
}

static void bt_machine_properties_dialog_class_init(BtMachinePropertiesDialogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  widget_label_quark=g_quark_from_static_string("BtMachinePropertiesDialog::widget-label");
  widget_parent_quark=g_quark_from_static_string("BtMachinePropertiesDialog::widget-parent");
  control_object_quark=g_quark_from_static_string("BtMachinePropertiesDialog::control-object");
  control_property_quark=g_quark_from_static_string("BtMachinePropertiesDialog::control-property");

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtMachinePropertiesDialogPrivate));

  gobject_class->set_property = bt_machine_properties_dialog_set_property;
  gobject_class->dispose      = bt_machine_properties_dialog_dispose;
  gobject_class->finalize     = bt_machine_properties_dialog_finalize;

  g_object_class_install_property(gobject_class,MACHINE_PROPERTIES_DIALOG_MACHINE,
                                  g_param_spec_object("machine",
                                     "machine construct prop",
                                     "Set machine object, the dialog handles",
                                     BT_TYPE_MACHINE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_WRITABLE|G_PARAM_STATIC_STRINGS));

}

GType bt_machine_properties_dialog_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof (BtMachinePropertiesDialogClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_machine_properties_dialog_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtMachinePropertiesDialog),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_machine_properties_dialog_init, // instance_init
    };
    type = g_type_register_static(GTK_TYPE_WINDOW,"BtMachinePropertiesDialog",&info,0);
  }
  return type;
}
