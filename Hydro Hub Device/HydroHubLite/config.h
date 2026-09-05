/*
 * config.h - Hydro Hub Lite
 *
 * A focused receiver for the current stage: listen to the Node, do the tank
 * maths, put it on the screen. No cloud, no WiFi, no BLE, no OTA, no pump
 * relay - the full Hub in ../Hydro-Hub still has all of that, and this is
 * deliberately not a replacement for it.
 *
 * Board: ESP32-S3 (same unit as the full Hub), 480x320 ILI9488/ILI9486 over
 * TFT_eSPI, Ra-02 SX1278 over RadioLib.
 *
 * ---------------------------------------------------------------------------
 * ONLY TWO SECTIONS NEED EDITING, AND BOTH ARE AT THE TOP OF THIS FILE:
 *
 *   1. TANK      how many tanks, their litres, water height, blind zone
 *   2. NETWORK   WiFi credentials and the OTA password, if you want
 *                over-the-air updates
 *
 * Everything below those - LoRa settings, pin numbers, timings, colours - is
 * either fixed by the hardware or has to match the Node, and should be left
 * alone unless you are changing the design.
 * ---------------------------------------------------------------------------
 */
#pragma once

#include <Arduino.h>

/* ========================================================================= */
/* 1. TANK - the geometry of the installation                                 */
/* ========================================================================= */
/*
 * The tanks are plumbed together, so the water sits at the same level in all
 * of them. One Node measures one tank and that level applies to the whole set;
 * the litres are summed. If that ever stops being true - separate tanks that
 * fill independently - this model breaks and each tank needs its own Node.
 *
 *      ultrasonic
 *          |             ^
 *          |  blind      |  TANK_BLIND_CM      (sensor -> full water line)
 *          v             v
 *      ~~~~~~~~~~~  100% ^
 *                        |  TANK_WATER_HEIGHT_CM
 *      ___________    0% v
 *
 * So the sensor reads TANK_BLIND_CM when full, and
 * TANK_BLIND_CM + TANK_WATER_HEIGHT_CM when empty.
 */

/* How many tanks the installation has. */
#define TANK_COUNT              2

/* Capacity of each tank in litres, one entry per tank. They are summed - list
 * them individually so tanks of different sizes add up correctly. */
#define TANK_LITERS_LIST        { 1000, 500 }

/* Water height in cm when the tank is 100% full. */
#define TANK_WATER_HEIGHT_CM    80

/* Distance in cm between the transducer face and the 100%-full water line. */
#define TANK_BLIND_CM           10

/*
 * Split-transducer parallax correction, in millimetres between the centres of
 * the two transducer barrels. The RCWL-1670 transmits and receives from
 * different places, so the sound path is a triangle rather than straight down:
 *
 *      d = sqrt( (L/2)^2 - (s/2)^2 )
 *
 * Uncorrected, it over-reads by ~4 mm at a 50 mm distance and under 1 mm past
 * 300 mm - so it matters most when the tank is nearly full, which is exactly
 * when the reading matters most. Measure s once with callipers on a production
 * module. Set to 0 to disable the correction.
 */
#define TANK_TRANSDUCER_SEP_MM  0

/* Air temperature assumed when the Node has no working temperature sensor.
 * The speed of sound moves ~0.18% per degree, so a 10 C error is ~1.8% of the
 * distance. Set it to the typical headspace temperature at your site. */
#define TANK_FALLBACK_TEMP_C    25.0f

/* ========================================================================= */
/* 2. NETWORK - optional, for over-the-air updates                            */
/* ========================================================================= */
/*
 * Entirely optional. At WIFI_ENABLED 0 the whole network layer is not even
 * compiled in: no WiFi stack, no OTA, and the Hub simply receives and
 * displays. Turn it on and you can reflash the Hub from the Arduino IDE
 * without a cable, which is what makes a month-long test practical.
 *
 * The receiver never depends on the network. If the router reboots, the Hub
 * keeps receiving and displaying and only OTA stops.
 */

/* 1 to enable WiFi and OTA. Fill in the two lines below as well. */
#define WIFI_ENABLED            0

#define WIFI_SSID               "your-wifi"
#define WIFI_PASSWORD           "your-password"

/* Once on the network the Hub appears in the IDE under
 * Tools > Port > "hydro-hub at 192.168.x.x".
 *
 * CHANGE THE PASSWORD. Without one, anyone on the network can flash arbitrary
 * firmware onto a device wired to your tank. */
#define OTA_HOSTNAME            "hydro-hub"
#define OTA_PASSWORD            "hydro-ota"

/* ========================================================================= */
/* LORA - MUST MATCH firmware/HydroNode/hn_config.h EXACTLY                   */
/* ========================================================================= */
/* Any single difference here means the two radios never hear each other, and
 * neither end produces an error saying why. */
