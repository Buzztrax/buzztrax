/** $Id: pattern.h,v 1.3 2004-04-23 12:38:18 ensonic Exp $
 * patterns/sequence
 *
 */

#ifndef BT_PATTERN_H
#define BT_PATTERN_H

typedef struct _BTPattern BTPattern;
typedef BTPattern *BTPatternPtr;
struct _BTPattern {
	BtMachinePtr machine;
	GValue **parameters;
	// GValue parameters[tick_len][machine->dparams_count]
	// with NULL as end-marker in tick_len
	GValue **parameter;
};

#endif /* BT_PATTERN_H */

