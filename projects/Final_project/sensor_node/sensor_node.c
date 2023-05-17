#include "contiki.h"
#include <stdio.h>
#include <limits.h>
#include "sys/etimer.h"

/*********************************   SENSOR   *******************************************/

// Sensor acquisition time
#define SECONDS_BETWEEN_READS 	(3 * CLOCK_SECOND)//Seconds between each read: >= 2

// DHT22 CONFIGURATION
#define DHT22_CONF_PIN		I2C_SCL_PIN
#define DHT22_CONF_PORT 	I2C_SCL_PORT
#include "dev/dht22.h"

/**********************************   NETWORK   ****************************************/
// NETWORK CONFIGURATION
#include "../network_configuration.h"

/* Data exchanged:
	- Temperature
	-Humidity
*/
struct packet data_to_send, ack;
static uint32_t missed_tx_count = 0;
struct simple_udp_connection udp_conn;
uip_ipaddr_t dest_ipaddr;

/*******************************   PROCESSES   ********************************************/

PROCESS(dht22_process, "DHT22 process");
PROCESS(send_data_process, "Data sending process");


// Declare an event to signal the completion of the second process
static process_event_t transmission_finished;

// Process autostart
AUTOSTART_PROCESSES(&dht22_process);


/***********************************   THREADS    *******************************************/
PROCESS_THREAD(dht22_process, ev, data)
{
  PROCESS_BEGIN();

  // Timer
  static struct etimer timer_sensor;

  // Activation of the sensor
  SENSORS_ACTIVATE(dht22);

  while(1) {
	etimer_set(&timer_sensor, SECONDS_BETWEEN_READS);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer_sensor));

    /* The standard sensor API may be used to read sensors individually, using
     * the `dht22.value(DHT22_READ_TEMP)` and `dht22.value(DHT22_READ_HUM)`,
     * however a single read operation (5ms) returns both values, so by using
     * the function below we save one extra operation
     */
	uint8_t status = dht22_read_all(&data_to_send.temperature, &data_to_send.humidity);
    if(status != DHT22_ERROR) {
      LOG_INFO("Temperature %02d.%02d ºC, Humidity %02d.%02d RH\n", data_to_send.temperature / 10, data_to_send.temperature % 10,
                                                                         data_to_send.humidity / 10, data_to_send.humidity % 10);

	  //send packet to master node
	  process_start(&send_data_process, NULL);

	  // Wait for the transmission process to finish
  	  PROCESS_WAIT_EVENT_UNTIL(ev == transmission_finished);

    } else {
      LOG_INFO("Failed to read data from DHT22 sensor.");
	  
	  data_to_send.temperature = SHRT_MIN;
	  data_to_send.humidity = SHRT_MIN;
    }
  }

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
  LOG_INFO("Received response from ");
  LOG_INFO_6ADDR(sender_addr);
#if LLSEC802154_CONF_ENABLED
  LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
#endif
  LOG_INFO_("\n");
}

PROCESS_THREAD(send_data_process, ev, data)
{

  PROCESS_BEGIN();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_sensor_callback);

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
	  missed_tx_count++;
    }

	//signal process ended
  process_post(&dht22_process, transmission_finished, NULL);


  PROCESS_END();
}

/**********************************************************************************/