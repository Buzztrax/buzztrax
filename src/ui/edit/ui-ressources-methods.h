/* $Id: ui-ressources-methods.h,v 1.2 2006-02-17 08:06:50 ensonic Exp $
 * defines all public methods of the main pages class
 */

#ifndef BT_UI_RESSOURCES_METHODS_H
#define BT_UI_RESSOURCES_METHODS_H

#include "ui-ressources.h"

extern BtUIRessources *bt_ui_ressources_new(void);

extern GdkPixbuf *bt_ui_ressources_get_pixbuf_by_machine(const BtMachine *machine);
extern GtkWidget *bt_ui_ressources_get_image_by_machine(const BtMachine *machine);
extern GtkWidget *bt_ui_ressources_get_image_by_machine_type(GType machine_type);

extern GdkColor *bt_ui_ressources_get_gdk_color(BtUIRessourcesColors color_type);
extern guint32 bt_ui_ressources_get_color_by_machine(const BtMachine *machine,BtUIRessourcesMachineColors color_type);

#endif // BT_UI_RESSOURCES_METHDOS_H
