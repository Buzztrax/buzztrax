/* $Id: pattern-properties-dialog-methods.h,v 1.2 2005-04-25 14:50:27 ensonic Exp $
 * defines all public methods of the pattern properties dialog class
 */

#ifndef BT_PATTERN_PROPERTIES_DIALOG_METHODS_H
#define BT_PATTERN_PROPERTIES_DIALOG_METHODS_H

#include "pattern-properties-dialog.h"
#include "edit-application.h"

extern BtPatternPropertiesDialog *bt_pattern_properties_dialog_new(const BtEditApplication *app,const BtPattern *pattern);
extern void bt_pattern_properties_dialog_apply(const BtPatternPropertiesDialog *self);

#endif // BT_PATTERN_PROPERTIES_DIALOG_METHDOS_H
