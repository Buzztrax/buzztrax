/* $Id: wire-analysis-dialog.c,v 1.16 2007-07-13 20:53:22 ensonic Exp $
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
 * SECTION:btwireanalysisdialog
 * @short_description: audio analysis for this wire
 *
 * The dialog shows a mono-spectrum analyzer and level-meters for left/right
 * channel.
 *
 * The dialog is not modal.
 */

/* @todo: make dialog resizable */

#define BT_EDIT
#define BT_WIRE_ANALYSIS_DIALOG_C

#include "bt-edit.h"

//-- property ids

enum {
  WIRE_ANALYSIS_DIALOG_APP=1,
  WIRE_ANALYSIS_DIALOG_WIRE
};

/* @todo: add more later:
 * waveform (oszilloscope)
 * panorama (spacescope)
 */
typedef enum {
  // needed to buffer
  ANALYZER_QUEUE=0,
  // real analyzers
  ANALYZER_LEVEL,
  ANALYZER_SPECTRUM,
  // this can be 'mis'used for an aoszilloscope by connecting to hand-off
  ANALYZER_FAKESINK,
  // how many elements are used
  ANALYZER_COUNT
} BtWireAnalyzer;

#define SPECT_BANDS 256
#define SPECT_WIDTH (SPECT_BANDS)
#define SPECT_HEIGHT 64
#define LEVEL_WIDTH (SPECT_BANDS)
#define LEVEL_HEIGHT 16

static const GtkRulerMetric ruler_metrics[] =
{
  {"Frequency", "Hz", 1.0, { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512 }, { 1, 2, 4, 8, 16 }},
};

struct _BtWireAnalysisDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the wire we analyze */
  BtWire *wire;

  /* the paint handler source id */
  guint paint_handler_id;
  /* the analyzer-graphs */
  GtkWidget *spectrum_drawingarea, *level_drawingarea;
  GdkGC *peak_gc;

    /* the gstreamer elements that is used */
  GstElement *analyzers[ANALYZER_COUNT];
  GList *analyzers_list;

  /* the analyzer results */
  gdouble rms[2], peak[2];
  guchar spect[SPECT_BANDS];

  // DEBUG
  //gdouble min_rms,max_rms, min_peak,max_peak;
  // DEBUG
};

static GtkDialogClass *parent_class=NULL;

//-- event handler

/*
 * on_wire_analyzer_change:
 *
 * #GstBus handler that listens for new data from analyzers and stores them away
 * for on_wire_analyzer_redraw().
 */
#if 0
static gboolean on_wire_analyzer_change(GstBus *bus, GstMessage *message, gpointer user_data) {
  BtWireAnalysisDialog *self=BT_WIRE_ANALYSIS_DIALOG(user_data);
  gboolean res=FALSE;
  g_assert(user_data);

  switch(GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ELEMENT: {
      const GstStructure *structure=gst_message_get_structure(message);
      const gchar *name = gst_structure_get_name(structure);

      if(!strcmp(name,"level")) {
        const GValue *l_rms,*l_peak;
        guint i;
        gdouble val;

        //GST_INFO("get level data");
        l_rms=(GValue *)gst_structure_get_value(structure, "rms");
        l_peak=(GValue *)gst_structure_get_value(structure, "peak");
        //l_decay=(GValue *)gst_structure_get_value(structure, "decay");
        // size of list is number of channels
        // we use -120db as the minimum db value
        switch(gst_value_list_get_size(l_rms)) {
          case 1:
              val=g_value_get_double(gst_value_list_get_value(l_rms,0));
              self->priv->rms[0]=isinf(val)?0.0:120.0+val;
              self->priv->rms[1]=self->priv->rms[0];
              val=g_value_get_double(gst_value_list_get_value(l_peak,0));
              self->priv->peak[0]=isinf(val)?0.0:120.0+val;
              self->priv->peak[1]=self->priv->peak[0];
            break;
          case 2:
            for(i=0;i<2;i++) {
              val=g_value_get_double(gst_value_list_get_value(l_rms,i));
              self->priv->rms[i]=isinf(val)?0.0:120.0+val;
              val=g_value_get_double(gst_value_list_get_value(l_peak,i));
              self->priv->peak[i]=isinf(val)?0.0:120.0+val;
            }
            break;
        }
        res=TRUE;
      }
      else if(!strcmp(name,"spectrum")) {
        const GValue *list;
        const GValue *value;
        guint i;

        //GST_INFO("get spectrum data");
        list = gst_structure_get_value (structure, "spectrum");
        // SPECT_BANDS=gst_value_list_get_size(list)
        for (i = 0; i < SPECT_BANDS; ++i) {
          value = gst_value_list_get_value (list, i);
          self->priv->spect[i] = g_value_get_uchar (value);
        }
      }
    } break;
    default:
      //GST_INFO("received bus message: type=%s",gst_message_type_get_name(GST_MESSAGE_TYPE(message)));
      break;
  }
  return(res);
}
#endif

