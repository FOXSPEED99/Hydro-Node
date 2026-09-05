/*
 * hn_config.h - every tunable value for the Hydro Node Section 1 firmware.
 *
 * Rule for this file: no number appears here without a sentence saying where it
 * came from. If you change one, change the reasoning with it.
 *
 * Sources referenced below:
 *   [SCH]  Hydro Node Parts & Schematic/Schematic/Hydro_Node_Schematic.SchDoc
 *   [DS]   component datasheets (RCWL-1670, DS18B20, ATmega328P)
 *   [REV]  the Stage 0 hardware review (issue IDs HW-nnn), see docs/HARDWARE.md
 */
#ifndef HN_CONFIG_H
#define HN_CONFIG_H

#include <stdint.h>

/* ------------------------------------------------------------------------- */
/* Identity                                                                   */
/* ------------------------------------------------------------------------- */

#define HN_FW_NAME              "Hydro Node"
#define HN_FW_VERSION           "0.1.0"
#define HN_FW_SECTION           "Section 1 - sensors & detection"

/* ------------------------------------------------------------------------- */
/* Deployment profile                                                         */
/* ------------------------------------------------------------------------- */
/*
 * ONE SWITCH FOR AN UNATTENDED DEPLOYMENT. Set to 1 before leaving a Node in
 * the field; leave at 0 for bench work.
 *
 * It exists because getting a field test right means changing three unrelated
 * things at once, and forgetting any one of them invalidates the result:
 *
 *   sleep on        without it the MCU runs at ~4 mA and the pack lasts weeks
 *   interval 120 s  the real cycle, not the 5 s bench cadence
 *   SERIAL OFF      this is the one people forget. The report is ~600 bytes,
 *                   and at 9600 baud that is 625 ms of blocking transmission
 *                   per cycle - roughly as much awake time as the entire
 *                   measurement plus the radio burst combined. Leaving it on
 *                   doubles the energy per cycle and makes any battery figure
 *                   you collect meaningless.
 */
#ifndef HN_PRODUCTION
#define HN_PRODUCTION           0
#endif

/* ------------------------------------------------------------------------- */
/* Build-time feature switches                                                */
/* ------------------------------------------------------------------------- */

/* Serial is a bench/diagnostic facility. A production Node with no one
 * watching should build with HN_SERIAL_ENABLED 0: it removes the UART, the
 * formatting code (~6 kB of flash) and the awake time spent shifting
 * characters out at 9600 baud. Section 5 will make this the default once LoRa
 * carries the data. */
#ifndef HN_SERIAL_ENABLED
#define HN_SERIAL_ENABLED       (!HN_PRODUCTION)
#endif

/* 9600 baud, not 115200. At F_CPU = 8 MHz the UART divisor for 115200 is 7.68,
 * which rounds to a -3.5 % baud error - outside the ~2 % a UART tolerates
 * before framing errors, and that is before the Pro Mini's ceramic resonator
 * adds its own +/-0.5 % [REV HW-024]. 9600 divides to within +0.16 %.
 * 38400 (+0.16 %) is the fastest safe alternative if you want a shorter awake
 * window during bench work. */
#ifndef HN_SERIAL_BAUD
#define HN_SERIAL_BAUD          9600UL
#endif

/* Emit the compact single-line record in addition to the human-readable block.
 * That line is the shape the Section 2 LoRa payload will be packed from, so it
 * is kept even during bench work to keep the two views honest with each other. */
#ifndef HN_REPORT_MACHINE_LINE
#define HN_REPORT_MACHINE_LINE  1
#endif

/* Build a sensors-only Node by setting this to 0. See the LoRa section at the
 * end of this file for the modem settings. Declared here because the pin
 * parking below depends on it. */
#ifndef HN_LORA_ENABLED
#define HN_LORA_ENABLED         1
#endif

/* The Ra-02 is soldered to the board and powered whenever the latch is on
 * [SCH: U3.3 = BATT+, U3 GND = switched GND], so "not using it" has to mean
 * something definite. Parking holds it in reset and off the SPI bus.
 *
 * This follows HN_LORA_ENABLED automatically: once the radio driver owns those
 * pins, parking them would fight it. Overridable for bench work where you want
 * the radio held down while the sensors run. */
