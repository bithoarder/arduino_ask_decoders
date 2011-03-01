#include <wiring.h>
#include <stdio.h>

#include "config.h"
#include "decode.h"
#include "progmem.h"

#include "decoders/norgo_nge101.h"

bool ask::g_printRecvBits = false;

///////////////////////////////////////////////////////////////////////////////

static uint16_t g_pulseQueue[128];
static uint8_t  g_pulseQueueHead = 0;
static uint8_t  g_pulseQueueTail = 0;
static uint32_t g_lastPulseAt;

static void signal();

static NorgoNge101 g_norgoNge101;

///////////////////////////////////////////////////////////////////////////////

void ask::init()
{
  pinMode(RADIO_PIN, INPUT); // the pin where the data output of the rf module is connected

  g_lastPulseAt = micros();

  attachInterrupt(RADIO_INTERRUPT, signal, CHANGE);
}

static void signal()
{
  uint8_t level = digitalRead(RADIO_PIN);
  if(level == (g_pulseQueueHead&1))
  {
    // we got the same pin state twice in a row, ignore it.
    return;
  }

  uint32_t now = micros();
  uint32_t dt = now - g_lastPulseAt;
  g_lastPulseAt = now;

  g_pulseQueue[g_pulseQueueHead] = dt<=0xffff ? dt : 0xffff;
  g_pulseQueueHead = (g_pulseQueueHead+1)&(sizeof(g_pulseQueue)/sizeof(g_pulseQueue[0])-1);
}

void ask::decode()
{
  while(g_pulseQueueHead != g_pulseQueueTail)
  {
    uint8_t level = g_pulseQueueTail&1;
    uint16_t duration = g_pulseQueue[g_pulseQueueTail];
    g_pulseQueueTail = (g_pulseQueueTail+1)&(sizeof(g_pulseQueue)/sizeof(g_pulseQueue[0])-1);

    if(g_printRecvBits)
    {
      printf_P(PSTR("Recv %u %u\n"), level, duration);
    }

    g_norgoNge101.decode(level, duration);
  }
}

