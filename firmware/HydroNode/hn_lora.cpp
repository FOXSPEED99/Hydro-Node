#include "hn_lora.h"
#include "hn_board.h"
#include "hn_config.h"

#if HN_LORA_ENABLED

#include <Arduino.h>
#include <SPI.h>
#include <avr/power.h>

/* ------------------------------------------------------------------------- */
/* SX1278 registers (LoRa mode)                                               */
/* ------------------------------------------------------------------------- */
#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08
#define REG_PA_CONFIG            0x09
#define REG_OCP                  0x0B
#define REG_FIFO_ADDR_PTR        0x0D
#define REG_FIFO_TX_BASE_ADDR    0x0E
#define REG_IRQ_FLAGS            0x12
#define REG_MODEM_CONFIG_1       0x1D
#define REG_MODEM_CONFIG_2       0x1E
#define REG_PREAMBLE_MSB         0x20
#define REG_PREAMBLE_LSB         0x21
#define REG_PAYLOAD_LENGTH       0x22
#define REG_MODEM_CONFIG_3       0x26
#define REG_SYNC_WORD            0x39
#define REG_DIO_MAPPING_1        0x40
#define REG_VERSION              0x42
#define REG_PA_DAC               0x4D

#define MODE_LONG_RANGE          0x80
#define MODE_SLEEP               0x00
#define MODE_STDBY               0x01
#define MODE_TX                  0x03

#define IRQ_TX_DONE_MASK         0x08

#define SX1278_VERSION           0x12

/* The Ra-02 routes its output through PA_BOOST, not RFO. Getting this wrong
 * transmits at a fraction of the expected power with no error anywhere. */
#define PA_BOOST                 0x80

static bool     s_present = false;
static uint16_t s_airtime_ms = 0;

/* 10 MHz is inside the SX1278's limit and well inside what an 8 MHz AVR can
 * produce (it will actually clock at F_CPU/2 = 4 MHz). MSB first, mode 0. */
static SPISettings s_spi(4000000, MSBFIRST, SPI_MODE0);

/* ------------------------------------------------------------------------- */
/* Register access                                                            */
/* ------------------------------------------------------------------------- */

static uint8_t spi_xfer(uint8_t addr, uint8_t value)
{
    SPI.beginTransaction(s_spi);
    digitalWrite(HN_PIN_LORA_NSS, LOW);
    SPI.transfer(addr);
    uint8_t back = SPI.transfer(value);
    digitalWrite(HN_PIN_LORA_NSS, HIGH);
    SPI.endTransaction();
    return back;
}

static uint8_t reg_read(uint8_t addr)          { return spi_xfer(addr & 0x7F, 0x00); }
static void    reg_write(uint8_t addr, uint8_t v) { (void)spi_xfer(addr | 0x80, v); }

static void set_mode(uint8_t mode)
{
    /* The LongRangeMode bit is only writable in SLEEP, so it is carried on
     * every mode write rather than set once. */
    reg_write(REG_OP_MODE, (uint8_t)(MODE_LONG_RANGE | mode));
}

/* ------------------------------------------------------------------------- */
/* Configuration                                                              */
/* ------------------------------------------------------------------------- */

static void set_frequency(uint32_t hz)
{
    /* frf = f / FSTEP, FSTEP = 32 MHz / 2^19. Computed as a 64-bit shift so it
     * stays exact on an 8-bit target with no floating point. */
    uint64_t frf = ((uint64_t)hz << 19) / 32000000ULL;
    reg_write(REG_FRF_MSB, (uint8_t)(frf >> 16));
    reg_write(REG_FRF_MID, (uint8_t)(frf >> 8));
    reg_write(REG_FRF_LSB, (uint8_t)(frf >> 0));
}

static void set_tx_power(int8_t dbm)
{
    if (dbm < 2)  dbm = 2;
    if (dbm > 17) dbm = 17;          /* +20 dBm needs PA_DAC boost and a duty
                                      * cycle limit; not used here. */
    reg_write(REG_PA_DAC, 0x84);     /* normal (non-boost) PA_DAC */
    reg_write(REG_PA_CONFIG, (uint8_t)(PA_BOOST | (uint8_t)(dbm - 2)));

    /* Over-current protection. The default 100 mA trips below what +17 dBm
     * actually draws on PA_BOOST, which shows up as reduced range rather than
     * as an error. 120 mA (trim 15) clears it. */
    reg_write(REG_OCP, (uint8_t)(0x20 | 0x0F));
}

static uint8_t bw_code(uint16_t khz)
{
    switch (khz) {
    case 7:   return 0;   case 10:  return 1;   case 15:  return 2;
    case 20:  return 3;   case 31:  return 4;   case 41:  return 5;
    case 62:  return 6;   case 125: return 7;   case 250: return 8;
    default:  return 9;   /* 500 kHz */
    }
}

