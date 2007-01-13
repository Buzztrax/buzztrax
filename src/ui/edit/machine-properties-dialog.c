/* $Id: machine-properties-dialog.c,v 1.60 2007-01-13 19:47:17 ensonic Exp $
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
 */ 

#define BT_EDIT
#define BT_MACHINE_PROPERTIES_DIALOG_C

#include "bt-edit.h"

//-- property ids

enum {
  MACHINE_PROPERTIES_DIALOG_APP=1,
  MACHINE_PROPERTIES_DIALOG_MACHINE
};

struct _BtMachinePropertiesDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtEditApplication *app;

  /* the underlying machine */
  BtMachine *machine;
  
  GtkWidget *main_toolbar,*preset_toolbar;
  GtkWidget *preset_box;
  GtkWidget *preset_list;
  
  /* widgets and their handlers */
  //GtkWidget *widgets;
  //gulong *notify_id,*change_id;
  
  //GtkWidget *vbox,*scrolled_window;
  //GHashTable *expanders;
};

static GtkDialogClass *parent_class=NULL;

static GQuark range_label_quark=0;
static GQuark range_parent_quark=0;

//-- event handler helper

static gboolean preset_list_edit_preset_meta(const BtMachinePropertiesDialog *self,gchar **name,gchar **comment) {
  gboolean result=FALSE;
  GtkWidget *dialog;
  
  dialog=GTK_WIDGET(bt_machine_preset_properties_dialog_new(self->priv->app,name,comment));
  gtk_window_set_transient_for(GTK_WINDOW(dialog),GTK_WINDOW(self));
  gtk_widget_show_all(dialog);

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
  GList *presets,*node;

  GST_INFO("rebuilding preset list");
  
  g_object_get(G_OBJECT(self->priv->machine),"machine",&machine,NULL);
  presets=gst_preset_get_preset_names(GST_PRESET(machine));
  
  // we store the string twice, as we use the pointer as the key in the hashmap
  // and the string gets copied
  store=gtk_list_store_new(2,G_TYPE_STRING,G_TYPE_POINTER);
  for(node=presets;node;node=g_list_next(node)) {
    //GST_INFO(" adding item : '%s'",node->data);
    gtk_list_store_append(store, &tree_iter);
    gtk_list_store_set(store,&tree_iter,0,node->data,1,node->data,-1);
  }
  gtk_tree_view_set_model(GTK_TREE_VIEW(self->priv->preset_list),GTK_TREE_MODEL(store));
  g_object_unref(store); // drop with treeview
  gst_object_unref(machine);
  GST_INFO("rebuilt preset list");
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

  g_value_init(&int_value,G_TYPE_INT);
  g_value_set_int(&int_value,(gint)value);
  if(!(str=bt_machine_describe_global_param_value(machine,index,&int_value))) {
    static gchar _str[10];
    
    g_sprintf(_str,"%d",(gint)value);
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
  
  g_value_init(&int_value,G_TYPE_INT);
  g_value_set_int(&int_value,(gint)value);
  if(!(str=bt_machine_describe_voice_param_value(machine,index,&int_value))) {
    static gchar _str[10];
    
    g_sprintf(_str,"%d",(gint)value);
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
  
  g_value_init(&uint_value,G_TYPE_UINT);
  g_value_set_uint(&uint_value,(guint)value);
  if(!(str=bt_machine_describe_global_param_value(machine,index,&uint_value))) {
    static gchar _str[10];
    
    g_sprintf(_str,"%u",(guint)value);
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
  
  g_value_init(&uint_value,G_TYPE_UINT);
  g_value_set_uint(&uint_value,(guint)value);
  if(!(str=bt_machine_describe_voice_param_value(machine,index,&uint_value))) {
    static gchar _str[10];
    
    g_sprintf(_str,"%u",(guint)value);
    str=_str;
  }
  return(str);
}

static void on_double_range_property_notify(const GstElement *machine,GParamSpec *property,gpointer user_data) {
  GtkWidget *widget=GTK_WIDGET(user_data);
  GtkLabel *label=GTK_LABEL(g_object_get_qdata(G_OBJECT(widget),range_label_quark));
  gdouble value;
  gchar str[30];
  
  g_assert(user_data);
  //GST_INFO("property value notify received : %s ",property->name);

  g_object_get(G_OBJECT(machine),property->name,&value,NULL);
  g_signal_handlers_block_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_double_range_property_changed,(gpointer)machine);
  gtk_range_set_value(GTK_RANGE(widget),value);
  g_signal_handlers_unblock_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_double_range_property_changed,(gpointer)machine);
  g_sprintf(str,"%lf",value);
  gtk_label_set_text(label,str);
}

static void on_double_range_property_changed(GtkRange *range,gpointer user_data) {
  GstElement *machine=GST_ELEMENT(user_data);
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(range));
  GtkLabel *label=GTK_LABEL(g_object_get_qdata(G_OBJECT(range),range_label_quark));
  gdouble value=gtk_range_get_value(range);
  gchar str[30];
  
  g_assert(user_data);
  GST_INFO("property value change received : %lf",value);

  g_signal_handlers_block_matched(machine,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_double_range_property_notify,(gpointer)range);
  g_object_set(machine,name,value,NULL);
  g_signal_handlers_unblock_matched(machine,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_double_range_property_notify,(gpointer)range);
  g_sprintf(str,"%lf",value);
  gtk_label_set_text(label,str);
}