#define HUB_LORA_FREQ_MHZ       433.0f
#define HUB_LORA_BW_KHZ         125.0f
#define HUB_LORA_SF             9
#define HUB_LORA_CR             5        /* 4/5 */
#define HUB_LORA_SYNC_WORD      0x42
#define HUB_LORA_PREAMBLE_LEN   8

/* Hashed to 16 bits on the wire; the string itself never flies. */
#define HUB_PAIR_ID             "SWS-PAIR-0001"

/* ========================================================================= */
/* PINS - as wired on the existing Hub board                                  */
/* ========================================================================= */
#define PIN_LORA_CS             21
#define PIN_LORA_MOSI           38
#define PIN_LORA_MISO           36
#define PIN_LORA_SCK            37
#define PIN_LORA_RST            5
#define PIN_LORA_DIO0           4

/* Two buttons, active low. Button A cycles the view; B is reserved. */
#define PIN_BUTTON_A            6
#define PIN_BUTTON_B            16
#define BUTTON_DEBOUNCE_MS      50

/* TFT pins live in the TFT_eSPI User_Setup, not here. */
#define TFT_ROTATION            1
#define SCREEN_W                480
#define SCREEN_H                320

/* ========================================================================= */
/* TIMING                                                                     */
/* ========================================================================= */
/* The Node transmits every ~2 minutes in production, every 5 s on the bench.
 * "Stale" means we have missed enough that the level on screen may no longer
 * be true; "lost" means the link is down and the screen must say so rather
 * than keep showing a comfortable number. */
#define LINK_STALE_MS           (3UL * 60UL * 1000UL)
#define LINK_LOST_MS            (10UL * 60UL * 1000UL)

#define SERIAL_BAUD             115200

/* ========================================================================= */
/* PALETTE - RGB565                                                           */
/* ========================================================================= */
/*
 * A dark surface, because this panel hangs on a wall and gets looked at after
 * dark. Water is a single hue whose depth carries magnitude; the four status
 * colours are reserved and are never used for anything decorative, so a colour
 * on this screen always means the same thing.
 *
 * Status colour NEVER carries meaning alone. Every pill and banner pairs it
 * with a glyph and a word, so the screen still reads correctly for a
 * colour-blind viewer and in the dark from across a room.
 */
#define C_SURFACE      0x18C3   /* #1a1a19 page                    */
#define C_PANEL        0x2124   /* #262625 raised card             */
#define C_HAIRLINE     0x39C7   /* #3a3a38 dividers                */
#define C_INK          0xFFFF   /* #ffffff primary text            */
#define C_INK_DIM      0xC616   /* #c3c2b7 secondary text          */
#define C_INK_MUTED    0x8C4F   /* #8a897f labels                  */
#define C_WATER        0x3C3C   /* #3987e5 water                   */
#define C_WATER_DEEP   0x1AF5   /* #1f5fae water, deep             */
#define C_WATER_GHOST  0x29EA   /* #2a3f57 water, stale/unknown    */
#define C_GOOD         0x0D01   /* #0ca30c                         */
#define C_WARNING      0xFD83   /* #fab219                         */
#define C_SERIOUS      0xEC0B   /* #ec835a                         */
#define C_CRITICAL     0xD1C7   /* #d03b3b                         */

/* ========================================================================= */
/* Internals - you do not normally need to touch anything below here        */
/* ========================================================================= */

/* How often to retry a dropped WiFi connection. Deliberately slow: an
 * association plus DHCP can take 5-10 s, and retrying sooner aborts the
 * attempt in progress, which looks like an endless connecting loop. */
#define WIFI_RETRY_MS           20000UL

/* ------------------------------------------------------------------------- */
/* Field log                                                                  */
/* ------------------------------------------------------------------------- */
/*
 * A month of running tells you nothing if all you have at the end is a screen
 * showing the current level. These are the counters worth having: how reliable
 * the link was, the worst outage, how far the battery moved, and how often
 * each sensor misbehaved.
 *
 * Kept in RAM and mirrored to NVS so a power cut does not erase the month.
 */
#define FIELDLOG_ENABLED        1

/* Persist every N accepted packets. At a 120 s cycle that is roughly hourly,
 * so a month costs ~700 NVS writes - nothing against its endurance, and at
 * most an hour of counters lost to a power cut. */
#define FIELDLOG_SAVE_EVERY     30

/* ------------------------------------------------------------------------- */
/* Watchdog                                                                   */
/* ------------------------------------------------------------------------- */
/* Reboot if the main loop stops running for this long. A hung Hub on a wall
 * for three weeks is indistinguishable from a dead one. */
#define HUB_WDT_TIMEOUT_S       30
