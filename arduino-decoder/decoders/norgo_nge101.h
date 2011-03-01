#ifndef DECODERS_NORGO_NGE101_H
#define DECODERS_NORGO_NGE101_H

#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////

class NorgoNge101
{
public:
  NorgoNge101();

  void decode(uint8_t level, uint32_t duration);

private:
  enum EPulse { PULSE_NOISE, PULSE_SHORT, PULSE_LONG } __attribute__((packed));

  void reset();

  EPulse decodePulse(uint8_t level, uint32_t duration);
  int8_t decodeManchester(EPulse pulse);
  void decodePayload();
  void decodeShortFrame();
  void decodeLongFrame();

  uint16_t nextChecksumMask(uint16_t mask);
  uint16_t calcChecksum(uint16_t m_header, uint64_t m_payload, uint8_t payloadlen);

  enum EState { STATE_PRE0, STATE_PRE1, STATE_PRE2, STATE_PRE3, STATE_HEADER, STATE_PAYLOAD, STATE_CHECKSUM } __attribute__((packed));
  EState m_state;

  uint8_t m_datalen;
  uint16_t m_header;
  uint64_t m_payload;
  uint16_t m_checksum;
  bool m_lastPulseWasShort;

  uint32_t m_lastShortFrameAt;
};

///////////////////////////////////////////////////////////////////////////////
#endif
