#include "contiki.h"
#include <stdio.h>
#include "sys/etimer.h"

#define SECONDS_BETWEEN_READS 5 //Seconds between each read
#define DHT22_CONF_PIN 9

#include "dev/dht22.h"

PROCESS(example_process, "Example Process");
AUTOSTART_PROCESSES(&example_process);

PROCESS_THREAD(example_process, ev, data)
{
  PROCESS_BEGIN();

  static struct etimer timer;

  while (1) {
    int16_t temperature;
    int16_t humidity;
    int8_t status;

    status = dht22_read_all(&temperature, &humidity);

    if (status == DHT22_SUCCESS) {
      printf("Temperature: %d°C, Humidity: %d%%\n", temperature, humidity);
    } else {
      printf("Failed to read data from DHT22 sensor. Error code: %d\n", status);
    }


    // Delay before the next reading
    etimer_set(&timer, CLOCK_SECOND * SECONDS_BETWEEN_READS);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
  }

  PROCESS_END();
}