/*
 * on_wire_analyzer_redraw:
 * @user_data: conatins the self pointer
 *
 * Called as idle function to repaint the analyzers. Data is gathered by
 * on_wire_analyzer_change()
 */
static gboolean on_wire_analyzer_redraw(gpointer user_data) {
  BtWireAnalysisDialog *self=BT_WIRE_ANALYSIS_DIALOG(user_data);

  if(!self->priv->peak_gc) return(TRUE);

  //GST_DEBUG("redraw analyzers");
  // draw levels
  if (self->priv->level_drawingarea) {
    GdkRectangle rect = { 0, 0, LEVEL_WIDTH, LEVEL_HEIGHT };
    GtkWidget *da=self->priv->level_drawingarea;
    gint middle=LEVEL_WIDTH>>1;

    gdk_window_begin_paint_rect (da->window, &rect);
    gdk_draw_rectangle (da->window, da->style->black_gc, TRUE, 0, 0, LEVEL_WIDTH, LEVEL_HEIGHT);
    //gtk_vumeter_set_levels(self->priv->vumeter[i], (gint)(rms[i]*10.0), (gint)(peak[i]*10.0));
    /* DEBUG
      if((self->priv->rms[0]<self->priv->min_rms) && !isinf(self->priv->rms[0])) {
        GST_DEBUG("levels: rms=%7.4lf",self->priv->rms[0]);
        self->priv->min_rms=self->priv->rms[0];
      }
      if((self->priv->rms[0]>self->priv->max_rms) && !isinf(self->priv->rms[0])) {
        GST_DEBUG("levels: rms=%7.4lf",self->priv->rms[0]);
        self->priv->max_rms=self->priv->rms[0];
      }
      if((self->priv->peak[0]<self->priv->min_peak) && !isinf(self->priv->peak[0])) {
        GST_DEBUG("levels: peak=%7.4lf",self->priv->peak[0]);
        self->priv->min_peak=self->priv->peak[0];
      }
      if((self->priv->peak[0]>self->priv->max_peak) && !isinf(self->priv->peak[0])) {
        GST_DEBUG("levels: peak=%7.4lf",self->priv->peak[0]);
        self->priv->max_peak=self->priv->peak[0];
      }
    // DEBUG */
    // use RMS or peak or both?
    gdk_draw_rectangle (da->window, da->style->white_gc, TRUE, middle-self->priv->rms[0], 0, self->priv->rms[0]+self->priv->rms[1], LEVEL_HEIGHT);
    gdk_draw_rectangle (da->window, self->priv->peak_gc, TRUE, middle-self->priv->peak[0], 0, 2, LEVEL_HEIGHT);
    gdk_draw_rectangle (da->window, self->priv->peak_gc, TRUE, (middle-1)+self->priv->peak[1], 0, 2, LEVEL_HEIGHT);
    gdk_window_end_paint (da->window);
    /* @todo: if stereo draw pan-meter (L-R, R-L) */
  }

  // draw spectrum
  if (self->priv->spectrum_drawingarea) {
    gint i;
    GdkRectangle rect = { 0, 0, SPECT_WIDTH, SPECT_HEIGHT };
    GtkWidget *da=self->priv->spectrum_drawingarea;

    gdk_window_begin_paint_rect (da->window, &rect);
    gdk_draw_rectangle (da->window, da->style->black_gc, TRUE, 0, 0, SPECT_BANDS, SPECT_HEIGHT);
    for (i = 0; i < SPECT_BANDS; i++) {
      gdk_draw_rectangle (da->window, da->style->white_gc, TRUE, i, SPECT_HEIGHT - self->priv->spect[i], 1, self->priv->spect[i]);
    }
    gdk_window_end_paint (da->window);
  }
  return(TRUE);
}

