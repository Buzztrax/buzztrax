/* use analysis results from a gstreamer pipleine as values for a fake input
 * device
 *
 * gcc -Wall -g gstinput.c -o gstinput `pkg-config gstreamer-0.10 --cflags --libs`
 */

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <linux/input.h>
#include <linux/uinput.h>

#include <gst/gst.h>

enum
{
  MODE_AUDIO = 0,
  MODE_VIDEO
};
enum
{
  MODE_AUDIO_LEVEL = 0,
  MODE_VIDEO_BRIGHTNESS,
  MODE_VIDEO_FACEDETECT,
  MODE_COUNT
};
static struct
{
  guint axes;
  gchar *name;
} modes[] = {
  {
  1, "audio level"}, {
  2, "video brightness"}, {
  2, "video face-position"}
};

static GMainLoop *main_loop = NULL;
static gint ufile = -1;
static gint av_mode, mode;      /* set from command line params */

static void
message_received (GstBus * bus, GstMessage * message, GstPipeline * pipeline)
{
  const GstStructure *s;

  s = gst_message_get_structure (message);
  g_print ("message from \"%s\" (%s): ",
      GST_STR_NULL (GST_ELEMENT_NAME (GST_MESSAGE_SRC (message))),
      gst_message_type_get_name (GST_MESSAGE_TYPE (message)));
  if (s) {
    gchar *sstr;

    sstr = gst_structure_to_string (s);
    puts (sstr);
    g_free (sstr);
  } else {
    puts ("no message details");
  }

  g_main_loop_quit (main_loop);
}

static void
analyzer_result (GstBus * bus, GstMessage * message, GstPipeline * pipeline)
{
  const GstStructure *s = gst_message_get_structure (message);
  const gchar *name = gst_structure_get_name (s);
  struct input_event event;

  if (strcmp (name, "level") == 0) {
    const GValue *list;
    const GValue *value;
    gdouble rms_dB, rms;

    list = gst_structure_get_value (s, "rms");
    value = gst_value_list_get_value (list, 0);
    rms_dB = g_value_get_double (value);
    rms = pow (10, rms_dB / 20);

    //printf ("rms value: %lf -> %lf\n", rms_dB, rms);

    // synthesize events
    memset (&event, 0, sizeof (event));
    gettimeofday (&event.time, NULL);
    event.type = EV_ABS;
    event.code = ABS_X;
    event.value = rms * G_MAXINT32;
    write (ufile, &event, sizeof (event));
  }
  if (strcmp (name, "GstVideoAnalyse") == 0) {
    gdouble b, bv;

    gst_structure_get_double (s, "brightness", &b);
    gst_structure_get_double (s, "brightness-variance", &bv);

    //printf ("brightness values: %lf -> %lf\n", b, bv);

    // synthesize events
    memset (&event, 0, sizeof (event));
    gettimeofday (&event.time, NULL);
    event.type = EV_ABS;
    event.code = ABS_X;
    event.value = b * G_MAXINT32;
    write (ufile, &event, sizeof (event));
    event.code = ABS_Y;
    event.value = bv * G_MAXINT32;
    write (ufile, &event, sizeof (event));
  }
  if (strcmp (name, "facedetect") == 0) {
    const GValue *list;
    gint faces;

    list = gst_structure_get_value (s, "faces");
    faces = gst_value_list_get_size (list);

    if (faces) {
      const GValue *value;
      const GstStructure *face;
      guint x, y, w, h;

      value = gst_value_list_get_value (list, 0);
      face = gst_value_get_structure (value);

      gst_structure_get_uint (face, "x", &x);
      gst_structure_get_uint (face, "y", &y);
      gst_structure_get_uint (face, "width", &w);
      gst_structure_get_uint (face, "height", &h);

      //printf ("face 1/%d, values: %u,%u : %u,%u\n", faces, x,y,w,h);

      // synthesize events
      memset (&event, 0, sizeof (event));
      gettimeofday (&event.time, NULL);
      event.type = EV_ABS;
      event.code = ABS_X;
      event.value = (guint64) (x - w / 2) * G_MAXINT32 / (320 - w);
      write (ufile, &event, sizeof (event));
      event.code = ABS_Y;
      event.value = (guint64) (y - h / 2) * (G_MAXINT32 / (240 - h));
      write (ufile, &event, sizeof (event));
    }
  }
}

