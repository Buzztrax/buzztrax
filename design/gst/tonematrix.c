/* $Id:
 * audio controller and mixing example
 *
 * gcc -Wall -g `pkg-config --cflags --libs gstreamer-0.10 gstreamer-controller-0.10 gtk+-2.0` tonematrix.c -o tonematrix
 */

#include <math.h>
#include <glib.h>
#include <gst/gst.h>
#include <gst/controller/gstcontroller.h>
#include <gst/controller/gstinterpolationcontrolsource.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

typedef struct {
  /* gtk components */
  GtkWidget *window;
  GtkWidget *metro[16];
  GdkColor white, black;
  guint blink_id;
  
  /* gstreamer components */
  GstElement *bin;
  GstElement *src[16];
  GstInterpolationControlSource *csrc[16];
  GstClockTime loop_time, step_time;
  GType wave_enum_type;
  
  /* state */
  guint scale;
  guint bpm;
  guint matrix[16][16];
  guint blink;
} App;


#define MAKE_COLOR(r,g,b) { 0L, (r)<<8, (g)<<8, (b)<<8 }
#define SET_COLOR(c,r,g,b) (c).pixel=0L; (c).red=(r)<<8; (c).green=(g)<<8; (c).blue=(b)<<8

#define WAVES 6

static struct {
  gint ix;
  gchar *name;
  GdkColor color;
} WaveMap[WAVES] = {
  { 4, "silence", MAKE_COLOR(0x00,0x00,0x00)},
  { 0, "sine", MAKE_COLOR(0x00,0x00,0x7F)},
  { 1, "square", MAKE_COLOR(0x00,0x00,0xFF)},
  { 2, "saw", MAKE_COLOR(0x00,0xFF,0x7F)},
  { 3, "triangle", MAKE_COLOR(0x7F,0xFF,0x00)},
  { 5, "white noise", MAKE_COLOR(0xFF,0xFF,0xFF)}  
};

static GQuark tpos,fpos;

/* hacked colorbutton that does not show a colorpicker */

typedef GtkColorButton MyColorButton;
typedef GtkColorButtonClass MyColorButtonClass;

G_DEFINE_TYPE(MyColorButton, my_color_button, GTK_TYPE_COLOR_BUTTON);

static void
my_color_button_class_init(MyColorButtonClass *klass)
{
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);
  button_class->clicked = NULL;
}

static void
my_color_button_init(MyColorButton *color_button)
{
  GList *c1 = gtk_container_get_children(GTK_CONTAINER(color_button));
  
  gtk_container_set_border_width (GTK_CONTAINER(color_button), 0);
  if(c1) {
    GtkWidget *w1=GTK_WIDGET(c1->data);

    gtk_container_set_border_width (GTK_CONTAINER (w1), 0);
    gtk_alignment_set (GTK_ALIGNMENT (w1), 0.5, 0.5, 1.0, 1.0);
    //gtk_alignment_set_padding (GTK_ALIGNMENT (w1), 0, 0, 0, 0);
    GList *c2 = gtk_container_get_children(GTK_CONTAINER(w1));
    if(c2) {
      GtkWidget *w2=GTK_WIDGET(c2->data);

      gtk_container_set_border_width (GTK_CONTAINER (w2), 0);
      gtk_frame_set_shadow_type (GTK_FRAME (w2), GTK_SHADOW_IN);
    }
  }
}

static GtkWidget*
my_color_button_new(void)
{
  return g_object_new(my_color_button_get_type(), NULL);
}

static GtkWidget*
my_color_button_new_with_color(GdkColor *color)
{
  return g_object_new(my_color_button_get_type(), "color", color, NULL);
}

/* gstreamer functions */

static void start_blink(App *app);

static void
message_received (GstBus *bus, GstMessage *message, App *app) {
  const GstStructure *s;

  s = gst_message_get_structure (message);
  g_print ("message from \"%s\" (%s): ",
      GST_STR_NULL (GST_ELEMENT_NAME (GST_MESSAGE_SRC (message))),
      gst_message_type_get_name (GST_MESSAGE_TYPE (message)));
  if (s) {
    gchar *sstr = gst_structure_to_string (s);
    puts (sstr);
    g_free (sstr);
  }
  gtk_main_quit();
}

