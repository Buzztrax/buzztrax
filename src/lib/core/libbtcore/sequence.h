/** $Id: sequence.h,v 1.1 2004-05-06 15:10:21 ensonic Exp $
 * class for the pattern sequence
 */

#ifndef BT_SEQUENCE_H
#define BT_SEQUENCE_H

#include <glib.h>
#include <glib-object.h>

#define BT_SEQUENCE_TYPE		        (bt_sequence_get_type ())
#define BT_SEQUENCE(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_SEQUENCE_TYPE, BtSequence))
#define BT_SEQUENCE_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_SEQUENCE_TYPE, BtSequenceClass))
#define BT_IS_SEQUENCE(obj)	        (G_TYPE_CHECK_TYPE ((obj), BT_SEQUENCE_TYPE))
#define BT_IS_SEQUENCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_SEQUENCE_TYPE))
#define BT_SEQUENCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_SEQUENCE_TYPE, BtSequenceClass))

/* type macros */

typedef struct _BtSequence BtSequence;
typedef struct _BtSequenceClass BtSequenceClass;
typedef struct _BtSequencePrivate BtSequencePrivate;

struct _BtSequence {
  GObject parent;
  
  /* private */
  BtSequencePrivate *private;
};
/* structure of the sequence class */
struct _BtSequenceClass {
  GObjectClass parent;
  
};

/* used by SEQUENCE_TYPE */
GType bt_sequence_get_type(void);


#endif // BT_SEQUENCE_H

