/* $Id: pattern-view.c,v 1.11 2007-11-24 11:50:28 ensonic Exp $
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/**
 * SECTION:btpatternview
 * @short_description: the editor main pattern table view
 * @see_also: #MainPagePatterns
 *
 * This widget derives from the #GtkTreeView to additionaly draw play-position
 * bars.
 */

#define BT_EDIT
#define BT_PATTERN_VIEW_C

#include "bt-edit.h"

enum {
  PATTERN_VIEW_APP=1,
  PATTERN_VIEW_PLAY_POSITION,
  PATTERN_VIEW_VISIBLE_ROWS
};


struct _BtPatternViewPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  G_POINTER_ALIAS(BtEditApplication *,app);

  /* position of playing pointer from 0.0 ... 1.0 */
  gdouble play_pos;

  /* number of visible rows, the height of one row */
  gulong visible_rows,row_height;

  /* cache some ressources */
  GdkWindow *window;
  GdkGC *play_pos_gc;
};

static GtkTreeViewClass *parent_class=NULL;

//-- event handler

//-- helper methods

#if 0
// this doesn't make the drawing better :(
static void bt_pattern_view_invalidate(const BtPatternView *self, gdouble old_pos, gdouble new_pos) {
  gdouble h=(gdouble)(self->priv->visible_rows*self->priv->row_height);
  gint y;

  y=(gint)(old_pos*h);
  gtk_widget_queue_draw_area(GTK_WIDGET(self),0,y-1,GTK_WIDGET(self)->allocation.width,y+1);
  y=(gint)(new_pos*h);
  gtk_widget_queue_draw_area(GTK_WIDGET(self),0,y-1,GTK_WIDGET(self)->allocation.width,y+1);
}
#endif

//-- constructor methods

/**
 * bt_pattern_view_new:
 * @app: the application the window belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtPatternView *bt_pattern_view_new(const BtEditApplication *app) {
  BtPatternView *self;

  if(!(self=BT_PATTERN_VIEW(g_object_new(BT_TYPE_PATTERN_VIEW,"app",app,NULL)))) {
    goto Error;
  }
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

//-- wrapper

//-- class internals

static void bt_pattern_view_realize(GtkWidget *widget) {
  BtPatternView *self = BT_PATTERN_VIEW(widget);

  // first let the parent realize itslf
  if(GTK_WIDGET_CLASS(parent_class)->realize) {
    (GTK_WIDGET_CLASS(parent_class)->realize)(widget);
  }
  self->priv->window=gtk_tree_view_get_bin_window(GTK_TREE_VIEW(self));

  // allocation graphical contexts for drawing the overlay lines
  self->priv->play_pos_gc=gdk_gc_new(self->priv->window);
  gdk_gc_set_rgb_fg_color(self->priv->play_pos_gc,bt_ui_ressources_get_gdk_color(BT_UI_RES_COLOR_PLAYLINE));
  gdk_gc_set_line_attributes(self->priv->play_pos_gc,2,GDK_LINE_SOLID,GDK_CAP_BUTT,GDK_JOIN_MITER);
}

static void bt_pattern_view_unrealize(GtkWidget *widget) {
  BtPatternView *self = BT_PATTERN_VIEW(widget);

  // first let the parent realize itslf
  if(GTK_WIDGET_CLASS(parent_class)->unrealize) {
    (GTK_WIDGET_CLASS(parent_class)->unrealize)(widget);
  }

  g_object_unref(self->priv->play_pos_gc);
  self->priv->play_pos_gc=NULL;
}

static gboolean bt_pattern_view_expose_event(GtkWidget *widget,GdkEventExpose *event) {
  BtPatternView *self = BT_PATTERN_VIEW(widget);

  //GST_INFO("!!!! self=%p",self);
  //if(!GTK_WIDGET_REALIZED(self)) return FALSE;

  // let the parent handle its expose
  if(GTK_WIDGET_CLASS(parent_class)->expose_event) {
    (GTK_WIDGET_CLASS(parent_class)->expose_event)(widget,event);
  }

  /* We need to check to make sure that the expose event is actually occuring on
   * the window where the table data is being drawn.  If we don't do this check,
   * row zero spanners can be drawn on top of the column headers.
   */
  if(self->priv->window==event->window) {
    gint w,y;
    gdouble h;
    GdkRectangle vr;

    if(G_UNLIKELY(!self->priv->row_height)) {
      GtkTreePath *path;
      GdkRectangle br;

      // determine row height
      path=gtk_tree_path_new_from_indices(0,-1);
      gtk_tree_view_get_background_area(GTK_TREE_VIEW(widget),path,NULL,&br);
      self->priv->row_height=br.height;
      gtk_tree_path_free(path);
      GST_INFO("view=%p, cell background visible rect: %d x %d, %d x %d",widget,br.x,br.y,br.width,br.height);
    }

    gtk_tree_view_get_visible_rect(GTK_TREE_VIEW(widget),&vr);

    w=widget->allocation.width;
    h=(gdouble)(self->priv->visible_rows*self->priv->row_height);

    y=(gint)(self->priv->play_pos*h);
    if((y>=vr.y) && (y<(vr.y+vr.height))) {
      gdk_draw_line(self->priv->window,self->priv->play_pos_gc,vr.x+0,y,vr.x+w,y);
    }
  }
  return(FALSE);
}

