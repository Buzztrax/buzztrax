/*
 * http://www.frogmouth.net/hid-doco/c537.html
 * gcc input.c -o input
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/input.h>

typedef unsigned char guint8;

int main(int argc, char **argv) {
  int fd;

  if (argc != 2) {
    fprintf(stderr, "usage: %s event-device - probably /dev/input/event1\n", argv[0]);
    exit(1);
  }
  if ((fd = open(argv[1], O_RDONLY)) < 0) {
    perror("evdev open");
    exit(1);
  }


  // device type
  char name[256]= "Unknown";

  if(ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0) {
      perror("evdev ioctl");
  }
  printf("name is %s\n\n", name);


  // device features
  #define test_bit(bit, array)    (array[bit/8] & (1<<(bit%8)))

  guint8 evtype_bitmask[EV_MAX/8 + 1];
  int yalv;

  memset(evtype_bitmask, 0, sizeof(evtype_bitmask));
  if(ioctl(fd, EVIOCGBIT(0, sizeof(evtype_bitmask)), evtype_bitmask) < 0) {
    perror("evdev ioctl");
  }

  printf("Supported event types:\n");
  for (yalv = 0; yalv < EV_MAX; yalv++) {
    if (test_bit(yalv, evtype_bitmask)) {
      /* this means that the bit is set in the event types list */
      printf("  Event type 0x%02x ", yalv);
      switch (yalv) {
        case EV_KEY :
          printf(" (Keys or Buttons)\n");
          break;
        case EV_ABS :
          printf(" (Absolute Axes)\n");
          break;
        case EV_LED :
          printf(" (LEDs)\n");
          break;
        case EV_REP :
          printf(" (Repeat)\n");
          break;
        default:
          printf(" (Unknown event type: 0x%04hx)\n", yalv);
       }
    }
  }
  puts("");


  // events
  printf("Events:\n");
  struct input_event ev;

  while(1) {
    read(fd, &ev, sizeof(struct input_event));
    printf("value %d, type 0x%x, code 0x%x\n",ev.value,ev.type,ev.code);
  }

  return 0;
}
