
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"

  /* Define variables, which go into .bss section (zero-initialized data) */
  .bss
  /* Next input signal edge expected: 0 (negative) or 1 (positive) */
  .global pulse_edge
pulse_edge:
  .long 0

  /* Next input signal edge expected: 0 (negative) or 1 (positive) */
  .global next_edge
next_edge:
  .long 0

  /* Counter started when signal value changes.
     Edge is "debounced" when the counter reaches zero. */
  .global debounce_counter
debounce_counter:
  .long 0

  /* Value to which debounce_counter gets reset.
     Set by the main program. */
  .global debounce_max_count
debounce_max_count:
  .long 0



  /* Total number of signal edges acquired */
  .global edge_count
edge_count:
  .long 0

  /* RTC IO number used to sample the input signal.
     Set by main program. */
  .global io_number
io_number:
  .long 0

  /* Code goes into .text section */
  .text
  .global entry
entry:
//  jump pulse_tick

  .global read_now
read_now:
  /* Load io_number */
  move r3, io_number
  ld r3, r3, 0

  /* Lower 16 IOs and higher need to be handled separately,
   * because r0-r3 registers are 16 bit wide.
   * Check which IO this is.
   */
  move r0, r3
  jumpr read_io_high, 16, ge

  /* Read the value of lower 16 RTC IOs into R0 */
  READ_RTC_REG(RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT_S, 16)
  rsh r0, r0, r3
  jump read_done

  /* Read the value of RTC IOs 16-17, into R0 */
read_io_high:
  READ_RTC_REG(RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT_S + 16, 2)
  sub r3, r3, 16
  rsh r0, r0, r3

read_done:
  and r0, r0, 1
  /* State of input changed? */
  move r3, next_edge
  ld r3, r3, 0
  add r3, r0, r3
  and r3, r3, 1
  jump changed, eq
  /* Not changed */
  /* Reset debounce_counter to debounce_max_count */
  move r3, debounce_max_count
  move r2, debounce_counter
  ld r3, r3, 0
  st r3, r2, 0
  /* End program */
  halt

  .global changed
changed:
  /* Input state changed */
  /* Has debounce_counter reached zero? */
  move r3, debounce_counter
  ld r2, r3, 0
  add r2, r2, 0 /* dummy ADD to use "jump if ALU result is zero" */
  jump edge_detected, eq
  /* Not yet. Decrement debounce_counter */
  sub r2, r2, 1
  st r2, r3, 0
  /* End program */
  halt

  .global edge_detected
edge_detected:
  /* Reset debounce_counter to debounce_max_count */
  move r3, debounce_max_count
  move r2, debounce_counter
  ld r3, r3, 0
  st r3, r2, 0
  /* Flip next_edge */
  move r3, next_edge
  ld r2, r3, 0
  add r2, r2, 1
  and r2, r2, 1
  st r2, r3, 0
  /* Increment edge_count */
  move r3, edge_count
  ld r2, r3, 0
  add r2, r2, 1
  st r2, r3, 0
  /* Full pulse detected? */
  move r3, pulse_edge
  ld r3, r3, 0
  add r3, r0, r3
  and r3, r3, 1
  halt