static void on_float_range_property_notify(const GstElement *machine,GParamSpec *property,gpointer user_data) {
  GtkWidget *widget=GTK_WIDGET(user_data);
  GtkLabel *label=GTK_LABEL(g_object_get_qdata(G_OBJECT(widget),range_label_quark));
  gfloat value;
  gchar str[30];
  
  g_assert(user_data);
  //GST_INFO("property value notify received : %s ",property->name);

  g_object_get(G_OBJECT(machine),property->name,&value,NULL);
  g_signal_handlers_block_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_float_range_property_changed,(gpointer)machine);
  gtk_range_set_value(GTK_RANGE(widget),value);
  g_signal_handlers_unblock_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_float_range_property_changed,(gpointer)machine);
  g_sprintf(str,"%f",value);
  gtk_label_set_text(label,str);
}

static void on_float_range_property_changed(GtkRange *range,gpointer user_data) {
  GstElement *machine=GST_ELEMENT(user_data);
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(range));
  GtkLabel *label=GTK_LABEL(g_object_get_qdata(G_OBJECT(range),range_label_quark));
  gfloat value=gtk_range_get_value(range);
  gchar str[30];
  
  g_assert(user_data);
  GST_INFO("property value change received : %f",value);

  g_signal_handlers_block_matched(machine,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_float_range_property_notify,(gpointer)range);
  g_object_set(machine,name,value,NULL);
  g_signal_handlers_unblock_matched(machine,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_float_range_property_notify,(gpointer)range);
  g_sprintf(str,"%f",value);
  gtk_label_set_text(label,str);
}


static void on_int_range_property_notify(const GstElement *machine,GParamSpec *property,gpointer user_data) {
  GtkWidget *widget=GTK_WIDGET(user_data);
  GtkLabel *label=GTK_LABEL(g_object_get_qdata(G_OBJECT(widget),range_label_quark));
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(g_object_get_qdata(G_OBJECT(widget),range_parent_quark));
  gint value;

  g_assert(user_data);

  g_object_get(G_OBJECT(machine),property->name,&value,NULL);
  //GST_INFO("property value notify received : %s => : %d",property->name,value);

  g_signal_handlers_block_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_int_range_property_changed,(gpointer)machine);
  gtk_range_set_value(GTK_RANGE(widget),(gdouble)value);
  g_signal_handlers_unblock_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_int_range_property_changed,(gpointer)machine);
  if(GST_IS_ELEMENT(machine)) {
    gtk_label_set_text(label,on_int_range_global_property_format_value(GTK_SCALE(widget),(gdouble)value,(gpointer)self->priv->machine));
  }
  else {
    gtk_label_set_text(label,on_int_range_voice_property_format_value(GTK_SCALE(widget),(gdouble)value,(gpointer)self->priv->machine));
  }
}

static void on_int_range_property_changed(GtkRange *range,gpointer user_data) {
  GstObject *param_parent=GST_OBJECT(user_data);
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(range));
  GtkLabel *label=GTK_LABEL(g_object_get_qdata(G_OBJECT(range),range_label_quark));
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(g_object_get_qdata(G_OBJECT(range),range_parent_quark));
  gdouble value=gtk_range_get_value(range);
  
  g_assert(user_data);
  //GST_INFO("property value change received");

  g_signal_handlers_block_matched(param_parent,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_int_range_property_notify,(gpointer)range);
  g_object_set(param_parent,name,(gint)value,NULL);
  g_signal_handlers_unblock_matched(param_parent,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_int_range_property_notify,(gpointer)range);
  if(GST_IS_ELEMENT(param_parent)) {
    gtk_label_set_text(label,on_int_range_global_property_format_value(GTK_SCALE(range),value,(gpointer)self->priv->machine));
  }
  else {
    gtk_label_set_text(label,on_int_range_voice_property_format_value(GTK_SCALE(range),value,(gpointer)self->priv->machine));
  }
}

static void on_uint_range_property_notify(const GstElement *machine,GParamSpec *property,gpointer user_data) {
  GtkWidget *widget=GTK_WIDGET(user_data);
  GtkLabel *label=GTK_LABEL(g_object_get_qdata(G_OBJECT(widget),range_label_quark));
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(g_object_get_qdata(G_OBJECT(widget),range_parent_quark));
  guint value;
  
  g_assert(user_data);
  //GST_INFO("property value notify received : %s ",property->name);

  g_object_get(G_OBJECT(machine),property->name,&value,NULL);
  g_signal_handlers_block_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_uint_range_property_changed,(gpointer)machine);
  gtk_range_set_value(GTK_RANGE(widget),(gdouble)value);
  g_signal_handlers_unblock_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_uint_range_property_changed,(gpointer)machine);
  if(GST_IS_ELEMENT(machine)) {
    gtk_label_set_text(label,on_uint_range_global_property_format_value(GTK_SCALE(widget),value,(gpointer)self->priv->machine));
  }
  else {
    gtk_label_set_text(label,on_uint_range_voice_property_format_value(GTK_SCALE(widget),value,(gpointer)self->priv->machine));
  }
}

static void on_uint_range_property_changed(GtkRange *range,gpointer user_data) {
  GstObject *param_parent=GST_OBJECT(user_data);
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(range));
  GtkLabel *label=GTK_LABEL(g_object_get_qdata(G_OBJECT(range),range_label_quark));
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(g_object_get_qdata(G_OBJECT(range),range_parent_quark));
  gdouble value=gtk_range_get_value(range);
  
  g_assert(user_data);
  //GST_INFO("property value change received");

  g_signal_handlers_block_matched(param_parent,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_uint_range_property_notify,(gpointer)range);
  g_object_set(param_parent,name,(guint)value,NULL);
  g_signal_handlers_unblock_matched(param_parent,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_uint_range_property_notify,(gpointer)range);
  if(GST_IS_ELEMENT(param_parent)) {
    gtk_label_set_text(label,on_uint_range_global_property_format_value(GTK_SCALE(range),value,(gpointer)self->priv->machine));
  }
  else {
    gtk_label_set_text(label,on_uint_range_voice_property_format_value(GTK_SCALE(range),value,(gpointer)self->priv->machine));
  }
}