#ifndef HN_PARK_LORA_PINS
#define HN_PARK_LORA_PINS       (!HN_LORA_ENABLED)
#endif

/* ------------------------------------------------------------------------- */
/* Measurement cycle                                                          */
/* ------------------------------------------------------------------------- */

/* Bench value. The production interval is 120 s and belongs to the Section 3
 * SleepManager, not to a delay() - see hn_delay_ms() in hn_board.h, which is
 * the single function that has to change. */
#ifndef HN_CYCLE_INTERVAL_MS
#if HN_PRODUCTION
#define HN_CYCLE_INTERVAL_MS    120000UL   /* the real cycle */
#else
#define HN_CYCLE_INTERVAL_MS    5000UL     /* bench: fast enough to watch */
#endif
#endif

/* ------------------------------------------------------------------------- */
/* Sleep (Section 3)                                                          */
/* ------------------------------------------------------------------------- */

/* Power-down between cycles instead of spinning in delay(). ~4.5 uA asleep
 * against ~4 mA awake. Off on the bench only because a sleeping Node is
 * awkward to reprogram - the bootloader needs the chip awake when you press
 * upload, and a 120 s sleep means waiting for it. */
#ifndef HN_SLEEP_ENABLED
#define HN_SLEEP_ENABLED        HN_PRODUCTION
#endif

/* Reset the chip if an awake cycle hangs for more than 8 seconds. A normal
 * cycle is under a second, so this only fires on a genuine fault - and on a
 * roof, a reboot is infinitely better than a Node that sits there flat.
 * The count of watchdog resets is reported at start-up. */
#ifndef HN_WDT_GUARD_ENABLED
#define HN_WDT_GUARD_ENABLED    1
#endif

/* ------------------------------------------------------------------------- */
/* Battery measurement                                                        */
/* ------------------------------------------------------------------------- */

/*
 * Calibration constant for the internal bandgap: 1.1 V * 1023 * 1000.
 *
 * The bandgap is only specified to +/-10%, so the default reads to about
 * +/-0.35 V, which is far too coarse to follow a lithium discharge curve. To
 * calibrate a board, once:
 *
 *   1. Build with HN_PRODUCTION 0 so the serial report is on.
 *   2. Measure the actual voltage on the VCC pin with a meter: V_real.
 *   3. Read the reported figure from the banner: V_reported.
 *   4. HN_BANDGAP_CAL = 1125300 * V_real / V_reported.
 *
 * That takes it to roughly 1%. Do it per board - the error is per chip, not
 * per design.
 */
#ifndef HN_BANDGAP_CAL
#define HN_BANDGAP_CAL          1125300UL
#endif

/* ------------------------------------------------------------------------- */
/* Ultrasonic - RCWL-1670 on J3                                               */
/* ------------------------------------------------------------------------- */

/* Samples per cycle. The power model in the Stage 0 review budgets five
 * 50 ms measurement cycles per wake (2.50 mA*s of the 10.05 mA*s total).
 * Five is also the smallest odd count that still supports a median plus
 * outlier rejection with a meaningful survivor count. */
#define HN_US_SAMPLES           5

/* Minimum survivors after outlier rejection before the mean is trusted.
 * 3 of 5 means a clear majority agreed. */
#define HN_US_MIN_ACCEPTED      3

/* Gap between triggers. [DS] gives the RCWL-1670 a 50 ms measurement cycle;
 * re-triggering inside that window returns the tail of the previous burst as a
 * ghost echo. 60 ms leaves 20 % margin. */
#define HN_US_SAMPLE_GAP_MS     60

/* Hard limits from the module's rated 2 cm - 400 cm range [DS], as round-trip
 * microseconds at ~343 m/s. A sample outside this window is not a measurement
 * and is discarded before the filter sees it. */
#define HN_US_ECHO_MIN_US       100U     /* ~1.7 cm */
#define HN_US_ECHO_MAX_US       25000U   /* ~4.3 m  */

