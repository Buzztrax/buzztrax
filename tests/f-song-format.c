/*
 * buzztrax song format fuzzing test
 *
 * build:
 * export LD_LIBRARY_PATH=$HOME/buzztrax/lib PKG_CONFIG_PATH=$HOME/buzztrax/lib/pkgconfig
 * clang -g -O1 -fsanitize=fuzzer f-song-format.c -I. -I../src/lib .libs/bt-test-application.o `pkg-config libbuzztrax-core --cflags --libs` -o f-song-format
 *
 * run:
 * ./f-song-format songs
 *  GST_DEBUG="bt-core:5" ./f-song-format songs
 */

#include "bt-check.h"

#include <libbuzztrax-core/core.h>
#include <stdint.h>

int
LLVMFuzzerTestOneInput (const uint8_t * data, size_t len)
{
  bt_setup_for_local_install ();
  bt_init (NULL, NULL);

  BtApplication *app = bt_test_application_new ();
  BtSong *song = bt_song_new (app);
  BtSongIO *song_io =
      bt_song_io_from_data ((gpointer) data, len, "audio/x-bzt-xml", NULL);
  if (song_io) {
    bt_song_io_load (song_io, song, NULL);
  }

  g_object_unref (song_io);
  g_object_unref (song);
  g_object_unref (app);

  return 0;                     // Non-zero return values are reserved for future use.
}