static void on_combobox_property_notify(const GstElement *machine,GParamSpec *property,gpointer user_data) {
  GtkWidget *widget=GTK_WIDGET(user_data);
  gint value;

  g_assert(user_data);
  //GST_INFO("property value notify received : %s ",property->name);
  
  g_object_get(G_OBJECT(machine),property->name,&value,NULL);
  g_signal_handlers_block_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_combobox_property_changed,(gpointer)machine);
  gtk_combo_box_set_active(GTK_COMBO_BOX(widget),value);
  g_signal_handlers_unblock_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_combobox_property_changed,(gpointer)machine);
}

static void on_combobox_property_changed(GtkComboBox *combobox, gpointer user_data) {
  GstObject *param_parent=GST_OBJECT(user_data);
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(combobox));
  gint value;
  
  g_assert(user_data);
  //GST_INFO("property value change received");

  value=gtk_combo_box_get_active(combobox);
  g_signal_handlers_block_matched(param_parent,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_combobox_property_notify,(gpointer)combobox);
  g_object_set(param_parent,name,value,NULL);
  g_signal_handlers_unblock_matched(param_parent,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_combobox_property_notify,(gpointer)combobox);
}

static void on_checkbox_property_notify(const GstElement *machine,GParamSpec *property,gpointer user_data) {
  GtkWidget *widget=GTK_WIDGET(user_data);
  gboolean value;
  
  g_assert(user_data);
  //GST_INFO("property value notify received : %s ",property->name);

  g_object_get(G_OBJECT(machine),property->name,&value,NULL);
  g_signal_handlers_block_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_checkbox_property_toggled,(gpointer)machine);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),value);
  g_signal_handlers_unblock_matched(widget,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_checkbox_property_toggled,(gpointer)machine);
}

static void on_checkbox_property_toggled(GtkToggleButton *togglebutton, gpointer user_data) {
  GstObject *param_parent=GST_OBJECT(user_data);
  const gchar *name=gtk_widget_get_name(GTK_WIDGET(togglebutton));
  gboolean value;
  
  g_assert(user_data);
  //GST_INFO("property value change received");

  value=gtk_toggle_button_get_active(togglebutton);
  g_signal_handlers_block_matched(param_parent,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_checkbox_property_notify,(gpointer)togglebutton);
  g_object_set(param_parent,name,value,NULL);
  g_signal_handlers_unblock_matched(param_parent,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_checkbox_property_notify,(gpointer)togglebutton);
}

static void on_toolbar_help_clicked(GtkButton *button,gpointer user_data) {
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);
  GstElement *machine;

  g_assert(user_data);

  // show help for machine
  g_object_get(self->priv->machine,"machine",&machine,NULL);
  bt_machine_action_help(machine);
  gst_object_unref(machine);
}

static void on_toolbar_about_clicked(GtkButton *button,gpointer user_data) {
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);
  BtMainWindow *main_window;
  GstElement *machine;
  
  g_assert(user_data);

  GST_INFO("context_menu about event occurred");
  // show info about machine
  g_object_get(G_OBJECT(self->priv->machine),"machine",&machine,NULL);
  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  bt_machine_action_about(machine,main_window);
  g_object_unref(main_window);
  gst_object_unref(machine);
}

static void on_toolbar_show_hide_clicked(GtkButton *button,gpointer user_data) {
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);

  g_assert(user_data);

  if(gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(button))) {
    gtk_widget_show(self->priv->preset_box);
  }
  else {
    gtk_widget_hide(self->priv->preset_box);
  }
}

static void on_toolbar_preset_add_clicked(GtkButton *button,gpointer user_data) {
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);
  GstElement *machine;
  gchar *name=NULL,*comment=NULL;
  
  g_object_get(G_OBJECT(self->priv->machine),"machine",&machine,NULL);
  
  GST_INFO("about to add a new preset : '%s'",name);
  
  // ask for name & comment
  if(preset_list_edit_preset_meta(self,&name,&comment)) {
    gst_preset_save_preset(GST_PRESET(machine),name);
    //gst_preset_set_meta(GST_PRESET(machine),new_name,"comment",comment);
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

    gtk_tree_model_get(model,&iter,1,&name,-1);
    g_object_get(G_OBJECT(self->priv->machine),"machine",&machine,NULL);
  
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
    gchar *old_name,*new_name,*comment=NULL;
    GstElement *machine;

    gtk_tree_model_get(model,&iter,1,&old_name,-1);

    g_object_get(G_OBJECT(self->priv->machine),"machine",&machine,NULL);
    
    GST_INFO("about to edit preset : '%s'",old_name);
    
    new_name=g_strdup(old_name);
    // change for name & comment
    if(preset_list_edit_preset_meta(self,&new_name,&comment)) {
      gst_preset_rename_preset(GST_PRESET(machine),old_name,new_name);
      //gst_preset_set_meta(GST_PRESET(machine),new_name,"comment",comment);
      preset_list_refresh(self);
    }
    g_free(old_name);
    gst_object_unref(machine);
  }
}

static void on_toolbar_preset_random_clicked(GtkButton *button,gpointer user_data) {
  const BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);
  GstElement *machine;

  g_object_get(G_OBJECT(self->priv->machine),"machine",&machine,NULL); 
  gst_preset_create_preset(GST_PRESET(machine));
  gst_object_unref(machine);
}