/* Give up on one sample after this long. Worst-case echo is 23.3 ms at 4 m;
 * 40 ms leaves room for the module's own start-up latency before the burst. */
#define HN_US_ECHO_TIMEOUT_MS   40U

/* Outlier window around the median, in microseconds of round trip.
 * 300 us is ~51 mm of distance. Wide enough to keep genuine surface ripple
 * (+/-5 to 20 mm while filling, per [REV HW-030]); narrow enough to throw out a
 * sidewall or obstruction echo, which lands hundreds of millimetres away. */
#define HN_US_OUTLIER_US        300U

/* Above this spread across the accepted samples the surface is moving or the
 * echo is unreliable, and the reading is reported UNSTABLE rather than OK.
 * 200 us is ~34 mm. */
#define HN_US_SPREAD_LIMIT_US   200U

/* Installation plausibility window - DISABLED BY DEFAULT, and it should stay
 * that way on this product.
 *
 * When non-zero, a stable reading outside this round-trip window is flagged
 * HN_STATUS_OUT_OF_RANGE. The raw value is still measured and still
 * transmitted - it is only labelled - but the label itself encodes tank
 * geometry, and tank geometry is the Hub's business, not the Node's.
 *
 * The Hub holds the blind zone, the tank height, the transducer standoff and
 * the volume curve, and it can be updated over Wi-Fi. A Node that flagged
 * readings against a window compiled into it would start crying OUT_OF_RANGE
 * the moment it was fitted to a taller tank - reporting a fault that is really
 * just a different installation, from the one place in the system that cannot
 * be corrected without a site visit.
 *
 * So both are 0 and the check compiles away. The only limits the Node still
 * enforces are HN_US_ECHO_MIN_US / MAX_US above, and those are different in
 * kind: they are the sensor's own physical range, not an assumption about
 * where the sensor was installed.
 *
 * Set them non-zero only for bench work, where a known target distance makes
 * an out-of-window reading genuinely informative. For reference, the geometry
 * [REV HW-023 v9] recorded was 0.05-0.15 m to a full water line and
 * 0.70-1.00 m to the tank floor - roughly 230 us to 7000 us. */
#define HN_US_PLAUSIBLE_MIN_US  0U
#define HN_US_PLAUSIBLE_MAX_US  0U

/* Duration of the pull-up presence probe on the echo line. The line only has to
 * settle through the 100 ohm series resistor and the harness capacitance, which
 * is a sub-microsecond time constant; 200 us is pure margin. While the probe is
 * active and the sensor IS connected, the pull-up sources ~70 uA - so 200 us
 * costs about 14 nA*s per cycle, which is nothing. Do not leave it on. */
#define HN_US_PRESENCE_PROBE_US 200U

/* ------------------------------------------------------------------------- */
/* Temperature - DS18B20 on J2, powered from a GPIO                           */
/* ------------------------------------------------------------------------- */

/* 9-bit resolution = 0.5 C, 93.75 ms conversion [DS].
 * 12-bit would be 0.0625 C but costs 750 ms of awake time. The extra
 * resolution buys nothing: [REV section 5] shows a 0.5 C error is +/-1.8 mm of
 * distance at 2 m, against a +/-28 mm headspace-gradient term that dominates
 * everything else. Power wins. */
#define HN_TEMP_RESOLUTION_BITS 9
#define HN_TEMP_CONFIG_BYTE     0x1F    /* R1:R0 = 00 -> 9-bit */
#define HN_TEMP_CONVERT_MS      94U     /* 93.75 ms, rounded up */

/* Hard ceiling on waiting for the conversion-complete signal. The sensor is
 * externally powered, so it reports completion by releasing the bus; this is
 * only the backstop for a sensor that never answers. */
#define HN_TEMP_CONVERT_TIMEOUT_MS 200U

/* Settling time after D3 goes high, before the first 1-Wire reset. Covers the
 * DS18B20's power-on reset and the 4.7 kohm pull-up charging the harness
 * capacitance. 10 ms is generous; the sensor draws ~1 uA idle while waiting. */
