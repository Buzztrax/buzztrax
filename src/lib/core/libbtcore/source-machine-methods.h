/* $Id: source-machine-methods.h,v 1.1 2004-07-30 15:15:51 ensonic Exp $
 * defines all public methods of the source machine class
 */

#ifndef BT_SOURCE_MACHINE_METHODS_H
#define BT_SOURCE_MACHINE_METHODS_H

#include "source-machine.h"

extern BtSourceMachine *bt_source_machine_new(const BtSong *song, const gchar *id, const gchar *plugin_name, glong voices);

#endif // BT_SOURCE_MACHINE_METHDOS_H
