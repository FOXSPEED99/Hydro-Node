#pragma once

#include <Arduino.h>

// ============================================================
// Smart Water System - Device 2 / Indoor Receiver Configuration
// Board: ESP32-S3-N16R8
// ============================================================

// -------------------- Build Features --------------------
#define ENABLE_TFT_DASHBOARD       1
#define ENABLE_QR_SETUP_SCREEN     1
#define ENABLE_SUPABASE            1
#define ENABLE_OTA_UPDATES         1
#define ENABLE_TASK_WATCHDOG       1
#define ENABLE_BOOT_TFT_SELF_TEST  1
#define ENABLE_SERIAL_DEBUG        1

// -------------------- Device Identity --------------------
// IMPORTANT: Change this per sold unit pair. The sensor device must use the same pair ID.
static const char DEVICE_PAIR_ID[] = "SWS-PAIR-0001";
static const char DEVICE_ID[] = "SWS-INDOOR-0001";  // legacy LoRa id; cloud id is MAC-derived (see CloudSync)
static const char FIRMWARE_VERSION[] = "1.2.4-hydrohub";

// -------------------- Debugging --------------------
// 0=errors only, 1=warn, 2=info, 3=debug, 4=verbose
static constexpr uint8_t DEBUG_LOG_LEVEL = 3;
static constexpr uint32_t SERIAL_BAUD_RATE = 115200;
static constexpr uint32_t DEBUG_HEALTH_LOG_INTERVAL_MS = 30000;

// -------------------- LoRa / Ra-02 SX1278 --------------------
static constexpr int LORA_CS_PIN = 21;
static constexpr int LORA_MOSI_PIN = 38;
static constexpr int LORA_MISO_PIN = 36;
static constexpr int LORA_SCK_PIN = 37;
static constexpr int LORA_RST_PIN = 5;
static constexpr int LORA_DIO0_PIN = 4;

// MUST match the Sensor Node's config.h. SF9/BW125 was chosen over the old
// SF12/BW62.5: air time drops from ~9s to ~0.5s per packet, which is what
// makes the battery-powered node last ~2 years. Still reaches hundreds of
// meters through walls at 433MHz.
static constexpr float LORA_FREQUENCY_MHZ = 433.0;
static constexpr float LORA_BANDWIDTH_KHZ = 125.0;
static constexpr uint8_t LORA_SPREADING_FACTOR = 9;
static constexpr uint8_t LORA_CODING_RATE = 5;
static constexpr uint8_t LORA_SYNC_WORD = 0x42;     // Device pair isolation layer 1.
static constexpr int8_t LORA_TX_POWER_DBM = 17;
static constexpr uint16_t LORA_PREAMBLE_LEN = 8;
static constexpr uint32_t LORA_RX_TIMEOUT_MS = 15000;
static constexpr uint32_t LORA_SIGNAL_LOST_MS = 30000;

// Device pair isolation layer 2: every packet must include {"pairId":"SWS-PAIR-0001"}.
static constexpr size_t LORA_MAX_PACKET_BYTES = 384;

// -------------------- TFT Display --------------------
// TFT_eSPI pin/driver setup is normally in the TFT_eSPI User_Setup file:
// ILI9488 or ILI9486, 480x320, CS=8, RST=9, DC=10, MOSI=11, SCLK=12.
static constexpr int TFT_WIDTH_PX = 480;
static constexpr int TFT_HEIGHT_PX = 320;
static constexpr uint8_t TFT_ROTATION = 1;

// Set to a GPIO only if the TFT LED/backlight is controlled by ESP32.
// Leave -1 when LED is wired directly to 3.3V.
static constexpr int TFT_BACKLIGHT_PIN = -1;
static constexpr uint8_t TFT_BACKLIGHT_LEVEL = 255;

// -------------------- Buttons / Relay / LEDs --------------------
static constexpr int PUMP_BUTTON_PIN = 6;
static constexpr int WIFI_BUTTON_PIN = 16;
static constexpr int WIFI_STATUS_LED_PIN = 15;
static constexpr int PUMP_RELAY_PIN = 17;
static constexpr int BUZZER_PIN = 7;   // confirmation chime when the pump turns on

static constexpr bool BUTTON_ACTIVE_LOW = true;
static constexpr uint32_t BUTTON_DEBOUNCE_MS = 50;
static constexpr uint32_t RELAY_MIN_SWITCH_MS = 1000;
static constexpr uint32_t BOOT_RESTORE_DELAY_MS = 1000;

