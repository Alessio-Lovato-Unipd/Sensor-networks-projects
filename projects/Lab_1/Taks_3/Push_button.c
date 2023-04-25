
#include "contiki.h"
#include "dev/etc/rgb-led/rgb-led.h"
#include "dev/button-hal.h"


/*---------------------------------------------------------------------------*/
PROCESS(Push_button, "Push_button process");
AUTOSTART_PROCESSES(&Push_button);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(Push_button, ev, data)
{


  PROCESS_BEGIN();

  while(1) {

    PROCESS_YIELD();

    if(ev == button_hal_press_event) { //button is being pressed
      rgb_led_set(RGB_LED_WHITE);
    } else if(ev == button_hal_release_event) { //Button has bean released
      rgb_led_off();
    } 
    
  }

  PROCESS_END();

}
/*---------------------------------------------------------------------------*/