static void segment_done(GstBus *bus, GstMessage *message, App *app) {
  gst_element_send_event(app->bin,
      gst_event_new_seek(1.0, GST_FORMAT_TIME,
          GST_SEEK_FLAG_SEGMENT,
          GST_SEEK_TYPE_SET, G_GUINT64_CONSTANT(0),
          GST_SEEK_TYPE_SET, app->loop_time));
  start_blink(app);
}

static void state_changed(GstBus *bus, GstMessage *message, App *app) {
  if(GST_MESSAGE_SRC(message) == GST_OBJECT(app->bin)) {
    GstState oldstate,newstate,pending;

    gst_message_parse_state_changed(message,&oldstate,&newstate,&pending);
    switch(GST_STATE_TRANSITION(oldstate,newstate)) {
      case GST_STATE_CHANGE_READY_TO_PAUSED:
        start_blink(app);
        gst_element_send_event(app->bin,
            gst_event_new_seek(1.0, GST_FORMAT_TIME,
                GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
                GST_SEEK_TYPE_SET, G_GUINT64_CONSTANT(0),
                GST_SEEK_TYPE_SET, app->loop_time));
        gst_element_set_state(app->bin, GST_STATE_PLAYING);
        break;
      default:
        break;
    }
  }
}


static void
set_loop(App *app)
{
  // TODO: calculate from bpm
  // lets assume 4 seconds loop time for now
  app->loop_time = GST_SECOND * 4;
  app->step_time = app->loop_time / 16;
}

static void
set_tones(App *app)
{
  guint i,note;
  gdouble f[3*12], freq, step;
  guint scales[][7]={
    /* c-dur */
    {0,2,4,5,7,9,11},
  };

  /* calculate frequencies */
  step=pow(2.0,(1.0/12.0));
  freq=220.0; // start with a A(-1)
  for (i=0; i<(3*12); i++) {
    f[i]=freq;
    freq*=step;
  }
  /* set frequencies for 16 tones for choosen scale */
  for (i=0; i<7; i++) {
    note=scales[0][i];
    g_object_set(G_OBJECT(app->src[i]), "freq", f[note], NULL);
    g_object_set(G_OBJECT(app->src[7+i]), "freq", f[12+note], NULL);
    if(14+i < 16)
      g_object_set(G_OBJECT(app->src[14+i]), "freq", f[24+note], NULL);
  }
}

