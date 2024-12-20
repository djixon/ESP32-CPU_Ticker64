#include "sdkconfig.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_types.h>
#include "esp_err.h"
#include "esp_intr_alloc.h"
#include "esp_attr.h"
#include "esp_freertos_hooks.h"
#include "esp_log.h"
#include "xtensa/core-macros.h"
#include "esp_ipc_isr.h"
#include "CPUticker64.h"


const char* TAG = "CPUticker64";

volatile Ticker64_t tickers[SOC_CPU_CORES_NUM];

//c prototypes for assembler functions
extern uint64_t GetTicker64(void);
extern void GetTicker64FromIsr(void* res);
extern void reset_ccount_high(void);
extern void reset_ccount_high_fromISR(void*);
// implementation
void IRAM_ATTR StartClockSnapshot(const uint8_t cpu_core_id)
{
  if(cpu_core_id < SOC_CPU_CORES_NUM){
   if(!tickers[cpu_core_id].SnapshotStartTaken){
    tickers[cpu_core_id].SnapshotStartTaken = true;
    if(esp_cpu_get_core_id() != cpu_core_id){ 
        esp_ipc_isr_call_blocking(GetTicker64FromIsr, (uint64_t *) &(tickers[cpu_core_id].ClockStartValue)); 
     } else {
      tickers[cpu_core_id].ClockStartValue=GetTicker64();        
     }
   } else {
  	  ESP_LOGW(TAG, "Already started");
   }
  } else {
	  ESP_LOGE(TAG,"CPU id larger than number of cores in SOC");
  }
}
void IRAM_ATTR StopClockSnapshot(const uint8_t cpu_core_id)
{
  if(tickers[cpu_core_id].SnapshotStartTaken){	
   if( esp_cpu_get_core_id() != cpu_core_id){ 
       // perform on other core
       esp_ipc_isr_call_blocking(GetTicker64FromIsr,(uint64_t *) &(tickers[cpu_core_id].ClockStopValue)); 
       //use second calibration value in difference calc 
       tickers[cpu_core_id].ClockDifferrence = tickers[cpu_core_id].ClockStopValue - tickers[cpu_core_id].ClockStartValue - tickers[cpu_core_id].AutoCalibrationWhenCalledFromOtherCore;
   } else { 
       // perform on the same core  
       tickers[cpu_core_id].ClockStopValue=GetTicker64();
       //use main calibration value in difference calc
       tickers[cpu_core_id].ClockDifferrence = tickers[cpu_core_id].ClockStopValue - tickers[cpu_core_id].ClockStartValue - tickers[cpu_core_id].AutoCalibration;
   }
   tickers[cpu_core_id].SnapshotStartTaken = false;
  } 
}
void IRAM_ATTR NativeAnulation(const uint8_t cpu_core_id)
{
     StartClockSnapshot(cpu_core_id); // call start followed by stop to get callibrated value independent on architecture
     StopClockSnapshot(cpu_core_id);
}

static void int_CPUticker64_setup(void*) 
{
  uint32_t cpu_core_id=esp_cpu_get_core_id();
	esp_err_t err;
	XTHAL_SET_CCOMPARE(2, 0); //we want an interrupt when CCOUNT reach 0, because at that moment we have to increase upper part, so we populate CCOMPARE2 register by 0
	err=esp_intr_alloc(ETS_INTERNAL_TIMER2_INTR_SOURCE, ESP_INTR_FLAG_LEVEL5 | ESP_INTR_FLAG_IRAM , NULL , NULL, (intr_handle_t*) &tickers[cpu_core_id].ticker_handle);
	assert(err==ESP_OK);
  reset_ccount_high();

    NativeAnulation(cpu_core_id);  
    tickers[cpu_core_id].AutoCalibration = tickers[cpu_core_id].ClockStopValue - tickers[cpu_core_id].ClockStartValue;
  	ESP_LOGI(TAG,"Auto calibrated value at CORE%"PRIu32" %"PRIu64" ticks",cpu_core_id, tickers[cpu_core_id].AutoCalibration);

    uint32_t another_core_id = 1 - cpu_core_id; //we do not switch cores here, just perform second calibration of another core when calling is performed from actual core

    NativeAnulation(another_core_id);  
    tickers[another_core_id].AutoCalibrationWhenCalledFromOtherCore = tickers[another_core_id].ClockStopValue - tickers[another_core_id].ClockStartValue;
  	ESP_LOGI(TAG,"Auto calibrated value at CORE%"PRIu32" when called from CORE%"PRIu32" %"PRIu64" ticks, due to core switching",another_core_id, cpu_core_id, tickers[another_core_id].AutoCalibrationWhenCalledFromOtherCore);  

  vTaskDelete(NULL);
}
void CORE0_CPUticker64_init(void) 
{
 if(!tickers[0].ticker_handle){
  tickers[0].ticker_handle = (void*) !NULL;
  ESP_LOGI("CPUticker64","Setting up CPUticker64 on CORE0 ...");
  xTaskCreatePinnedToCore(&int_CPUticker64_setup, "tick_c0", 2000, NULL, 6, NULL, 0);
 }
}
void CORE0_CPUticker64_free(void) 
{
 if(tickers[0].ticker_handle){
  esp_err_t err = esp_intr_free(tickers[0].ticker_handle);
  assert(err==ESP_OK);
  if(esp_cpu_get_core_id()!= 0){
   esp_ipc_isr_call_blocking(reset_ccount_high_fromISR, NULL);  
  } else {
    reset_ccount_high();
  }
  tickers[0].ticker_handle = NULL;
  ESP_LOGI("CPUticker64","Ticker on CORE0 free ");
 }
}
void  CORE1_CPUticker64_init(void) 
{
 if(!tickers[1].ticker_handle){
  tickers[1].ticker_handle = (void*) !NULL; // just temporary to guard this function until real handle is allocated
  ESP_LOGI("CPUticker64","Setting up CPUticker64 on CORE1 ...");
  xTaskCreatePinnedToCore(&int_CPUticker64_setup, "tick_c1", 2000, NULL, 6, NULL, 1);
 }
}
void CORE1_CPUticker64_free(void) 
{
 if(tickers[1].ticker_handle){
  esp_err_t err = esp_intr_free(tickers[1].ticker_handle);
  assert(err==ESP_OK);
  if(esp_cpu_get_core_id()!=1){
    esp_ipc_isr_call_blocking(reset_ccount_high_fromISR, NULL);  
  } else {
    reset_ccount_high();
  }
  tickers[1].ticker_handle = NULL;
  ESP_LOGI("CPUticker64","Ticker on CORE1 free ");
 }
}
inline uint64_t IRAM_ATTR GetClock64(void)
{
  return GetTicker64();	
}
uint64_t IRAM_ATTR ClockGetCallibratedDifference(const uint8_t cpu_core_id)
{
  if(cpu_core_id < SOC_CPU_CORES_NUM){
   return tickers[cpu_core_id].ClockDifferrence;	
  } else {
   return -1;
  }	
}

