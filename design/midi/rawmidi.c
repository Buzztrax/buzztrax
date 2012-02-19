/* test raw-midi io
 *
 * gcc -Wall -g `pkg-config gstreamer-0.10 --cflags --libs` rawmidi.c -o rawmidi
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gst/gst.h>

#define MIDI_CMD_MASK            0xf0
#define MIDI_CH_MASK             0x0f
#define MIDI_NOTE_OFF            0x80
#define MIDI_NOTE_ON             0x90
#define MIDI_POLY_AFTER_TOUCH    0xa0
#define MIDI_CONTROL_CHANGE      0xb0
#define MIDI_CHANNEL_AFTER_TOUCH 0xd0
#define MIDI_PITCH_WHEEL_CHANGE  0xe0
#define MIDI_SYS_EX_START        0xf0
#define MIDI_SYS_EX_END          0xf7
#define MIDI_TIMING_CLOCK        0xf8
#define MIDI_TRANSPORT_START     0xfa
#define MIDI_TRANSPORT_CONTINUE  0xfb
#define MIDI_TRANSPORT_STOP      0xfc
#define MIDI_ACTIVE_SENSING      0xfe
#define MIDI_NON_REALTIME        0x7e

gint
main(gint argc, gchar **argv) {
  gchar *devnode="/dev/snd/midiC1D0";
  gint io;
  
  gst_init(&argc, &argv);
  
  if(argc>1) {
    devnode=argv[1];
  }

  // we never receive anything back :/
  if((io=open(devnode,O_NONBLOCK|O_RDWR|O_SYNC))>0) {
    gchar data[20]={0,},*dp;
    gsize ct,h;

    data[0]=MIDI_SYS_EX_START;
    data[1]=MIDI_NON_REALTIME;
    data[2]=0x7f; // SysEx channel, set to "disregard"
    data[3]=0x06; // General Information
    data[4]=0x01; // Identity Request
    data[5]=MIDI_SYS_EX_END;

    printf("sending identity request to: %s\n",devnode);
    ct=write(io,data,6);
    printf("%"G_GSSIZE_FORMAT" bytes sent\n",ct);
    if(ct<6) {
      goto done;
    }
    memset(data, 0, sizeof(data));
    printf("receiving identity replay from: %s\n",devnode);
    dp=data;h=0;
    do {
      ct=read(io,dp,15-h);
      if (ct != -1) {
        printf("%"G_GSSIZE_FORMAT" bytes read\n",ct);
        dp=&dp[ct];
        h+=ct;
        printf("have %"G_GSSIZE_FORMAT" bytes\n",h);
      }
    } while (h<15);
    GST_MEMDUMP("reply",(guint8 *)data,15);
    printf("sysex start     : 0x%02hhx==0x%02hhx\n",data[0],MIDI_SYS_EX_START);
    printf("non realtime    : 0x%02hhx==0x%02hhx\n",data[1],MIDI_NON_REALTIME);
    printf("channel         : 0x%02hhx==0x7f\n",data[2]);
    printf("sub id          : 0x%02hhx==0x06 0x%02hhx==0x02\n",data[3],data[4]);
    printf("manufacturer id : 0x%02hhx\n",data[5]);
    printf("family code     : 0x%02hhx 0x%02hhx\n",data[6],data[7]);
    printf("model code      : 0x%02hhx 0x%02hhx\n",data[8],data[9]);
    printf("version number  : 0x%02hhx 0x%02hhx 0x%02hhx 0x%02hhx\n",data[10],data[11],data[12],data[13]);
    printf("sysex end       : 0x%02hhx==0x%02hhx\n",data[14],MIDI_SYS_EX_END);
    // 5:   manufacturer id (if 0, then id is next two bytes)
    //      e.g. 0x42 for Korg
    // 6,7: family code
    // 8,9: model code
    // 10,11,12,13: version number
done:
    close(io);
  } else {
    fprintf(stderr,"failed to open device: %s\n",devnode);
  }
  return 0;
}
