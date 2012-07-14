// http://www.mail-archive.com/linux-input@vger.kernel.org/msg00063.html
// gcc uinput.c -o uinput

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/time.h>
#include <unistd.h>

// Test the uinput module

int
main (void)
{
  struct uinput_user_dev uinp;
  struct input_event event;
  int ufile, retcode, i;

  ufile = open ("/dev/uinput", O_WRONLY | O_NDELAY);
  if (ufile == -1) {
    printf ("Could not open uinput: %s\n", strerror (errno));
    return -1;
  }

  memset (&uinp, 0, sizeof (uinp));
  strncpy (uinp.name, "simulated mouse", 20);
  uinp.id.version = 4;
  uinp.id.bustype = BUS_USB;

  ioctl (ufile, UI_SET_EVBIT, EV_KEY);
  ioctl (ufile, UI_SET_EVBIT, EV_REL);
  ioctl (ufile, UI_SET_RELBIT, REL_X);
  ioctl (ufile, UI_SET_RELBIT, REL_Y);

  for (i = 0; i < 256; i++) {
    ioctl (ufile, UI_SET_KEYBIT, i);
  }

  ioctl (ufile, UI_SET_KEYBIT, BTN_MOUSE);

  // create input device in input subsystem
  retcode = write (ufile, &uinp, sizeof (uinp));
  printf ("First write returned %d.\n", retcode);

  retcode = ioctl (ufile, UI_DEV_CREATE);
  if (retcode == -1) {
    printf ("Error create uinput device: %s\n", strerror (errno));
    return -1;
  }

  printf ("test prepared\n");
  sleep (10);

  for (i = 0; i < 100; i++) {
    // move pointer upleft by 2 pixels
    memset (&event, 0, sizeof (event));
    gettimeofday (&event.time, NULL);
    event.type = EV_REL;
    event.code = REL_X;
    event.value = -2;
    write (ufile, &event, sizeof (event));

    memset (&event, 0, sizeof (event));
    gettimeofday (&event.time, NULL);
    event.type = EV_REL;
    event.code = REL_Y;
    event.value = -2;
    write (ufile, &event, sizeof (event));

    memset (&event, 0, sizeof (event));
    gettimeofday (&event.time, NULL);
    event.type = EV_SYN;
    event.code = SYN_REPORT;
    event.value = 0;
    write (ufile, &event, sizeof (event));

    // wait just a moment
    usleep (10000);
  }

  printf ("test done\n");

  // destroy the device
  ioctl (ufile, UI_DEV_DESTROY);

  close (ufile);
}