static void on_preset_list_row_activated(GtkTreeView *tree_view,GtkTreePath *path,GtkTreeViewColumn *column,gpointer user_data) {
  const BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);
  GtkTreeModel *model;
  GtkTreeIter iter;
  
  // get current preset from list
  model=gtk_tree_view_get_model(tree_view);
  if(gtk_tree_model_get_iter(model,&iter,path)) {
    gchar *name;
    BtMainWindow *main_window;
    GstElement *machine;

    gtk_tree_model_get(model,&iter,1,&name,-1);

    g_object_get(G_OBJECT(self->priv->machine),"machine",&machine,NULL);
    g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
    
    GST_INFO("about to load preset : '%s'",name);
    gst_preset_load_preset(GST_PRESET(machine),name);
    g_object_unref(main_window);
    gst_object_unref(machine);
  }
}

static void on_toolbar_style_changed(const BtSettings *settings,GParamSpec *arg,gpointer user_data) {
  BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);
  gchar *toolbar_style;
  
  g_object_get(G_OBJECT(settings),"toolbar-style",&toolbar_style,NULL);
  
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
  GtkWidget *parent=GTK_WIDGET(user_data);
  gint height=requisition->height,width=-1;
  gint max_height=gdk_screen_get_height(gdk_screen_get_default());

  GST_INFO("#### box size req %d x %d (max-height=%d)", requisition->width,requisition->height,max_height);
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

static GtkWidget *make_checkbox_widget(const BtMachinePropertiesDialog *self, GstObject *machine,GParamSpec *property) {
  GtkWidget *widget;
  gchar *signal_name;
  gboolean value;

  g_object_get(G_OBJECT(machine),property->name,&value,NULL);

  widget=gtk_check_button_new();
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),value);
  gtk_widget_set_name(GTK_WIDGET(widget),property->name);
 
  signal_name=g_alloca(9+strlen(property->name));
  g_sprintf(signal_name,"notify::%s",property->name);
  g_signal_connect(G_OBJECT(machine), signal_name, (GCallback)on_checkbox_property_notify, (gpointer)widget);
  g_signal_connect(G_OBJECT(widget), "toggled", (GCallback)on_checkbox_property_toggled, (gpointer)machine);

  return(widget);
}

static GtkWidget *make_int_range_widget(const BtMachinePropertiesDialog *self, GstObject *machine,GParamSpec *property,GValue *range_min,GValue *range_max,GtkWidget *label) {
  GtkWidget *widget;
  gchar *signal_name;
  gint value;
  
  g_object_get(G_OBJECT(machine),property->name,&value,NULL);
  //step=(int_property->maximum-int_property->minimum)/1024.0;
  widget=gtk_hscale_new_with_range(g_value_get_int(range_min),g_value_get_int(range_max),1.0);
  gtk_scale_set_draw_value(GTK_SCALE(widget),/*TRUE*/FALSE);
  //gtk_scale_set_value_pos(GTK_SCALE(widget),GTK_POS_RIGHT);
  gtk_range_set_value(GTK_RANGE(widget),value);
  gtk_widget_set_name(GTK_WIDGET(widget),property->name);
  g_object_set_qdata(G_OBJECT(widget),range_label_quark,(gpointer)label);
  g_object_set_qdata(G_OBJECT(widget),range_parent_quark,(gpointer)self);
  // @todo add numerical entry as well ?

  signal_name=g_alloca(9+strlen(property->name));
  g_sprintf(signal_name,"notify::%s",property->name);
  g_signal_connect(G_OBJECT(machine), signal_name, (GCallback)on_int_range_property_notify, (gpointer)widget);
  g_signal_connect(G_OBJECT(widget), "value-changed", (GCallback)on_int_range_property_changed, (gpointer)machine);
  if(GST_IS_ELEMENT(machine)) {
    g_signal_connect(G_OBJECT(widget), "format-value", (GCallback)on_int_range_global_property_format_value, (gpointer)self->priv->machine);
  }
  else {
    g_signal_connect(G_OBJECT(widget), "format-value", (GCallback)on_int_range_voice_property_format_value, (gpointer)self->priv->machine);
  }

  g_signal_emit_by_name(G_OBJECT(widget),"value-changed");
  
  return(widget);
}

static GtkWidget *make_uint_range_widget(const BtMachinePropertiesDialog *self, GstObject *machine,GParamSpec *property,GValue *range_min,GValue *range_max,GtkWidget *label) {
  GtkWidget *widget;
  gchar *signal_name;
  gint value;
  
  g_object_get(G_OBJECT(machine),property->name,&value,NULL);
  //step=(int_property->maximum-int_property->minimum)/1024.0;
  widget=gtk_hscale_new_with_range(g_value_get_uint(range_min),g_value_get_uint(range_max),1.0);
  gtk_scale_set_draw_value(GTK_SCALE(widget),/*TRUE*/FALSE);
  //gtk_scale_set_value_pos(GTK_SCALE(widget),GTK_POS_RIGHT);
  gtk_range_set_value(GTK_RANGE(widget),value);
  gtk_widget_set_name(GTK_WIDGET(widget),property->name);
  g_object_set_qdata(G_OBJECT(widget),range_label_quark,(gpointer)label);
  g_object_set_qdata(G_OBJECT(widget),range_parent_quark,(gpointer)self);
  // @todo add numerical entry as well ?
  
  signal_name=g_alloca(9+strlen(property->name));
  g_sprintf(signal_name,"notify::%s",property->name);
  g_signal_connect(G_OBJECT(machine), signal_name, (GCallback)on_uint_range_property_notify, (gpointer)widget);
  g_signal_connect(G_OBJECT(widget), "value-changed", (GCallback)on_uint_range_property_changed, (gpointer)machine);
  if(GST_IS_ELEMENT(machine)) {
    g_signal_connect(G_OBJECT(widget), "format-value", (GCallback)on_uint_range_global_property_format_value, (gpointer)self->priv->machine);
  }
  else {
    g_signal_connect(G_OBJECT(widget), "format-value", (GCallback)on_uint_range_voice_property_format_value, (gpointer)self->priv->machine);
  }

  g_signal_emit_by_name(G_OBJECT(widget),"value-changed");
  
  return(widget);
}

