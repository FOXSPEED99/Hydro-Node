/*
 * hn_board.h - the board itself: pin map, safe idle states, and the two timing
 * hooks the Section 3 SleepManager will replace.
 *
 * Every pin number below was extracted from the Altium schematic, not guessed.
 * The netlist evidence for each one is in docs/HARDWARE.md; the short form is
 * in the comments here so you can check a pin without leaving the file.
 *
 * Target: Arduino Pro Mini 3.3 V / 8 MHz (ATmega328P). VCC is fed straight from
 * the 3.6 V battery pack; the on-board regulator is bypassed. The MODULE's
 * ground is the switched ground downstream of Q1, so the whole MCU is dead
 * until the magnet latch turns it on.
 */
#ifndef HN_BOARD_H
#define HN_BOARD_H

#include <Arduino.h>
#include <stdint.h>

/* ------------------------------------------------------------------------- */
/* Pin map                                                                    */
/* ------------------------------------------------------------------------- */

/* Ultrasonic, J3 "Ultrasonic" (B4B-XH-A).
 *
 *   J3.1 = switched GND
 *   J3.2 = BATT+                (always on - see the note below)
 *   J3.3 -> R1 100R -> D8       ECHO on the schematic
 *   J3.4 -> R2 100R -> D6       TRIG on the schematic
 *
 * ECHO is on D8 because D8 is PB0 = ICP1, the ATmega328P's input capture pin.
 * That placement is deliberate - the Stage 0 review raised it as HW-018 against
 * the previous revision ("Echo is on D7 instead of D8, so hardware input
 * capture is unavailable") and this board is the fix. The driver uses the
 * capture unit accordingly, which also lets the CPU idle-sleep through the
 * flight time instead of spinning in pulseIn().
 *
 * If a built harness is found swapped, set the two pin macros below to match
 * the working wiring. ECHO on D8 uses Timer1 input capture; ECHO on any other
 * digital pin uses the software pulse-width path in hn_ultrasonic.cpp.
 *
 * The harness is a CROSS-OVER cable: the RCWL-1670's own pads run
 * GND / RX(TRIG) / TX(ECHO) / +5V, so positions 2 and 4 are swapped relative to
 * this connector. That is by design and confirmed, but it is not marked on the
 * board, so check continuity before first power-up. The two 100 ohm series
 * resistors limit a mis-wired harness to ~33 mA of contention rather than
 * letting two push-pull outputs fight at full current. */
#define HN_PIN_US_TRIG      8
#define HN_PIN_US_ECHO      6

/* Temperature, J2 "Temp" (B3B-XH-A), DS18B20.
 *
 *   J2.1 -> R4 100R -> D4       1-Wire data
 *   J2.2 = switched GND
 *   J2.3 -> D3, and R7 4.7k pull-up is referenced to D3 (NOT to VCC)
 *
 * The sensor is powered FROM A GPIO and the bus pull-up hangs off that same
 * GPIO. Driving D3 low therefore depowers the sensor and removes the pull-up's
 * drain in one action, so the entire 1-Wire subsystem costs nothing between
 * readings. This is the right pattern and the reason the driver must never
 * leave D3 high. */
#define HN_PIN_TEMP_POWER   3
#define HN_PIN_TEMP_DATA    4

/* Flow switch, J1 "Flow Switch" (B2B-XH-A).
 *
 *   J1.1 = switched GND
 *   J1.2 = the flow node: R6 1M pull-up to BATT+, C2 100nF to GND,
 *          R5 100R -> D5 (digital), R3 330R -> A2 (analogue)
 *
 * The external 1 Mohm pull-up is what keeps standby current at ~3.6 uA with the
 * contact closed, instead of the ~110 uA an internal pull-up would cost. The
 * MCU's internal pull-up on D5 must therefore stay OFF - permanently. */
#define HN_PIN_FLOW_DIGITAL 5
#define HN_PIN_FLOW_ANALOG  A2
#define HN_ADC_CH_FLOW      2    /* A2 = ADC2, for the DIDR0 buffer disable */

/* Buzzer LS1 -> R9 100R -> D7. Section 4 owns it. Section 1 only parks it low
 * so it is silent and defined. */
#define HN_PIN_BUZZER       7

/*
 * !!  A1 IS THE POWER LATCH'S SHUTDOWN LINE  !!
 *
 *   A1 -> R11 100k -> U2.1 (74HC74 1CLR, ACTIVE LOW)
 *
 * Driving A1 low clears the latch, opens Q1 and cuts the switched ground - the
 * device turns itself off and only a magnet brings it back. A1 must stay a
 * high-impedance INPUT at all times in this firmware. Never pinMode(A1, OUTPUT).
 * The deliberate shutdown path belongs to a later section (low-battery
 * cut-off), and it is the only thing allowed to touch this pin.
 */
#define HN_PIN_LATCH_CLEAR  A1

/* LoRa, U3 Ra-02. Section 2 owns these; Section 1 only parks them. */
#define HN_PIN_LORA_DIO0    2
#define HN_PIN_LORA_RESET   9
#define HN_PIN_LORA_NSS     10
#define HN_PIN_LORA_MOSI    11
#define HN_PIN_LORA_MISO    12
#define HN_PIN_LORA_SCK     13

/* ------------------------------------------------------------------------- */
/* Board lifecycle                                                            */
/* ------------------------------------------------------------------------- */

/* Put every pin into its documented idle state. Call once, first thing in
 * setup(), before any module's begin(). */
void hn_board_begin();

/* ------------------------------------------------------------------------- */
/* Timing hooks - the SleepManager seam                                       */
/* ------------------------------------------------------------------------- */

/*
 * These two functions are the ONLY places this firmware waits. Section 3
 * replaces their bodies with a watchdog-timed power-down and a timed idle
 * sleep; nothing else in the sensor layer has to change. Keep it that way -
 * do not call delay() directly from a sensor module.
 */
void hn_delay_ms(uint32_t ms);   /* uint32_t, not uint16_t: the production
                                  * cycle interval is 120000 ms and a 16-bit
                                  * parameter would silently truncate it. */

/* Sleep the CPU until any interrupt fires. Timers, the input capture unit and
 * the UART keep running in IDLE, so this is safe to use inside a measurement.
 * Returns immediately if interrupts are disabled. */
void hn_idle_once();

/* ------------------------------------------------------------------------- */
/* ADC power management                                                       */
/* ------------------------------------------------------------------------- */

/* The Arduino core enables the ADC in init() and leaves it on forever, which
 * costs a few hundred microamps of analogue supply current. These bracket the
 * one place that needs it. analogRead() will hang if the ADC is disabled, so
 * every analogRead() in this firmware sits between these two calls. */
void hn_adc_enable();
void hn_adc_disable();

#endif /* HN_BOARD_H */
