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
 * SECTION:btwireanalysisdialog
 * @short_description: audio analysis for this wire
 *
 * The dialog shows a mono-spectrum analyzer and level-meters for left/right
 * channel.
 *
 * The dialog is not modal.
 */
/* @idea: nice monitoring ideas:
 * http://www.music-software-reviews.com/adobe_audition_2.html
 *
 * @todo: shall we add a volume and panorama control to the dialog as well?
 * - volume to the right of the spectrum
 * - panorama below the spectrum
 *
 * @todo: we might need a lock for the resize and redraw
 */
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

//#define SPECT_BANDS 256
//#define SPECT_WIDTH (SPECT_BANDS)
//#define SPECT_HEIGHT 64
//#define LEVEL_WIDTH (SPECT_BANDS)
#define LEVEL_HEIGHT 16
#define LOW_VUMETER_VAL -90.0

#if 0
// not used right now
static const GtkRulerMetric ruler_metrics[] =
{
  {"Frequency", "Hz", 1.0, { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512 }, { 1, 2, 4, 8, 16 }},
};
#endif

struct _BtWireAnalysisDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the wire we analyze */
  BtWire *wire;

  /* the analyzer-graphs */
  GtkWidget *spectrum_drawingarea, *level_drawingarea;
  GdkGC *peak_gc;
  GdkGC *grid_gc;

  /* the gstreamer elements that is used */
  GstElement *analyzers[ANALYZER_COUNT];
  GList *analyzers_list;

  /* the analyzer results */
  gdouble rms[2], peak[2];
  gfloat *spect;
  
  guint spect_height;
  guint spect_bands;
  gfloat height_scale;
  
  GstClock *clock;

  // DEBUG
  //gdouble min_rms,max_rms, min_peak,max_peak;
  // DEBUG
};

static GtkDialogClass *parent_class=NULL;

static gint8 grid_dash_list[]= {1};

//-- event handler

/*
 * on_wire_analyzer_redraw:
 * @user_data: conatins the self pointer
 *
 * Called as idle function to repaint the analyzers. Data is gathered by
 * on_wire_analyzer_change()
 */
static gboolean redraw_level(gpointer user_data) {
  BtWireAnalysisDialog *self=BT_WIRE_ANALYSIS_DIALOG(user_data);

  if(!GTK_WIDGET_REALIZED(self)) return(TRUE);
  if(!GDK_IS_GC(self->priv->peak_gc)) return(TRUE);

  //GST_DEBUG("redraw analyzers");
  // draw levels
  if (self->priv->level_drawingarea) {
    GdkRectangle rect = { 0, 0, self->priv->spect_bands, LEVEL_HEIGHT };
    GtkWidget *da=self->priv->level_drawingarea;
    gint middle=self->priv->spect_bands>>1;

    gdk_window_begin_paint_rect (da->window, &rect);
    gdk_draw_rectangle (da->window, da->style->black_gc, TRUE, 0, 0, self->priv->spect_bands, LEVEL_HEIGHT);
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
    gdk_draw_rectangle (da->window, da->style->white_gc, TRUE, middle    -self->priv->rms[0], 0, self->priv->rms[0]+self->priv->rms[1], LEVEL_HEIGHT);
    gdk_draw_rectangle (da->window, self->priv->peak_gc, TRUE, middle    -self->priv->peak[0], 0, 2, LEVEL_HEIGHT);
    gdk_draw_rectangle (da->window, self->priv->peak_gc, TRUE, (middle-1)+self->priv->peak[1], 0, 2, LEVEL_HEIGHT);
    gdk_window_end_paint (da->window);
    /* @todo: if stereo draw pan-meter (L-R, R-L) */
  }
  return(TRUE);
}

