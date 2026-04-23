// ============================================================
// RF.Guru 8-Port LoRa Switch
// Converted from CircuitPython to Arduino C++ for RP2040
// RF.Guru - ON6URE
// ============================================================

#include <Arduino.h>
#include <FatFS.h>
#include <FatFSUSB.h>
#include <SPI.h>
#include <LoRa.h>
#include <Crypto.h>
#include <AES.h>
#include <hardware/watchdog.h>

#include "pins.h"
#include "attenuator.h"
#include "config.h"
#include "cli.h"

// Default AES-128 CBC key, matches LoRa868Stick/include/config.h
static const uint8_t DEFAULT_AES_KEY[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

// ============================================================
// VERSION
// ============================================================

#define VERSION "RF.Guru_8P_Switch_LoRa 1.0"

// 3-byte packet header (destination, source, id). Matches the
// CircuitPython receiver that checked `packet[:3] == b"<\xaa\x01"`.
static const uint8_t LORA_HEADER[] = { 0x3C, 0xAA, 0x01 };

// ============================================================
// ANSI color helpers (match the Python look)
// ============================================================

static void purple(const char *msg) {
    unsigned long s = millis() / 1000;
    Serial.printf("\x1b[38;5;104m[%lu] %s\x1b[0m\r\n", s, msg);
}

static void yellow(const char *msg) {
    Serial.printf("\x1b[38;5;220m%s\x1b[0m\r\n", msg);
}

static void red(const char *msg) {
    Serial.printf("\x1b[1;5;31m -- %s\x1b[0m\r\n", msg);
}

static void green(const char *msg) {
    Serial.printf("\x1b[1;5;32m%s\x1b[0m\r\n", msg);
}

// ============================================================
// Globals (declared extern in config.h)
// ============================================================

Config config;
volatile bool driveConnected = false;
static volatile bool drivePlugEvent = false;
static volatile bool driveUnplugEvent = false;

static void setDefaults(Config &c) {
    c.name = "sw0";
    c.defaultPort = 1;
    c.loraFrequency = 868.000f;
    c.txPower = 2;
    c.loraMode = "fast";
    memcpy(c.aesKey, DEFAULT_AES_KEY, 16);
}

// Parse a 32-char hex string into 16 bytes. Returns false if malformed.
static bool parseHexKey(const String &hex, uint8_t out[16]) {
    if (hex.length() != 32) return false;
    for (int i = 0; i < 16; i++) {
        char a = hex.charAt(i * 2);
        char b = hex.charAt(i * 2 + 1);
        int hi = (a >= '0' && a <= '9') ? a - '0' :
                 (a >= 'a' && a <= 'f') ? a - 'a' + 10 :
                 (a >= 'A' && a <= 'F') ? a - 'A' + 10 : -1;
        int lo = (b >= '0' && b <= '9') ? b - '0' :
                 (b >= 'a' && b <= 'f') ? b - 'a' + 10 :
                 (b >= 'A' && b <= 'F') ? b - 'A' + 10 : -1;
        if (hi < 0 || lo < 0) return false;
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    return true;
}

static const char *defaultYaml() {
    return
        "# ============================================================\n"
        "# RF.Guru 8-Port LoRa Switch Configuration\n"
        "# Edit this file and eject the USB drive to apply.\n"
        "# ============================================================\n"
        "\n"
        "# Device name - this switch listens for packets addressed to\n"
        "# this name from the controller.\n"
        "name: \"sw0\"\n"
        "\n"
        "# Default active port at boot (1-8, 0 = all off)\n"
        "default_port: 1\n"
        "\n"
        "# LoRa RX frequency in MHz\n"
        "lora_frequency: 868.000\n"
        "\n"
        "# LoRa TX power in dBm (2-23) - used for ACK transmission.\n"
        "# Lower = less RFI into USB / less range.\n"
        "tx_power: 2\n"
        "\n"
        "# LoRa modem profile (MUST match the transmitter):\n"
        "#   fast - SF7 / BW125k / CR4-5  (default, ~50ms airtime)\n"
        "#   slow - SF12 / BW125k / CR4-8 (+10dB range, ~1.3s airtime)\n"
        "lora_mode: \"fast\"\n"
        "\n"
        "# AES-128 CBC key as 32 hex chars (must match the transmitter)\n"
        "aes_key: \"000102030405060708090a0b0c0d0e0f\"\n";
}

static bool yamlParse(const char *yaml, Config &cfg) {
    String line;
    while (*yaml) {
        line = "";
        while (*yaml && *yaml != '\n')
            line += *yaml++;
        if (*yaml == '\n')
            yaml++;

        line.trim();
        if (line.startsWith("#") || line.length() == 0)
            continue;

        int colonIndex = line.indexOf(':');
        if (colonIndex <= 0)
            continue;

        String key = line.substring(0, colonIndex);
        String value = line.substring(colonIndex + 1);
        key.trim();
        value.trim();

        {
            bool inQuote = false;
            int cut = -1;
            for (int i = 0; i < (int)value.length(); i++) {
                char c = value.charAt(i);
                if (c == '"') inQuote = !inQuote;
                else if (c == '#' && !inQuote) { cut = i; break; }
            }
            if (cut >= 0) value = value.substring(0, cut);
            value.trim();
        }

        value.replace("\"", "");
        value.trim();

        if (key == "name")                 cfg.name = value;
        else if (key == "default_port")    cfg.defaultPort = constrain(value.toInt(), 0, 8);
        else if (key == "lora_frequency")  cfg.loraFrequency = value.toFloat();
        else if (key == "tx_power")        cfg.txPower = constrain(value.toInt(), 2, 23);
        else if (key == "lora_mode") {
            String v = value; v.toLowerCase();
            if (v == "fast" || v == "slow") cfg.loraMode = v;
            else Serial.println("[config] lora_mode: expected 'fast' or 'slow', ignoring");
        }
        else if (key == "aes_key") {
            uint8_t k[16];
            if (parseHexKey(value, k)) memcpy(cfg.aesKey, k, 16);
            else Serial.println("[config] aes_key: expected 32 hex chars, ignoring");
        }
    }
    return true;
}

// ============================================================
// USB drive callbacks
// ============================================================

static void unplug(uint32_t i) {
    (void)i;
    driveConnected = false;
    driveUnplugEvent = true;
    rp2040.reboot();
}

static void plug(uint32_t i) {
    (void)i;
    driveConnected = true;
    drivePlugEvent = true;
    // Keep FatFS mounted so CLI reads (ls/cat) still work. USB MSC
    // accesses flash at the block layer, and reboot-on-eject ensures
    // any host-side changes take effect cleanly.
}

static bool mountable(uint32_t i) {
    (void)i;
    driveConnected = true;
    return true;
}

// ============================================================
// macOS metadata cleanup
// ============================================================

static bool rmRecursive(const String &path) {
    Dir d = FatFS.openDir(path.c_str());
    while (d.next()) {
        String name = d.fileName();
        String full = path;
        if (!full.endsWith("/")) full += "/";
        full += name;
        if (d.isDirectory()) rmRecursive(full);
        else FatFS.remove(full.c_str());
    }
    if (path != "/") return FatFS.rmdir(path.c_str());
    return true;
}

static void cleanMacMetadata() {
    int removed = 0;
    Dir d = FatFS.openDir("/");
    while (d.next()) {
        String name = d.fileName();
        String full = "/" + name;
        bool isMeta = name.startsWith("._") || name == ".DS_Store" ||
                      name == ".Spotlight-V100" || name == ".Trashes" ||
                      name == ".fseventsd" || name == ".TemporaryItems" ||
                      name == ".apdisk";
        if (!isMeta) continue;
        if (d.isDirectory()) {
            if (rmRecursive(full)) removed++;
        } else {
            if (FatFS.remove(full.c_str())) removed++;
        }
    }
    if (removed > 0) {
        Serial.printf("Cleaned %d macOS metadata entries.\r\n", removed);
    }
}

// ============================================================
// RF port helpers
// ============================================================

static const uint8_t PORT_PINS[8] = {
    PIN_RF1, PIN_RF2, PIN_RF3, PIN_RF4,
    PIN_RF5, PIN_RF6, PIN_RF7, PIN_RF8,
};

static void allPortsOff() {
    for (int i = 0; i < 8; i++) {
        digitalWrite(PORT_PINS[i], LOW);
    }
}

static void portOn(int n) {
    if (n >= 1 && n <= 8) {
        digitalWrite(PORT_PINS[n - 1], HIGH);
    }
}

static float roundToHalf(float v) {
    return lroundf(v * 2.0f) / 2.0f;
}

// ============================================================
// Packet handling
// ============================================================

// Transmit a plaintext ACK with the 3-byte header, so the LoRa868Stick
// in "rx" mode can show it to the user (stick only decodes the header,
// not the payload). Keep it short - airtime on slow mode is expensive.
// Simple TX, mirrors LoRa868Stick's loraSend() exactly.
void sendAck(const char *text) {
    uint8_t frame[96];
    frame[0] = 0x3C;
    frame[1] = 0xAA;
    frame[2] = 0x01;
    size_t textLen = strlen(text);
    if (textLen > sizeof(frame) - 3) textLen = sizeof(frame) - 3;
    memcpy(frame + 3, text, textLen);

    Serial.printf("[ack] tx '%s' (%u bytes)\r\n",
                  text, (unsigned)(3 + textLen));

    LoRa.beginPacket();
    LoRa.write(frame, 3 + textLen);
    LoRa.endPacket();
}

static void handleSwitchRequest(const char *payload) {
    char name[16] = {0};
    char portStr[8] = {0};
    char attStr[16] = {0};

    const char *p1 = strchr(payload, '/');
    if (!p1) { yellow("Malformed packet"); return; }

    int nameLen = p1 - payload;
    if (nameLen >= (int)sizeof(name)) nameLen = sizeof(name) - 1;
    memcpy(name, payload, nameLen);
    name[nameLen] = '\0';

    const char *portStart = p1 + 1;
    const char *p2 = strchr(portStart, '/');
    int portLen = p2 ? (p2 - portStart) : (int)strlen(portStart);
    if (portLen >= (int)sizeof(portStr)) portLen = sizeof(portStr) - 1;
    memcpy(portStr, portStart, portLen);
    portStr[portLen] = '\0';

    if (p2) {
        strncpy(attStr, p2 + 1, sizeof(attStr) - 1);
    } else {
        attStr[0] = '0';
        attStr[1] = '\0';
    }

    int setport = atoi(portStr);
    float setatt = roundToHalf(atof(attStr));

    char buf[160];
    snprintf(buf, sizeof(buf),
             "PORT REQ: Name: %s Port: %d Attenuator: %.1f",
             name, setport, setatt);
    purple(buf);

    if (strcmp(name, config.name.c_str()) != 0) {
        snprintf(buf, sizeof(buf),
                 "Name mismatch: got '%s' (len=%d), expected '%s' (len=%d) - no ACK",
                 name, (int)strlen(name),
                 config.name.c_str(), (int)config.name.length());
        yellow(buf);
        return;
    }
    Serial.printf("[switch] name match '%s' - will ACK after apply\r\n", name);

    allPortsOff();

    if (setatt == 0.0f) {
        attenuatorDisable();
    } else {
        if (setatt > 33.0f) setatt = 33.0f;
        setatt -= 1.5f;
        if (setatt < 0.0f) setatt = 0.0f;
        attenuatorSet(setatt);
    }

    int activePort = -1;
    if (setport == 0) {
        attenuatorDisable();
        activePort = 0;
    } else if (setport >= 1 && setport <= 8) {
        portOn(setport);
        snprintf(buf, sizeof(buf), "PORT REQ: Turned port %d on", setport);
        purple(buf);
        activePort = setport;
    } else {
        portOn(config.defaultPort);
        snprintf(buf, sizeof(buf),
                 "PORT REQ: Wrong Port NR Turned default port %d on",
                 config.defaultPort);
        purple(buf);
        activePort = config.defaultPort;
    }

    char ack[64];
    snprintf(ack, sizeof(ack), "ACK:%s/%d/%.1f",
             config.name.c_str(), activePort, setatt);
    sendAck(ack);
}

// ============================================================
// LoRa receive
// ============================================================

// XOR 16 bytes in place: dst ^= src
static void xorBlock16(uint8_t *dst, const uint8_t *src) {
    for (int i = 0; i < 16; i++) dst[i] ^= src[i];
}

// AES-128 CBC decrypt. Input = 16-byte IV || ciphertext (multiple of 16).
// Strips PKCS7 padding. Returns plaintext length, or 0 on error.
// Matches LoRa868Stick/src/main.cpp decryptMessage().
static size_t aesCbcDecrypt(const uint8_t *payload, size_t payloadLen,
                            uint8_t *out, size_t outSize) {
    if (payloadLen < 32 || (payloadLen - 16) % 16 != 0) return 0;
    size_t cipherLen = payloadLen - 16;
    if (cipherLen > outSize) return 0;

    AES128 aes;
    aes.setKey(config.aesKey, 16);

    const uint8_t *iv = payload;
    const uint8_t *ct = payload + 16;
    const uint8_t *prev = iv;
    for (size_t off = 0; off < cipherLen; off += 16) {
        aes.decryptBlock(out + off, ct + off);
        xorBlock16(out + off, prev);
        prev = ct + off;
    }

    uint8_t pad = out[cipherLen - 1];
    if (pad == 0 || pad > 16) return 0;
    // Validate padding bytes to reject garbage/wrong-key packets.
    for (size_t i = cipherLen - pad; i < cipherLen; i++) {
        if (out[i] != pad) return 0;
    }
    return cipherLen - pad;
}

static void hexPreview(char *out, size_t outSize,
                       const uint8_t *data, int len, int maxBytes = 16) {
    size_t pos = 0;
    int n = len < maxBytes ? len : maxBytes;
    for (int i = 0; i < n && pos + 3 < outSize; i++) {
        pos += snprintf(out + pos, outSize - pos, "%02x ", data[i]);
    }
    if (len > maxBytes && pos + 4 < outSize) {
        snprintf(out + pos, outSize - pos, "...");
    }
}

static void loraPoll() {
    int packetSize = LoRa.parsePacket();
    if (packetSize <= 0) return;

    watchdog_update();

    // Read the whole frame so we can log it regardless of routing decision
    uint8_t raw[256];
    int rawLen = 0;
    while (LoRa.available() && rawLen < (int)sizeof(raw)) {
        raw[rawLen++] = (uint8_t)LoRa.read();
    }
    int rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();

    char hex[64];
    hexPreview(hex, sizeof(hex), raw, rawLen);

    char line[192];
    snprintf(line, sizeof(line),
             "RX %d bytes rssi=%d snr=%.1f hex=[%s]",
             rawLen, rssi, snr, hex);
    purple(line);

    if (rawLen <= (int)sizeof(LORA_HEADER)) {
        yellow("  -> too short, dropped");
        return;
    }

    if (memcmp(raw, LORA_HEADER, sizeof(LORA_HEADER)) != 0) {
        yellow("  -> header mismatch, dropped");
        return;
    }

    const uint8_t *cipher = raw + sizeof(LORA_HEADER);
    int cipherLen = rawLen - sizeof(LORA_HEADER);

    uint8_t plainBuf[256];
    size_t plainLen = aesCbcDecrypt(cipher, cipherLen, plainBuf,
                                    sizeof(plainBuf) - 1);
    if (plainLen == 0) {
        char buf[96];
        snprintf(buf, sizeof(buf),
                 "  -> decrypt failed (cipher=%d bytes, wrong key?)", cipherLen);
        yellow(buf);
        return;
    }
    plainBuf[plainLen] = '\0';

    handleSwitchRequest((const char *)plainBuf);
}

// ============================================================
// Default config writer (chunked, flush-per-chunk)
// ============================================================

static void writeDefaultConfig(const char *path) {
    FatFS.remove(path);
    File f = FatFS.open(path, "w");
    if (!f) {
        red("ERR: cannot open config.txt for write");
        return;
    }
    const char *def = defaultYaml();
    size_t defLen = strlen(def);
    size_t total = 0;
    const size_t CHUNK = 128;
    while (total < defLen) {
        size_t want = defLen - total;
        if (want > CHUNK) want = CHUNK;
        size_t w = f.write((const uint8_t *)def + total, want);
        if (w == 0 || w == (size_t)-1) {
            red("config.txt chunk write failed");
            break;
        }
        total += w;
        f.flush();
    }
    f.close();
    Serial.printf("Wrote default config.txt (%u bytes)\r\n", (unsigned)total);
    yamlParse(def, config);
}

// ============================================================
// SETUP
// ============================================================

void setup() {
    // RF port pins - start HIGH then pull LOW (matches Python init order)
    const uint8_t initHighPins[] = {
        PIN_RF1, PIN_RF2, PIN_RF3, PIN_RF4,
        PIN_RF5, PIN_RF6, PIN_RF7, PIN_RF8,
    };
    for (unsigned i = 0; i < sizeof(initHighPins); i++) {
        pinMode(initHighPins[i], OUTPUT);
        digitalWrite(initHighPins[i], HIGH);
        delay(10);
    }
    for (unsigned i = 0; i < sizeof(initHighPins); i++) {
        digitalWrite(initHighPins[i], LOW);
        delay(10);
    }

    const uint8_t attPins[] = {
        PIN_ATT1, PIN_ATT2, PIN_ATT3, PIN_ATT4,
        PIN_ATT5, PIN_ATT6, PIN_ATT_ACT,
    };
    for (unsigned i = 0; i < sizeof(attPins); i++) {
        pinMode(attPins[i], OUTPUT);
        digitalWrite(attPins[i], LOW);
        delay(10);
    }

    Serial.begin(115200);
    delay(2000);
    Serial.println("\r\n[boot] starting");

    // --- FatFS with format fallback ------------------------------
    Serial.println("[boot] FatFS.begin()");
    Serial.flush();
    bool fsOk = FatFS.begin();
    if (!fsOk) {
        Serial.println("[boot] FatFS mount failed - formatting (5s to abort)...");
        Serial.flush();
        delay(5000);
        if (FatFS.format() && FatFS.begin()) {
            Serial.println("[boot] FatFS formatted & mounted.");
            fsOk = true;
        } else {
            red("FatFS unrecoverable.");
        }
    } else {
        Serial.println("[boot] FatFS mounted.");
    }
    if (fsOk) {
        fatfs::f_setlabel("LORA8PSW");
        cleanMacMetadata();
    }

    setDefaults(config);

    // --- Load / create config ------------------------------------
    const char *filePath = "/config.txt";
    bool exists = fsOk && FatFS.exists(filePath);
    Serial.printf("[boot] config.txt exists: %s\r\n", exists ? "yes" : "no");

    bool parsed = false;
    if (exists) {
        File file = FatFS.open(filePath, "r");
        if (file) {
            String yamlContent;
            while (file.available()) yamlContent += (char)file.read();
            file.close();
            yamlContent.trim();
            Serial.printf("[boot] config bytes: %d\r\n", yamlContent.length());

            if (yamlContent == "firmwareupdate") {
                red("Firmware update requested.");
                FatFS.remove(filePath);
                delay(500);
                red("Rebooting into UF2 bootloader...");
                delay(500);
                reset_usb_boot(0, 0);
                while (true);
            }

            if (yamlContent.length() > 0) {
                yamlParse(yamlContent.c_str(), config);
                parsed = true;
            }
        }
    }

    if (fsOk && !parsed) {
        Serial.println("[boot] config missing/empty - writing default.");
        Serial.flush();
        writeDefaultConfig(filePath);
    }

    // --- USB mass storage ----------------------------------------
    Serial.println("[boot] starting FatFSUSB");
    Serial.flush();
    delay(200);
    FatFSUSB.onUnplug(unplug);
    FatFSUSB.onPlug(plug);
    FatFSUSB.driveReady(mountable);
    FatFSUSB.begin();
    // USB re-enumerates to add MSC; CDC disconnects then comes back.
    // Long pause so the host tty reattaches before further boot logs.
    delay(5000);
    Serial.println("[boot] FatFSUSB up");

    char banner[128];
    snprintf(banner, sizeof(banner), "%s -=- %s", config.name.c_str(), VERSION);
    red(banner);
    Serial.printf("[boot] config: name=%s default_port=%d freq=%.3f tx_power=%d\r\n",
                  config.name.c_str(), config.defaultPort,
                  config.loraFrequency, config.txPower);

    // --- Default port --------------------------------------------
    allPortsOff();
    if (config.defaultPort >= 1 && config.defaultPort <= 8) {
        portOn(config.defaultPort);
        Serial.printf("[boot] default port %d active\r\n", config.defaultPort);
    } else {
        Serial.println("[boot] all ports off");
    }

    // --- LoRa ----------------------------------------------------
    // GP8/10/11 are SPI1 pins on RP2040 (not SPI0).
    Serial.println("[boot] LoRa SPI1 setup");
    SPI1.setRX(PIN_SPI_MISO);
    SPI1.setTX(PIN_SPI_MOSI);
    SPI1.setSCK(PIN_SPI_CLK);
    SPI1.begin();

    LoRa.setPins(PIN_LORA_CS, PIN_LORA_RST, -1);
    LoRa.setSPI(SPI1);
    LoRa.setSPIFrequency(1000000);

    Serial.printf("[boot] LoRa.begin(%ld Hz)\r\n", (long)(config.loraFrequency * 1E6));
    Serial.flush();
    if (!LoRa.begin((long)(config.loraFrequency * 1E6))) {
        red("LoRa INIT ERROR - check wiring / CS / RST");
        // Do NOT hang forever - CLI still needs to work so user can debug.
    } else {
        LoRa.setTxPower(config.txPower);
        LoRa.enableCrc();

        // Apply modem profile. Both ends MUST match.
        //   fast: SF7 / BW125k / CR4-5 (matches LoRa868Stick defaults)
        //   slow: SF12 / BW125k / CR4-8 (max range, ~25x slower)
        if (config.loraMode == "slow") {
            LoRa.setSpreadingFactor(12);
            LoRa.setSignalBandwidth(125E3);
            LoRa.setCodingRate4(8);
        } else {
            LoRa.setSpreadingFactor(7);
            LoRa.setSignalBandwidth(125E3);
            LoRa.setCodingRate4(5);
        }

        char msg[128];
        snprintf(msg, sizeof(msg),
                 "LoRa OK (freq=%.3f MHz pwr=%d dBm mode=%s)",
                 config.loraFrequency, config.txPower, config.loraMode.c_str());
        green(msg);
    }

    // --- CLI -----------------------------------------------------
    cliBegin();

    // --- Watchdog ------------------------------------------------
    watchdog_enable(8000, true);
    Serial.println("[boot] watchdog enabled (8s)");
    yellow("Waiting for LoRa packet ...");
}

// ============================================================
// MAIN LOOP
// ============================================================

static unsigned long lastHeartbeat = 0;

static void heartbeat() {
    unsigned long now = millis();
    if (now - lastHeartbeat < 10000) return;
    lastHeartbeat = now;
    Serial.printf("[hb] up=%lus heap=%u drv=%s\r\n",
                  (unsigned long)(now / 1000),
                  (unsigned)rp2040.getFreeHeap(),
                  driveConnected ? "yes" : "no");
}

void loop() {
    watchdog_update();

    // Surface USB plug/unplug events from ISR context to the console
    if (drivePlugEvent) {
        drivePlugEvent = false;
        yellow("[usb] drive mounted by host");
    }
    if (driveUnplugEvent) {
        driveUnplugEvent = false;
        yellow("[usb] drive ejected - rebooting");
    }

    cliTick();
    heartbeat();

    // LoRa RX keeps running regardless of USB drive state. USB MSC
    // accesses flash below the FatFS layer; GPIO / LoRa-SPI / port
    // switching don't touch the filesystem at all.
    loraPoll();
    delay(5);
}
