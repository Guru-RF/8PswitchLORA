// ============================================================
// RF.Guru 8-Port LoRa Switch - shared Config struct
// ============================================================

#pragma once

#include <Arduino.h>

struct Config {
    String  name;
    int     defaultPort;
    float   loraFrequency;
    int     txPower;
    String  loraMode;   // "fast" or "slow"
    uint8_t aesKey[16];
};

extern Config config;
extern volatile bool driveConnected;

// defined in main.cpp
void sendAck(const char *text);
void savePersistentState(int port, float attDb);
int   currentActivePort();    // 0 = none, 1..8
float currentActiveAttDb();   // < 0 = attenuator disabled
