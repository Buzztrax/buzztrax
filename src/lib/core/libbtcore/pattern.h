/** $Id: pattern.h,v 1.1 2004-04-19 13:43:37 ensonic Exp $
 * patterns/sequence
 *
 */

#ifndef BT_PATTERN_H
#define BT_PATTERN_H

typedef struct BTPattern BTPattern;
typedef BTPattern *BTPatternPtr;
struct BTPattern {
	BtMachinePtr machine;
	// parameters[tick-len] // with NULL end-marker
	// *parameter
};

#endif /* BT_PATTERN_H */

