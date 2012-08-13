/* test accelerators for toolbar items
 *
 * gcc -Wall -g `pkg-config gtk+-2.0 --cflags --libs` acceltoolbar.c -o acceltoolbar
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>

static GtkWidget *window = NULL;
static GtkWidget *play_button = NULL;


static void
destroy (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
}

/*
#0  on_play_clicked (button=0x8099800, user_data=0x808f850) at acceltoolbar.c:25
#1  0xb79f9109 in g_cclosure_marshal_VOID__VOID (closure=0x80993f8, return_value=0x0, n_param_values=1, param_values=0xbfbc630c, invocation_hint=0xbfbc621c,  marshal_data=0x8048da1) at gmarshal.c:77
#2  0xb79ebd3b in g_closure_invoke (closure=0x80993f8, return_value=0x0, n_param_values=1, param_values=0xbfbc630c, invocation_hint=0xbfbc621c) at gclosure.c:490
#3  0xb79fc193 in signal_emit_unlocked_R (node=0x8098f40, detail=0, instance=0x8099800, emission_return=0x0, instance_and_params=0xbfbc630c) at gsignal.c:2440
#4  0xb79fd6b7 in g_signal_emit_valist (instance=0x8099800, signal_id=105, detail=0, var_args=0xbfbc654c "�������������e��;�\236�\030\207\t\b\220f��\004") at gsignal.c:2199
#5  0xb79fd885 in g_signal_emit (instance=0x8099800, signal_id=105, detail=0) at gsignal.c:2243
#6  0xb7e5aa8f in closure_accel_activate (closure=0x8098718, return_value=0xbfbc6690, n_param_values=4, param_values=0xbfbc676c, invocation_hint=0xbfbc667c, marshal_data=0x0) at gtkwidget.c:4088
#7  0xb79ebd3b in g_closure_invoke (closure=0x8098718, return_value=0xbfbc6690, n_param_values=4, param_values=0xbfbc676c, invocation_hint=0xbfbc667c) at gclosure.c:490
#8  0xb79fc193 in signal_emit_unlocked_R (node=0x8084fd0, detail=551, instance=0x8082820, emission_return=0xbfbc692c, instance_and_params=0xbfbc676c) at gsignal.c:2440
#9  0xb79fd47f in g_signal_emit_valist (instance=0x8082820, signal_id=86, detail=551, var_args=0xbfbc69b8 "�i��/\224��H�\a\b\217\001") at gsignal.c:2209
#10 0xb79fd885 in g_signal_emit (instance=0x8082820, signal_id=86, detail=551) at gsignal.c:2243
#11 0xb7c09619 in IA__gtk_accel_group_activate (accel_group=0x8082820, accel_quark=551, acceleratable=0x807b048, accel_key=65474, accel_mods=0) at gtkaccelgroup.c:739
#12 0xb7c0972c in IA__gtk_accel_groups_activate (object=0x807b048, accel_key=65474, accel_mods=0) at gtkaccelgroup.c:777
#13 0xb7e6bdff in IA__gtk_window_activate_key (window=0x807b048, event=0x8069508) at gtkwindow.c:7981
#14 0xb7e6d0cf in gtk_window_key_press_event (widget=0x807b048, event=0x8069508) at gtkwindow.c:4958
#15 0xb7d1aea2 in _gtk_marshal_BOOLEAN__BOXED (closure=0x8073200, return_value=0xbfbc6c00, n_param_values=2, param_values=0xbfbc6cdc, invocation_hint=0xbfbc6bec, marshal_data=0xb7e6d090) at gtkmarshalers.c:84
#16 0xb79ea537 in g_type_class_meta_marshal (closure=0x8073200, return_value=0xbfbc6c00, n_param_values=2, param_values=0xbfbc6cdc, invocation_hint=0xbfbc6bec, marshal_data=0xcc) at gclosure.c:567
#17 0xb79ebd3b in g_closure_invoke (closure=0x8073200, return_value=0xbfbc6c00, n_param_values=2, param_values=0xbfbc6cdc, invocation_hint=0xbfbc6bec) at gclosure.c:490
#18 0xb79fc7e3 in signal_emit_unlocked_R (node=0x80732f8, detail=0, instance=0x807b048, emission_return=0xbfbc6e9c, instance_and_params=0xbfbc6cdc) at gsignal.c:2478
#19 0xb79fd47f in g_signal_emit_valist (instance=0x807b048, signal_id=38, detail=0, var_args=0xbfbc6f20 "8o��\b\225\006\bH�\a\bq���H�\a\b��\006\b") at gsignal.c:2209
#20 0xb79fd885 in g_signal_emit (instance=0x807b048, signal_id=38, detail=0) at gsignal.c:2243
#21 0xb7e54548 in gtk_widget_event_internal (widget=0x807b048, event=0x8069508) at gtkwidget.c:4675
#22 0xb7d12e27 in IA__gtk_propagate_event (widget=0x807b048, event=0x8069508) at gtkmain.c:2291
#23 0xb7d14182 in IA__gtk_main_do_event (event=0x8069508) at gtkmain.c:1537
#24 0xb7b724ca in gdk_event_dispatch (source=0x806d470, callback=0, user_data=0x0) at gdkevents-x11.c:2351
#25 0xb794a77c in g_main_context_dispatch (context=0x806d4b8) at gmain.c:2061
#26 0xb794dc1f in g_main_context_iterate (context=0x806d4b8, block=1, dispatch=1, self=0x80a0718) at gmain.c:2694
#27 0xb794dfc9 in g_main_loop_run (loop=0x80ea6a8) at gmain.c:2898
#28 0xb7d14614 in IA__gtk_main () at gtkmain.c:1144
#29 0x080491e5 in main (argc=1073741826, argv=0xb79f90c0) at acceltoolbar.c:120
*/

