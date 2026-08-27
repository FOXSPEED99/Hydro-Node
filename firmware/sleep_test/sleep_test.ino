/*
 * HYDRO NODE — SLEEP CURRENT TEST
 * ================================
 *
 * This sketch does nothing except go to sleep. Its whole purpose is to let you
 * measure the sleep current one step at a time, so you find out WHERE the
 * current is going instead of guessing.
 *
 *
 * HOW TO USE IT
 * -------------
 * 1. Change the STAGE number below.
 * 2. Flash it.
 * 3. UNPLUG THE USB-TTL ADAPTER.  <-- this matters, see WARNING below
 * 4. Watch the LED blink to confirm which stage is running.
 * 5. Wait for the blinking to stop, then read the meter.
 * 6. Write the number in the table at the bottom of this file.
 * 7. Repeat with the next STAGE.
 *
 * Seven flashes, about half an hour. At the end you will know exactly what
 * every microamp is doing.
 *
 *
 * *** WARNING — UNPLUG THE PROGRAMMER BEFORE MEASURING ***
 * A USB-TTL adapter feeds current backwards into the board through the RX and
 * DTR pins. With it plugged in, every reading you take is wrong. Flash, then
 * pull all six programmer wires off, THEN measure.
 *
 *
 * HOW TO READ THE BLINKS
 * ----------------------
 * At power-up the LED tells you what is happening, then stops forever.
 *
 *   First group, SLOW blinks  = which stage is running. STAGE 0 blinks once,
 *                               STAGE 1 blinks twice, and so on.
 *   Second group, FAST blinks = the radio check (only from STAGE 1 up):
 *        2 fast blinks  -> radio answered correctly and is now asleep. Good.
 *        6 fast blinks  -> the radio did not answer. Your reading is
 *                          meaningless until you fix the SPI wiring.
 *
 * After the blinking stops the board sleeps and the meter reading is stable.
 *
 *
 * WHAT EACH STAGE ADDS  (each stage keeps everything from the stages before it)
 * ----------------------------------------------------------------------------
 *   0  Nothing switched off. Plain sleep. The radio has never been told to
 *      sleep, so it is sitting in standby. Expect a big number, around 1.7 mA.
 *      This is your baseline.
 *
 *   1  + the radio is commanded to sleep.
 *      Expect a huge drop, to roughly 0.1 mA. This proves the radio talks.
 *
 *   2  + the ADC and the analog comparator are switched off, and the unused
 *      peripherals are unclocked.
 *
 *   3  + pin 13 is driven LOW, so the on-board LED is properly off.
 *      *** THE DROP FROM STAGE 2 TO STAGE 3 IS EXACTLY WHAT THE LED COSTS. ***
 *      In stages 1 and 2 pin 13 is deliberately left as INPUT_PULLUP, which is
 *      the state we suspect your real firmware is leaving it in. If stage 3 is
 *      about 0.04 mA lower than stage 2, the LED was the problem.
 *
 *   4  + every unused pin is given a defined state instead of floating.
 *
 *   5  + the brown-out detector is switched off during sleep.
 *      Expect around 0.02 mA of drop. If you see NO drop, your Pro Mini's BOD
 *      fuse is already disabled, which is good news and one less thing to do.
 *
 *   6  + the watchdog timer runs and wakes the chip every 8 seconds.
 *      This is the only stage that behaves like the real product. The rise
 *      from stage 5 is what the watchdog costs, and you must pay it — it is
 *      what wakes the node every 2 minutes.
 *
 *
 * WHERE TO PUT THE METER
 * ----------------------
 * In series with the battery, same place you measured the 0.10 mA.
 * On the milliamp range you cannot resolve better than about 0.01 mA, which is
 * not enough to see the steps between stages 3, 4 and 5. Use the microamp (uA)
 * range, or put a 10 ohm resistor in the battery negative lead and measure the
 * millivolts across it — 1 mV across 10 ohm is exactly 0.1 mA.
 *
 *
 * BOARD SETTINGS IN THE ARDUINO IDE
 * ---------------------------------
 * Board:     Arduino Pro or Pro Mini
 * Processor: ATmega328P (3.3V, 8 MHz)
 *
 * No libraries needed. SPI is bit-banged below on purpose, so this sketch
 * cannot be thrown off by a library version.
 */

#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>


/* ==========================================================================
 *  CHANGE THIS NUMBER.  0, then 1, then 2 ... up to 6.
 * ========================================================================== */
#define STAGE 0
/* ========================================================================== */