static GtkWidget *make_float_range_widget(const BtMachinePropertiesDialog *self, GstObject *machine,GParamSpec *property,GValue *range_min,GValue *range_max,GtkWidget *label) {
  GtkWidget *widget;
  gchar *signal_name;
  gfloat step,value;
  gfloat value_min=g_value_get_float(range_min);
  gfloat value_max=g_value_get_float(range_max);

  g_object_get(G_OBJECT(machine),property->name,&value,NULL);
  step=(value_max-value_min)/1024.0;

  widget=gtk_hscale_new_with_range(value_min,value_max,step);
  gtk_scale_set_draw_value(GTK_SCALE(widget),/*TRUE*/FALSE);
  //gtk_scale_set_value_pos(GTK_SCALE(widget),GTK_POS_RIGHT);
  gtk_range_set_value(GTK_RANGE(widget),value);
  gtk_widget_set_name(GTK_WIDGET(widget),property->name);
  g_object_set_qdata(G_OBJECT(widget),range_label_quark,(gpointer)label);
  g_object_set_qdata(G_OBJECT(widget),range_parent_quark,(gpointer)self);
  // @todo add numerical entry as well ?

  signal_name=g_alloca(9+strlen(property->name));
  g_sprintf(signal_name,"notify::%s",property->name);
  g_signal_connect(G_OBJECT(machine), signal_name, (GCallback)on_float_range_property_notify, (gpointer)widget);
  g_signal_connect(G_OBJECT(widget), "value-changed", (GCallback)on_float_range_property_changed, (gpointer)machine);
  //g_signal_connect(G_OBJECT(widget), "format-value", (GCallback)on_float_range_global_property_format_value, (gpointer)machine);
  
  g_signal_emit_by_name(G_OBJECT(widget),"value-changed");

  return(widget);
}

static GtkWidget *make_double_range_widget(const BtMachinePropertiesDialog *self, GstObject *machine,GParamSpec *property,GValue *range_min,GValue *range_max,GtkWidget *label) {
  GtkWidget *widget;
  gchar *signal_name;
  gdouble step,value;
  gdouble value_min=g_value_get_double(range_min);
  gdouble value_max=g_value_get_double(range_max);

  g_object_get(G_OBJECT(machine),property->name,&value,NULL);
  step=(value_max-value_min)/1024.0;

  widget=gtk_hscale_new_with_range(value_min,value_max,step);
  gtk_scale_set_draw_value(GTK_SCALE(widget),/*TRUE*/FALSE);
  //gtk_scale_set_value_pos(GTK_SCALE(widget),GTK_POS_RIGHT);
  gtk_range_set_value(GTK_RANGE(widget),value);
  gtk_widget_set_name(GTK_WIDGET(widget),property->name);
  g_object_set_qdata(G_OBJECT(widget),range_label_quark,(gpointer)label);
  g_object_set_qdata(G_OBJECT(widget),range_parent_quark,(gpointer)self);
  // @todo add numerical entry as well ?

  signal_name=g_alloca(9+strlen(property->name));
  g_sprintf(signal_name,"notify::%s",property->name);
  g_signal_connect(G_OBJECT(machine), signal_name, (GCallback)on_double_range_property_notify, (gpointer)widget);
  g_signal_connect(G_OBJECT(widget), "value-changed", (GCallback)on_double_range_property_changed, (gpointer)machine);
  //g_signal_connect(G_OBJECT(widget), "format-value", (GCallback)on_double_range_global_property_format_value, (gpointer)machine);
  
  g_signal_emit_by_name(G_OBJECT(widget),"value-changed");

  return(widget);
}

static GtkWidget *make_combobox_widget(const BtMachinePropertiesDialog *self, GstObject *machine,GParamSpec *property,GValue *range_min,GValue *range_max) {
  GtkWidget *widget;
  gchar *signal_name;
  GParamSpecEnum *enum_property=G_PARAM_SPEC_ENUM(property);
  GEnumClass *enum_class=enum_property->enum_class;
  GEnumValue *enum_value;
  gint value;

  widget=gtk_combo_box_new_text();        
  for(value=enum_class->minimum;value<=enum_class->maximum;value++) {
    enum_value=g_enum_get_value(enum_class, value);
    gtk_combo_box_append_text(GTK_COMBO_BOX(widget),enum_value->value_nick);
  }
  g_object_get(machine,property->name,&value,NULL);
  gtk_combo_box_set_active(GTK_COMBO_BOX(widget),value);
  gtk_widget_set_name(GTK_WIDGET(widget),property->name);
  
  signal_name=g_alloca(9+strlen(property->name));
  g_sprintf(signal_name,"notify::%s",property->name);
  g_signal_connect(G_OBJECT(machine), signal_name, (GCallback)on_combobox_property_notify, (gpointer)widget);
  g_signal_connect(G_OBJECT(widget), "changed", (GCallback)on_combobox_property_changed, (gpointer)machine);

  return(widget);
}