/* returns a property for the given property_id for this object */
static void bt_pattern_view_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtPatternView *self = BT_PATTERN_VIEW(object);
  return_if_disposed();
  switch (property_id) {
    case PATTERN_VIEW_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_pattern_view_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtPatternView *self = BT_PATTERN_VIEW(object);
  //gdouble old_pos;

  return_if_disposed();
  switch (property_id) {
    case PATTERN_VIEW_APP: {
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->app);
      //GST_DEBUG("set the app for sequence_view: %p",self->priv->app);
    } break;
    case PATTERN_VIEW_PLAY_POSITION: {
      //old_pos = self->priv->play_pos;
      self->priv->play_pos = g_value_get_double(value);
      if(GTK_WIDGET_REALIZED(GTK_WIDGET(self))) {
        gtk_widget_queue_draw(GTK_WIDGET(self));
        //bt_pattern_view_invalidate(self,old_pos,self->priv->play_pos);
      }
    } break;
    case PATTERN_VIEW_VISIBLE_ROWS: {
      self->priv->visible_rows = g_value_get_ulong(value);
      if(GTK_WIDGET_REALIZED(GTK_WIDGET(self))) {
        gtk_widget_queue_draw(GTK_WIDGET(self));
      }
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_pattern_view_dispose(GObject *object) {
  BtPatternView *self = BT_PATTERN_VIEW(object);
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  g_object_try_weak_unref(self->priv->app);

  g_object_try_unref(self->priv->play_pos_gc);

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_pattern_view_finalize(GObject *object) {
  //BtPatternView *self = BT_PATTERN_VIEW(object);

  //GST_DEBUG("!!!! self=%p",self);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_pattern_view_init(GTypeInstance *instance, gpointer g_class) {
  BtPatternView *self = BT_PATTERN_VIEW(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_PATTERN_VIEW, BtPatternViewPrivate);
}

static void bt_pattern_view_class_init(BtPatternViewClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS(klass);

  parent_class=g_type_class_ref(GTK_TYPE_TREE_VIEW);
  g_type_class_add_private(klass,sizeof(BtPatternViewPrivate));

  gobject_class->set_property = bt_pattern_view_set_property;
  gobject_class->get_property = bt_pattern_view_get_property;
  gobject_class->dispose      = bt_pattern_view_dispose;
  gobject_class->finalize     = bt_pattern_view_finalize;

  // override some gtkwidget methods
  gtkwidget_class->realize = bt_pattern_view_realize;
  gtkwidget_class->unrealize = bt_pattern_view_unrealize;
  gtkwidget_class->expose_event = bt_pattern_view_expose_event;

  g_object_class_install_property(gobject_class,PATTERN_VIEW_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "Set application object, the window belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,PATTERN_VIEW_PLAY_POSITION,
                                  g_param_spec_double("play-position",
                                     "play position prop.",
                                     "The current playing position as a fraction",
                                     0.0,
                                     1.0,
                                     0.0,
                                     G_PARAM_WRITABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,PATTERN_VIEW_VISIBLE_ROWS,
                                  g_param_spec_ulong("visible-rows",
                                     "visible rows prop.",
                                     "The number of currntly visible sequence rows",
                                     0,
                                     G_MAXULONG,
                                     0,
                                     G_PARAM_WRITABLE|G_PARAM_STATIC_STRINGS));
}

GType bt_pattern_view_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    const GTypeInfo info = {
      sizeof(BtPatternViewClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_pattern_view_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtPatternView),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_pattern_view_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(GTK_TYPE_TREE_VIEW,"BtPatternView",&info,0);
  }
  return type;
}
