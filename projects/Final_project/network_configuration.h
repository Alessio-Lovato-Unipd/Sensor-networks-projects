#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

//NETWORK DEFINITION
#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define SECONDS_BETWEEN_TRANSMISSION 3
#define SEND_INTERVAL		  (SECONDS_BETWEEN_TRANSMISSION * CLOCK_SECOND)



// UDP receiver callback

void udp_sensor_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen);

// PACKETS EXCHANGED 
struct packet {
    int16_t temperature, humidity;
};
