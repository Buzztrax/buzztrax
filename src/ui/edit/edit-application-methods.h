/* $Id: edit-application-methods.h,v 1.5 2006-08-13 20:24:33 ensonic Exp $
 * defines all public methods of the edit application class
 */

#ifndef BT_EDIT_APPLICATION_METHODS_H
#define BT_EDIT_APPLICATION_METHODS_H

#include "edit-application.h"

extern BtEditApplication *bt_edit_application_new(void);

extern gboolean bt_edit_application_new_song(const BtEditApplication *self);
extern gboolean bt_edit_application_load_song(const BtEditApplication *self,const char *file_name);
extern gboolean bt_edit_application_save_song(const BtEditApplication *self,const char *file_name);

extern gboolean bt_edit_application_run(const BtEditApplication *self);
extern gboolean bt_edit_application_load_and_run(const BtEditApplication *self, const gchar *input_file_name);

extern void bt_edit_application_show_about(const BtEditApplication *self) ;

#endif // BT_EDIT_APPLICATION_METHDOS_H