static gboolean redraw_spectrum(gpointer user_data) {
  BtWireAnalysisDialog *self=BT_WIRE_ANALYSIS_DIALOG(user_data);

  if(!GTK_WIDGET_REALIZED(self)) return(TRUE);
  if(!GDK_IS_GC(self->priv->peak_gc)) return(TRUE);

  // draw spectrum
  if (self->priv->spectrum_drawingarea) {
    guint i,x,y;
    guint spect_bands=self->priv->spect_bands;
    guint spect_height=self->priv->spect_height;
    GdkRectangle rect = { 0, 0, spect_bands, self->priv->spect_height };
    GtkWidget *da=self->priv->spectrum_drawingarea;

    gdk_window_begin_paint_rect (da->window, &rect);
    gdk_draw_rectangle (da->window, da->style->black_gc, TRUE, 0, 0, spect_bands, spect_height);
    // draw grid lines
    y=spect_height/2;
    gdk_draw_line(da->window,self->priv->grid_gc,0,y,spect_bands,y);
    x=spect_bands/4;
    gdk_draw_line(da->window,self->priv->grid_gc,x,0,x,spect_height);
    x=spect_bands/2;
    gdk_draw_line(da->window,self->priv->grid_gc,x,0,x,spect_height);
    x=spect_bands/4+spect_bands/2;
    gdk_draw_line(da->window,self->priv->grid_gc,x,0,x,spect_height);
    // draw frequencies
    if(self->priv->spect) {
      /* @todo: draw grid under spectrum
       * 0... <srat>
       * the bin center frequencies are f(i)=i*srat/spect_bands
       */
      for (i = 0; i < spect_bands; i++) {
        //db_value=20.0*log10(self->priv->spect[i]);
        gdk_draw_rectangle (da->window, da->style->white_gc, TRUE, i, -self->priv->spect[i], 1, spect_height + self->priv->spect[i]);
      }
    }
    gdk_window_end_paint (da->window);
  }
  return(TRUE);
}

static void bt_wire_analysis_dialog_realize(GtkWidget *widget,gpointer user_data) {
  BtWireAnalysisDialog *self=BT_WIRE_ANALYSIS_DIALOG(user_data);

  GST_DEBUG("dialog realize");
  self->priv->peak_gc=gdk_gc_new(GTK_WIDGET(self)->window);
  gdk_gc_set_rgb_fg_color(self->priv->peak_gc,bt_ui_resources_get_gdk_color(BT_UI_RES_COLOR_ANALYZER_PEAK));
  self->priv->grid_gc=gdk_gc_new(GTK_WIDGET(self)->window);
  gdk_gc_set_rgb_fg_color(self->priv->grid_gc,bt_ui_resources_get_gdk_color(BT_UI_RES_COLOR_GRID_LINES));
  gdk_gc_set_line_attributes(self->priv->grid_gc,1,GDK_LINE_ON_OFF_DASH,GDK_CAP_BUTT,GDK_JOIN_MITER);
  gdk_gc_set_dashes(self->priv->grid_gc,0,grid_dash_list,1);
}

static gboolean bt_wire_analysis_dialog_level_expose(GtkWidget *widget,GdkEventExpose *event,gpointer user_data) {
  BtWireAnalysisDialog *self=BT_WIRE_ANALYSIS_DIALOG(user_data);

  redraw_level((gpointer)self);
  return(TRUE);
}

static gboolean bt_wire_analysis_dialog_spectrum_expose(GtkWidget *widget,GdkEventExpose *event,gpointer user_data) {
  BtWireAnalysisDialog *self=BT_WIRE_ANALYSIS_DIALOG(user_data);

  redraw_spectrum((gpointer)self);
  return(TRUE);
}

