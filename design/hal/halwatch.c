/** $Id$
 * test hal device monitoring
 *
 * gcc -Wall -g `pkg-config glib-2.0 dbus-glib-1 hal --cflags --libs` halwatch.c -o halwatch
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <hal/libhal.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

static void device_added(LibHalContext *ctx, const gchar *udi) {
  gchar **cap;
  gchar *parent_udi;
  size_t n;
  
  if(!(cap=libhal_device_get_property_strlist(ctx,udi,"info.capabilities",NULL))) {
    return;
  }

  for(n=0;cap[n];n++) {
    if(!strcmp(cap[n],"alsa")) {
      parent_udi=libhal_device_get_property_string(ctx,udi,"info.parent",NULL);
      parent_udi=libhal_device_get_property_string(ctx,parent_udi,"info.parent",NULL);

      printf("alsa device added: type=%s, device_file=%s, vendor=%s\n",
        libhal_device_get_property_string(ctx,udi,"alsa.type",NULL),
        libhal_device_get_property_string(ctx,udi,"alsa.device_file",NULL),
        libhal_device_get_property_string(ctx,parent_udi,"info.vendor",NULL)
      );
    }
    else if(!strcmp(cap[n],"input")) {
      printf("input device added: producs=%s\n",
        libhal_device_get_property_string(ctx,udi,"input.product",NULL)
      );
    }
    else {
      printf("  non alsa device added: udi=%s\n",udi); 
    }
  }
}

int main(int argc, char **argv) {
  int ret=0;
  LibHalContext *ctx;
  DBusError dbus_error;
  DBusConnection *dbus_conn;
  GMainLoop *loop;
  
  /* init dbus */
  dbus_error_init(&dbus_error);
  dbus_conn=dbus_bus_get(DBUS_BUS_SYSTEM,&dbus_error);
  if(dbus_error_is_set(&dbus_error)) {
    g_warning("Could not connect to system bus %s\n", dbus_error.message);
    ret=1;
    goto dbus_init_error;
  }
  dbus_connection_setup_with_g_main(dbus_conn,NULL);
  dbus_connection_set_exit_on_disconnect(dbus_conn,FALSE);
  
  /* init hal */
  if(!(ctx=libhal_ctx_new())) {
    g_warning("Could not create hal context\n");
    ret=1;
    goto hal_new_error;
  }
  libhal_ctx_set_dbus_connection(ctx,dbus_conn);
  libhal_ctx_set_device_added(ctx,device_added);
  if(!(libhal_ctx_init(ctx,&dbus_error))) {
    g_warning("Could not init hal %s\n", dbus_error.message);
    ret=1;
    goto hal_init_error;
  }
  
  /* start main loop */
  loop=g_main_loop_new(NULL,FALSE);
  g_main_loop_run(loop);
  
  /* shut down */
  g_main_loop_unref(loop);
hal_init_error:
  libhal_ctx_free(ctx);
hal_new_error:

dbus_init_error:
  dbus_error_free(&dbus_error);
  return(ret);
}
