
#include "contiki.h"
#include "dev/leds.h"


#define SECONDS_BETWEEN_TOGGLES 2

/*---------------------------------------------------------------------------*/
PROCESS(timer_process, "timer_process");
AUTOSTART_PROCESSES(&timer_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(timer_process, ev, data)
{

  static struct etimer timer;

  PROCESS_BEGIN();

  /* Setup a periodic timer that expires after 10 seconds. */
  etimer_set(&timer, CLOCK_SECOND * SECONDS_BETWEEN_TOGGLES);

  while(1) {

    leds_toggle(LEDS_GREEN);

    /* Wait for the periodic timer to expire and then restart the timer. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);    
  }

  PROCESS_END();

}
/*---------------------------------------------------------------------------*/