/* --------------------------------------------------------------------------
 * PIN MAP — taken from the schematic netlist, not guessed.
 * -------------------------------------------------------------------------- */
const uint8_t PIN_DIO0     = 2;    // U3.5  radio interrupt out
const uint8_t PIN_SPARE    = 3;    // free since HW-053 was closed
const uint8_t PIN_TEMP     = 4;    // R4 -> DS18B20 data. 4k7 pull-up on the board
const uint8_t PIN_FLOW_D   = 5;    // R5 -> flow sense. 1M pull-up on the board
const uint8_t PIN_TRIG     = 6;    // R2 -> J3.4 ultrasonic
const uint8_t PIN_BUZZ     = 7;    // R15 -> buzzer
const uint8_t PIN_ECHO     = 8;    // R1 -> J3.3 ultrasonic
const uint8_t PIN_RADIO_RST= 9;    // U3.4  radio reset, active low
const uint8_t PIN_NSS      = 10;   // U3.15
const uint8_t PIN_MOSI     = 11;   // U3.14
const uint8_t PIN_MISO     = 12;   // U3.13
const uint8_t PIN_SCK      = 13;   // U3.12  --- AND the on-board LED ---
const uint8_t PIN_REED     = A0;   // bare wire since R13 was removed (HW-067)
const uint8_t PIN_LATCH    = A1;   // -> R9 100k -> 74HC74 pin 1, the latch reset
const uint8_t PIN_FLOW_A   = A2;   // R3 -> flow sense


/* --------------------------------------------------------------------------
 * BIT-BANGED SPI TO THE RA-02
 * -------------------------------------------------------------------------- */
static uint8_t spiXfer(uint8_t out)
{
  uint8_t in = 0;
  for (int8_t i = 7; i >= 0; i--) {
    digitalWrite(PIN_MOSI, (out >> i) & 1);
    digitalWrite(PIN_SCK, HIGH);
    in = (in << 1) | (digitalRead(PIN_MISO) ? 1 : 0);
    digitalWrite(PIN_SCK, LOW);
  }
  return in;
}

static uint8_t radioRead(uint8_t reg)
{
  digitalWrite(PIN_NSS, LOW);
  spiXfer(reg & 0x7F);
  uint8_t v = spiXfer(0x00);
  digitalWrite(PIN_NSS, HIGH);
  return v;
}

static void radioWrite(uint8_t reg, uint8_t val)
{
  digitalWrite(PIN_NSS, LOW);
  spiXfer(reg | 0x80);
  spiXfer(val);
  digitalWrite(PIN_NSS, HIGH);
}


/* --------------------------------------------------------------------------
 * BLINK — the only way this sketch talks to you, since a serial adapter would
 * ruin the measurement.
 * -------------------------------------------------------------------------- */
static void blink(uint8_t times, uint16_t onMs, uint16_t offMs)
{
  pinMode(PIN_SCK, OUTPUT);
  for (uint8_t i = 0; i < times; i++) {
    digitalWrite(PIN_SCK, HIGH);
    delay(onMs);
    digitalWrite(PIN_SCK, LOW);
    delay(offMs);
  }
}


/* --------------------------------------------------------------------------
 * WATCHDOG — stage 6 only. Interrupt mode, NOT reset mode.
 * -------------------------------------------------------------------------- */
ISR(WDT_vect)
{
  // Nothing to do. Waking up is the whole job.
}

static void watchdogEvery8s(void)
{
  cli();
  wdt_reset();
  MCUSR &= ~(1 << WDRF);                                // clear the reset flag
  WDTCSR |= (1 << WDCE) | (1 << WDE);                   // open the change window
  WDTCSR  = (1 << WDIE) | (1 << WDP3) | (1 << WDP0);    // interrupt only, 8 s
  sei();
}


