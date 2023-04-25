// Program to tutn on a led in a Firefly 
#include "contiki.h"
#include "dev/leds.h"

/*---------------------------------------------------------------------------*/
PROCESS(basic_leds, "Basic leds process");
AUTOSTART_PROCESSES(&basic_leds);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(basic_leds, ev, data)
{

  PROCESS_BEGIN();

  //initialize state of leds
  leds_init();

  //turn on the green leds
  leds_on(LEDS_GREEN);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