static gboolean bt_machine_properties_dialog_init_preset_box(const BtMachinePropertiesDialog *self) {
  GtkTooltips *tips=gtk_tooltips_new();
  GtkWidget *scrolled_window;
  GtkWidget *tool_item;
  GtkTreeSelection *tree_sel;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *tree_col;

  self->priv->preset_box=gtk_vbox_new(FALSE,0);

  // add preset controls toolbar
  self->priv->preset_toolbar=gtk_toolbar_new();

  tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_ADD));
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(tool_item),GTK_TOOLTIPS(tips),_("Add new preset"),NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->preset_toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_preset_add_clicked),(gpointer)self);

  tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_REMOVE));
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(tool_item),GTK_TOOLTIPS(tips),_("Remove preset"),NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->preset_toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_preset_remove_clicked),(gpointer)self);
  
  tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_EDIT));
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(tool_item),GTK_TOOLTIPS(tips),_("Edit preset name and comment"),NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->preset_toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_preset_edit_clicked),(gpointer)self);
  
  tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_NEW));
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(tool_item),GTK_TOOLTIPS(tips),_("Generate and load random preset"),NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->preset_toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_preset_random_clicked),(gpointer)self);  
  
  gtk_box_pack_start(GTK_BOX(self->priv->preset_box),self->priv->preset_toolbar,FALSE,FALSE,0);

  // add preset list
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_IN);
  self->priv->preset_list=gtk_tree_view_new();
  g_object_set(self->priv->preset_list,"enable-search",FALSE,"rules-hint",TRUE,"fixed-height-mode",TRUE,NULL);
  tree_sel=gtk_tree_view_get_selection(GTK_TREE_VIEW(self->priv->preset_list));
  gtk_tree_selection_set_mode(tree_sel,GTK_SELECTION_SINGLE);
  g_signal_connect(G_OBJECT(self->priv->preset_list), "row-activated", G_CALLBACK(on_preset_list_row_activated), (gpointer)self);

  // add cell renderers
  renderer=gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer),"xalign",0.0,NULL);
  if((tree_col=gtk_tree_view_column_new_with_attributes(_("Preset"),renderer,"text",0,NULL))) {
    g_object_set(tree_col,"sizing",GTK_TREE_VIEW_COLUMN_FIXED,"fixed-width",125,NULL);
    gtk_tree_view_insert_column(GTK_TREE_VIEW(self->priv->preset_list),tree_col,-1);
  }
  else GST_WARNING("can't create treeview column");
  
  // add list data
  preset_list_refresh(self);

  gtk_container_add(GTK_CONTAINER(scrolled_window),self->priv->preset_list);
  gtk_box_pack_start(GTK_BOX(self->priv->preset_box),GTK_WIDGET(scrolled_window),TRUE,TRUE,0);
  
  // the list is static, don't free
  //g_list_free(presets);
  return(TRUE);
}