static void
init_pipeline(App *app)
{
  GstBus *bus;
  GstElement *src,*mix,*sink;
  GstInterpolationControlSource *csrc;
  GstController *ctrl;
  GValue val = { 0, };
  GstClockTime st;
  guint i,j;
  gdouble vol;

  app->bin = gst_pipeline_new ("song");
  
  bus = gst_pipeline_get_bus (GST_PIPELINE (app->bin));
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
  g_signal_connect (bus, "message::error", G_CALLBACK(message_received), app);
  g_signal_connect (bus, "message::warning", G_CALLBACK(message_received), app);
  g_signal_connect (bus, "message::segment-done", G_CALLBACK(segment_done), app);
  g_signal_connect (bus, "message::state-changed", G_CALLBACK(state_changed), app);
  
  sink = gst_element_factory_make ("autoaudiosink", NULL);
  mix = gst_element_factory_make ("adder", NULL);
  gst_bin_add_many (GST_BIN(app->bin), mix, sink, NULL);
  gst_element_link (mix, sink);
  
  set_loop(app);
  st = app->step_time;
  
  /* create sources and setup the controllers */
  for (i=0; i<16; i++) {
    app->src[i] = src = gst_element_factory_make ("audiotestsrc", NULL);
    gst_bin_add (GST_BIN(app->bin), src);
    gst_element_link (src, mix);
    
    if(G_UNLIKELY(app->wave_enum_type==0)) {
      /* grab the enum type after we make the first instance */
      app->wave_enum_type=g_type_from_name("GstAudioTestSrcWave");
      g_assert(app->wave_enum_type!=0);    
    }
    
    /* we can have 16 tones at a time, but as there is always some cancelation,
     * lower the volume a bit less */
    vol=1.0/10.0;

    ctrl = gst_controller_new (G_OBJECT (src), "volume", "wave", NULL);
    /* volume control */
    csrc = gst_interpolation_control_source_new ();
    gst_controller_set_control_source (ctrl, "volume", GST_CONTROL_SOURCE (csrc));
    gst_interpolation_control_source_set_interpolation_mode (csrc, GST_INTERPOLATE_LINEAR);
    g_value_init (&val, G_TYPE_DOUBLE);
    for (j=0;j<16;j++) {
      g_value_set_double (&val, 0.0);
      gst_interpolation_control_source_set (csrc, j*st, &val);
      g_value_set_double (&val, vol);
      gst_interpolation_control_source_set (csrc, (j+0.05)*st, &val);
      g_value_set_double (&val, 0.0);
      gst_interpolation_control_source_set (csrc, (j+0.90)*st, &val);
    }
    g_value_unset(&val);
    
    /* wave control */
    app->csrc[i] = csrc = gst_interpolation_control_source_new ();
    gst_controller_set_control_source (ctrl, "wave", GST_CONTROL_SOURCE (csrc));
    gst_interpolation_control_source_set_interpolation_mode (csrc, GST_INTERPOLATE_NONE);
    g_value_init (&val, app->wave_enum_type);
    for (j=0;j<16;j++) {
      g_value_set_enum (&val, WaveMap[0].ix);
      gst_interpolation_control_source_set (csrc, j*st, &val);
    }
    g_value_unset(&val);
  }
  
  set_tones(app);
  
  /* play */
  gst_element_set_state (app->bin, GST_STATE_PAUSED);
  GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(app->bin), GST_DEBUG_GRAPH_SHOW_ALL & ~GST_DEBUG_GRAPH_SHOW_CAPS_DETAILS, "tonematrix");
  puts("prepared");
}

static void
done_pipeline(App *app)
{
  gst_element_set_state (app->bin, GST_STATE_NULL);
  gst_object_unref (app->bin);
}

/* gtk functions */

/* this is a hack :/, 
 * we would need to control some property on the sink and have a notify on it
 * - but that would not be thread safe either
 */
static gboolean
on_blink(gpointer data)
{
  App *app = (App *)data;

  gtk_color_button_set_color(GTK_COLOR_BUTTON(app->metro[app->blink]),&app->black);
  app->blink++;
  if (app->blink<16) {
    gtk_color_button_set_color(GTK_COLOR_BUTTON(app->metro[app->blink]),&app->white);
    return TRUE;
  }
  app->blink_id=0;
  return FALSE;
}

static void
start_blink(App *app)
{
  if(app->blink_id) {
    g_source_remove(app->blink_id);
    if (app->blink<16) {
      gtk_color_button_set_color(GTK_COLOR_BUTTON(app->metro[app->blink]),&app->black);
    }
  }
  gtk_color_button_set_color(GTK_COLOR_BUTTON(app->metro[0]),&app->white);
  app->blink=0;
  app->blink_id=g_timeout_add_full(G_PRIORITY_HIGH, GST_TIME_AS_MSECONDS(app->step_time), on_blink, (gpointer)app, NULL);
}

static void
on_trigger(GtkButton *widget,gpointer data)
{
  App *app = (App *)data;
  GValue val = { 0, };
  guint i,j;
  
  /* get position */
  i=GPOINTER_TO_UINT(g_object_get_qdata(G_OBJECT(widget),tpos));
  j=GPOINTER_TO_UINT(g_object_get_qdata(G_OBJECT(widget),fpos));
  app->matrix[j][i]++;
  if(app->matrix[j][i] == WAVES)
    app->matrix[j][i]=0;

  /* update cell color */
  gtk_color_button_set_color(GTK_COLOR_BUTTON(widget), &WaveMap[app->matrix[j][i]].color);
  
  /* update controller */
  g_value_init(&val, app->wave_enum_type);
  g_value_set_enum(&val, WaveMap[app->matrix[j][i]].ix);
  gst_interpolation_control_source_set(app->csrc[j], i*app->step_time, &val);
  g_value_unset(&val);
}

