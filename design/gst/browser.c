/* $Id: browser.c,v 1.2 2004-04-08 13:30:35 waffel Exp $ */

/* prints all available gst plugins to the console 
*/

#include <stdio.h>
#include <gst/gst.h>

int main(int argc, char **argv) {
  GstElementFactory *elementFact;
  GstElement *element;
  const GList *elements;
  
  g_print("%lf\n",G_MAXFLOAT);
  
  gst_init(&argc, &argv);
  elements = gst_registry_pool_feature_list (GST_TYPE_ELEMENT_FACTORY);
  while (elements) {
    elementFact = (GstElementFactory *) (elements->data);
    
    g_print("name %s [%s]\n", elementFact->details.longname, elementFact->details.klass);
    
    elements = g_list_next (elements);
  }
  exit (0);
}