static gboolean bt_machine_properties_dialog_init_ui(const BtMachinePropertiesDialog *self) {
  BtMainWindow *main_window;
  GtkWidget *param_box,*vbox,*hbox;
  GtkWidget *expander,*label,*table,*scrolled_window;
  GtkWidget *widget1,*widget2;
  GtkWidget *tool_item;
  GtkTooltips *tips=gtk_tooltips_new();
  gchar *id,*title;
  GdkPixbuf *window_icon=NULL;
  gulong i,k,global_params,voices,voice_params,params;
  GParamSpec *property;
  GValue *range_min,*range_max;
  GType param_type,base_type;
  GstElement *machine;
  BtSettings *settings;
  
  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,"settings",&settings,NULL);
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
  g_object_get(self->priv->machine,
    "id",&id,
    "global-params",&global_params,
    "voice-params",&voice_params,
    "voices",&voices,
    "machine",&machine,
    NULL);
  title=g_strdup_printf(_("%s properties"),id);
  gtk_window_set_title(GTK_WINDOW(self),title);
  g_free(id);g_free(title);
    
  // add widgets to the dialog content area
  // should we use a hpaned or hbox for the presets?
  hbox=gtk_hbox_new(FALSE,12);
  param_box=gtk_vbox_new(FALSE,0);
  //gtk_container_set_border_width(GTK_CONTAINER(param_box),6);
  gtk_box_pack_start(GTK_BOX(hbox),param_box,TRUE,TRUE,0);
   
  // create preset pane
  if(GST_IS_PRESET(machine)) {
    if(bt_machine_properties_dialog_init_preset_box(self)) {
      gtk_box_pack_end(GTK_BOX(hbox),self->priv->preset_box,TRUE,TRUE,0);
    }
  }

  // create toolbar
  self->priv->main_toolbar=gtk_toolbar_new();

  tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_ABOUT));
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(tool_item),GTK_TOOLTIPS(tips),_("Info about this machine"),NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->main_toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_about_clicked),(gpointer)self);

  tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_HELP));
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(tool_item),GTK_TOOLTIPS(tips),_("Help for this machine"),NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->main_toolbar),GTK_TOOL_ITEM(tool_item),-1);
  if(!GST_IS_HELP(machine)) {
    gtk_widget_set_sensitive(tool_item,FALSE);
  }
  else {
    g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_help_clicked),(gpointer)self);
  }

  // @todo: add copy/paste buttons
  
  tool_item=GTK_WIDGET(gtk_toggle_tool_button_new_from_stock(GTK_STOCK_INDEX));
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(tool_item),GTK_TOOLTIPS(tips),_("Show/Hide preset pane"),NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->main_toolbar),GTK_TOOL_ITEM(tool_item),-1);
  if(!GST_IS_PRESET(machine)) {
    gtk_widget_set_sensitive(tool_item,FALSE);
  }
  else {
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(tool_item),TRUE);
    g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_show_hide_clicked),(gpointer)self);
  }

  // let settings control toolbar style
  on_toolbar_style_changed(settings,NULL,(gpointer)self);
  g_signal_connect(G_OBJECT(settings), "notify::toolbar-style", G_CALLBACK(on_toolbar_style_changed), (gpointer)self);

  gtk_box_pack_start(GTK_BOX(param_box),self->priv->main_toolbar,FALSE,FALSE,0);

  GST_INFO("machine has %d global properties, %d voice properties and %d voices",global_params,voice_params,voices);

  if(global_params+voices*voice_params) {
    // machine controls inside a scrolled window
    scrolled_window=gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_NONE);
    vbox=gtk_vbox_new(FALSE,0);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),vbox);
    gtk_box_pack_start(GTK_BOX(param_box),scrolled_window,TRUE,TRUE,0);
    g_signal_connect(G_OBJECT(vbox),"size-request",G_CALLBACK(on_box_size_request),(gpointer)scrolled_window);

    if(global_params) {
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
        gtk_box_pack_start(GTK_BOX(vbox),expander,TRUE,TRUE,0);
        
        // add global machine controls into the table
        table=gtk_table_new(/*rows=*/params+1,/*columns=*/3,/*homogenous=*/FALSE);

        for(i=0,k=0;i<global_params;i++) {
          if(bt_machine_is_global_param_trigger(self->priv->machine,i)) continue;
          if(voice_params && bt_machine_get_voice_param_index(self->priv->machine,bt_machine_get_global_param_name(self->priv->machine,i),NULL)>-1) continue;
          property=bt_machine_get_global_param_spec(self->priv->machine,i);
          GST_INFO("global property %p has name '%s','%s'",property,property->name,bt_machine_get_global_param_name(self->priv->machine,i));
          // get name
          label=gtk_label_new((gchar *)bt_machine_get_global_param_name(self->priv->machine,i));
          gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
          gtk_table_attach(GTK_TABLE(table),label, 0, 1, k, k+1, GTK_FILL,GTK_SHRINK, 2,1);

          param_type=bt_machine_get_global_param_type(self->priv->machine,i);
          while((base_type=g_type_parent(param_type))) param_type=base_type;
          GST_INFO("... base type is : %s",g_type_name(param_type));

          range_min=bt_machine_get_global_param_min_value(self->priv->machine,i);
          range_max=bt_machine_get_global_param_max_value(self->priv->machine,i);
          // DEBUG
          if(range_min && range_max) {
            gchar *str_min=g_strdup_value_contents(range_min);
            gchar *str_max=g_strdup_value_contents(range_max);
            GST_INFO("... has range : %s ... %s",str_min,str_max);
            g_free(str_min);g_free(str_max);
          }
          // DEBUG

          // implement widget types
          switch(param_type) {
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
            case G_TYPE_ENUM:
              widget1=make_combobox_widget(self,GST_OBJECT(machine),property,range_min,range_max);
              widget2=NULL;
              break;
            default: { 
              gchar *str=g_strdup_printf("unhandled type \"%s\"",G_PARAM_SPEC_TYPE_NAME(property));
              widget1=gtk_label_new(str);g_free(str);
              widget2=NULL;
            }
          }
          if(range_min) { g_free(range_min);range_min=NULL; }
          if(range_max) { g_free(range_max);range_max=NULL; }
          
          gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),widget1,g_param_spec_get_blurb(property),NULL);
          if(!widget2) {
            gtk_table_attach(GTK_TABLE(table),widget1, 1, 3, k, k+1, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
          }
          else {
            gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),widget2,g_param_spec_get_blurb(property),NULL);
            gtk_table_attach(GTK_TABLE(table),widget1, 1, 2, k, k+1, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
            /* @todo how can we avoid the wobble here?
             * hack would be to set some 'good' default size
             * if we use GTK_FILL|GTK_EXPAND than it uses too much space (same as widget1)
             */
            gtk_widget_set_size_request(widget2,50,-1);
            if(GTK_IS_LABEL(widget2)) {
              gtk_label_set_ellipsize(GTK_LABEL(widget2),PANGO_ELLIPSIZE_END);
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
    }
    if(voices*voice_params) {
      gulong j;
      gchar *name;
      GstObject *machine_voice;

      params=voice_params;
      for(i=0;i<voice_params;i++) {
        if(bt_machine_is_voice_param_trigger(self->priv->machine,i)) params--;
      }      
      for(j=0;j<voices;j++) {
        name=g_strdup_printf(_("voice %lu properties"),j+1);
        expander=gtk_expander_new(name);
        gtk_expander_set_expanded(GTK_EXPANDER(expander),TRUE);
        g_free(name);
        gtk_box_pack_start(GTK_BOX(vbox),expander,TRUE,TRUE,0);
        
        machine_voice=gst_child_proxy_get_child_by_index(GST_CHILD_PROXY(machine),j);
        
        // add voice machine controls into the table
        table=gtk_table_new(/*rows=*/params+1,/*columns=*/2,/*homogenous=*/FALSE);

        for(i=0,k=0;i<voice_params;i++) {
          if(bt_machine_is_voice_param_trigger(self->priv->machine,i)) continue;

          property=bt_machine_get_voice_param_spec(self->priv->machine,i);
          GST_INFO("voice property %p has name '%s','%s'",property,property->name,bt_machine_get_voice_param_name(self->priv->machine,i));
          // get name
          label=gtk_label_new((gchar *)bt_machine_get_voice_param_name(self->priv->machine,i));
          gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
          gtk_table_attach(GTK_TABLE(table),label, 0, 1, k, k+1, GTK_FILL,GTK_SHRINK, 2,1);

          param_type=bt_machine_get_voice_param_type(self->priv->machine,i);
          while((base_type=g_type_parent(param_type))) param_type=base_type;
          GST_INFO("... base typoe is : %s",g_type_name(param_type));
            
          range_min=bt_machine_get_voice_param_min_value(self->priv->machine,i);
          range_max=bt_machine_get_voice_param_max_value(self->priv->machine,i);
          // DEBUG
          if(range_min && range_max) {
            gchar *str_min=g_strdup_value_contents(range_min);
            gchar *str_max=g_strdup_value_contents(range_max);
            GST_INFO("... has range : %s ... %s",str_min,str_max);
            g_free(str_min);g_free(str_max);
          }
          // DEBUG

          // implement widget types
          switch(param_type) {
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
            case G_TYPE_ENUM:
              widget1=make_combobox_widget(self,GST_OBJECT(machine),property,range_min,range_max);
              widget2=NULL;
              break;
            default: {
              gchar *str=g_strdup_printf("unhandled type \"%s\"",G_PARAM_SPEC_TYPE_NAME(property));
              widget1=gtk_label_new(str);g_free(str);
              widget2=NULL;
            }
          }
          if(range_min) { g_free(range_min);range_min=NULL; }
          if(range_max) { g_free(range_max);range_max=NULL; }
          
          gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),widget1,g_param_spec_get_blurb(property),NULL);
          if(!widget2) {
            gtk_table_attach(GTK_TABLE(table),widget1, 1, 3, k, k+1, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
          }
          else {
            gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),widget2,g_param_spec_get_blurb(property),NULL);
            gtk_table_attach(GTK_TABLE(table),widget1, 1, 2, k, k+1, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
            /* @todo how can we avoid the wobble here?
             * hack would be to set some 'good' default size
             * if we use GTK_FILL|GTK_EXPAND than it uses too much space (same as widget1)
             */
            gtk_widget_set_size_request(widget2,50,-1);
            if(GTK_IS_LABEL(widget2)) {
              gtk_label_set_ellipsize(GTK_LABEL(widget2),PANGO_ELLIPSIZE_END);
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
    }
  }
  else {
    gtk_container_add(GTK_CONTAINER(param_box),gtk_label_new(_("machine has no params")));
  }
  gtk_container_add(GTK_CONTAINER(self),hbox);
  
  g_object_try_unref(machine);
  g_object_try_unref(main_window);
  g_object_try_unref(settings);
  return(TRUE);
}

//-- constructor methods

/**
 * bt_machine_properties_dialog_new:
 * @app: the application the dialog belongs to
 * @machine: the machine to create the dialog for
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtMachinePropertiesDialog *bt_machine_properties_dialog_new(const BtEditApplication *app,const BtMachine *machine) {
  BtMachinePropertiesDialog *self;

  if(!(self=BT_MACHINE_PROPERTIES_DIALOG(g_object_new(BT_TYPE_MACHINE_PROPERTIES_DIALOG,"app",app,"machine",machine,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_machine_properties_dialog_init_ui(self)) {
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
static void bt_machine_properties_dialog_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case MACHINE_PROPERTIES_DIALOG_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    case MACHINE_PROPERTIES_DIALOG_MACHINE: {
      g_value_set_object(value, self->priv->machine);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_machine_properties_dialog_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case MACHINE_PROPERTIES_DIALOG_APP: {
      g_object_try_unref(self->priv->app);
      self->priv->app = g_object_try_ref(g_value_get_object(value));
      //GST_DEBUG("set the app for settings_dialog: %p",self->priv->app);
    } break;
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

  gulong j,voices;
  GstElement *machine;
  GstObject *machine_voice;
  
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  
  // disconnect all handlers that are connected to params
  g_object_get(self->priv->machine,"machine",&machine,"voices",&voices,NULL);
  g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_double_range_property_notify,NULL);
  g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_int_range_property_notify,NULL);
  g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_uint_range_property_notify,NULL);
  g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_checkbox_property_notify,NULL);
  g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_combobox_property_notify,NULL);
  for(j=0;j<voices;j++) {
    machine_voice=gst_child_proxy_get_child_by_index(GST_CHILD_PROXY(machine),j);
    g_signal_handlers_disconnect_matched(machine_voice,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_double_range_property_notify,NULL);
    g_signal_handlers_disconnect_matched(machine_voice,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_int_range_property_notify,NULL);
    g_signal_handlers_disconnect_matched(machine_voice,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_uint_range_property_notify,NULL);
    g_signal_handlers_disconnect_matched(machine_voice,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_checkbox_property_notify,NULL);
    g_signal_handlers_disconnect_matched(machine_voice,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_combobox_property_notify,NULL);
  }
  g_object_unref(machine);
  
  g_object_try_unref(self->priv->app);
  g_object_try_unref(self->priv->machine);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_machine_properties_dialog_finalize(GObject *object) {
  //BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG(object);

  //GST_DEBUG("!!!! self=%p",self);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_machine_properties_dialog_init(GTypeInstance *instance, gpointer g_class) {
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_MACHINE_PROPERTIES_DIALOG, BtMachinePropertiesDialogPrivate);
}

static void bt_machine_properties_dialog_class_init(BtMachinePropertiesDialogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  range_label_quark=g_quark_from_static_string("BtMachinePropertiesDialog::range-label");
  range_parent_quark=g_quark_from_static_string("BtMachinePropertiesDialog::range-parent");

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtMachinePropertiesDialogPrivate));
  
  gobject_class->set_property = bt_machine_properties_dialog_set_property;
  gobject_class->get_property = bt_machine_properties_dialog_get_property;
  gobject_class->dispose      = bt_machine_properties_dialog_dispose;
  gobject_class->finalize     = bt_machine_properties_dialog_finalize;

  g_object_class_install_property(gobject_class,MACHINE_PROPERTIES_DIALOG_APP,
                                  g_param_spec_object("app",
                                     "app construct prop",
                                     "Set application object, the dialog belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,MACHINE_PROPERTIES_DIALOG_MACHINE,
                                  g_param_spec_object("machine",
                                     "machine construct prop",
                                     "Set machine object, the dialog handles",
                                     BT_TYPE_MACHINE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

}

GType bt_machine_properties_dialog_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
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