#define HN_TEMP_POWERUP_MS      10U

/* Plausible water/headspace temperature. Outside this the reading is flagged,
 * not discarded - the raw register value is still transmitted. */
#define HN_TEMP_MIN_C           (-15)
#define HN_TEMP_MAX_C           (85)

/* The DS18B20 powers up with 0x0550 (+85.0 C) in its temperature register. If
 * that exact value comes back, the conversion did not complete - it is the
 * documented signature of a sensor that lost power mid-conversion, not a real
 * 85 C reading. */
#define HN_TEMP_POR_RAW         0x0550

/* ------------------------------------------------------------------------- */
/* 1-Wire bus timing (Maxim AN126, standard speed), microseconds              */
/* ------------------------------------------------------------------------- */

#define HN_OW_RESET_LOW_US      480U
#define HN_OW_PRESENCE_WAIT_US  70U
#define HN_OW_RESET_TAIL_US     410U
#define HN_OW_WRITE1_LOW_US     6U
#define HN_OW_WRITE1_TAIL_US    64U
#define HN_OW_WRITE0_LOW_US     60U
#define HN_OW_WRITE0_TAIL_US    10U
#define HN_OW_READ_LOW_US       6U
#define HN_OW_READ_SAMPLE_US    9U
#define HN_OW_READ_TAIL_US      55U
/* How long to wait for the bus to be idle-high before starting a reset.
 * If it never rises, the data line is shorted to ground. */
#define HN_OW_IDLE_TIMEOUT_US   250U

/* ------------------------------------------------------------------------- */
/* Flow switch on J1                                                          */
/* ------------------------------------------------------------------------- */

/* A mechanical contact on a cable running to a fill pipe chatters. Sample
 * several times and vote. */
#define HN_FLOW_SAMPLES         5
#define HN_FLOW_SAMPLE_GAP_MS   10
/* Below this many agreeing samples the switch is bouncing hard enough that the
 * state is not trustworthy. */
#define HN_FLOW_MIN_AGREE       4

/* ADC thresholds on A2, in counts of 1023, ratiometric to VCC.
 *
 * The flow node is a 1 Mohm pull-up to BATT+ [SCH: R6] with a 100 nF filter
 * [SCH: C2], and the switch shorts it to ground. Reading it as an analogue
 * voltage - not just a logic level - is what makes a partial fault visible:
 * water across the contacts or a corroded cable presents a resistance rather
 * than a short, and lands the node in mid-air.
 *
 * The bands are set against the ATmega328P's own input thresholds so the
 * digital and analogue views never disagree in normal operation:
 *   V_IL = 0.3*VCC = 307 counts,  V_IH = 0.6*VCC = 614 counts.
 *
 *   <= 205 (20 %)  contact closed. Safely below V_IL.
 *   >= 717 (70 %)  contact open.   Safely above V_IH.
 *   in between     fault: a resistive path of roughly 250 kohm to 2.3 Mohm.
 *
 * The 70 % (not 90 %) open threshold is deliberate: with a 1 Mohm source
 * impedance, the ADC pin's own worst-case leakage of 1 uA over temperature
 * would pull the reading down by up to a full volt. 70 % tolerates that. */
#define HN_FLOW_ADC_CLOSED_MAX  205U
#define HN_FLOW_ADC_OPEN_MIN    717U

/* Settling delay before each ADC conversion. With the switch open the source
 * impedance is 1 Mohm, far above the 10 kohm the ATmega328P's sample-and-hold
 * is specified for; the 14 pF S/H capacitor needs ~140 us to charge to within
 * 10 time constants. 500 us is comfortable. A dummy conversion is discarded on
 * top of this - see hn_flow.cpp. */
#define HN_FLOW_ADC_SETTLE_US   500U

