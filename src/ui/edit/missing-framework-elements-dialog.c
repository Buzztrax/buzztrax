/* Buzztrax
 * Copyright (C) 2007 Buzztrax team <buzztrax-devel@buzztrax.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
/**
 * SECTION:btmissingframeworkelementsdialog
 * @short_description: missing core and application elements
 *
 * A dialog to inform about missing core and application elements.
 */
/*
 * TODO(ensonic): support gst-codec-install
 * Since gst-plugin-base-0.10.15 there is gst_install_plugins_supported().
 * If is supported we could do:
 *
 * GstInstallPluginsReturn result;
 * gchar *details[] = {
 *   gst_missing_element_installer_detail_new(factory-name1),
 *   NULL
 * };
 * result = gst_install_plugins_sync (details, NULL);
 * if (result == GST_INSTALL_PLUGINS_SUCCESS ||
 *     result == GST_INSTALL_PLUGINS_PARTIAL_SUCCESS) {
 *   gst_update_registry ();
 * }
*/
#define BT_EDIT
#define BT_MISSING_FRAMEWORK_ELEMENTS_DIALOG_C

#include "bt-edit.h"
#include <glib/gprintf.h>

//-- property ids

enum
{
  MISSING_FRAMEWORK_ELEMENTS_DIALOG_CORE_ELEMENTS = 1,
  MISSING_FRAMEWORK_ELEMENTS_DIALOG_EDIT_ELEMENTS
};

struct _BtMissingFrameworkElementsDialogPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* list of missing elements */
  GList *core_elements, *edit_elements;

  GtkWidget *ignore_button;
};

//-- the class

G_DEFINE_TYPE (BtMissingFrameworkElementsDialog,
    bt_missing_framework_elements_dialog, GTK_TYPE_DIALOG);


//-- event handler

//-- helper methods

static void
make_listview (GtkWidget * vbox, GList * missing_elements, const gchar * msg)
{
  GtkWidget *label, *missing_list, *missing_list_view;
  GList *node;
  gchar *missing_text, *ptr;
  gint length = 0;

  label = gtk_label_new (msg);
  g_object_set (label, "xalign", 0.0, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  for (node = missing_elements; node; node = g_list_next (node)) {
    length += 2 + strlen ((gchar *) (node->data));
  }
  ptr = missing_text = g_malloc (length);
  for (node = missing_elements; node; node = g_list_next (node)) {
    length = g_sprintf (ptr, "%s\n", (gchar *) (node->data));
    ptr = &ptr[length];
  }
  ptr[-1] = '\0';               // remove last '\n'

  missing_list = gtk_text_view_new ();
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (missing_list), FALSE);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (missing_list), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (missing_list), GTK_WRAP_WORD);
  gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW
          (missing_list)), missing_text, -1);
  gtk_widget_show (missing_list);
  g_free (missing_text);

  missing_list_view = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (missing_list_view),
      GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (missing_list_view),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (missing_list_view), missing_list);
  gtk_widget_show (missing_list_view);
  gtk_box_pack_start (GTK_BOX (vbox), missing_list_view, TRUE, TRUE, 0);
}

