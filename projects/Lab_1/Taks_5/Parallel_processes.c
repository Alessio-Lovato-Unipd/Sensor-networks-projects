
#include "contiki.h"
#include "dev/leds.h"
#include "dev/etc/rgb-led/rgb-led.h"
#include "dev/button-hal.h"

#include <stdio.h> /* For printf() */


#define SECONDS_BETWEEN_TOGGLES 2

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
PROCESS(timer_process, "timer_process");
PROCESS(Push_button, "Push_button process");
AUTOSTART_PROCESSES(&timer_process, &Push_button);
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


PROCESS_THREAD(Push_button, ev, data)
{


  PROCESS_BEGIN();

  while(1) {

    PROCESS_YIELD();

    if(ev == button_hal_press_event) { //Button is pressed
      rgb_led_set(RGB_LED_WHITE);
    } else if(ev == button_hal_release_event) { //Button is not pressed
      rgb_led_off();
    } 
    
  }

  PROCESS_END();

}
