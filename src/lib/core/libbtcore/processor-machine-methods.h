/* $Id: processor-machine-methods.h,v 1.1 2004-07-30 15:15:51 ensonic Exp $
 * defines all public methods of the processor machine class
 */

#ifndef BT_PROCESSOR_MACHINE_METHODS_H
#define BT_PROCESSOR_MACHINE_METHODS_H

#include "processor-machine.h"

extern BtProcessorMachine *bt_processor_machine_new(const BtSong *song, const gchar *id, const gchar *plugin_name, glong voices);

#endif // BT_PROCESSOR_MACHINE_METHDOS_H
