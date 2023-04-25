
#include "contiki.h"
#include "dev/etc/rgb-led/rgb-led.h"
#include "clock.h"
#include <stdbool.h> /* For booleans */


//Macro to define timer
#define TICKS_PER_SECOND 128 //default number of ticks in the clock

/*---------------------------------------------------------------------------*/
PROCESS(basic_leds_rgb, "RGB leds process");
AUTOSTART_PROCESSES(&basic_leds_rgb);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(basic_leds_rgb, ev, data)
{

  PROCESS_BEGIN();

  //toggle a white led every second
  bool state = false;
  while(1) {
    state = !state;
    if (state)
      rgb_led_set(RGB_LED_WHITE);
    else 
      rgb_led_off();
    clock_wait(TICKS_PER_SECOND);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
