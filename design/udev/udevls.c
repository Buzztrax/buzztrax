/* list udev devices for a certain category
 *
 * building:
 * gcc -Wall -g udevls.c -o udevls `pkg-config glib-2.0 gudev-1.0 --cflags --libs`
 *
 * running:
 * ./udevls
 * ./udevls input
 *
 * G_SLICE=always-malloc G_DEBUG=gc-friendly GLIBCPP_FORCE_NEW=1 GLIBCXX_FORCE_NEW=1 \
 * valgrind --tool=memcheck --leak-check=full --leak-resolution=high --trace-children=yes --num-callers=20 --log-file=./valgrind.log ./udevls
 *
 * does not happen with udev 166-5.1 anymore
 */

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <gudev/gudev.h>

gint
main (gint argc, gchar * argv[])
{
  GUdevClient *client;
  GUdevDevice *device;
  GList *list, *node;
  gchar *subsystem_name;
  const gchar *udi;
  const gchar *devnode1;
  const gchar *devnode2;

  if (argc < 2)
    subsystem_name = "input";
  else
    subsystem_name = argv[1];

  g_type_init ();

  client = g_udev_client_new (NULL);

  list = g_udev_client_query_by_subsystem (client, subsystem_name);
  if (list) {
    for (node = list; node; node = g_list_next (node)) {
      device = (GUdevDevice *) node->data;

      udi = g_udev_device_get_sysfs_path (device);
      devnode1 = g_udev_device_get_device_file (device);

      /*
         printf("subsys=%8s, devtype=%15s, name=%10s, number=%2s, devnode=%s, driver=%s\n",
         g_udev_device_get_subsystem(device), g_udev_device_get_devtype(device),
         g_udev_device_get_name(device), g_udev_device_get_number(device),
         devnode1, g_udev_device_get_driver(device));
       */

      if (devnode1) {
        devnode2 = g_udev_device_get_device_file (device);
        printf ("%c | %p,%s | %p,%s\n", ((devnode1 != devnode2) ? '!' : ' '),
            devnode1, devnode1, devnode2, devnode2);
      }
      g_object_unref (device);
    }
    g_list_free (list);
  }
  g_object_unref (client);
  return 0;
}
