#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"

#include "net/mac/tsch/tsch.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

//NETWORK DEFINITION
#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

// Time of connection to network after MAX_NUM_OF_MISSING
#define RECONNECTION_SECONDS 10

// Max number of packets not delivered
#define MAX_NUM_OF_MISSING 5

// Time between reconnection after MAX_NUM_OF_MISSING
#define SECONDS_BETWEEN_RECONNECTION 20


// PACKETS EXCHANGED 
struct packet {
    int16_t temperature, humidity;
};
