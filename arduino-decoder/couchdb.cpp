#include <stdio.h>

#include <avr/eeprom.h>

#include <SPI.h>
#include <Ethernet.h>
#include <utility/socket.h>

#include "couchdb.h"
#include "config.h"
#include "progmem.h"

using namespace couch;

///////////////////////////////////////////////////////////////////////////////

enum EState { STATE_IDLE, STATE_CONNECTING, STATE_CLOSE_WAIT, STATE_FAILED, STATE_FAILED_WAIT } __attribute__((packed));
static EState g_state;

static uint32_t g_pauseUntil;

static uint8_t g_head;
static uint8_t g_count;
static const uint8_t QUEUE_LEN = 16; // must be power of 2
static Document g_documentQueue[QUEUE_LEN]; // 10 bytes each?

#define SOCKFD 0

static void stateIdle();
static void stateConnecting();
static void stateCloseWait();
static void stateFailed();
static void stateFailedWait();

///////////////////////////////////////////////////////////////////////////////

uint8_t Document::formatDoc(char *dst, uint8_t dstlen) const
{
  switch(type)
  {
    case TYPE_POWERUSE_REALTIME:
      return snprintf_P(dst, dstlen, PSTR("chan=%u;id=%u;realtime=%lu"),
                        RealtimePowerUse.channel, RealtimePowerUse.id, RealtimePowerUse.realtime);
    case TYPE_POWERUSE_CUMULATIVE:
      return snprintf_P(dst, dstlen, PSTR("chan=%u;id=%u;cumulative=%lu"),
                        CumulativePowerUse.channel, CumulativePowerUse.id, CumulativePowerUse.cumulative);
    default:
      return 0;
  }
}

const prog_char *Document::url() const
{
  switch(type)
  {
    case TYPE_POWERUSE_REALTIME:
      return PSTR("/" COUCHDB_DBNAME "/_design/" COUCHDB_DESIGNNAME "/_update/realtime");
    case TYPE_POWERUSE_CUMULATIVE:
      return PSTR("/" COUCHDB_DBNAME "/_design/" COUCHDB_DESIGNNAME "/_update/cumulative");
    default:
      return PSTR("");
  }
}

///////////////////////////////////////////////////////////////////////////////

void couch::init()
{
  W5100.init();
#if NET_ADDR == EEPROM
  uint8_t ip_mac[4+6];
  eeprom_read_block(ip_mac, (const void*)0x3f0, sizeof(ip_mac));
  if(ip_mac[0] == 255)
  {
    printf_P(PSTR("PANIC: missing IP address"));
    for(;;) /**/ ;
  }
  W5100.setMACAddress(ip_mac+4);
  W5100.setIPAddress(ip_mac);
#else
  uint8_t mac[6] = NET_MAC;
  W5100.setMACAddress(mac);
  uint8_t ip[4] = NET_ADDR;
  W5100.setIPAddress(ip);
#endif

  uint8_t mask[4] = NET_MASK;
  W5100.setSubnetMask(mask);

  uint8_t gateway[4] = NET_GATEWAY;
  W5100.setGatewayIp(gateway);

  g_head = 0;
  g_count = 0;

  g_state = STATE_IDLE;
}

void couch::update()
{
  switch(g_state)
  {
    case STATE_IDLE: return stateIdle();
    case STATE_CONNECTING: return stateConnecting();
    case STATE_CLOSE_WAIT: return stateCloseWait();
    case STATE_FAILED: return stateFailed();
    case STATE_FAILED_WAIT: return stateFailedWait();
  }
}

void couch::queueDocument(const Document &doc)
{
  g_documentQueue[g_head] = doc;
  g_head = (g_head+1)&(QUEUE_LEN-1);
  if(g_count < QUEUE_LEN) g_count += 1;
}

static void stateIdle()
{
  if(g_count != 0)
  {
    printf_P(PSTR("CouchDB connecting\n"));

    socket(SOCKFD, SnMR::TCP, 1100, 0);

    uint8_t addr[4] = COUCHDB_ADDR;
    connect(SOCKFD, addr, COUCHDB_PORT);

    g_state = STATE_CONNECTING;
  }
}

static void stateConnecting()
{
  uint8_t sockstatus = W5100.readSnSR(SOCKFD);
  if(sockstatus == SnSR::ESTABLISHED)
  {
    printf_P(PSTR("CouchDB connected, sending doc\n"));

    uint8_t tail = (g_head-g_count)&(QUEUE_LEN-1);

    char buffer[64];

    // get the length of the document, it's needed in the http header.
    uint8_t bodylen = g_documentQueue[tail].formatDoc(buffer, sizeof(buffer));

    uint8_t doclen = snprintf_P(buffer, sizeof(buffer), PSTR("POST %S HTTP/1.0\r\n"), g_documentQueue[tail].url());
    send(SOCKFD, (const uint8_t*)buffer, doclen);

    doclen = snprintf_P(buffer, sizeof(buffer), PSTR("Content-Type: application/x-www-form-urlencoded\r\n"));
    send(SOCKFD, (const uint8_t*)buffer, doclen);

    doclen = snprintf_P(buffer, sizeof(buffer), PSTR("Content-Length: %u\r\n\r\n"), bodylen);
    send(SOCKFD, (const uint8_t*)buffer, doclen);

    bodylen = g_documentQueue[tail].formatDoc(buffer, sizeof(buffer));
    send(SOCKFD, (const uint8_t*)buffer, bodylen);

    g_count -= 1;

    g_state = STATE_CLOSE_WAIT;
  }
  else if(sockstatus == SnSR::CLOSED)
  {
    printf_P(PSTR("CouchDB connection failed\n"));
    g_state = STATE_FAILED;
  }
}

static void stateCloseWait()
{
  uint8_t sockstatus = W5100.readSnSR(SOCKFD);
  if(sockstatus == SnSR::CLOSE_WAIT || sockstatus == SnSR::CLOSED)
  {
    printf_P(PSTR("CouchDB conection closed\n"));
    close(SOCKFD);
    g_state = STATE_IDLE;
  }
}

static void stateFailed()
{
  close(SOCKFD);
  g_pauseUntil = millis() + 5000;
  g_state = STATE_FAILED_WAIT;
}

static void stateFailedWait()
{
  if(uint32_t(millis()-g_pauseUntil) >= 0)
    g_state = STATE_IDLE;
}

///////////////////////////////////////////////////////////////////////////////
