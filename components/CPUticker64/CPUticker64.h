#pragma once

#define NOP __asm__ __volatile__("NOP \n")

typedef struct Ticker64 {
  volatile intr_handle_t ticker_handle;   // we need it to be able to deinitialize and free allocated resources once they are not needed anymore
  uint64_t ClockStartValue;
  uint64_t ClockStopValue;
  uint64_t ClockDifferrence;
  uint64_t AutoCalibration;
  uint64_t AutoCalibrationWhenCalledFromOtherCore;
  volatile bool SnapshotStartTaken; 
} Ticker64_t; 

uint64_t GetClock64(void);

void CORE0_CPUticker64_init(void);
void CORE0_CPUticker64_free(void);

void CORE1_CPUticker64_init(void);
void CORE1_CPUticker64_free(void);

void StartClockSnapshot(const uint8_t cpu_core_id);  // call this before code you want to measure
void StopClockSnapshot(const uint8_t cpu_core_id);   // call this after code you want to measure
uint64_t ClockGetCallibratedDifference(const uint8_t cpu_core_id); //call this to get properly calibrated result
