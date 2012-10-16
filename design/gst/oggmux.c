/* Record n seconds of beep to an ogg file with intermediate seeks
 *
 * gcc -Wall -g oggmux.c -o oggmux `pkg-config gstreamer-0.10 --cflags --libs`
 *
 * ./oggmux 0; oggz-validate oggmux.ogg
 * ./oggmux 1; oggz-validate oggmux.ogg
 */

#include <gst/gst.h>

#include <stdio.h>
#include <stdlib.h>

gint
main (gint argc, gchar ** argv)
{
  GstElement *bin;
  GstSeekFlags flags = GST_SEEK_FLAG_NONE;
  gint i;

  gst_init (&argc, &argv);

  if (argc > 1) {
    if (argv[1][0] == '1')
      flags = GST_SEEK_FLAG_FLUSH;
  }

  bin =
      gst_parse_launch
      ("audiotestsrc ! vorbisenc ! oggmux ! filesink location=oggmux.ogg",
      NULL);

  gst_element_set_state (GST_ELEMENT (bin), GST_STATE_PAUSED);
  if (!gst_element_seek (bin, 1.0, GST_FORMAT_TIME, flags,
          GST_SEEK_TYPE_SET, GST_SECOND * 0,
          GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
    puts ("seek failed");
  else
    puts ("seek accepted");
  gst_element_set_state (GST_ELEMENT (bin), GST_STATE_PLAYING);
  for (i = 0; i < 3; i++) {
    // play a bit
    g_usleep (G_USEC_PER_SEC / 10);
    // seek back to zero
    if (!gst_element_seek (bin, 1.0, GST_FORMAT_TIME, flags,
            GST_SEEK_TYPE_SET, GST_SECOND * 0,
            GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
      puts ("seek failed");
    else
      puts ("seek accepted");
  }
  // play a bit more
  g_usleep (G_USEC_PER_SEC / 10);
  gst_element_send_event (GST_ELEMENT (bin), gst_event_new_eos ());
  g_usleep (G_USEC_PER_SEC / 10);

  gst_element_set_state (bin, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (bin));
  return 0;
}
