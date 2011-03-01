#define NET_ADDR EEPROM
//#define NET_ADDR {192, 168, 1, 200}
//#define NET_MAC {0x90, 0xa2, 0xda, 0x00, 0x25, 0x65}

#define NET_GATEWAY {0, 0, 0, 0}
#define NET_MASK {255, 255, 255, 0}

#define COUCHDB_ADDR {192, 168, 1, 40}
#define COUCHDB_PORT 5984
#define COUCHDB_DBNAME "ask"
#define COUCHDB_DESIGNNAME "couchapp"

#define RADIO_PIN 2
#if RADIO_PIN == 2
#define RADIO_INTERRUPT 0
#else
#error Unknown radio pin/interrupt
#endif