static gboolean
bt_missing_framework_elements_dialog_init_ui (const
    BtMissingFrameworkElementsDialog * self)
{
  GtkWidget *label, *icon, *hbox, *vbox;
  gchar *str;
  gboolean res = TRUE;
  //GdkPixbuf *window_icon=NULL;

  gtk_widget_set_name (GTK_WIDGET (self), "Missing GStreamer elements");

  // create and set window icon
  /*
     if((window_icon=bt_ui_resources_get_icon_pixbuf_by_machine(self->priv->machine))) {
     gtk_window_set_icon(GTK_WINDOW(self),window_icon);
     g_object_unref(window_icon);
     }
   */

  // set dialog title
  gtk_window_set_title (GTK_WINDOW (self), _("Missing GStreamer elements"));

  // add dialog commision widgets (okay, cancel)
  gtk_dialog_add_button (GTK_DIALOG (self), _("_OK"), GTK_RESPONSE_ACCEPT);

  gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_ACCEPT);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);

  icon =
      gtk_image_new_from_icon_name (self->
      priv->core_elements ? "dialog-error" : "dialog-warning",
      GTK_ICON_SIZE_DIALOG);
  gtk_container_add (GTK_CONTAINER (hbox), icon);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  label = gtk_label_new (NULL);
  str =
      g_strdup_printf ("<big><b>%s</b></big>", _("Missing GStreamer elements"));
  gtk_label_set_markup (GTK_LABEL (label), str);
  g_object_set (label, "xalign", 0.0, NULL);
  g_free (str);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  if (self->priv->core_elements) {
    GST_DEBUG ("%d missing core elements",
        g_list_length (self->priv->core_elements));
    make_listview (vbox, self->priv->core_elements,
        _
        ("The elements listed below are missing from your installation, but are required."));
  }
  if (self->priv->edit_elements) {
    gchar *machine_ignore_list;
    GList *edit_elements = NULL;

    GST_DEBUG ("%d missing edit elements",
        g_list_length (self->priv->edit_elements));

    bt_child_proxy_get (self->priv->app, "settings::missing-machines",
        &machine_ignore_list, NULL);

    if (machine_ignore_list) {
      GList *node;
      gchar *name;
      gboolean have_elements = FALSE;

      for (node = self->priv->edit_elements; node; node = g_list_next (node)) {
        name = (gchar *) (node->data);
        // if this is the message ("starts with "->") or is not in the ignored list, append
        if (name[0] == '-') {
          // if all elements between two messages are ignored, drop the message too
          if (have_elements) {
            edit_elements = g_list_append (edit_elements, node->data);
            have_elements = FALSE;
          }
        } else if (!strstr (machine_ignore_list, name)) {
          edit_elements = g_list_append (edit_elements, node->data);
          have_elements = TRUE;
        }
      }
      GST_DEBUG ("filtered to %d missing edit elements",
          g_list_length (edit_elements));
    } else {
      edit_elements = self->priv->edit_elements;
    }

    if (edit_elements) {
      make_listview (vbox, edit_elements,
          _
          ("The elements listed below are missing from your installation, but are recommended for full functionality."));

      self->priv->ignore_button =
          gtk_check_button_new_with_label (_("don't warn again"));
      gtk_box_pack_start (GTK_BOX (vbox), self->priv->ignore_button, FALSE,
          FALSE, 0);

      if (machine_ignore_list) {
        g_list_free (edit_elements);
      }
    } else {
      // if we have only non-critical elements and ignore them already, don't show the dialog.
      if (!self->priv->core_elements) {
        GST_INFO ("no new missing elements to show");
        res = FALSE;
      }
    }
  }
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))),
      hbox, TRUE, TRUE, 0);

  return res;
}

//-- constructor methods

/**
 * bt_missing_framework_elements_dialog_new:
 * @core_elements: list of missing core elements
 * @edit_elements: list of missing edit elements
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case there is nothing new to show
 */
BtMissingFrameworkElementsDialog *
bt_missing_framework_elements_dialog_new (GList * core_elements,
    GList * edit_elements)
{
  BtMissingFrameworkElementsDialog *self =
      BT_MISSING_FRAMEWORK_ELEMENTS_DIALOG (g_object_new
      (BT_TYPE_MISSING_FRAMEWORK_ELEMENTS_DIALOG, "core-elements",
          core_elements, "edit-elements", edit_elements, NULL));
  if (!bt_missing_framework_elements_dialog_init_ui (self)) {
    goto EmptyLists;
  }
  return self;
EmptyLists:
  gtk_widget_destroy (GTK_WIDGET (self));
  return NULL;
}

//-- methods

