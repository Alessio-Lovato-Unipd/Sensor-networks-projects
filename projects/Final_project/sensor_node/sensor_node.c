#include "contiki.h"
#include <stdio.h>
#include "sys/etimer.h"

#define SECONDS_BETWEEN_READS 3 //Seconds between each read: >= 2

// DHT22 CONFIGURATION
#define DHT22_CONF_PIN		I2C_SCL_PIN
#define DHT22_CONF_PORT 	I2C_SCL_PORT
#include "dev/dht22.h"


PROCESS(dht22_process, "DHT22 process");
AUTOSTART_PROCESSES(&dht22_process);

PROCESS_THREAD(dht22_process, ev, data)
{
  PROCESS_BEGIN();

  static struct etimer timer_sensor;
  SENSORS_ACTIVATE(dht22); //activation of the sensor
  int16_t temperature, humidity;

  while(1) {
	
    etimer_set(&timer_sensor, CLOCK_SECOND * SECONDS_BETWEEN_READS);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer_sensor));

    /* The standard sensor API may be used to read sensors individually, using
     * the `dht22.value(DHT22_READ_TEMP)` and `dht22.value(DHT22_READ_HUM)`,
     * however a single read operation (5ms) returns both values, so by using
     * the function below we save one extra operation
     */
    if(dht22_read_all(&temperature, &humidity) != DHT22_ERROR) {
      printf("Temperature %02d.%02d ºC, ", temperature / 10, temperature % 10);
      printf("Humidity %02d.%02d RH\n", humidity / 10, humidity % 10);
    } else {
      printf("Failed to read data from DHT22 sensor.");
    }
  }

  PROCESS_END();
}