static void bt_wire_analysis_dialog_realize(GtkWidget *widget,gpointer user_data) {
  BtWireAnalysisDialog *self=BT_WIRE_ANALYSIS_DIALOG(user_data);

  GST_DEBUG("dialog realize");
  self->priv->peak_gc=gdk_gc_new(GTK_WIDGET(self)->window);
  gdk_gc_set_rgb_fg_color(self->priv->peak_gc,bt_ui_ressources_get_gdk_color(BT_UI_RES_COLOR_ANALYZER_PEAK));
}

static gboolean bt_wire_analysis_dialog_expose(GtkWidget *widget,GdkEventExpose *event,gpointer user_data) {
  BtWireAnalysisDialog *self=BT_WIRE_ANALYSIS_DIALOG(user_data);

  on_wire_analyzer_redraw((gpointer)self);
  return(TRUE);
}

static void on_wire_analyzer_change(GstBus * bus, GstMessage * message, gpointer user_data) {
  BtWireAnalysisDialog *self=BT_WIRE_ANALYSIS_DIALOG(user_data);
  const GstStructure *structure=gst_message_get_structure(message);
  const gchar *name = gst_structure_get_name(structure);
  gboolean change=FALSE;

  g_assert(user_data);

  if(!strcmp(name,"level")) {
    const GValue *l_rms,*l_peak;
    guint i;
    gdouble val;

    //GST_INFO("get level data");
    l_rms=(GValue *)gst_structure_get_value(structure, "rms");
    l_peak=(GValue *)gst_structure_get_value(structure, "peak");
    //l_decay=(GValue *)gst_structure_get_value(structure, "decay");
    // size of list is number of channels
    // we use -120db as the minimum db value
    switch(gst_value_list_get_size(l_rms)) {
      case 1:
          val=g_value_get_double(gst_value_list_get_value(l_rms,0));
          self->priv->rms[0]=isinf(val)?0.0:120.0+val;
          self->priv->rms[1]=self->priv->rms[0];
          val=g_value_get_double(gst_value_list_get_value(l_peak,0));
          self->priv->peak[0]=isinf(val)?0.0:120.0+val;
          self->priv->peak[1]=self->priv->peak[0];
        break;
      case 2:
        for(i=0;i<2;i++) {
          val=g_value_get_double(gst_value_list_get_value(l_rms,i));
          self->priv->rms[i]=isinf(val)?0.0:120.0+val;
          val=g_value_get_double(gst_value_list_get_value(l_peak,i));
          self->priv->peak[i]=isinf(val)?0.0:120.0+val;
        }
        break;
    }
    change=TRUE;
  }
  else if(!strcmp(name,"spectrum")) {
    const GValue *list;
    const GValue *value;
    guint i;

    //GST_INFO("get spectrum data");
    list = gst_structure_get_value (structure, "spectrum");
    // SPECT_BANDS=gst_value_list_get_size(list)
    for (i = 0; i < SPECT_BANDS; ++i) {
      value = gst_value_list_get_value (list, i);
      self->priv->spect[i] = g_value_get_uchar (value);
    }
    change=TRUE;
  }

  if(!self->priv->paint_handler_id && change) {
    // add idle-handler that redraws gfx
    self->priv->paint_handler_id=g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,on_wire_analyzer_redraw,(gpointer)self,NULL);
  }
}

//-- helper methods

/*
 * bt_wire_analysis_dialog_make_element:
 * @self: the wire-analysis dialog
 * @part: which analyzer element to create
 * @factory_name: the element-factories name (also used to build the elemnt name)
 *
 * Create analyzer elements. Store them in the analyzers array and link them into the list.
 */
static gboolean bt_wire_analysis_dialog_make_element(const BtWireAnalysisDialog *self,BtWireAnalyzer part, const gchar *factory_name) {
  gboolean res=FALSE;
  gchar *name;

  // add analyzer element
  name=g_alloca(strlen(factory_name)+16);g_sprintf(name,"%s_%p",factory_name,self->priv->wire);
  if(!(self->priv->analyzers[part]=gst_element_factory_make(factory_name,name))) {
    GST_ERROR("failed to create %s",factory_name);goto Error;
  }
  self->priv->analyzers_list=g_list_prepend(self->priv->analyzers_list,self->priv->analyzers[part]);
  res=TRUE;
Error:
  return(res);
}


