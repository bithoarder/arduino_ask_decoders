#include <wiring.h>
#include <HardwareSerial.h>
#include <stdio.h>

#include "decode.h"
#include "couchdb.h"
#include "progmem.h"

///////////////////////////////////////////////////////////////////////////////

static int uart_putchar(char c, FILE *stream) { Serial.write(c); return 1; }
static int uart_getchar(FILE *stream) { return Serial.read(); }
static FILE mystdout = {NULL, 0, _FDEV_SETUP_RW, 0, 0, uart_putchar, &uart_getchar, NULL};

void setup()
{
  stdin = &mystdout;
  stdout = &mystdout;
  Serial.begin(115200);
  printf_P(PSTR("GO\n"));

  ask::init();
  couch::init();
}

void loop()
{
  ask::decode();
  couch::update();
  
  int cmd = getchar();
  if(cmd == 'd')
  {
    ask::g_printRecvBits = !ask::g_printRecvBits;
    printf_P(PSTR("g_printRecvBits=%u\n"), ask::g_printRecvBits);
  }
}
