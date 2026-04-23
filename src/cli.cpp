// ============================================================
// RF.Guru 8-Port LoRa Switch - Serial CLI
// Small REPL over USB CDC. Pattern lifted from SmartFOX.
// ============================================================

#include "cli.h"
#include "config.h"
#include "pins.h"
#include "attenuator.h"

#include <Arduino.h>
#include <FatFS.h>
#include <LoRa.h>

static const uint8_t PORT_PINS[8] = {
    PIN_RF1, PIN_RF2, PIN_RF3, PIN_RF4,
    PIN_RF5, PIN_RF6, PIN_RF7, PIN_RF8,
};

static String buf;

static void allPortsOffLocal() {
    for (int i = 0; i < 8; i++) digitalWrite(PORT_PINS[i], LOW);
}

static void help() {
    Serial.println(F("CLI commands:"));
    Serial.println(F("  help                 - this list"));
    Serial.println(F("  status               - print config + runtime state"));
    Serial.println(F("  ls                   - list files on the drive"));
    Serial.println(F("  cat                  - print config.txt"));
    Serial.println(F("  port <0..8>          - activate RF port (0 = all off)"));
    Serial.println(F("  att <dB>             - set attenuator (0..31.5, - disables)"));
    Serial.println(F("  freq <MHz>           - retune LoRa RX"));
    Serial.println(F("  power <5..23>        - set LoRa TX power (dBm)"));
    Serial.println(F("  free                 - uptime + free heap"));
    Serial.println(F("  formatdisk           - WIPE the USB drive and reboot"));
    Serial.println(F("  reboot               - software reset"));
    Serial.println(F("  bootloader           - jump to UF2 bootloader"));
    Serial.println(F("  rxtest [sec]         - listen for LoRa packets for N seconds"));
    Serial.println(F("  ack [text]           - manually TX a plaintext ACK (debug TX path)"));
}

// Listen for raw LoRa packets for `sec` seconds and dump every byte with RSSI/SNR.
// Does NOT decrypt or validate header - useful when you're debugging "am I hearing
// ANYTHING on this freq?"
static void rxtest(int sec) {
    if (sec <= 0) sec = 10;
    Serial.printf("rxtest: listening for %d seconds...\r\n", sec);
    unsigned long start = millis();
    unsigned long timeoutMs = (unsigned long)sec * 1000UL;
    int count = 0;
    while (millis() - start < timeoutMs) {
        int packetSize = LoRa.parsePacket();
        if (packetSize > 0) {
            count++;
            int rssi = LoRa.packetRssi();
            float snr = LoRa.packetSnr();
            Serial.printf("  #%d len=%d rssi=%d snr=%.1f hex=[",
                          count, packetSize, rssi, snr);
            int n = 0;
            while (LoRa.available() && n < 32) {
                uint8_t b = LoRa.read();
                Serial.printf("%02x ", b);
                n++;
            }
            // Drain remaining
            while (LoRa.available()) (void)LoRa.read();
            if (packetSize > 32) Serial.print("...");
            Serial.println("]");
        }
        delay(1);
    }
    Serial.printf("rxtest: %d packet%s received.\r\n",
                  count, count == 1 ? "" : "s");
}

static void status() {
    Serial.printf("name:           %s\r\n", config.name.c_str());
    Serial.printf("default_port:   %d\r\n", config.defaultPort);
    Serial.printf("lora_frequency: %.3f MHz\r\n", config.loraFrequency);
    Serial.printf("tx_power:       %d dBm\r\n", config.txPower);
    Serial.printf("lora_mode:      %s\r\n", config.loraMode.c_str());
    Serial.print("aes_key:        ");
    for (int i = 0; i < 16; i++) Serial.printf("%02x", config.aesKey[i]);
    Serial.println();
    Serial.printf("driveConnected: %s\r\n", driveConnected ? "yes" : "no");
    Serial.printf("uptime:         %lu s\r\n", (unsigned long)(millis() / 1000));
    Serial.printf("free heap:      %u bytes\r\n", (unsigned)rp2040.getFreeHeap());

    int activePort = -1;
    for (int i = 0; i < 8; i++) {
        if (digitalRead(PORT_PINS[i]) == HIGH) { activePort = i + 1; break; }
    }
    Serial.printf("active port:    %d\r\n", activePort);
    Serial.printf("att_act:        %s\r\n",
                  digitalRead(PIN_ATT_ACT) == HIGH ? "enabled" : "disabled");
    Serial.printf("lora rssi (last): %d dBm\r\n", LoRa.packetRssi());
}

static void cat() {
    File f = FatFS.open("/config.txt", "r");
    if (!f) { Serial.println(F("cat: cannot open /config.txt")); return; }
    while (f.available()) Serial.write(f.read());
    f.close();
    Serial.println();
}