void setup()
{
  /* ---- 1. Say which stage this is: STAGE+1 slow blinks ---- */
  blink(STAGE + 1, 300, 300);
  delay(800);


#if STAGE >= 1
  /* ---- 2. Talk to the radio and put it to sleep ----
   *
   * The SX1278 powers up in standby and draws about 1.6 mA there. Nothing
   * else on this board comes close to that, so this is always the first
   * thing to fix and the first thing to prove.
   */
  pinMode(PIN_RADIO_RST, OUTPUT); digitalWrite(PIN_RADIO_RST, HIGH);  // not reset
  pinMode(PIN_NSS,  OUTPUT);      digitalWrite(PIN_NSS,  HIGH);       // deselected
  pinMode(PIN_SCK,  OUTPUT);      digitalWrite(PIN_SCK,  LOW);
  pinMode(PIN_MOSI, OUTPUT);      digitalWrite(PIN_MOSI, LOW);
  pinMode(PIN_MISO, INPUT);
  delay(10);

  uint8_t version = radioRead(0x42);   // RegVersion. An SX1278 answers 0x12.

  // RegOpMode. The LongRangeMode bit can only be changed while the chip is
  // already in sleep, so this takes two writes: sleep first, then sleep again
  // with the LoRa bit set.
  radioWrite(0x01, 0x00);              // FSK  + SLEEP
  delay(2);
  radioWrite(0x01, 0x80);              // LoRa + SLEEP
  delay(2);
  uint8_t mode = radioRead(0x01);

  bool radioOk = (version == 0x12) && (mode == 0x80);
  blink(radioOk ? 2 : 6, 80, 220);
  delay(800);

  /* Park the SPI pins.
   *
   * In stages 1 and 2 we deliberately leave them as INPUT_PULLUP. That is the
   * state we suspect your real firmware leaves pin 13 in, and it is what makes
   * the on-board LED glow at about 40 uA while looking switched off.
   * Stage 3 is what fixes it, so the difference between the two readings is a
   * direct measurement of the LED.
   */
  #if STAGE >= 3
    pinMode(PIN_NSS,  OUTPUT); digitalWrite(PIN_NSS,  HIGH);   // keep radio deselected
    pinMode(PIN_SCK,  OUTPUT); digitalWrite(PIN_SCK,  LOW);    // LED OFF, properly
    pinMode(PIN_MOSI, OUTPUT); digitalWrite(PIN_MOSI, LOW);
    pinMode(PIN_MISO, INPUT_PULLUP);                           // radio releases it
  #else
    pinMode(PIN_NSS,  INPUT_PULLUP);
    pinMode(PIN_SCK,  INPUT_PULLUP);                           // <-- lights the LED
    pinMode(PIN_MOSI, INPUT_PULLUP);
    pinMode(PIN_MISO, INPUT_PULLUP);
  #endif
#endif


#if STAGE >= 2
  /* ---- 3. Switch off the analogue hardware ----
   *
   * Order matters: clear ADEN first, THEN take the ADC's clock away. Do it the
   * other way round and the ADC is stuck on with no way to reach the bit.
   */
  ADCSRA = 0;                 // ADC off. Worth 200-300 uA if left running.
  ACSR  |= (1 << ACD);        // analog comparator off. Tens of uA.
  power_all_disable();        // stop the clocks to ADC, timers, TWI, USART, SPI

  /* From here on delay() and millis() do NOT work — Timer0 has no clock.
   * That is why all the blinking happens above this line. */
#endif


#if STAGE >= 4
  /* ---- 4. Give every pin a defined state ----
   *
   * A floating CMOS input drifts around its switching threshold, and while it
   * sits there both halves of the input stage conduct at once. Several
   * floating pins add up to tens of microamps.
   *
   * These pins connect to NOTHING, so a pull-up is safe and free:
   */
  pinMode(PIN_REED,  INPUT_PULLUP);   // A0 — bare since R13 came off
  pinMode(PIN_SPARE, INPUT_PULLUP);   // D3
  pinMode(A3, INPUT_PULLUP);
  pinMode(A4, INPUT_PULLUP);
  pinMode(A5, INPUT_PULLUP);
  pinMode(A6, INPUT_PULLUP);
  pinMode(A7, INPUT_PULLUP);
  pinMode(0,  INPUT_PULLUP);          // RX
  pinMode(1,  INPUT_PULLUP);          // TX

  /* These pins go to J3, which is unplugged for this test, so they are
   * floating too. In the finished product TRIG becomes an output driven low. */
  pinMode(PIN_TRIG, INPUT_PULLUP);
  pinMode(PIN_ECHO, INPUT_PULLUP);

  /* The buzzer is a piezo — a capacitor. Driving the pin low costs nothing
   * and stops it floating. */
  pinMode(PIN_BUZZ, OUTPUT);
  digitalWrite(PIN_BUZZ, LOW);

  /* The radio drives DIO0 low while it sleeps, so leave it a plain input.
   * A pull-up here would just push current into the radio's output. */
  pinMode(PIN_DIO0, INPUT);

  /* ---------------------------------------------------------------------
   * THESE THREE MUST STAY PLAIN INPUTS. DO NOT ADD PULL-UPS.
   *
   * A1 reaches the 74HC74's reset pin through R9. A2 and D5 reach the flow
   * sense node. All three already have a defined level from resistors on the
   * board, so they are not floating and need no help.
   *
   * Putting a pull-up on a pin that touches the latch circuit is exactly what
   * broke the board last week (HW-067). Leave them alone.
   * --------------------------------------------------------------------- */
  pinMode(PIN_LATCH,  INPUT);
  pinMode(PIN_FLOW_A, INPUT);
  pinMode(PIN_FLOW_D, INPUT);
  pinMode(PIN_TEMP,   INPUT);   // held high by R7 4k7 on the board
#endif


#if STAGE >= 6
  /* ---- 5. Start the watchdog. Stage 6 only. ---- */
  watchdogEvery8s();
#endif
}


