#include "contiki.h"
#include <stdio.h>
#include <limits.h>
#include "sys/etimer.h"
#include <stdlib.h>

/*********************************   NODE OPTION   *******************************************/

#define DUMMY

/*********************************   SENSOR   *******************************************/

// Sensor acquisition time
#define SECONDS_BETWEEN_READS 	(60 * CLOCK_SECOND)//Seconds between each read: >= 2

#ifndef DUMMY
	// DHT22 CONFIGURATION
	#define DHT22_CONF_PIN		I2C_SCL_PIN
	#define DHT22_CONF_PORT 	I2C_SCL_PORT
	#include "dev/dht22.h"
#endif

/**********************************   NETWORK   ****************************************/
// NETWORK CONFIGURATION
#include "../network_configuration.h"

/* Data exchanged:
	- Temperature
	-Humidity
*/
struct packet data_to_send;
struct simple_udp_connection udp_conn;
uip_ipaddr_t dest_ipaddr;

uint8_t missed_delivery = 0; // Number of packet not delivery to the receiver node
bool ack = false;	// Variable to identify if node obtains the ack from receiver

static void udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen);

/*******************************   PROCESSES   ********************************************/

PROCESS(main_process, "general process");
PROCESS(dht22_process, "DHT22 process");
PROCESS(send_data_process, "Data sending process");


// Declare an event to signal the completion of the second process
static process_event_t measure_finished;
static process_event_t transmission_finished;

// Process autostart
AUTOSTART_PROCESSES(&main_process);


/***********************************  	MAIN THREAD    *******************************************/
PROCESS_THREAD(main_process, ev, data)
{
	// Timer
	static struct etimer timer_sensing;
	PROCESS_BEGIN();

	/* Initialize UDP connection */
  	simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);

		
	while (1)
	{
		//measure process
		process_start(&dht22_process, NULL);

		// Wait for the measurement process to finish
		PROCESS_WAIT_EVENT_UNTIL(ev == measure_finished);

		process_start(&send_data_process, NULL);

		//if not sending data terminate process
		PROCESS_WAIT_EVENT_UNTIL(ev == transmission_finished);

		// Wait time before next measurement
		etimer_set(&timer_sensing, SECONDS_BETWEEN_READS);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer_sensing));

	}
	PROCESS_END();
}

/*******************************	MEASUREMENT THREAD	*********************************************/

PROCESS_THREAD(dht22_process, ev, data)
{
	PROCESS_BEGIN();

	#ifndef DUMMY
		// Activation of the sensor
		SENSORS_ACTIVATE(dht22);

		/* The standard sensor API may be used to read sensors individually, using
		* the `dht22.value(DHT22_READ_TEMP)` and `dht22.value(DHT22_READ_HUM)`,
		* however a single read operation (5ms) returns both values, so by using
		* the function below we save one extra operation
		*/

		if(dht22_read_all(&data_to_send.temperature, &data_to_send.humidity) != DHT22_ERROR) {
			LOG_INFO("Temperature %02d.%02d ºC, Humidity %02d.%02d RH\n", data_to_send.temperature / 10, data_to_send.temperature % 10,
																			data_to_send.humidity / 10, data_to_send.humidity % 10);
			} else {
			LOG_INFO("Failed to read data from DHT22 sensor.\n");
			data_to_send.temperature = SHRT_MIN;
			data_to_send.humidity = SHRT_MIN;
		}
	#else
		data_to_send.temperature = rand() % 500;      // Returns a pseudo-random integer between 0 and RAND_MAX.
		data_to_send.humidity = rand() % 1000;
		LOG_INFO("Temperature %02d.%02d ºC, Humidity %02d.%02d RH\n", data_to_send.temperature / 10, data_to_send.temperature % 10,
																		data_to_send.humidity / 10, data_to_send.humidity % 10);
	#endif   

	//signal process ended
	process_post(&main_process, measure_finished, NULL);

	PROCESS_END();
}

/*******************************	NETWORK THREAD	*********************************************/
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{

  ack = true;
  LOG_INFO("Received response from ");
  LOG_INFO_6ADDR(sender_addr);
#if LLSEC802154_CONF_ENABLED
  LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
#endif
  LOG_INFO_("\n");
}

PROCESS_THREAD(send_data_process, ev, data)
{
	static struct etimer timer;

	PROCESS_BEGIN();

	ack = false;	// set new ack to be received

	if(NETSTACK_ROUTING.node_is_reachable() &&
	NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {

			/* Send to DAG root */
			LOG_INFO("Sending request to ");
			LOG_INFO_6ADDR(&dest_ipaddr);
			LOG_INFO_("\n");
			simple_udp_sendto(&udp_conn, &data_to_send, sizeof(data_to_send), &dest_ipaddr);
			LOG_INFO("Data sent to: ");
			LOG_INFO_6ADDR(&dest_ipaddr);
			LOG_INFO_("\n");
			// Keep up receiver for ROUTING_TIME seconds to exchange data from other packets
			etimer_set(&timer, ROUTING_TIME);
			PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
		} else {
			if (NETSTACK_ROUTING.node_is_reachable()){
				LOG_ERR("Node not reachable\n");
			} else {
				LOG_ERR("DODAG not reachable\n");
			}
		}

	if (!ack)	// Packet not delivered
		{	
		missed_delivery++;
		LOG_INFO("MISSING: %u\n", missed_delivery);
		if (missed_delivery >= MAX_NUM_OF_MISSING)
		{
			while(!ack) { //Try to make a connection every RECONNECTION_SECONDS seconds
			LOG_INFO("Retry a new connection to the network...\n");

			if(NETSTACK_ROUTING.node_is_reachable() &&
					NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
				simple_udp_sendto(&udp_conn, &data_to_send, sizeof(data_to_send), &dest_ipaddr);
				etimer_set(&timer, RECONNECTION_SECONDS * CLOCK_SECOND);
				PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
			} else {
				LOG_INFO("No connection possible\n");
			}

			if (!ack) {	//ACK not received yet
				LOG_INFO("New try in %u seconds\n", SECONDS_BETWEEN_RECONNECTION);

				// Wait some seconds before trying again
				etimer_set(&timer, SECONDS_BETWEEN_RECONNECTION * CLOCK_SECOND);	//Routing timer is used only to not create a new variable.
				PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));	// Wait time before sending packet again
			} else {
				LOG_INFO("Connection successfull\n");
				missed_delivery = 0;
			}
			}
		}
		} else {
		missed_delivery = 0;
		}

	//signal process ended
	process_post(&main_process, transmission_finished, NULL);


	PROCESS_END();
}
