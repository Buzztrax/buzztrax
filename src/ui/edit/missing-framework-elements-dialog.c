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

struct _BtMissingFrameworkElementsDialog
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* list of missing elements */
  GList *core_elements, *edit_elements;

  GtkImage *icon;
  GtkBox *vbox;
  GtkWidget *ignore_button;
};

//-- the class

G_DEFINE_TYPE (BtMissingFrameworkElementsDialog,
    bt_missing_framework_elements_dialog, GTK_TYPE_DIALOG);


//-- event handler

//-- helper methods

/**
 * Return: the container of the new list
 */
static GtkWidget*
make_listview (GtkBox * vbox, GList * missing_elements, const gchar * msg)
{
  GtkWidget *label, *missing_list, *missing_list_view;
  gchar *missing_text;

  label = gtk_label_new (msg);
  g_object_set (label, "xalign", 0.0, NULL);
  gtk_box_append (vbox, label);

  missing_text = bt_strjoin_list (missing_elements);

  missing_list = gtk_text_view_new ();
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (missing_list), FALSE);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (missing_list), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (missing_list), GTK_WRAP_WORD);
  gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW
          (missing_list)), missing_text, -1);
  g_free (missing_text);

  missing_list_view = gtk_scrolled_window_new ();
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (missing_list_view),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (missing_list_view), missing_list);
  gtk_box_append (vbox, missing_list_view);

  return missing_list_view;
}

static gboolean
bt_missing_framework_elements_dialog_init_ui (const
    BtMissingFrameworkElementsDialog * self)
{
  gboolean res = TRUE;

  gtk_image_set_from_icon_name (self->icon,
      self->core_elements ? "dialog-error" : "dialog-warning");

  if (self->core_elements) {
    GST_DEBUG ("%d missing core elements",
        g_list_length (self->core_elements));
    make_listview (self->vbox, self->core_elements,
        _("The elements listed below are missing from your installation, but are required."));
  }
  if (self->edit_elements) {
    gchar *machine_ignore_list;
    GList *edit_elements = NULL;

    GST_DEBUG ("%d missing edit elements",
        g_list_length (self->edit_elements));

    bt_child_proxy_get (self->app, "settings::missing-machines",
        &machine_ignore_list, NULL);

    if (machine_ignore_list) {
      GList *node;
      gchar *name;
      gboolean have_elements = FALSE;

      for (node = self->edit_elements; node; node = g_list_next (node)) {
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
      edit_elements = self->edit_elements;
    }

    if (edit_elements) {
      GtkWidget* scrolledwindow = make_listview (self->vbox, edit_elements,
          _("The elements listed below are missing from your installation, but are recommended for full functionality."));

      gtk_box_insert_child_after (self->vbox, self->ignore_button, scrolledwindow);
      gtk_widget_set_visible (self->ignore_button, TRUE);

      if (machine_ignore_list) {
        g_list_free (edit_elements);
      }
    } else {
      // if we have only non-critical elements and ignore them already, don't show the dialog.
      if (!self->core_elements) {
        GST_INFO ("no new missing elements to show");
        res = FALSE;
      }
    }
  }

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
    g_object_ref_sink (self);
    g_object_unref (self);
    return NULL;
  }
  return self;
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

  if (self->ignore_button
      && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->ignore_button))) {
    BtSettings *settings;
    gchar *machine_ignore_list;
    GList *node, *edit_elements = NULL;
    gchar *ptr, *name;
    gint length = 0, offset = 0;

    g_object_get (self->app, "settings", &settings, NULL);
    g_object_get (settings, "missing-machines", &machine_ignore_list, NULL);
    GST_INFO ("was ignoring : [%s]", machine_ignore_list);

    if (machine_ignore_list) {
      offset = length = strlen (machine_ignore_list);
    }
    for (node = self->edit_elements; node; node = g_list_next (node)) {
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
      // TODO(ensonic):: refactor to reuse code in tools/bt_strjoin_list()
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
  return_if_disposed_self ();
  switch (property_id) {
    case MISSING_FRAMEWORK_ELEMENTS_DIALOG_CORE_ELEMENTS:
      self->core_elements = g_value_get_pointer (value);
      break;
    case MISSING_FRAMEWORK_ELEMENTS_DIALOG_EDIT_ELEMENTS:
      self->edit_elements = g_value_get_pointer (value);
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

  return_if_disposed_self ();
  self->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_object_unref (self->app);

  gtk_widget_dispose_template (GTK_WIDGET (self), BT_TYPE_MISSING_FRAMEWORK_ELEMENTS_DIALOG);
  
  G_OBJECT_CLASS (bt_missing_framework_elements_dialog_parent_class)->dispose
      (object);
}

static void
bt_missing_framework_elements_dialog_init (BtMissingFrameworkElementsDialog *
    self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  
  self->app = bt_edit_application_new ();
}

static void
    bt_missing_framework_elements_dialog_class_init
    (BtMissingFrameworkElementsDialogClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

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

  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  
  gtk_widget_class_set_template_from_resource (widget_class,
      "/org/buzztrax/ui/missing-framework-elements-dialog.ui");

  gtk_widget_class_bind_template_child (widget_class, BtMissingFrameworkElementsDialog, icon);
  gtk_widget_class_bind_template_child (widget_class, BtMissingFrameworkElementsDialog, ignore_button);
  gtk_widget_class_bind_template_child (widget_class, BtMissingFrameworkElementsDialog, vbox);
}