static gboolean bt_wire_analysis_dialog_init_ui(const BtWireAnalysisDialog *self) {
  BtMainWindow *main_window;
  BtMachine *src_machine,*dst_machine;
  BtSong *song;
  GstBin *bin;
  GstBus *bus;
  gchar *src_id,*dst_id,*title;
  //GdkPixbuf *window_icon=NULL;
  GtkWidget *vbox, *hbox;
  GtkWidget *ruler;
  GtkWidgetClass *ruler_class;

  gtk_widget_set_name(GTK_WIDGET(self),_("wire analysis"));

  g_object_get(self->priv->app,"main-window",&main_window,"song",&song,NULL);
  gtk_window_set_transient_for(GTK_WINDOW(self),GTK_WINDOW(main_window));
  gtk_window_set_resizable(GTK_WINDOW(self), FALSE);

  /* @todo: create and set *proper* window icon (analyzer, scope)
  if((window_icon=bt_ui_ressources_get_pixbuf_by_wire(self->priv->wire))) {
    gtk_window_set_icon(GTK_WINDOW(self),window_icon);
  }
  */

  /* DEBUG
  self->priv->min_rms=1000.0;
  self->priv->max_rms=-1000.0;
  self->priv->min_peak=1000.0;
  self->priv->max_peak=-1000.0;
  // DEBUG */

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

  vbox = gtk_vbox_new(FALSE, 0);

  /* add scales for spectrum analyzer drawable */
  /* @todo: we need to use a gtk_table() and also add a vruler with levels */
  ruler=gtk_hruler_new();
  gtk_ruler_set_range(GTK_RULER(ruler),0.0,/*srat/20.0*/2205.0,-10.0,15.0);
  //gtk_ruler_set_metric(GTK_RULER(ruler),&ruler_metrics[0]);
  ruler_class=GTK_WIDGET_GET_CLASS(ruler);
  ruler_class->motion_notify_event = NULL;
  gtk_widget_set_size_request(GTK_WIDGET(ruler),-1,30);
  gtk_box_pack_start(GTK_BOX(vbox), ruler, FALSE, FALSE,0);

  /* add spectrum canvas */
  self->priv->spectrum_drawingarea=gtk_drawing_area_new ();
  gtk_widget_set_size_request(self->priv->spectrum_drawingarea, SPECT_WIDTH, SPECT_HEIGHT);
  gtk_box_pack_start(GTK_BOX(vbox), self->priv->spectrum_drawingarea, FALSE, FALSE, 0);

  /* spacer */
  gtk_container_add (GTK_CONTAINER (vbox), gtk_label_new(" "));

  /* add scales for level meter */
  hbox = gtk_hbox_new(FALSE, 0);
  ruler=gtk_hruler_new();
  gtk_ruler_set_range(GTK_RULER(ruler),100.0,0.0,-10.0,15.0);
  //gtk_ruler_set_metric(GTK_RULER(ruler),&ruler_metrics[0]);
  ruler_class=GTK_WIDGET_GET_CLASS(ruler);
  ruler_class->motion_notify_event = NULL;
  gtk_widget_set_size_request(GTK_WIDGET(ruler),-1,30);
  gtk_box_pack_start(GTK_BOX(hbox), ruler, TRUE, TRUE, 0);
  ruler=gtk_hruler_new();
  gtk_ruler_set_range(GTK_RULER(ruler),0.0,100.0,-10.0,15.0);
  //gtk_ruler_set_metric(GTK_RULER(ruler),&ruler_metrics[0]);
  ruler_class=GTK_WIDGET_GET_CLASS(ruler);
  ruler_class->motion_notify_event = NULL;
  gtk_widget_set_size_request(GTK_WIDGET(ruler),-1,30);
  gtk_box_pack_start(GTK_BOX(hbox), ruler, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  /* add level-meter canvas */
  self->priv->level_drawingarea = gtk_drawing_area_new ();
  gtk_widget_set_size_request (self->priv->level_drawingarea, LEVEL_WIDTH, LEVEL_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox), self->priv->level_drawingarea, FALSE, FALSE, 0);

  gtk_container_set_border_width(GTK_CONTAINER (self), 6);
  gtk_container_add (GTK_CONTAINER (self), vbox);

  // create fakesink
  if(!bt_wire_analysis_dialog_make_element(self,ANALYZER_FAKESINK,"fakesink")) return(FALSE);
  g_object_set (G_OBJECT(self->priv->analyzers[ANALYZER_FAKESINK]),
      "sync", FALSE, "qos", FALSE, "silent", TRUE,
      NULL);
  // create spectrum analyzer
  if(!bt_wire_analysis_dialog_make_element(self,ANALYZER_SPECTRUM,"spectrum")) return(FALSE);
  g_object_set (G_OBJECT(self->priv->analyzers[ANALYZER_SPECTRUM]),
      "bands", SPECT_BANDS, "threshold", -70, "message", TRUE,
      NULL);
  // create level meter
  if(!bt_wire_analysis_dialog_make_element(self,ANALYZER_LEVEL,"level")) return(FALSE);
  g_object_set(G_OBJECT(self->priv->analyzers[ANALYZER_LEVEL]),
      "interval",(GstClockTime)(0.1*GST_SECOND),"message",TRUE,
      "peak-ttl",(GstClockTime)(0.2*GST_SECOND),"peak-falloff", 20.0,
      NULL);
  // create queue
  if(!bt_wire_analysis_dialog_make_element(self,ANALYZER_QUEUE,"queue")) return(FALSE);

  g_object_set(G_OBJECT(self->priv->wire),"analyzers",self->priv->analyzers_list,NULL);

  // @todo: remove
  //bt_application_add_bus_watch(BT_APPLICATION(self->priv->app),on_wire_analyzer_change,(gpointer)self);
  g_object_get(G_OBJECT(song),"bin", &bin, NULL);
  bus=gst_element_get_bus(GST_ELEMENT(bin));
  g_signal_connect(bus, "message::element", G_CALLBACK(on_wire_analyzer_change), (gpointer)self);
  gst_object_unref(bus);
  gst_object_unref(bin);

  // allocate visual ressources after the window has been realized
  g_signal_connect(G_OBJECT(self),"realize",G_CALLBACK(bt_wire_analysis_dialog_realize),(gpointer)self);
  // redraw when needed
  g_signal_connect(G_OBJECT(self),"expose-event",G_CALLBACK(bt_wire_analysis_dialog_expose),(gpointer)self);

  g_object_try_unref(song);
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
  GST_DEBUG("dialog created and shown");
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
  BtSong *song;

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  /* DEBUG
  GST_DEBUG("levels: rms =%7.4lf .. %7.4lf",self->priv->min_rms ,self->priv->max_rms);
  GST_DEBUG("levels: peak=%7.4lf .. %7.4lf",self->priv->min_peak,self->priv->max_peak);
  // DEBUG */

  if(self->priv->paint_handler_id) {
    g_source_remove(self->priv->paint_handler_id);
  }
  g_object_try_unref(self->priv->peak_gc);

  // @todo: remove
  //bt_application_remove_bus_watch(BT_APPLICATION(self->priv->app),on_wire_analyzer_change,(gpointer)self);

  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  if(song) {
    GstBin *bin;
    GstBus *bus;

    g_object_get(G_OBJECT(song),"bin", &bin, NULL);

    bus=gst_element_get_bus(GST_ELEMENT(bin));
    g_signal_handlers_disconnect_matched(bus,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_wire_analyzer_change,NULL);
    gst_object_unref(bus);
    gst_object_unref(bin);
    g_object_unref(song);
  }

  // this destroys the analyzers too
  g_object_set(G_OBJECT(self->priv->wire),"analyzers",NULL,NULL);
  g_list_free(self->priv->analyzers_list);

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
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WIRE_ANALYSIS_DIALOG_WIRE,
                                  g_param_spec_object("wire",
                                     "wire construct prop",
                                     "Set wire object, the dialog handles",
                                     BT_TYPE_WIRE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

}

GType bt_wire_analysis_dialog_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    const GTypeInfo info = {
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
