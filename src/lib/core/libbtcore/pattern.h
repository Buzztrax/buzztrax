/* $Id: pattern.h,v 1.5 2004-05-11 16:16:38 ensonic Exp $
 * patterns/sequence
 *
 */

#ifndef BT_PATTERN_H
#define BT_PATTERN_H

typedef struct _BTPattern BTPattern;
typedef BTPattern *BTPatternPtr;
struct _BTPattern {
	BtMachinePtr machine;
	// GValue parameters[tick_len][machine->dparams_count]
	// with NULL as end-marker in tick_len
	GValue **parameters;	// every elemnt is a pointer to GValue or NULL
};

typedef struct _BTTimeLine BTTimeLine;
typedef BTTimeLine *BTTimeLinePtr;
struct _BTTimeLine {
	BTPatternPtr *tracks;	// PatternPtr or NULL
	GValue ***parameter;		// array with positions in Pattern->parameters or NULL
};

#endif /* BT_PATTERN_H */

