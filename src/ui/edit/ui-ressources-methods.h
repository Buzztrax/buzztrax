/* $Id: ui-ressources-methods.h,v 1.1 2005-02-22 07:31:10 ensonic Exp $
 * defines all public methods of the main pages class
 */

#ifndef BT_UI_RESSOURCES_METHODS_H
#define BT_UI_RESSOURCES_METHODS_H

#include "ui-ressources.h"

extern BtUIRessources *bt_ui_ressources_new(void);

extern GdkPixbuf *bt_ui_ressources_get_pixbuf_by_machine(const BtMachine *machine);
extern GtkWidget *bt_ui_ressources_get_image_by_machine(const BtMachine *machine);
extern GtkWidget *bt_ui_ressources_get_image_by_machine_type(GType machine_type);

#endif // BT_UI_RESSOURCES_METHDOS_H