/**
 * bt_missing_framework_elements_dialog_apply:
 * @self: the dialog which settings to apply
 *
 * Makes the dialog settings effective.
 */
void
bt_missing_framework_elements_dialog_apply (const
    BtMissingFrameworkElementsDialog * self)
{

  if (self->priv->ignore_button
      && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->
              priv->ignore_button))) {
    BtSettings *settings;
    gchar *machine_ignore_list;
    GList *node, *edit_elements = NULL;
    gchar *ptr, *name;
    gint length = 0, offset = 0;

    g_object_get (self->priv->app, "settings", &settings, NULL);
    g_object_get (settings, "missing-machines", &machine_ignore_list, NULL);
    GST_INFO ("was ignoring : [%s]", machine_ignore_list);

    if (machine_ignore_list) {
      offset = length = strlen (machine_ignore_list);
    }
    for (node = self->priv->edit_elements; node; node = g_list_next (node)) {
      name = (gchar *) (node->data);
      if (name[0] != '-') {
        if (!offset || !strstr (machine_ignore_list, name)) {
          edit_elements = g_list_append (edit_elements, name);
          length += 2 + strlen ((gchar *) name);
        }
      }
    }
    if (length) {
      GST_INFO ("enlarging to %d bytes", length);
      machine_ignore_list = g_realloc (machine_ignore_list, length);
    }
    if (machine_ignore_list) {
      ptr = &machine_ignore_list[offset];
      for (node = edit_elements; node; node = g_list_next (node)) {
        length = g_sprintf (ptr, "%s,", (gchar *) (node->data));
        ptr = &ptr[length];
      }
    }
    g_list_free (edit_elements);
    GST_INFO ("now ignoring : [%s]", machine_ignore_list);

    g_object_set (settings, "missing-machines", machine_ignore_list, NULL);
    g_object_unref (settings);
  }
}

//-- wrapper

//-- class internals

static void
bt_missing_framework_elements_dialog_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec)
{
  BtMissingFrameworkElementsDialog *self =
      BT_MISSING_FRAMEWORK_ELEMENTS_DIALOG (object);
  return_if_disposed ();
  switch (property_id) {
    case MISSING_FRAMEWORK_ELEMENTS_DIALOG_CORE_ELEMENTS:
      self->priv->core_elements = g_value_get_pointer (value);
      break;
    case MISSING_FRAMEWORK_ELEMENTS_DIALOG_EDIT_ELEMENTS:
      self->priv->edit_elements = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_missing_framework_elements_dialog_dispose (GObject * object)
{
  BtMissingFrameworkElementsDialog *self =
      BT_MISSING_FRAMEWORK_ELEMENTS_DIALOG (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_missing_framework_elements_dialog_parent_class)->dispose
      (object);
}

static void
bt_missing_framework_elements_dialog_init (BtMissingFrameworkElementsDialog *
    self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self,
      BT_TYPE_MISSING_FRAMEWORK_ELEMENTS_DIALOG,
      BtMissingFrameworkElementsDialogPrivate);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
}

static void
    bt_missing_framework_elements_dialog_class_init
    (BtMissingFrameworkElementsDialogClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass,
      sizeof (BtMissingFrameworkElementsDialogPrivate));

  gobject_class->set_property =
      bt_missing_framework_elements_dialog_set_property;
  gobject_class->dispose = bt_missing_framework_elements_dialog_dispose;

  g_object_class_install_property (gobject_class,
      MISSING_FRAMEWORK_ELEMENTS_DIALOG_CORE_ELEMENTS,
      g_param_spec_pointer ("core-elements", "core-elements construct prop",
          "Set missing core-elements list, the dialog handles",
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      MISSING_FRAMEWORK_ELEMENTS_DIALOG_EDIT_ELEMENTS,
      g_param_spec_pointer ("edit-elements", "edit-elements construct prop",
          "Set missing edit-elements list, the dialog handles",
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));
}
