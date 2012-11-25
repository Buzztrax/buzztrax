/* test moditying keyboard leds
 *
 * building:
 * gcc -Wall -g setled.c -o setled `pkg-config glib-2.0 --cflags --libs`
 *
 * damn, access to the devices needs root privileges to run
 */
/* based on:
 * http://sampo.kapsi.fi/ledcontrol/
 * http://www.netzmafia.de/skripten/hardware/keyboard/tastatur-leds.html
 *
 * http://www.net-security.org/article.php?id=83
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/kd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <glib.h>

#define TTY_NAME "/dev/console"
//#define TTY_NAME "/dev/tty7"

/* Graphics tty (X) */
#define TYPE_GRAPHICS 0
/* Normal text tty */
#define TYPE_TEXT 1

static gint fd = -1;
static gint type = TYPE_GRAPHICS;

/* open the tty */
static gboolean
led_init ()
{
  int i;

  fd = open (TTY_NAME, O_RDONLY);
  if (fd == -1) {
    fprintf (stderr, "cannot open tty %s (%s)\n", TTY_NAME, g_strerror (errno));
    return FALSE;
  }

  /* Get text/graphics mode */
  if (ioctl (fd, KDGETMODE, &i)) {
    fprintf (stderr, "KDGETMODE on tty %s failed (%s)\n", TTY_NAME,
        g_strerror (errno));
    close (fd);
    fd = -1;
    return FALSE;
  }
  if (i == KD_GRAPHICS) {
    printf ("graphic mode\n");
    type = TYPE_GRAPHICS;
  } else {
    printf ("text mode\n");
    type = TYPE_TEXT;
  }

  return TRUE;
}

static gboolean
led_done ()
{
  if (fd >= 0) {
    close (fd);
    fd = -1;
  }
  return TRUE;
}

/*
 * AND's and OR's the flags into the current LED settings on tty *name.
 * Now optimized for the bare minimum of ioctl's.
 * Returns flags actually set or -1 on error.
 */
static gint
led_set (gint and, gint or)
{
  char current;
  char ref;
  char new;

  /* Get the tty status */
  if (type == TYPE_GRAPHICS) {
    /* In X we assume LEDs to be in "correct" state */
    if (ioctl (fd, KDGETLED, &current)) {
      fprintf (stderr, "KDGETLED on tty %s failed (%s)\n", TTY_NAME,
          g_strerror (errno));
      return -1;
    }
    ref = current;
  } else {
    if (ioctl (fd, KDGETLED, &current)) {
      fprintf (stderr, "KDGETLED on tty %s failed (%s)\n", TTY_NAME,
          g_strerror (errno));
      return -1;
    }
    if (ioctl (fd, KDGKBLED, &ref)) {
      fprintf (stderr, "KDGKBLED on tty %s failed (%s)", TTY_NAME,
          g_strerror (errno));
      return -1;
    }
  }

  new = (ref & and) | or;

  if (new != current) {
    if (ioctl (fd, KDSETLED, new)) {
      fprintf (stderr, "KDSETLED on tty %s failed (%s)\n", TTY_NAME,
          g_strerror (errno));
      return -1;
    }
  }

  return new;
}

gint
main (gint argc, gchar ** argv)
{
  gint i;
  gint saved;

  if (!led_init ())
    return (-1);

  saved = led_set (7, 0);
  // cycle through all combinations: LED_SCR,LED_CAP,LED_NUM
  for (i = 0; i < 8; i++) {
    printf ("SCR %d CAP %d NUM %d\n", ((i & LED_SCR) == LED_SCR),
        ((i & LED_CAP) == LED_CAP), ((i & LED_NUM) == LED_NUM));
    led_set (0, i);
    usleep (250000);
  }
  led_set (0, saved);

  led_done ();
  return 0;
}
