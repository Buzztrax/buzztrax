/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
/**
 * SECTION:btedit
 * @short_description: buzztrax graphical editor application
 * @see_also: #BtEditApplication
 *
 * Implements the body of the buzztrax GUI editor.
 *
 * You can try to run the uninstalled program via
 * <informalexample><programlisting>
 *   libtool --mode=execute buzztrax-edit
 * </programlisting></informalexample>
 * to enable debug output add:
 * <informalexample><programlisting>
 *  --gst-debug="*:2,bt-*:3" for not-so-much-logdata or
 *  --gst-debug="*:2,bt-*:4" for a-lot-logdata
 * </programlisting></informalexample>
 *
 * Example songs can be found in <filename>./test/songs/</filename>.
 */

#define BT_EDIT
#define BT_EDIT_C

#include "bt-edit.h"
#include <glib/gprintf.h>

#ifdef ENABLE_NLS
#ifdef HAVE_X11_XLOCALE_H
  /* defines a more portable setlocale for X11 (_Xsetlocale) */
#include <X11/Xlocale.h>
#else
#include <locale.h>
#endif
#endif

gint
main (gint argc, gchar ** argv)
{
  bt_setup_for_local_install ();

#ifdef ENABLE_NLS
  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif /* ENABLE_NLS */

  bt_setup_for_local_install ();

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "bt-edit", 0,
      "music production environment / editor ui");

  g_set_application_name ("Buzztrax");
  g_set_prgname ("buzztrax-edit");
  gtk_window_set_default_icon_name ("buzztrax");
  
  // give some global context info
  g_setenv ("PULSE_PROP_media.role", "production", TRUE);

  GST_INFO ("starting: thread=%p", g_thread_self ());
  
  gst_init (&argc, &argv);
  
  extern gboolean bt_memory_audio_src_plugin_init (GstPlugin * const plugin);
  gst_plugin_register_static (GST_VERSION_MAJOR,
      GST_VERSION_MINOR,
      "memoryaudiosrc",
      "Plays audio from memory",
      bt_memory_audio_src_plugin_init,
      VERSION, "LGPL", PACKAGE, PACKAGE_NAME, "http://www.buzztrax.org");

  BtAdwAppEdit *app = bt_adw_app_edit_new ();
  int result = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);
  
  return result;
}
