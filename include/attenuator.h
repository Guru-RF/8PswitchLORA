// ============================================================
// RF.Guru 8-Port LoRa Switch - 6-bit Attenuator
// 0.0 .. 31.5 dB in 0.5 dB steps. Bits are active LOW.
// Index = db * 2 (0..63). Each bit set -> that line goes LOW.
// ============================================================

#pragma once

#include <Arduino.h>
#include "pins.h"

static inline void attenuatorDisable() {
    digitalWrite(PIN_ATT_ACT, LOW);
}

static void attenuatorSet(float db) {
    if (db < 0.0f) {
        attenuatorDisable();
        return;
    }
    int idx = (int)lroundf(db * 2.0f);
    if (idx < 0) idx = 0;
    if (idx > 63) idx = 63;

    digitalWrite(PIN_ATT_ACT, HIGH);
    digitalWrite(PIN_ATT1, (idx & 0x20) ? LOW : HIGH);
    digitalWrite(PIN_ATT2, (idx & 0x10) ? LOW : HIGH);
    digitalWrite(PIN_ATT3, (idx & 0x08) ? LOW : HIGH);
    digitalWrite(PIN_ATT4, (idx & 0x04) ? LOW : HIGH);
    digitalWrite(PIN_ATT5, (idx & 0x02) ? LOW : HIGH);
    digitalWrite(PIN_ATT6, (idx & 0x01) ? LOW : HIGH);
}
