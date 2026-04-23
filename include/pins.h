// ============================================================
// RF.Guru 8-Port LoRa Switch - Hardware Pin Definitions
// RP2040 GPIO Mapping
// ============================================================

#pragma once

// RF switch ports (active HIGH enables the port)
#define PIN_RF1   23
#define PIN_RF2   22
#define PIN_RF3   14
#define PIN_RF4   13
#define PIN_RF5    0
#define PIN_RF6    1
#define PIN_RF7    2
#define PIN_RF8    3

// Attenuator control bits (6 bits, 0..31.5 dB in 0.5 dB steps)
// Bits are active LOW: LOW = bit set in attenuation index.
#define PIN_ATT1   6
#define PIN_ATT2   5
#define PIN_ATT3  26
#define PIN_ATT4  27
#define PIN_ATT5  28
#define PIN_ATT6  29

// Attenuator enable (HIGH = attenuator in-line)
#define PIN_ATT_ACT 25

// LoRa SPI0 (RFM95)
#define PIN_SPI_MISO   8
#define PIN_SPI_CLK   10
#define PIN_SPI_MOSI  11
#define PIN_LORA_CS   21
#define PIN_LORA_RST  20