static gboolean on_delayed_idle_wire_analyzer_change(gpointer user_data) {
  gconstpointer * const params=(gconstpointer *)user_data;
  BtWireAnalysisDialog *self=BT_WIRE_ANALYSIS_DIALOG(params[0]);
  GstMessage *message=(GstMessage *)params[1];
  const GstStructure *structure=gst_message_get_structure(message);
  const gchar *name = gst_structure_get_name(structure);
  
  if(!self)
    goto done;

  g_object_remove_weak_pointer(G_OBJECT(self),(gpointer *)&params[0]);

  if(!strcmp(name,"level")) {
    const GValue *l_cur,*l_peak;
    guint i;
    gdouble val;

    //GST_INFO("get level data");
    //l_cur=(GValue *)gst_structure_get_value(structure, "rms");
    l_cur=(GValue *)gst_structure_get_value(structure, "peak");
    //l_peak=(GValue *)gst_structure_get_value(structure, "peak");
    l_peak=(GValue *)gst_structure_get_value(structure, "decay");
    // size of list is number of channels
    switch(gst_value_list_get_size(l_cur)) {
      case 1: // mono
          val=g_value_get_double(gst_value_list_get_value(l_cur,0));
          if(isinf(val) || isnan(val)) val=LOW_VUMETER_VAL;
          self->priv->rms[0]=-(LOW_VUMETER_VAL-val);
          self->priv->rms[1]=self->priv->rms[0];
          val=g_value_get_double(gst_value_list_get_value(l_peak,0));
          if(isinf(val) || isnan(val)) val=LOW_VUMETER_VAL;
          self->priv->peak[0]=-(LOW_VUMETER_VAL-val);
          self->priv->peak[1]=self->priv->peak[0];
        break;
      case 2: // stereo
        for(i=0;i<2;i++) {
          val=g_value_get_double(gst_value_list_get_value(l_cur,i));
          if(isinf(val) || isnan(val)) val=LOW_VUMETER_VAL;
          self->priv->rms[i]=-(LOW_VUMETER_VAL-val);
          val=g_value_get_double(gst_value_list_get_value(l_peak,i));
          if(isinf(val) || isnan(val)) val=LOW_VUMETER_VAL;
          self->priv->peak[i]=-(LOW_VUMETER_VAL-val);
        }
        break;
    }
    gtk_widget_queue_draw(self->priv->level_drawingarea);
  }
  else if(!strcmp(name,"spectrum")) {
    const GValue *list;
    const GValue *value;
    guint i, spect_bands=self->priv->spect_bands,size;
    gfloat height_scale=self->priv->height_scale;

    //GST_INFO("get spectrum data");
    if((list = gst_structure_get_value (structure, "magnitude"))) {
      size=gst_value_list_get_size(list);
      if(size==spect_bands) {
        for(i=0;i<spect_bands;i++) {
          value=gst_value_list_get_value(list,i);
          self->priv->spect[i]=height_scale*g_value_get_float(value);
        }
        gtk_widget_queue_draw(self->priv->spectrum_drawingarea);
      }
    }
    else if((list = gst_structure_get_value (structure, "spectrum"))) {
      size=gst_value_list_get_size(list);
      if(size==spect_bands) {
        for(i=0;i<spect_bands;i++) {
          value=gst_value_list_get_value(list,i);
          self->priv->spect[i]=height_scale*(gfloat)g_value_get_uchar(value);
        }
        gtk_widget_queue_draw(self->priv->spectrum_drawingarea);
      }
    }
  }
  
done:
  gst_message_unref(message);
  g_free(params);
  return(FALSE);
}

static gboolean on_delayed_wire_analyzer_change(GstClock *clock,GstClockTime time,GstClockID id,gpointer user_data) {
  // the callback is called froma clock thread
  if(GST_CLOCK_TIME_IS_VALID(time))
    g_idle_add(on_delayed_idle_wire_analyzer_change,user_data);
  else
    g_free(user_data);
  return(TRUE);
}

static void on_wire_analyzer_change(GstBus * bus, GstMessage * message, gpointer user_data) {
  BtWireAnalysisDialog *self=BT_WIRE_ANALYSIS_DIALOG(user_data);
  const GstStructure *structure=gst_message_get_structure(message);
  const gchar *name = gst_structure_get_name(structure);

  g_assert(user_data);

  if((!strcmp(name,"level")) || (!strcmp(name,"spectrum"))) {  
    GstElement *meter=GST_ELEMENT(GST_MESSAGE_SRC(message));
    
    if((meter==self->priv->analyzers[ANALYZER_LEVEL]) ||
      (meter==self->priv->analyzers[ANALYZER_SPECTRUM])) {
      GstClockTime timestamp, duration;
      GstClockTime waittime=GST_CLOCK_TIME_NONE;
  
      if(gst_structure_get_clock_time (structure, "running-time", &timestamp) &&
        gst_structure_get_clock_time (structure, "duration", &duration)) {
        /* wait for middle of buffer */
        waittime=timestamp+duration/2;
      }
      else if(gst_structure_get_clock_time (structure, "endtime", &timestamp)) {
        if(!strcmp(name,"level")) {
          /* level send endtime as stream_time and not as running_time */
          waittime=gst_segment_to_running_time(&GST_BASE_TRANSFORM(meter)->segment, GST_FORMAT_TIME, timestamp);
        }
        else {
          waittime=timestamp;
        }
      }
      if(GST_CLOCK_TIME_IS_VALID(waittime)) {
        gconstpointer *params=g_new(gconstpointer,2);
        GstClockID clock_id;
        GstClockTime basetime=gst_element_get_base_time(meter);
  
        //GST_WARNING("target %"GST_TIME_FORMAT" %"GST_TIME_FORMAT,
        //  GST_TIME_ARGS(endtime),GST_TIME_ARGS(waittime));
      
        params[0]=(gpointer)self;
        params[1]=(gpointer)gst_message_ref(message);
        g_object_add_weak_pointer(G_OBJECT(self),(gpointer *)&params[0]);
        clock_id=gst_clock_new_single_shot_id(self->priv->clock,waittime+basetime);
        gst_clock_id_wait_async(clock_id,on_delayed_wire_analyzer_change,(gpointer)params);
        gst_clock_id_unref(clock_id);
      }
    }
  }
}

