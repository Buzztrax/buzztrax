/* test timing quantization
 *
 * gcc -Wall -g `pkg-config gstreamer-0.10 --cflags --libs` timing.c -o timing
 *
 * ./timing <sampling-rate> <bpm> <tpb>
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <gst/gst.h>
#include <gst/audio/audio.h>

static void timing(const guint rate,const gdouble ticks_per_minute) {
  guint i;
  GstClockTime ts,qtime;
  gdouble atime=0.0;
  GstClockTimeDiff qdiff,adiff,aqdiff;
  guint64 samples,asamples=G_GUINT64_CONSTANT(0);
  gdouble samples_done,adjust;
  guint samples_per_buffer;
  //gdouble wait_per_position       =(   GST_SECOND*60.0)/ticks_per_minute;
  GstClockTime wait_per_position  =(GstClockTime)(0.5+((GST_SECOND*60.0)/ticks_per_minute));
  gdouble ideal_samples_per_buffer=                 ((gdouble)rate*60.0)/ticks_per_minute;
  
  printf("wait per position=%lf, time per sample=%lf, sample per buffer=%lf\n",
    (gdouble)wait_per_position,
    ((gdouble)GST_SECOND/(gdouble)rate),
    ideal_samples_per_buffer);

  printf("tick %15s %15s %15s : %15s %15s : %15s\n","time","q-time","q-diff","a-time","a-diff","aq-diff");
  for(i=0;i<15;i++) {
    ts = (GstClockTime)(wait_per_position*i);
    // this happens in buzztard
    // we quantize to sample resolution
    samples = gst_util_uint64_scale(ts,(guint64)rate,GST_SECOND);
    qtime = gst_util_uint64_scale(samples,GST_SECOND,(guint64)rate);
    qdiff = (gint64)ts-(gint64)qtime;
    
    // this happens in the machine
    // this would lead to adjust beeing always 0.0
    //atime = gst_util_uint64_scale(accum,GST_SECOND,(guint64)rate);
    // this how many samples we should have done according to accumalated time
    samples_done = atime*(gdouble)rate/(gdouble)GST_SECOND;
    adjust = samples_done-(gdouble)asamples;
    samples_per_buffer=(guint)(ideal_samples_per_buffer+adjust);
    adiff = (gint64)ts-(gint64)atime;
    
    aqdiff = atime-qtime;
    
    printf("%2u : %15"G_GUINT64_FORMAT" %15"G_GUINT64_FORMAT" %15"G_GINT64_FORMAT" : %15"G_GINT64_FORMAT" %15"G_GINT64_FORMAT" : %15"G_GINT64_FORMAT" %lf=%11.5lf-%11.5lf\n",
      i,ts,qtime,qdiff,(guint64)atime,adiff,aqdiff,
      adjust,(gdouble)asamples,samples_done);

    asamples+=(guint64)samples_per_buffer;
    atime   +=wait_per_position;
  }
}

int main(int argc, char **argv) {
  /* use commandline args for these */
  gulong beats_per_minute=130,ticks_per_beat=4;
  guint rate=GST_AUDIO_DEF_RATE;

  /* init gstreamer */
  gst_init(&argc, &argv);
  g_log_set_always_fatal(G_LOG_LEVEL_WARNING);
  
  if(argc>1) rate=atoi(argv[1]);
  if(argc>2) beats_per_minute=atoi(argv[2]);
  if(argc>3) ticks_per_beat=atoi(argv[3]);

  const gdouble ticks_per_minute=(gdouble)(beats_per_minute*ticks_per_beat);
  timing(rate,ticks_per_minute);

  exit (0);
}