static void configure_modem()
{
    set_frequency(HN_LORA_FREQ_HZ);

    reg_write(REG_FIFO_TX_BASE_ADDR, 0x00);

    /* MODEM_CONFIG_1: bandwidth[7:4] | coding rate[3:1] | implicit header[0].
     * Explicit header, so the Hub is told the length and can reject a
     * truncated packet before it reaches our own CRC. */
    reg_write(REG_MODEM_CONFIG_1,
              (uint8_t)((bw_code(HN_LORA_BW_KHZ) << 4) |
                        (((uint8_t)(HN_LORA_CODING_RATE - 4)) << 1)));

    /* MODEM_CONFIG_2: spreading factor[7:4] | TxContinuous[3] | payload CRC[2].
     * Payload CRC on: the radio then drops corrupted packets itself, before
     * hn_packet_decode ever sees them. */
    reg_write(REG_MODEM_CONFIG_2,
              (uint8_t)(((uint8_t)HN_LORA_SPREADING_FACTOR << 4) | 0x04));

    /* MODEM_CONFIG_3: low-data-rate optimise[3] | AGC auto on[2].
     * LDRO is required when a symbol lasts longer than 16 ms, which at 125 kHz
     * means SF11 and above. Computed rather than hardcoded so changing the
     * spreading factor cannot silently break the link. */
    const bool ldro = ((1UL << HN_LORA_SPREADING_FACTOR) / (uint32_t)HN_LORA_BW_KHZ) > 16UL;
    reg_write(REG_MODEM_CONFIG_3, (uint8_t)((ldro ? 0x08 : 0x00) | 0x04));

    reg_write(REG_PREAMBLE_MSB, (uint8_t)(HN_LORA_PREAMBLE_LEN >> 8));
    reg_write(REG_PREAMBLE_LSB, (uint8_t)(HN_LORA_PREAMBLE_LEN & 0xFF));

    reg_write(REG_SYNC_WORD, HN_LORA_SYNC_WORD);

    set_tx_power(HN_LORA_TX_POWER_DBM);

    reg_write(REG_DIO_MAPPING_1, 0x40);   /* DIO0 = TxDone */
}

/* ------------------------------------------------------------------------- */
/* Public API                                                                 */
/* ------------------------------------------------------------------------- */

bool hn_lora_begin()
{
    s_present = false;

    power_spi_enable();                  /* Section 1 shut the SPI clock off */

    pinMode(HN_PIN_LORA_NSS, OUTPUT);
    digitalWrite(HN_PIN_LORA_NSS, HIGH);
    pinMode(HN_PIN_LORA_RESET, OUTPUT);
    pinMode(HN_PIN_LORA_DIO0, INPUT);

    SPI.begin();

    /* Reset pulse. The datasheet asks for >=100 us low and 5 ms of settling. */
    digitalWrite(HN_PIN_LORA_RESET, LOW);
    hn_delay_ms(1);
    digitalWrite(HN_PIN_LORA_RESET, HIGH);
    hn_delay_ms(10);

    if (reg_read(REG_VERSION) != SX1278_VERSION) {
        /* The Ra-02 is soldered to this board, not on a connector, so this is
         * a manufacturing fault rather than a missing module. Leaving SPI
         * powered down keeps the failure cheap. */
        SPI.end();
        power_spi_disable();
        return false;
    }

    set_mode(MODE_SLEEP);                /* LongRangeMode is only settable here */
    hn_delay_ms(1);
    set_mode(MODE_STDBY);

    configure_modem();

    set_mode(MODE_SLEEP);
    s_present = true;
    return true;
}

bool hn_lora_present() { return s_present; }

uint16_t hn_lora_last_airtime_ms() { return s_airtime_ms; }

void hn_lora_sleep()
{
    if (s_present) set_mode(MODE_SLEEP);
}

bool hn_lora_send(const uint8_t *data, uint8_t len)
{
    if (!s_present || len == 0) return false;

    set_mode(MODE_STDBY);

    reg_write(REG_FIFO_ADDR_PTR, 0x00);
    SPI.beginTransaction(s_spi);
    digitalWrite(HN_PIN_LORA_NSS, LOW);
    SPI.transfer(REG_FIFO | 0x80);
    for (uint8_t i = 0; i < len; ++i) SPI.transfer(data[i]);
    digitalWrite(HN_PIN_LORA_NSS, HIGH);
    SPI.endTransaction();

    reg_write(REG_PAYLOAD_LENGTH, len);
    reg_write(REG_IRQ_FLAGS, 0xFF);      /* write-1-to-clear */

    uint32_t started = millis();
    set_mode(MODE_TX);

    /*
     * Wait on DIO0 rather than polling the IRQ register: it costs no SPI
     * traffic, and idling between checks keeps the CPU out of a spin loop for
     * the whole airtime. Timer0 wakes us every ~2 ms regardless, so the poll
     * interval is bounded without a timer of our own.
     */
    bool done = false;
    while ((uint16_t)(millis() - started) < HN_LORA_TX_TIMEOUT_MS) {
        if (digitalRead(HN_PIN_LORA_DIO0) == HIGH) { done = true; break; }
        hn_idle_once();
    }

    if (done) s_airtime_ms = (uint16_t)(millis() - started);

    reg_write(REG_IRQ_FLAGS, 0xFF);
    set_mode(MODE_SLEEP);                /* 0.2 uA - this line is the battery */

    return done;
}

#else  /* !HN_LORA_ENABLED */

bool     hn_lora_begin()   { return false; }
bool     hn_lora_present() { return false; }
void     hn_lora_sleep()   {}
bool     hn_lora_send(const uint8_t *, uint8_t) { return false; }
uint16_t hn_lora_last_airtime_ms() { return 0; }

#endif
