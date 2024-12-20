#include <xtensa/coreasm.h>
#include <xtensa/corebits.h>
#include <xtensa/config/system.h>
#include <xtensa/xtensa-xer.h>
#include "sdkconfig.h"
#include "soc/soc.h"

// we will use  (index 244 !!!! register), Xtensa internal special register inside core itself (each core has its own "private" register)
// not utilized by Espressif design and that way no memory will be used at all, elimininating requirements for synchronization
 
#define  CCOUNT_HIGH_PART  244

    .section .iram1,"ax"
    .global     xt_highint5
    .type       xt_highint5, @function
    .align      4

xt_highint5:
    esync
    rsr  a0, CCOUNT_HIGH_PART
    addi a0, a0, 1           // increment it 
    wsr  a0, CCOUNT_HIGH_PART
    xor a0, a0, a0          // populate A0 with zero  
    wsr a0, CCOMPARE2       // write to CCOMPARE2 to clear interrupt flag so it can happen again 
    esync
.L_xt_highint5_exit:
    rsr     a0, EXCSAVE_5    // recover a0 from EXCSAVE_5 
    rfi     5                // return from interrupt priority 5

/////////////////////////////////////////////////////////////////////////////////////////////
// here is our function which will properly handle two 32bits reads
// in case if wrapping happens in between them

  .section .iram1,"ax"
    .global     GetTicker64
    .type       GetTicker64, @function
    .align      4

GetTicker64:
    entry   sp, 32
    esync
    rsr  a3, CCOUNT_HIGH_PART    // take upper part and store in a3 
    esync
    rsr  a2, CCOUNT              // take lower part in a2
    retw

/////////////////////////////////////////////////////////////////////////////////////////////
  .section .iram1,"ax"
    .global     GetTicker64FromIsr
    .type       GetTicker64FromIsr, @function
    .align      4

// a2 contains uint64_t* 
GetTicker64FromIsr:
    esync
    rsr  a3, CCOUNT_HIGH_PART    // take upper part and store in a3 
    memw
    memw
    s32i a3, a2, 4               // upper part at +4 offset 
    esync
    rsr  a4, CCOUNT              // take lower part in a4
    memw
    memw
    s32i a4, a2, 0               // lower part at 0 offset
    ret

/////////////////////////////////////////////////////////////////////////////////////////////

    .section    .iram1, "ax"
    .align      4
    .global     reset_ccount_high
    .type       reset_ccount_high, @function

reset_ccount_high:
    entry   sp, 32
    xor a3, a3, a3
    esync  
    wsr  a3, CCOUNT_HIGH_PART                   
    retw

////////////////////////////////////////////////////////////////////////////////////

    .section    .iram1, "ax"
    .align      4
    .global     reset_ccount_high_fromISR
    .type       reset_ccount_high_fromISR, @function

reset_ccount_high_fromISR:
    xor a3, a3, a3
    esync  
    wsr  a3, CCOUNT_HIGH_PART                   
    ret

////////////////////////////////////////////////////////////////////////////////////