static void on_size_allocate(GtkWidget *widget,GtkAllocation *allocation,gpointer user_data) {
  BtWireAnalysisDialog *self=BT_WIRE_ANALYSIS_DIALOG(user_data);
  guint i;

  /*GST_INFO ("%d x %d", allocation->width, allocation->height); */
  self->priv->spect_height = allocation->height;
  self->priv->height_scale = allocation->height / 64.0;
  self->priv->spect_bands = allocation->width;
  self->priv->spect = g_renew (gfloat, self->priv->spect, self->priv->spect_bands);
  for(i=0;i<self->priv->spect_bands;i++)
    self->priv->spect[i]=(gfloat)(-allocation->height);

  g_object_set (G_OBJECT (self->priv->analyzers[ANALYZER_SPECTRUM]),
    "bands", self->priv->spect_bands, NULL);
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
  name=g_alloca(strlen("analyzer_")+strlen(factory_name)+16);g_sprintf(name,"analyzer_%s_%p",factory_name,self->priv->wire);
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
  gboolean res=TRUE;

  gtk_widget_set_name(GTK_WIDGET(self),"wire analysis");

  g_object_get(self->priv->app,"main-window",&main_window,"song",&song,NULL);
  gtk_window_set_transient_for(GTK_WINDOW(self),GTK_WINDOW(main_window));

  /* @todo: create and set *proper* window icon (analyzer, scope)
  if((window_icon=bt_ui_resources_get_pixbuf_by_wire(self->priv->wire))) {
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
  // machine -> machine connection analyzer
  title=g_strdup_printf(_("%s -> %s analysis"),src_id,dst_id);
  gtk_window_set_title(GTK_WINDOW(self),title);
  g_free(src_id);g_free(dst_id);g_free(title);
  g_object_unref(src_machine);
  g_object_unref(dst_machine);

  vbox = gtk_vbox_new(FALSE, 0);

  /* add scales for spectrum analyzer drawable */
  /* @todo: we need to use a gtk_table() and also add a vruler with levels */
  ruler=gtk_hruler_new();
  gtk_ruler_set_range(GTK_RULER(ruler),0.0,/*srat/20.0*/2205.0,-10.0,200.0);
  GTK_RULER_GET_CLASS(ruler)->draw_pos = NULL;
  gtk_widget_set_size_request(GTK_WIDGET(ruler),-1,30);
  gtk_box_pack_start(GTK_BOX(vbox), ruler, FALSE, FALSE,0);

  /* add spectrum canvas */
  self->priv->spectrum_drawingarea=gtk_drawing_area_new ();
  gtk_widget_set_size_request(self->priv->spectrum_drawingarea, self->priv->spect_bands, self->priv->spect_height);
  g_signal_connect (G_OBJECT (self->priv->spectrum_drawingarea), "size-allocate",
      G_CALLBACK (on_size_allocate), (gpointer) self);
  gtk_box_pack_start(GTK_BOX(vbox), self->priv->spectrum_drawingarea, TRUE, TRUE, 0);

  /* spacer */
  gtk_box_pack_start (GTK_BOX (vbox), gtk_label_new(" "), FALSE, FALSE, 0);

  /* add scales for level meter */
  hbox = gtk_hbox_new(FALSE, 0);
  ruler=gtk_hruler_new();
  gtk_ruler_set_range(GTK_RULER(ruler),100.0,0.0,-10.0,30.0);
  //gtk_ruler_set_metric(GTK_RULER(ruler),&ruler_metrics[0]);
  GTK_RULER_GET_CLASS(ruler)->draw_pos = NULL;
  gtk_widget_set_size_request(GTK_WIDGET(ruler),-1,30);
  gtk_box_pack_start(GTK_BOX(hbox), ruler, TRUE, TRUE, 0);
  ruler=gtk_hruler_new();
  gtk_ruler_set_range(GTK_RULER(ruler),0.0,100.5,-10.0,30.0);
  //gtk_ruler_set_metric(GTK_RULER(ruler),&ruler_metrics[0]);
  GTK_RULER_GET_CLASS(ruler)->draw_pos = NULL;
  gtk_widget_set_size_request(GTK_WIDGET(ruler),-1,30);
  gtk_box_pack_start(GTK_BOX(hbox), ruler, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  /* add level-meter canvas */
  self->priv->level_drawingarea = gtk_drawing_area_new ();
  gtk_widget_set_size_request (self->priv->level_drawingarea, self->priv->spect_bands, LEVEL_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox), self->priv->level_drawingarea, FALSE, FALSE, 0);

  gtk_container_set_border_width(GTK_CONTAINER (self), 6);
  gtk_container_add (GTK_CONTAINER (self), vbox);

  // create fakesink
  if(!bt_wire_analysis_dialog_make_element(self,ANALYZER_FAKESINK,"fakesink")) {
    res=FALSE;
    goto Error;
  }
  g_object_set (G_OBJECT(self->priv->analyzers[ANALYZER_FAKESINK]),
      "sync", FALSE, "qos", FALSE, "silent", TRUE, "async", FALSE,
      NULL);
  // create spectrum analyzer
  if(!bt_wire_analysis_dialog_make_element(self,ANALYZER_SPECTRUM,"spectrum")) {
    res=FALSE;
    goto Error;
  }
  g_object_set (G_OBJECT(self->priv->analyzers[ANALYZER_SPECTRUM]),
      "interval",(GstClockTime)(0.1*GST_SECOND),"message",TRUE,
      "bands", self->priv->spect_bands, "threshold", -80,
      NULL);
  // create level meter
  if(!bt_wire_analysis_dialog_make_element(self,ANALYZER_LEVEL,"level")) {
    res=FALSE;
    goto Error;
  }
  g_object_set(G_OBJECT(self->priv->analyzers[ANALYZER_LEVEL]),
      "interval",(GstClockTime)(0.1*GST_SECOND),"message",TRUE,
      "peak-ttl",(GstClockTime)(0.3*GST_SECOND),"peak-falloff", 80.0,
      NULL);
  // create queue
  if(!bt_wire_analysis_dialog_make_element(self,ANALYZER_QUEUE,"queue")) {
    res=FALSE;
    goto Error;
  }
  g_object_set(G_OBJECT(self->priv->analyzers[ANALYZER_QUEUE]),
      "max-size-buffers",1,"max-size-bytes",0,"max-size-time",G_GUINT64_CONSTANT(0),
      "leaky",2,NULL);

  g_object_set(G_OBJECT(self->priv->wire),"analyzers",self->priv->analyzers_list,NULL);

  g_object_get(G_OBJECT(song),"bin", &bin, NULL);
  bus=gst_element_get_bus(GST_ELEMENT(bin));
  g_signal_connect(bus, "message::element", G_CALLBACK(on_wire_analyzer_change), (gpointer)self);
  gst_object_unref(bus);
  self->priv->clock=gst_pipeline_get_clock (GST_PIPELINE(bin));
  gst_object_unref(bin);

  // allocate visual ressources after the window has been realized
  g_signal_connect(G_OBJECT(self),"realize",G_CALLBACK(bt_wire_analysis_dialog_realize),(gpointer)self);
  // redraw when needed
  g_signal_connect(G_OBJECT(self->priv->level_drawingarea),"expose-event",G_CALLBACK(bt_wire_analysis_dialog_level_expose),(gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->spectrum_drawingarea),"expose-event",G_CALLBACK(bt_wire_analysis_dialog_spectrum_expose),(gpointer)self);

Error:
  g_object_unref(song);
  g_object_unref(main_window);
  return(res);
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
  gtk_widget_destroy(GTK_WIDGET(self));
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
  
  if(self->priv->clock) gst_object_unref(self->priv->clock);

  g_object_try_unref(self->priv->peak_gc);
  g_object_try_unref(self->priv->grid_gc);

  GST_DEBUG("!!!! removing signal handler");

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
  GST_DEBUG("!!!! free analyzers");
  g_object_set(G_OBJECT(self->priv->wire),"analyzers",NULL,NULL);

  g_object_try_unref(self->priv->app);
  g_object_try_unref(self->priv->wire);

  GST_DEBUG("!!!! done");

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_wire_analysis_dialog_finalize(GObject *object) {
  BtWireAnalysisDialog *self = BT_WIRE_ANALYSIS_DIALOG(object);

  GST_DEBUG("!!!! self=%p",self);

  g_list_free(self->priv->analyzers_list);

  GST_DEBUG("!!!! done");

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_wire_analysis_dialog_init(GTypeInstance *instance, gpointer g_class) {
  BtWireAnalysisDialog *self = BT_WIRE_ANALYSIS_DIALOG(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_WIRE_ANALYSIS_DIALOG, BtWireAnalysisDialogPrivate);
  self->priv->spect_height = 64;
  self->priv->spect_bands = 256;
  self->priv->height_scale = 1.0;
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
  if (G_UNLIKELY(type == 0)) {
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