static void
on_play_clicked (GtkButton * button, gpointer user_data)
{
  if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (button))) {
    gtk_label_set_text (GTK_LABEL (user_data), "now playing");
    g_message ("now playing");
  } else {
    g_message ("play deativated");
  }
}

static void
on_stop_clicked (GtkButton * button, gpointer user_data)
{
  gtk_label_set_text (GTK_LABEL (user_data), "stop");
  g_message ("stop");
  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (play_button),
      FALSE);
}

static void
init ()
{
  GtkWidget *vbox;
  GtkWidget *tb, *ti;
  GtkAccelGroup *accel_group;
  GtkWidget *label;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Toolbar with accelerator");
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (destroy), NULL);

  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  // message area
  label = gtk_label_new ("");

  // regular toolbar
  tb = gtk_toolbar_new ();
  gtk_container_add (GTK_CONTAINER (vbox), tb);

  ti = GTK_WIDGET (gtk_toggle_tool_button_new_from_stock
      (GTK_STOCK_MEDIA_PLAY));
#if 0
  // this leads to: gtk_widget_set_accel_path: assertion `GTK_WIDGET_GET_CLASS (widget)->activate_signal != 0' failed
  // fix localy in gtktoolbutton.c:261 : widget_class->activate_signal = toolbutton_signals[CLICKED];
  gtk_widget_set_accel_path (ti, "<Buzztard-Main>/MainToolbar/Play",
      accel_group);
  gtk_accel_map_add_entry ("<Buzztard-Main>/MainToolbar/Play", GDK_F5, 0);
#endif
#if 1
  // this does not toggle the toggle button
  gtk_widget_add_accelerator (ti, "clicked", accel_group, GDK_F5, 0, 0);
#endif
#if 0
  // this does not trigger anything at all
  GtkActionGroup *action_group;
  GtkAction *action;

  action_group = gtk_action_group_new ("MainToolbar");
  action =
      GTK_ACTION (gtk_toggle_action_new ("Play", "Play", "Play this song",
          GTK_STOCK_MEDIA_PLAY));
  gtk_action_set_accel_path (action, "<Buzztard-Main>/MainToolbar/Play");
  gtk_action_group_add_action_with_accel (action_group, action, "F5");
  gtk_action_connect_proxy (action, ti);
#endif
  gtk_toolbar_insert (GTK_TOOLBAR (tb), GTK_TOOL_ITEM (ti), -1);
  g_signal_connect (G_OBJECT (ti), "clicked", G_CALLBACK (on_play_clicked),
      (gpointer) label);
  play_button = ti;

  ti = GTK_WIDGET (gtk_tool_button_new_from_stock (GTK_STOCK_MEDIA_STOP));
  //gtk_widget_set_accel_path (ti, "<Buzztard-Main>/MainToolbar/Play",accel_group);
  //gtk_accel_map_add_entry ("<Buzztard-Main>/MainToolbar/Play", GDK_F8, 0);
  gtk_widget_add_accelerator (ti, "clicked", accel_group, GDK_F8, 0, 0);
  gtk_toolbar_insert (GTK_TOOLBAR (tb), GTK_TOOL_ITEM (ti), -1);
  g_signal_connect (G_OBJECT (ti), "clicked", G_CALLBACK (on_stop_clicked),
      (gpointer) label);

  // message area
  gtk_container_add (GTK_CONTAINER (vbox), label);
  gtk_container_add (GTK_CONTAINER (vbox),
      gtk_label_new ("F5: play, F8: stop"));

  gtk_widget_show_all (window);
}

static void
done ()
{

}

int
main (int argc, char **argv)
{
  gtk_init (&argc, &argv);

  init ();
  gtk_main ();
  done ();

  return (0);
}