void loop()
{
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  cli();
  sleep_enable();

#if STAGE >= 5
  /* Switch off the brown-out detector for the duration of the sleep.
   *
   * This must sit exactly here. The chip holds the "BOD off" bit for only a
   * few clock cycles after it is set, so sleep_cpu() has to be almost the next
   * instruction. Put anything else between them and it silently forgets, you
   * get no saving, and nothing tells you it failed.
   */
  sleep_bod_disable();
#endif

  sei();
  sleep_cpu();
  sleep_disable();

  /* Stages 0-5 never get here: nothing can wake them, so they sleep forever
   * and the meter reading is rock steady. Stage 6 arrives here every 8 seconds
   * when the watchdog fires, does nothing, and goes straight back to sleep. */
}


/* ==========================================================================
 *  RESULTS — measured on the hand-built board, 2026-08-27
 *  Pro Mini + Ra-02 only, no sensors connected.
 * ==========================================================================
 *
 *   Stage | What it adds                       | Measured  | Change
 *   ------|------------------------------------|-----------|-----------
 *     0   | nothing — radio still in standby   |  1.87 mA  |    —
 *     1   | radio asleep                       |  0.68 mA  | -1190 uA
 *     2   | ADC + comparator off               |  0.27 mA  |  -410 uA
 *     3   | pin 13 driven low                  |  0.020 mA |  -250 uA  <--
 *     4   | all pins defined                   |  0.020 mA |     0
 *     5   | brown-out detector off             |   4.8 uA  |   -15 uA
 *     6   | watchdog running (the real thing)  |   7.8 uA  |   +3.0 uA
 *
 *  FINAL: 7.8 uA against a 25 uA target. Beaten by more than three times.
 *  Two-year margin on the 4400 mAh pack goes from 1.41x to 2.91x at SF7, and
 *  SF9 goes from 0.92x (fails) to 1.39x (passes).
 *
 *
 *  THE ONE SURPRISE — STAGE 3 DROPPED 250 uA, NOT THE 40 uA PREDICTED
 *
 *  Pin 13 is two things on one wire: the LED, and SCK going to the Ra-02.
 *
 *  As INPUT_PULLUP the pin is not driven. The internal ~35k pull-up and the
 *  LED divide against each other and the pin settles at the LED's forward
 *  voltage, about 1.8 V. That is neither a valid high nor a valid low, and
 *  the SX1278's SCK input is looking at it. A CMOS input held mid-rail turns
 *  on both halves of its input stage at once and conducts continuously.
 *
 *      through the LED itself .................  ~40 uA
 *      crossbar current inside the SX1278 ..... ~210 uA
 *      ---------------------------------------------------
 *      measured total ......................... 250 uA
 *
 *  So most of it burns in the radio module, not in the LED. The LED is not
 *  just a load — it is a clamp that holds a logic input at an invalid level
 *  whenever the pin is left undriven.
 *
 *  Driving D13 low fixes it, and that is what got this board to 7.8 uA. But
 *  the bootloader flashes D13 on every reset and SPI.end() releases the pin,
 *  so a firmware-only fix has to be remembered forever, with no warning when
 *  it is not — the only symptom is a glow you cannot see in daylight.
 *
 *  ==> DESOLDER THE D13 LED (or its series resistor). See HW-046.
 *
 *
 *  THE OTHER RESULT WORTH KNOWING — STAGE 4 CHANGED NOTHING
 *
 *  Giving eleven floating pins a defined state measured ZERO change, against
 *  a predicted "tens of microamps". Keep doing it — it is free, and floating
 *  input current rises with temperature — but it was not the problem here.
 * ========================================================================== */
