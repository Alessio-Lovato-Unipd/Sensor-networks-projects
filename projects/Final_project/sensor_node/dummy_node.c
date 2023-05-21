#include "contiki.h"
#include <stdio.h>
#include <limits.h>
#include "sys/etimer.h"
#include <time.h>
#include <stdlib.h>
#include "lpm.h"


#include "sys/energest.h"

#define DUMMY
/*********************************   SENSOR   *******************************************/

// Sensor acquisition time
#define SECONDS_BETWEEN_READS 	(3 * CLOCK_SECOND)//Seconds between each read: >= 2

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

bool receiver_busy = false;
#define ROUTING_TIME (CLOCK_SECOND * 10)
/*******************************   PROCESSES   ********************************************/

PROCESS(main_process, "general process");
PROCESS(dht22_process, "DHT22 process");
PROCESS(send_data_process, "Data sending process");
PROCESS(energest_process, "Energest process");


// Declare an event to signal the completion of the second process
static process_event_t measure_finished;
static process_event_t transmission_finished;

// Process autostart
AUTOSTART_PROCESSES(&main_process, &energest_process);

/***********************************   THREADS    *******************************************/
PROCESS_THREAD(main_process, ev, data)
{
	// Timer
	static struct etimer timer_sensing;
	PROCESS_BEGIN();

	/* Initialize low powe control*/
	lpm_init();

	/* Initialize UDP connection */
	simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
											UDP_SERVER_PORT, udp_sensor_callback);
	
	//Low Power settings
	lpm_set_max_pm(LPM_PM2);
	NETSTACK_RADIO.off();
	while (1)
	{
		lpm_enter();
		etimer_set(&timer_sensing, SECONDS_BETWEEN_READS);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer_sensing));
		lpm_exit();

		//measure process
		process_start(&dht22_process, NULL);

		// Wait for the measurement process to finish
		PROCESS_WAIT_EVENT_UNTIL(ev == measure_finished);

		process_start(&send_data_process, NULL);

		//if not sending data terminate process
		PROCESS_WAIT_EVENT_UNTIL(ev == transmission_finished);

	}

	PROCESS_END();
}
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

/**********************************************************************************/
void udp_sensor_callback(struct simple_udp_connection *c,
				 const uip_ipaddr_t *sender_addr,
				 uint16_t sender_port,
				 const uip_ipaddr_t *receiver_addr,
				 uint16_t receiver_port,
				 const uint8_t *data,
				 uint16_t datalen)
{
	receiver_busy = true;
	LOG_INFO("Received response from ");
	LOG_INFO_6ADDR(sender_addr);
#if LLSEC802154_CONF_ENABLED
	LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
#endif
	LOG_INFO_("\n");
	receiver_busy = false;
}

PROCESS_THREAD(send_data_process, ev, data)
{
	static struct etimer routing_timer;

	PROCESS_BEGIN();
	//Turn on radio
	NETSTACK_RADIO.on();

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
		} else {
			LOG_INFO("Not reachable yet\n");
		}

	// Keep up receiver 10 seconds to receive data
	etimer_set(&routing_timer, ROUTING_TIME);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&routing_timer));

	// turn off radio again
	NETSTACK_RADIO.off();

	//signal process ended
	process_post(&main_process, transmission_finished, NULL);


	PROCESS_END();
}

/**********************************************************************************/
static inline unsigned long
to_seconds(uint64_t time)
{
  return (unsigned long)(time / ENERGEST_SECOND);
}

PROCESS_THREAD(energest_process, ev, data)
{
  static struct etimer periodic_timer;

  PROCESS_BEGIN();

  etimer_set(&periodic_timer, CLOCK_SECOND * 30);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);

    /*
     * Update all energest times. Should always be called before energest
     * times are read.
     */
    energest_flush();

    printf("\nEnergest:\n");
    printf(" CPU          %4lus LPM      %4lus DEEP LPM %4lus  Total time %lus\n",
           to_seconds(energest_type_time(ENERGEST_TYPE_CPU)),
           to_seconds(energest_type_time(ENERGEST_TYPE_LPM)),
           to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()));
    printf(" Radio LISTEN %4lus TRANSMIT %4lus OFF      %4lus\n",
           to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)),
           to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()
                      - energest_type_time(ENERGEST_TYPE_TRANSMIT)
                      - energest_type_time(ENERGEST_TYPE_LISTEN)));
  }

  PROCESS_END();
}