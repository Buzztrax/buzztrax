/* $Id: e-bt-settings-dialog.c,v 1.9 2005-09-14 10:16:35 ensonic Exp $
 */

#include "m-bt-edit.h"

//-- globals

//-- fixtures

static void test_setup(void) {
  bt_edit_setup();
}

static void test_teardown(void) {
  bt_edit_teardown();
}

//-- tests

// create app and then unconditionally destroy window
BT_START_TEST(test_create_dialog) {
  BtEditApplication *app;
  BtMainWindow *main_window;
  GtkWidget *dialog;

  app=bt_edit_application_new();
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT(app)->ref_count);
  fail_unless(app != NULL, NULL);

  // get window
  g_object_get(app,"main-window",&main_window,NULL);
  fail_unless(main_window != NULL, NULL);

  // create, show and destroy dialog
  dialog=GTK_WIDGET(bt_settings_dialog_new(app));
  gtk_widget_show_all(dialog);
  // leave out that line! (modal dialog)
  //gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
  
  // close window
  g_object_unref(main_window);
  gtk_widget_destroy(GTK_WIDGET(main_window));
  while(gtk_events_pending()) gtk_main_iteration();
  
  // free application
  GST_INFO("app->ref_ct=%d",G_OBJECT(app)->ref_count);
  g_object_checked_unref(app);  
}
BT_END_TEST

TCase *bt_settings_dialog_example_case(void) {
  TCase *tc = tcase_create("BtSettingsDialogExamples");
  
  tcase_add_test(tc,test_create_dialog);
  // we *must* use a checked fixture, as only this runns in the same context
  tcase_add_checked_fixture(tc, test_setup, test_teardown);
  // we need to disable the default timeout of 3 seconds a little
  tcase_set_timeout(tc, 0);
  return(tc);
}
