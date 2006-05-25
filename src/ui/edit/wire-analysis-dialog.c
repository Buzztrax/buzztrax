// $Id: wire-analysis-dialog.c,v 1.2 2006-05-25 16:29:21 ensonic Exp $
/**
 * SECTION:btwireanalysisdialog
 * @short_description: audio analysis for this wire
 */ 

#define BT_EDIT
#define BT_WIRE_ANALYSIS_DIALOG_C

#include "bt-edit.h"

//-- property ids

enum {
  WIRE_ANALYSIS_DIALOG_APP=1,
  WIRE_ANALYSIS_DIALOG_WIRE
};

struct _BtWireAnalysisDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtEditApplication *app;

  /* the underlying machine */
  BtWire *wire;
};

static GtkDialogClass *parent_class=NULL;

//-- event handler

//-- helper methods

static gboolean bt_wire_analysis_dialog_init_ui(const BtWireAnalysisDialog *self) {
  BtMainWindow *main_window;
  BtMachine *src_machine,*dst_machine;
  gchar *src_id,*dst_id,*title;
  //GdkPixbuf *window_icon=NULL;

  g_object_get(self->priv->app,"main-window",&main_window,NULL);
  gtk_window_set_transient_for(GTK_WINDOW(self),GTK_WINDOW(main_window));
  
  /* @todo: create and set *proper* window icon
  if((window_icon=bt_ui_ressources_get_pixbuf_by_wire(self->priv->wire))) {
    gtk_window_set_icon(GTK_WINDOW(self),window_icon);
  }
  */
  
  // leave the choice of width to gtk
  gtk_window_set_default_size(GTK_WINDOW(self),-1,200);
  // set a title
  g_object_get(self->priv->wire,"src",&src_machine,"dst",&dst_machine,NULL);
  g_object_get(src_machine,"id",&src_id,NULL);
  g_object_get(dst_machine,"id",&dst_id,NULL);
  title=g_strdup_printf(_("%s -> %s analysis"),src_id,dst_id);
  gtk_window_set_title(GTK_WINDOW(self),title);
  g_free(src_id);g_free(dst_id);g_free(title);
  g_object_try_unref(src_machine);
  g_object_try_unref(dst_machine);
  
  /*
   * @todo: add spectrum analyzer drawable
   * @todo: add big level meter (with scales)
   * @todo: if stereo add pan-meter (whats the propper name?)
   */
  
  g_object_try_unref(main_window);
  return(TRUE);
}

//-- constructor methods

/**
 * bt_wire_analysis_dialog_new:
 * @app: the application the dialog belongs to
 * @wire: the wire to create the dialog for
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtWireAnalysisDialog *bt_wire_analysis_dialog_new(const BtEditApplication *app,const BtWire *wire) {
  BtWireAnalysisDialog *self;

  if(!(self=BT_WIRE_ANALYSIS_DIALOG(g_object_new(BT_TYPE_WIRE_ANALYSIS_DIALOG,"app",app,"wire",wire,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_wire_analysis_dialog_init_ui(self)) {
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
static void bt_wire_analysis_dialog_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtWireAnalysisDialog *self = BT_WIRE_ANALYSIS_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case WIRE_ANALYSIS_DIALOG_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    case WIRE_ANALYSIS_DIALOG_WIRE: {
      g_value_set_object(value, self->priv->wire);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given preferences for this object */
static void bt_wire_analysis_dialog_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtWireAnalysisDialog *self = BT_WIRE_ANALYSIS_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case WIRE_ANALYSIS_DIALOG_APP: {
      g_object_try_unref(self->priv->app);
      self->priv->app = g_object_try_ref(g_value_get_object(value));
      //GST_DEBUG("set the app for settings_dialog: %p",self->priv->app);
    } break;
    case WIRE_ANALYSIS_DIALOG_WIRE: {
      g_object_try_unref(self->priv->wire);
      self->priv->wire = g_object_try_ref(g_value_get_object(value));
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_wire_analysis_dialog_dispose(GObject *object) {
  BtWireAnalysisDialog *self = BT_WIRE_ANALYSIS_DIALOG(object);
  
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
    
  g_object_try_unref(self->priv->app);
  g_object_try_unref(self->priv->wire);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_wire_analysis_dialog_finalize(GObject *object) {
  //BtWireAnalysisDialog *self = BT_WIRE_ANALYSIS_DIALOG(object);

  //GST_DEBUG("!!!! self=%p",self);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_wire_analysis_dialog_init(GTypeInstance *instance, gpointer g_class) {
  BtWireAnalysisDialog *self = BT_WIRE_ANALYSIS_DIALOG(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_WIRE_ANALYSIS_DIALOG, BtWireAnalysisDialogPrivate);
}

static void bt_wire_analysis_dialog_class_init(BtWireAnalysisDialogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtWireAnalysisDialogPrivate));
  
  gobject_class->set_property = bt_wire_analysis_dialog_set_property;
  gobject_class->get_property = bt_wire_analysis_dialog_get_property;
  gobject_class->dispose      = bt_wire_analysis_dialog_dispose;
  gobject_class->finalize     = bt_wire_analysis_dialog_finalize;

  g_object_class_install_property(gobject_class,WIRE_ANALYSIS_DIALOG_APP,
                                  g_param_spec_object("app",
                                     "app construct prop",
                                     "Set application object, the dialog belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,WIRE_ANALYSIS_DIALOG_WIRE,
                                  g_param_spec_object("wire",
                                     "wire construct prop",
                                     "Set wire object, the dialog handles",
                                     BT_TYPE_WIRE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

}

GType bt_wire_analysis_dialog_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtWireAnalysisDialogClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_wire_analysis_dialog_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtWireAnalysisDialog),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_wire_analysis_dialog_init, // instance_init
    };
    type = g_type_register_static(GTK_TYPE_WINDOW,"BtWireAnalysisDialog",&info,0);
  }
  return type;
}