// WiFi button gestures (spec §6.2/§6.3). A release before the long-press
// threshold is a short press (QR screen); holding for WIFI_LONG_PRESS_MS
// toggles WiFi off/on (only once setup is complete). Holding BOTH buttons for
// FACTORY_RESET_HOLD_MS performs a full factory reset.
static constexpr uint32_t WIFI_LONG_PRESS_MS = 5000;
static constexpr uint32_t FACTORY_RESET_HOLD_MS = 10000;

// -------------------- Water Calculations (spec §5) --------------------
// Flow classification thresholds in liters/minute (tune during field testing).
static constexpr float FLOW_WEAK_LPM_MAX = 8.0f;   // below this: weak/slow flow
static constexpr float FLOW_GOOD_LPM_MAX = 18.0f;  // below this: good; above: strong
// Level-rate estimation window: readings arrive every ~2 minutes from the
// Sensor Node, so keep enough history to smooth single-reading noise.
static constexpr uint8_t LEVEL_HISTORY_LEN = 8;
// Ignore rate estimates older than this (sensor offline in between).
static constexpr uint32_t LEVEL_HISTORY_MAX_AGE_MS = 30UL * 60UL * 1000UL;

// Optional built-in RGB LED on many ESP32-S3 dev boards.
static constexpr int RGB_LED_PIN = 48;
static constexpr uint8_t RGB_LED_COUNT = 1;
static constexpr uint8_t RGB_LED_BRIGHTNESS = 40;

// -------------------- WiFi Provisioning --------------------
// Reconnect cadence when WiFi drops. Kept long on purpose: an association + DHCP
// can take 5-10s, and retrying faster aborts the in-progress attempt before it
// completes (which looked like an endless "sta is connecting" loop).
static constexpr uint32_t WIFI_CONNECT_CHECK_MS = 20000;
// If a device with saved WiFi creds still can't connect after this window, assume
// the creds are stale and start BLE provisioning so the app can re-provision it.
// Longer than two reconnect windows so a merely-slow join isn't misjudged.
static constexpr uint32_t WIFI_BLE_FALLBACK_MS = 45000;
static constexpr uint32_t WIFI_SETUP_SCREEN_TIMEOUT_MS = 180000;

// QR/deep-link target shown on the setup screen. The app parses the `hub` query
// parameter (the cloud device id) from whatever this QR encodes.
static const char PROVISIONING_BASE_URL[] = "https://bleuland.app/hydrohub/pair";

// -------------------- Supabase (Hydro Hub cloud) --------------------
// The anon key is a public client key (safe to embed, same as the mobile app);
// all access is gated by RLS + SECURITY DEFINER RPCs. Never embed a service key.
static const char SUPABASE_URL[] = "https://aidpejxlofvdrtemurft.supabase.co";
static const char SUPABASE_ANON_KEY[] = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImFpZHBlanhsb2Z2ZHJ0ZW11cmZ0Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NjU4Njk4OTIsImV4cCI6MjA4MTQ0NTg5Mn0.U_Wljw9AjoAaJxI8ZtQFOSS432N4-_FtPf-5sI82X44";

// Development convenience. For production, set this to 0 and provide SUPABASE_ROOT_CA.
#define SUPABASE_TLS_INSECURE_DEV_MODE 1
static const char SUPABASE_ROOT_CA[] = "";

// Cloud sync cadence. The device_sync RPC returns an adaptive poll interval, but
// this is the floor/idle rate used when nothing is pending.
static constexpr uint32_t CLOUD_SYNC_INTERVAL_MS = 2000;
static constexpr uint32_t HTTP_TIMEOUT_MS = 8000;

// -------------------- FreeRTOS --------------------
static constexpr uint32_t LORA_TASK_STACK = 8192;
static constexpr uint32_t DISPLAY_TASK_STACK = 8192;
static constexpr uint32_t CLOUD_TASK_STACK = 12288;
static constexpr uint32_t BUTTON_TASK_STACK = 4096;

static constexpr BaseType_t LORA_TASK_CORE = 0;
static constexpr BaseType_t CLOUD_TASK_CORE = 0;
static constexpr BaseType_t DISPLAY_TASK_CORE = 1;
static constexpr BaseType_t BUTTON_TASK_CORE = 1;

// -------------------- Watchdog --------------------
static constexpr uint32_t TASK_WATCHDOG_TIMEOUT_S = 20;
