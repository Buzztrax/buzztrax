/** $Id: pattern.h,v 1.2 2004-04-19 18:30:42 ensonic Exp $
 * patterns/sequence
 *
 */

#ifndef BT_PATTERN_H
#define BT_PATTERN_H

typedef struct _BTPattern BTPattern;
typedef BTPattern *BTPatternPtr;
struct _BTPattern {
	BtMachinePtr machine;
	// parameters[tick-len] // with NULL end-marker
	// *parameter
};

#endif /* BT_PATTERN_H */