/* Re-read delay after a mid-rail sample.
 *
 * This is not optional, and the reason is the board's own RC. The flow node is
 * 1 Mohm [SCH: R6] into 100 nF [SCH: C2], so tau = 100 ms. The contact shorts
 * the node to ground, which collapses it in microseconds - but when the contact
 * OPENS, the node has to climb back through that 1 Mohm:
 *
 *   to V_IH  (0.6*VCC)  ->  0.92 * tau  =  92 ms
 *   to the 70 % ADC threshold ->  1.20 * tau  = 120 ms
 *
 * For that ~120 ms the node genuinely sits at mid-rail, which is exactly the
 * signature of a resistive fault. A normal end-of-fill would occasionally be
 * reported as water in the connector.
 *
 * The two are separable by persistence: an RC transient passes, a fault does
 * not. So a mid-rail reading triggers one re-read after 3*tau, and only a
 * second mid-rail result is called a fault. 300 ms is 95 % of the way up, well
 * clear of the threshold. This costs nothing in the normal case - it only runs
 * when something looked wrong. */
#define HN_FLOW_SETTLE_MS       300U

/* Polarity. An HT-60 class paddle flow switch is normally-open and closes when
 * water flows, so the node is pulled LOW while the tank is filling.
 * VERIFY THIS AGAINST THE FITTED SWITCH - a normally-closed part inverts the
 * whole meaning and this is the one line to change. */
#define HN_FLOW_FILLING_IS_LOW   1

#endif /* HN_CONFIG_H */

/* ------------------------------------------------------------------------- */
/* LoRa - Ra-02 / SX1278 (Section 2)                                          */
/* ------------------------------------------------------------------------- */

/*
 * MODEM SETTINGS - THESE MUST MATCH THE HUB EXACTLY.
 * The Hub's values live in "Hydro Hub Device/HydroHubLite/config.h"; any
 * difference in these eight numbers means the two radios simply never hear
 * each other, with no error message on either end to say why.
 */
#define HN_LORA_FREQ_HZ         433000000UL  /* Ra-02 is SX1278: 410-525 MHz  */
#define HN_LORA_BW_KHZ          125
#define HN_LORA_CODING_RATE     5            /* 4/5                           */
#define HN_LORA_SYNC_WORD       0x42         /* pair isolation, layer 1       */
#define HN_LORA_PREAMBLE_LEN    8

/*
 * Spreading factor: the single biggest lever on battery life.
 *
 * Airtime roughly doubles per step, and the radio draws ~100 mA for all of it.
 * For this 19-byte packet, against 4.4 Ah usable over two years:
 *
 *   SF7   57 ms    5.7 mA*s/TX    1.8 Ah    2.5x margin
 *   SF8  103 ms   10.3 mA*s/TX    2.4 Ah    1.8x margin
 *   SF9  185 ms   18.5 mA*s/TX    3.6 Ah    1.2x margin  <- barely passes
 *
 * SF9 is the ceiling the Stage 0 review set (HW-031) and it is where bring-up
 * starts, because it has ~5 dB more link margin than SF7 and the first job is
 * simply to hear each other. Once the Hub is showing RSSI and SNR, drop to the
 * lowest SF that still has comfortable margin - roughly 10 dB of SNR headroom
 * - and the battery gets that 2x back. Decide it from the measurement, not
 * from this comment.
 */
#define HN_LORA_SPREADING_FACTOR 9

/* +17 dBm on PA_BOOST, matching the Hub's LORA_TX_POWER_DBM. The review
 * (HW-006, HW-031) suggests dropping toward +14 dBm once the link is
 * characterised: it nearly halves the peak current, which keeps the two
 * LS14500 cells inside their 50 mA continuous rating and reduces the rail
 * droop that HW-042 is about. Do that with data, after the antenna is
 * properly mounted - a good antenna is worth more dB than the PA is. */
#define HN_LORA_TX_POWER_DBM    17

/* Hard backstop on a transmission. Worst case airtime here is ~200 ms; if
 * TxDone has not arrived in 2 s the radio has stopped responding. */
#define HN_LORA_TX_TIMEOUT_MS   2000U

/* Device identity. DEVICE_PAIR_ID must be byte-identical to the Hub's, and is
 * hashed to 16 bits before it goes on the wire - see hn_packet.h. */
#define HN_PAIR_ID              "SWS-PAIR-0001"
#define HN_NODE_ID              1
