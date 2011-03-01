#ifndef ASK_COUCH_H
#define ASK_COUCH_H

#include <stdint.h>
#include <avr/pgmspace.h>

///////////////////////////////////////////////////////////////////////////////

namespace couch
{

enum EDocType
{
  TYPE_POWERUSE_REALTIME,
  TYPE_POWERUSE_CUMULATIVE,
} __attribute__((packed));

struct Document
{
  EDocType type;

  union
  {
    struct { uint8_t channel; uint8_t id; uint32_t realtime; } RealtimePowerUse;
    struct { uint8_t channel; uint8_t id; uint32_t cumulative; } CumulativePowerUse;
  };

  uint8_t formatDoc(char *dst, uint8_t dstlen) const;
  const prog_char *url() const;
};

void init();
void update();

void queueDocument(const Document &doc);

}

///////////////////////////////////////////////////////////////////////////////
#endif
