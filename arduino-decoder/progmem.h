#include <avr/pgmspace.h>

// a workaround for a compiler warning
// see http://old.nabble.com/C%2B%2B-with-PSTR-td26664456.html

#undef PROGMEM
#define PROGMEM __attribute__(( section(".progmem.data") ))
