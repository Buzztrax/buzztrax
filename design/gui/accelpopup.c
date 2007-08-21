/** $Id: accelpopup.c,v 1.1 2007-08-21 19:55:32 ensonic Exp $
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

static void destroy(GtkWidget *widget,gpointer data) {
  gtk_main_quit();
}

static void on_menu_activate(GtkMenuItem *menuitem,gpointer user_data) {
  gtk_label_set_text(GTK_LABEL(user_data),"main menu");
}

static void on_context_menu_activate(GtkMenuItem *menuitem,gpointer user_data) {
  gtk_label_set_text(GTK_LABEL(user_data),"context menu");
}

static gboolean on_label_button_press_event(GtkWidget *widget,GdkEventButton *event,gpointer user_data) {
  if(event->button==3) {
    gtk_menu_popup(GTK_MENU(user_data),NULL,NULL,NULL,NULL,3,gtk_get_current_event_time());
    return(TRUE);
  }
  return(FALSE);
}

static void init() {
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *button;
  GtkWidget *mb,*pm,*sm,*mi;
  GtkAccelGroup *accel_group;
  GtkWidget *label;

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

  // popup menu
  pm=gtk_menu_new();
  gtk_menu_set_accel_group(GTK_MENU(pm), accel_group);
  gtk_menu_set_accel_path(GTK_MENU(pm),"<Main>/Context");

  mi=gtk_menu_item_new_with_label("popup item");
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (mi), "<Main>/Context/PopupItem");
  gtk_accel_map_add_entry ("<Main>/Context/PopupItem", GDK_P, GDK_CONTROL_MASK|GDK_SHIFT_MASK);
  gtk_menu_shell_append(GTK_MENU_SHELL(pm),mi);
  gtk_widget_show(mi);
  g_signal_connect(G_OBJECT(mi),"activate",G_CALLBACK(on_context_menu_activate),(gpointer)label);

  button=gtk_button_new_with_label("Popup");
  g_signal_connect(G_OBJECT(button), "button-press-event", G_CALLBACK(on_label_button_press_event), (gpointer)pm);
  gtk_container_add(GTK_CONTAINER(vbox),button);
  
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
