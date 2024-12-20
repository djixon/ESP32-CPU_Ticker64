#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "soc/soc_caps.h"
#include "CPUticker64.h"

void app_main(void)
{
  CORE0_CPUticker64_init();
  CORE1_CPUticker64_init();

//EXAMPLE 1

  while(true){

      StartClockSnapshot(0); 
       NOP;
      StopClockSnapshot(0); 

      StartClockSnapshot(1); 
       NOP;
       NOP;
      StopClockSnapshot(1);


      ESP_LOGI("main", "test on CORE0: diff %"PRIu64"  test on CORE1 diff %"PRIu64, ClockGetCallibratedDifference(0), ClockGetCallibratedDifference(1));
      vTaskDelay(1000 / portTICK_PERIOD_MS); 

    }


//EXAMPLE 2
/*
  uint8_t iter = 0;
  while(true){ 
   uint64_t val= GetClock64();  // this function reads 64bit clock now at core which called this function, 
                                // because both assembler functions ISR itself and GetTicker64() are now aware of core which calls it
                                // and use different memory locations for higher part of CCOUNT for each core separately

   ESP_LOGI("main", "iter=%"PRIu8" 64bit clock ticker 0x%"PRIx64,iter, val);
   vTaskDelay(1000 / portTICK_PERIOD_MS); 
   iter++;

   if(iter == 30){
     // once we see that upper part is incremented (which means that interrupt resources are allocated and we have 64bit ticker)
     // let's release them
     // and wait to see will that upper part is reset to 0 and backing to 32bit mode 
      CORE0_CPUticker64_free();
      CORE1_CPUticker64_free();
      ESP_LOGI("main", "Upper part is reset to 0 and CCOUNT brang back to 32 bit mode, until ISR is allocated again making it 64bit ");
       
     //if allocation of interrupt is made again later, upper part of CCOUNT on each core will be reset to 0 through setup procedure     

   } else if(iter == 60){
     CORE0_CPUticker64_init();
     CORE1_CPUticker64_init();
   }
  } 

*/
}
