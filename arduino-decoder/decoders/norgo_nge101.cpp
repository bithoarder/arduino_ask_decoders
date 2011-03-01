#include "norgo_nge101.h"
#include "../couchdb.h"
#include "../progmem.h"

#include <stdio.h>
#include <wiring.h>

///////////////////////////////////////////////////////////////////////////////

NorgoNge101::NorgoNge101()
{
  m_lastShortFrameAt = 0;
  reset();
}

void NorgoNge101::reset()
{
  m_datalen = 0;
  m_header = 0;
  m_payload = 0;
  m_checksum = 0;
  m_lastPulseWasShort = false;
  m_state = STATE_PRE0;
}

void NorgoNge101::decode(uint8_t level, uint32_t duration)
{
  EPulse pulse = decodePulse(level, duration);
  int8_t bit = decodeManchester(pulse);
  if(bit < 0) return;

  switch(m_state)
  {
    case STATE_PRE0:
      if(bit == 1) m_state = STATE_PRE1;
      break;

    case STATE_PRE1:
      if(bit == 0) m_state = STATE_PRE2;
      break;

    case STATE_PRE2:
      if(bit == 1) m_state = STATE_PRE3;
      else m_state = STATE_PRE0;
      break;

    case STATE_PRE3:
      if(bit == 0)
      {
        m_datalen = 0;
        m_state = STATE_HEADER;
      }
      else
      {
        m_state = STATE_PRE0;
      }
      break;

    case STATE_HEADER:
      m_header |= uint16_t(bit)<<m_datalen;
      m_datalen += 1;
      if(m_datalen == 12)
      {
        m_datalen = 0;
        m_state = STATE_PAYLOAD;
      }
      break;

    case STATE_PAYLOAD:
      m_payload |= uint64_t(bit)<<m_datalen;
      m_datalen += 1;
      if(m_datalen == (m_header&1?36:20))
      {
        m_datalen = 0;
        m_state = STATE_CHECKSUM;
      }
      break;

    case STATE_CHECKSUM:
      m_checksum |= uint16_t(bit)<<m_datalen;
      m_datalen += 1;
      if(m_datalen == 15)
      {
        decodePayload();
        reset();
      }
      break;
  }
}

void NorgoNge101::decodePayload()
{
  bool longframe = m_header&1;
  uint8_t payloadlen = longframe?36:20;
  uint16_t cchecksum = calcChecksum(m_header, m_payload, payloadlen);

  if(m_checksum == cchecksum)
  {
    if(longframe)
      decodeLongFrame();
    else
      decodeShortFrame();
  }
}

void NorgoNge101::decodeShortFrame()
{
  uint32_t now = millis();

  uint32_t pulseDuration = m_payload&0x7ffffl;
  uint32_t realtimeUsage = 60l*60l*1024l / pulseDuration;

  if(uint32_t(now-m_lastShortFrameAt) >= 5000)
  {
    printf_P(PSTR("RealTime: %lu W\n"), realtimeUsage);

    couch::Document doc;
    doc.type = couch::TYPE_POWERUSE_REALTIME;
    doc.RealtimePowerUse.channel = ((m_header>>1)&0b111) + 1;
    doc.RealtimePowerUse.id = (m_header>>4)&0b11111111;
    doc.RealtimePowerUse.realtime = realtimeUsage;
    couch::queueDocument(doc);

    m_lastShortFrameAt = now;
  }
}

void NorgoNge101::decodeLongFrame()
{
  uint64_t pulseCount = m_payload&0x3ffffffffll;

  printf_P(PSTR("Count: %lu W\n"), uint32_t(pulseCount));

  couch::Document doc;
  doc.type = couch::TYPE_POWERUSE_CUMULATIVE;
  doc.CumulativePowerUse.channel = ((m_header>>1)&0b111) + 1;
  doc.CumulativePowerUse.id = (m_header>>4)&0b11111111;
  doc.CumulativePowerUse.cumulative = pulseCount;
  couch::queueDocument(doc);
}

NorgoNge101::EPulse NorgoNge101::decodePulse(uint8_t level, uint32_t duration)
{
  if(duration>=300 && duration<=600) return PULSE_SHORT;
  if(duration>=800 && duration<=1100) return PULSE_LONG;
  return PULSE_NOISE;
}

int8_t NorgoNge101::decodeManchester(EPulse pulse)
{
  if(pulse==PULSE_SHORT)
  {
    m_lastPulseWasShort = !m_lastPulseWasShort;
    if(!m_lastPulseWasShort)
      return 0;
  }
  else if(pulse==PULSE_LONG && !m_lastPulseWasShort)
  {
    return 1;
  }
  else
  {
    reset();
  }

  return -1;
}

uint16_t NorgoNge101::nextChecksumMask(uint16_t mask)
{
  static const uint16_t checksum_taps[] = {
    0x4880, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x2080, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000};
  uint16_t next_mask = mask>>1;
  for(uint8_t i=0; i<15; i++)
    if(mask&(1<<i))
      next_mask ^= checksum_taps[i];
  return next_mask;
}

uint16_t NorgoNge101::calcChecksum(uint16_t m_header, uint64_t m_payload, uint8_t payloadlen)
{
  uint16_t checksum = 0;
  uint16_t mask = 0x0001;
  for(uint8_t i=payloadlen-1; i!=0xff; i--)
  {
    mask = nextChecksumMask(mask);
    if((m_payload>>i)&1)
      checksum ^= mask;
  }
  for(uint8_t i=12-1; i!=0xff; i--)
  {
    mask = nextChecksumMask(mask);
    if((m_header>>i)&1)
      checksum ^= mask;
  }
  return checksum;
}