static void ls() {
    Dir d = FatFS.openDir("/");
    uint32_t count = 0, total = 0;
    while (d.next()) {
        String name = d.fileName();
        uint32_t sz = d.fileSize();
        total += sz;
        count++;
        Serial.print("  ");
        if (sz < 1024)
            Serial.printf("%6u B  ", (unsigned)sz);
        else if (sz < 1024 * 1024)
            Serial.printf("%6.1f KB ", sz / 1024.0f);
        else
            Serial.printf("%6.2f MB ", sz / 1048576.0f);
        Serial.println(name);
    }
    Serial.printf("%lu file%s, %.1f KB total\r\n",
                  (unsigned long)count, count == 1 ? "" : "s", total / 1024.0f);
}

static void exec(String line) {
    line.trim();
    if (line.length() == 0) return;

    int sp = line.indexOf(' ');
    String head = sp < 0 ? line : line.substring(0, sp);
    String arg = sp < 0 ? String() : line.substring(sp + 1);
    arg.trim();

    if (head == "help" || head == "?") {
        help();
    } else if (head == "status") {
        status();
    } else if (head == "ls") {
        ls();
    } else if (head == "cat") {
        cat();
    } else if (head == "port") {
        int n = arg.toInt();
        if (n < 0 || n > 8) { Serial.println(F("port: 0..8")); return; }
        allPortsOffLocal();
        if (n >= 1) {
            digitalWrite(PORT_PINS[n - 1], HIGH);
            Serial.printf("port %d on\r\n", n);
        } else {
            Serial.println(F("all ports off"));
        }
        savePersistentState(n, currentActiveAttDb());
    } else if (head == "att") {
        if (arg.length() == 0 || arg == "-") {
            attenuatorDisable();
            Serial.println(F("attenuator disabled"));
            savePersistentState(currentActivePort(), -1.0f);
        } else {
            float db = arg.toFloat();
            attenuatorSet(db);
            Serial.printf("attenuator -> %.1f dB\r\n", db);
            savePersistentState(currentActivePort(), db);
        }
    } else if (head == "freq") {
        double mhz = arg.toFloat();
        if (mhz < 100.0 || mhz > 1000.0) {
            Serial.println(F("freq: expected MHz in LoRa range"));
            return;
        }
        config.loraFrequency = (float)mhz;
        LoRa.end();
        if (!LoRa.begin((long)(mhz * 1E6))) {
            Serial.println(F("LoRa re-init failed"));
            return;
        }
        LoRa.setTxPower(config.txPower);
        LoRa.enableCrc();
        Serial.printf("lora tuned -> %.3f MHz\r\n", mhz);
    } else if (head == "power") {
        int p = arg.toInt();
        if (p < 2 || p > 23) { Serial.println(F("power: 2..23")); return; }
        config.txPower = p;
        LoRa.setTxPower(p);
        Serial.printf("tx_power -> %d dBm\r\n", p);
    } else if (head == "free") {
        Serial.printf("uptime:    %lu s\r\n", (unsigned long)(millis() / 1000));
        Serial.printf("free heap: %u bytes\r\n", (unsigned)rp2040.getFreeHeap());
    } else if (head == "formatdisk") {
        Serial.println(F("formatdisk: wiping filesystem in 3s..."));
        Serial.flush();
        delay(3000);
        FatFS.end();
        if (FatFS.format())
            Serial.println(F("format ok, rebooting..."));
        else
            Serial.println(F("format FAILED, rebooting anyway..."));
        Serial.flush();
        delay(200);
        rp2040.reboot();
    } else if (head == "rxtest") {
        rxtest(arg.length() ? arg.toInt() : 10);
    } else if (head == "ack") {
        String text = arg.length() ? arg : String("ACK:test");
        Serial.printf("manual ack: '%s'\r\n", text.c_str());
        sendAck(text.c_str());
        Serial.println(F("manual ack returned to CLI"));
    } else if (head == "reboot") {
        Serial.println(F("rebooting..."));
        delay(100);
        rp2040.reboot();
    } else if (head == "bootloader") {
        Serial.println(F("jumping to UF2 bootloader..."));
        delay(200);
        reset_usb_boot(0, 0);
        while (true);
    } else {
        Serial.print(F("? "));
        Serial.println(line);
    }
}

void cliBegin() {
    buf.reserve(128);
    Serial.println(F("CLI ready. Type 'help'."));
}

void cliTick() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\r' || c == '\n') {
            if (buf.length() > 0) {
                String line = buf;
                buf = "";
                Serial.println();
                exec(line);
            }
        } else if (c == 0x7f || c == 0x08) {
            if (buf.length() > 0) {
                buf.remove(buf.length() - 1);
                Serial.print("\b \b");
            }
        } else if (c >= 0x20 && c < 0x7f) {
            if (buf.length() < 200) {
                buf += c;
                Serial.write(c);
            }
        }
    }
}
