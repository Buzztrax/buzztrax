/** $Id: pattern.h,v 1.4 2004-04-26 14:43:29 ensonic Exp $
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

