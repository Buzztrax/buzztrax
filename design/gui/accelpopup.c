/** $Id$
 * test popup menus with accelerator keys
 *
 * gcc -Wall -g `pkg-config gtk+-2.0 --cflags --libs` accelpopup.c -o accelpopup
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>

static GtkWidget *window=NULL;


static void destroy(GtkWidget *widget,gpointer data) {
  gtk_main_quit();
}

static void on_menu_activate(GtkMenuItem *menuitem,gpointer user_data) {
  gtk_label_set_text(GTK_LABEL(user_data),"main menu");
}

static void on_context_menu1_activate(GtkMenuItem *menuitem,gpointer user_data) {
  gtk_label_set_text(GTK_LABEL(user_data),"context menu 1");
}

static void on_context_menu2_activate(GtkMenuItem *menuitem,gpointer user_data) {
  gtk_label_set_text(GTK_LABEL(user_data),"context menu 2");
}

static void on_popup_button_clicked(GtkButton *widget,gpointer user_data) {
  gtk_menu_popup(GTK_MENU(user_data),NULL,NULL,NULL,NULL,3,gtk_get_current_event_time());
}

static void on_check_toggled(GtkToggleButton *togglebutton,gpointer user_data) {
  if(gtk_toggle_button_get_active(togglebutton)) {
    gtk_window_add_accel_group(GTK_WINDOW(window),GTK_ACCEL_GROUP(user_data));
    g_signal_emit_by_name (window, "keys-changed", 0);
    puts("2nd accel group added");
  }
  else {
    gtk_window_remove_accel_group(GTK_WINDOW(window),GTK_ACCEL_GROUP(user_data));
    puts("2nd accel group removed");
  }
}

static void init() {
  GtkWidget *vbox,*hbox;
  GtkWidget *button;
  GtkWidget *mb,*pm,*sm,*mi;
  GtkAccelGroup *accel_group;
  GtkWidget *label,*ck;

  window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "Popup with accelerator");
  g_signal_connect(G_OBJECT(window), "destroy",	G_CALLBACK (destroy), NULL);

  accel_group=gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(window),accel_group);

  vbox=gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(window),vbox);

  // message area
  label=gtk_label_new("");

  // regular menu
  mb=gtk_menu_bar_new();
  gtk_container_add(GTK_CONTAINER(vbox),mb);
  mi=gtk_menu_item_new_with_label("Menu");
  gtk_container_add(GTK_CONTAINER(mb),mi);

  sm=gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi),sm);

  mi=gtk_image_menu_item_new_from_stock(GTK_STOCK_NEW,accel_group);
  gtk_container_add(GTK_CONTAINER(sm),mi);
  g_signal_connect(G_OBJECT(mi),"activate",G_CALLBACK(on_menu_activate),(gpointer)label);

  hbox=gtk_hbox_new(FALSE,6);
  gtk_container_add(GTK_CONTAINER(vbox),hbox);

  // popup menu 1
  pm=gtk_menu_new();
  gtk_menu_set_accel_group(GTK_MENU(pm), accel_group);
  gtk_menu_set_accel_path(GTK_MENU(pm),"<Main>/Context1");

  mi=gtk_menu_item_new_with_label("popup item");
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (mi), "<Main>/Context1/PopupItem");
  gtk_accel_map_add_entry ("<Main>/Context1/PopupItem", GDK_P, GDK_CONTROL_MASK);
  gtk_menu_shell_append(GTK_MENU_SHELL(pm),mi);
  gtk_widget_show(mi);
  g_signal_connect(G_OBJECT(mi),"activate",G_CALLBACK(on_context_menu1_activate),(gpointer)label);

  button=gtk_button_new_with_label("Popup 1");
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_popup_button_clicked), (gpointer)pm);
  gtk_container_add(GTK_CONTAINER(hbox),button);

  // popup menu 2
  accel_group=gtk_accel_group_new();
  // if I only add it later, all accelerators get ignored
  //gtk_window_add_accel_group(GTK_WINDOW(window),accel_group);

  pm=gtk_menu_new();
  gtk_menu_set_accel_group(GTK_MENU(pm), accel_group);
  gtk_menu_set_accel_path(GTK_MENU(pm),"<Main>/Context2");

  mi=gtk_menu_item_new_with_label("popup item");
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (mi), "<Main>/Context2/PopupItem");
  gtk_accel_map_add_entry ("<Main>/Context2/PopupItem", GDK_O, GDK_CONTROL_MASK);
  gtk_menu_shell_append(GTK_MENU_SHELL(pm),mi);
  gtk_widget_show(mi);
  g_signal_connect(G_OBJECT(mi),"activate",G_CALLBACK(on_context_menu2_activate),(gpointer)label);

  button=gtk_button_new_with_label("Popup 2");
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_popup_button_clicked), (gpointer)pm);
  gtk_container_add(GTK_CONTAINER(hbox),button);

  ck=gtk_check_button_new_with_label("<- enable accel");
  gtk_container_add(GTK_CONTAINER(hbox),ck);
  g_signal_connect (G_OBJECT(ck), "toggled", G_CALLBACK (on_check_toggled), (gpointer)accel_group);

  // message area
  gtk_container_add(GTK_CONTAINER(vbox),label);

  gtk_widget_show_all(window);
}

static void done() {

}

int main(int argc, char **argv) {
  gtk_init(&argc,&argv);

  init();
  gtk_main();
  done();

  return(0);
}