static void
on_destroy(GtkWidget *widget,gpointer data)
{
  gtk_main_quit();
}

static void
init_ui(App *app)
{
  GtkWidget *vbox, *hbox;
  GtkWidget *matrix, *trigger;
  guint i,j;
  
  /* quarks */
  tpos = g_quark_from_static_string("Matrix:Time");
  fpos = g_quark_from_static_string("Matrix:Freq");
  
  /* metronome colors */
  SET_COLOR(app->white,0xFF,0xFF,0xFF);
  SET_COLOR(app->black,0x00,0x00,0x00);

  /* the ui window */
  app->window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(app->window), "Tone Matrix");
  g_signal_connect(G_OBJECT(app->window), "destroy",	G_CALLBACK (on_destroy), (gpointer)app);

  vbox=gtk_vbox_new(FALSE,3);
  gtk_container_add(GTK_CONTAINER(app->window),vbox);

  /* hbox with
       combo for scale
       spinner for tempo
   */
  hbox=gtk_hbox_new(FALSE,3);
  gtk_container_add(GTK_CONTAINER(vbox),hbox);
  
  //gtk_container_add(GTK_CONTAINER(hbox), gtk_label_new ("scale"));
  //gtk_container_add(GTK_CONTAINER(hbox), gtk_label_new ("tempo"));
  
  gtk_container_add(GTK_CONTAINER(hbox), gtk_label_new ("click the cells to trigger tones"));
  
  /* hbox with
       table with tone trigger buttons + metronome
       table with tone color->wave legend
   */
  hbox=gtk_hbox_new(FALSE,3);
  gtk_container_add(GTK_CONTAINER(vbox),hbox);
  
  matrix=gtk_table_new(17,16,TRUE);
  gtk_container_add(GTK_CONTAINER(hbox),matrix);
  for (i=0; i<16; i++) {
    for (j=0; j<16; j++) {
      trigger=my_color_button_new();
      g_object_set_qdata(G_OBJECT(trigger), tpos, GUINT_TO_POINTER(j));
      g_object_set_qdata(G_OBJECT(trigger), fpos, GUINT_TO_POINTER(i));
      gtk_table_attach_defaults(GTK_TABLE(matrix),trigger,j,j+1,i,i+1);
      g_signal_connect(G_OBJECT(trigger), "clicked", G_CALLBACK (on_trigger), (gpointer)app);
    }
  }
  gtk_table_set_row_spacing(GTK_TABLE(matrix),15,6);
  for (j=0; j<16; j++) {
    app->metro[j]=my_color_button_new_with_color(&app->black);
    gtk_table_attach_defaults(GTK_TABLE(matrix),app->metro[j],j,j+1,16,17);
  }

  matrix=gtk_table_new(WAVES,2,TRUE);
  gtk_container_add(GTK_CONTAINER(hbox),matrix);
  for (i=0; i<WAVES; i++) {
    gtk_table_attach_defaults(GTK_TABLE(matrix),my_color_button_new_with_color(&WaveMap[i].color),0,1,i,i+1);
    gtk_table_attach_defaults(GTK_TABLE(matrix),gtk_label_new (WaveMap[i].name),1,2,i,i+1);
  }

  gtk_widget_show_all(app->window);
}

gint
main (gint argc, gchar **argv)
{
  App app = {0, };

  /* init libraries */
  gst_init (&argc, &argv);
  gst_controller_init (&argc, &argv);
  gtk_init (&argc, &argv);
  
  /* set defaults */
  app.bpm=120;

  init_ui(&app);
  init_pipeline(&app);
  gtk_main();
  done_pipeline(&app);

  return 0;
}