gint
main (gint argc, gchar ** argv)
{
  struct uinput_user_dev uinp;
  GstElement *bin;
  GstElement *e, *elems[10];
  GstCaps *caps;
  GstBus *bus;
  guint i, ne = 0;

  gst_init (&argc, &argv);

  if (argc > 1) {
    mode = atoi (argv[1]);
    if (mode >= MODE_COUNT)
      mode = MODE_AUDIO_LEVEL;
  } else {
    mode = MODE_AUDIO_LEVEL;
  }
  switch (mode) {
    case MODE_AUDIO_LEVEL:
      av_mode = MODE_AUDIO;
      break;
    case MODE_VIDEO_BRIGHTNESS:
    case MODE_VIDEO_FACEDETECT:
      av_mode = MODE_VIDEO;
      break;
  }

  puts (modes[mode].name);

  // setup input device
  ufile = open ("/dev/uinput", O_WRONLY | O_NDELAY);
  if (ufile == -1) {
    printf ("Could not open uinput: %s\n", strerror (errno));
    return -1;
  }

  memset (&uinp, 0, sizeof (uinp));
  strncpy (uinp.name, "gstreamer input", 20);
  uinp.id.version = 4;
  uinp.id.bustype = BUS_USB;
  for (i = 0; i < modes[mode].axes; i++) {
    uinp.absmin[i] = 0;
    uinp.absmax[i] = G_MAXINT32;
  }

  ioctl (ufile, UI_SET_EVBIT, EV_ABS);
  ioctl (ufile, UI_SET_ABSBIT, ABS_X);
  if (modes[mode].axes > 1)
    ioctl (ufile, UI_SET_ABSBIT, ABS_Y);
  if (modes[mode].axes > 2)
    ioctl (ufile, UI_SET_ABSBIT, ABS_Z);

  // create input device in input subsystem
  if (write (ufile, &uinp, sizeof (uinp)) == -1) {
    printf ("Error writing to uinput: %s\n", strerror (errno));
    return -1;
  }

  if (ioctl (ufile, UI_DEV_CREATE) == -1) {
    printf ("Error create uinput device: %s\n", strerror (errno));
    return -1;
  }
  // setup gstreamer pipleine
  bin = gst_pipeline_new ("analyzer");
  bus = gst_pipeline_get_bus (GST_PIPELINE (bin));
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
  g_signal_connect (bus, "message::error", G_CALLBACK (message_received), bin);
  g_signal_connect (bus, "message::warning", G_CALLBACK (message_received),
      bin);
  g_signal_connect (bus, "message::element", G_CALLBACK (analyzer_result), bin);
  gst_object_unref (G_OBJECT (bus));

  switch (av_mode) {
    case MODE_AUDIO:
      elems[ne++] = gst_element_factory_make ("autoaudiosrc", NULL);
      break;
    case MODE_VIDEO:
      elems[ne++] = gst_element_factory_make ("autovideosrc", NULL);
      elems[ne++] = e = gst_element_factory_make ("capsfilter", NULL);
      caps = gst_caps_from_string ("video/x-raw-yuv,width=320,height=240");
      g_object_set (e, "caps", caps, NULL);
      gst_caps_unref (caps);
      elems[ne++] = gst_element_factory_make ("colorspace", NULL);
      break;
  }
  switch (mode) {
    case MODE_AUDIO_LEVEL:
      elems[ne++] = gst_element_factory_make ("level", NULL);
      break;
    case MODE_VIDEO_BRIGHTNESS:
      elems[ne++] = gst_element_factory_make ("videoanalyse", NULL);
      break;
    case MODE_VIDEO_FACEDETECT:
      // need some blur/denoise
      elems[ne++] = e = gst_element_factory_make ("facedetect", NULL);
      g_object_set (e,
          "flags", 1,
          "display", FALSE,
          "scale-factor", (gdouble) 1.3,
          "min-neighbors", 5,
          "min-size-width", 60, "min-size-height", 60, NULL);
      break;
  }
  elems[ne++] = gst_element_factory_make ("fakesink", NULL);

  gst_bin_add (GST_BIN (bin), elems[0]);
  for (i = 1; i < ne; i++) {
    gst_bin_add (GST_BIN (bin), elems[i]);
    if (!gst_element_link (elems[i - 1], elems[i])) {
      GST_WARNING ("Can't link elements");
      return -1;
    }
  }

  // run mainloop
  main_loop = g_main_loop_new (NULL, FALSE);
  gst_element_set_state (bin, GST_STATE_PLAYING);
  g_main_loop_run (main_loop);

  // shutdown pipeline
  gst_element_set_state (bin, GST_STATE_NULL);
  gst_object_unref (bin);

  // destroy the device
  ioctl (ufile, UI_DEV_DESTROY);
  close (ufile);

  return 0;
}
