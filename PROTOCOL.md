# RF.Guru 8-Port LoRa Switch — Protocol Specification

This document describes the over-the-air protocol used between a **LoRa controller** (typically [LoRa868Stick](https://github.com/Guru-RF/LoRa868Stick)) and the **8-port LoRa switch**. It covers the RF modem configuration, packet framing, encryption, command grammar, and the ACK reply. Anyone writing a new controller, reverse-engineering traffic, or porting to another MCU should be able to build an interoperable implementation from this document alone.

---

## 1. RF Modem Configuration

All packets use standard Semtech LoRa modulation on the 868 MHz ISM band.

| Parameter       | Default value | Configurable via |
|-----------------|--------------|------------------|
| Frequency       | 868.000 MHz  | `lora_frequency` |
| TX power (ACK)  | 2 dBm (PA_BOOST) | `tx_power` |
| Sync word       | 0x12 (public LoRa) | (hard-coded) |
| CRC             | enabled      | (hard-coded) |
| Preamble length | 8 symbols    | (library default) |

Two interoperable modem profiles are defined. **Both ends must use the same profile** or they will not hear each other.

| Profile | Spreading factor | Bandwidth | Coding rate | Typical airtime (18-byte payload) | Link budget vs. `fast` |
|---------|------------------|-----------|-------------|-----------------------------------|------------------------|
| **fast** *(default)* | SF7  | 125 kHz | 4/5 | ~50 ms   | 0 dB |
| **slow**             | SF12 | 125 kHz | 4/8 | ~1.3 s   | +10 dB |

Configured by the `lora_mode` key in the switch's `/config.txt`.

---

## 2. Packet Framing

Every over-the-air packet — command or ACK — starts with a fixed 3-byte framing header:

```
+--------+--------+--------+----------------------+
| 0x3C   | 0xAA   | 0x01   | payload (0..250 B)   |
+--------+--------+--------+----------------------+
 byte 0   byte 1   byte 2   bytes 3..N
```

- **Byte 0 — `0x3C` (`<`)** — magic / protocol identifier
- **Byte 1 — `0xAA`** — source / from-address marker
- **Byte 2 — `0x01`** — packet version / flags

Any received packet whose first three bytes don't match is dropped.

The framing header is sent **as-is**, not encrypted and not CRC'd at the application layer. The LoRa MAC CRC already protects the whole frame, which is why the application layer can stay this simple.

---

## 3. Command Payload (Controller → Switch)

### 3.1 Encryption

Payload bytes 3..N carry an **AES-128 CBC**-encrypted command. The layout is:

```
+-----------------+-------------------------------+
| IV (16 bytes)   | ciphertext (16 * k bytes)     |
+-----------------+-------------------------------+
```

- **IV** — 16 random bytes, freshly generated for every TX
- **ciphertext** — the plaintext command, **PKCS#7-padded** to a 16-byte boundary, then CBC-encrypted with the 128-bit shared key

The minimum payload size is therefore **16 + 16 = 32 bytes of ciphertext**, plus the 3-byte framing header = **35 bytes on-air**.

The receiver **must** validate the PKCS#7 padding. Any padding byte outside the range `0x01..0x10`, or inconsistent padding bytes, indicates a wrong key or a corrupted packet and the payload must be discarded (the switch logs `Decrypt failed (wrong key?)` in this case).

### 3.2 Plaintext grammar

After decryption and padding removal, the plaintext is ASCII in one of two forms:

```
name/port
name/port/att
```

| Field | Type | Range | Meaning |
|-------|------|-------|---------|
| `name` | string | 1..15 chars | Target device name. Switch ignores commands addressed to any other name. |
| `port` | integer | 0..8 | 1..8 = activate that RF port. 0 = all ports off. Values outside this range → fallback to `default_port`. |
| `att`  | decimal | 0.0..33.0 | (optional) Requested attenuation in dB. Defaults to `0` (attenuator bypassed) when omitted. |

The attenuator, when active, has a **~1.5 dB minimum insertion loss** baked into the board. The firmware subtracts 1.5 from the requested value before driving the DAC, so `att=1.5` → 0 dB extra, `att=31.5` → 30 dB extra. Requests above 33 dB are clamped to 33 dB (i.e. 31.5 dB on the DAC).

### 3.3 Examples

```
sw0/1          → activate RF port 1 on the switch named "sw0", attenuator off
sw0/3/6.0      → activate RF port 3, attenuator at 6.0 dB
sw0/0          → all ports off
sw7/2/12.5     → (ignored unless config.name == "sw7")
```

---

## 4. ACK Payload (Switch → Controller)

After successfully applying a command, the switch transmits a **plaintext** ACK using the same 3-byte framing header. The ACK is sent at `tx_power` (default 2 dBm to minimise RFI into the switch's own USB CDC), using the same modem profile.

### 4.1 Format

```
ACK:<name>/<port>/<att>
```

Where:
- `name` — the switch's configured `name` (echo of what the controller addressed)
- `port` — the port number actually activated (matches the request, or `default_port` on fallback, or `0` if all ports were turned off)
- `att` — the attenuation that was applied, in dB, one decimal (`0.0`, `6.0`, `12.5`, …)

Example: `ACK:sw0/3/6.0`

### 4.2 Intentionally plaintext

The ACK is **not** encrypted. This is deliberate:

- The controller ([LoRa868Stick](https://github.com/Guru-RF/LoRa868Stick)) in `rx` mode only strips the 3-byte header and prints the rest as text — it doesn't implement the AES layer for receive. Plaintext ACK means the operator immediately sees it on their serial terminal.
- The ACK carries no secret — it's information the controller already has (it initiated the command).
- Keeping the ACK short saves airtime, which matters in `slow` mode (1.3 s per TX).

If a future controller firmware wants encrypted ACKs, it would either decrypt them (and the switch would need matching TX code) or the switch can be extended to encrypt — the framing header stays the same either way.

### 4.3 Timing

The switch fires the ACK **immediately** after applying the command, before returning to RX. The controller must therefore stay in RX for at least one airtime window (~80 ms in `fast` mode, ~2 s in `slow` mode) after its TX if it wants to catch the ACK.

---

## 5. Shared Secret — AES-128 Key

The AES-128 key is a 16-byte (128-bit) shared secret. It is configured as a 32-character hex string.

- Switch side: `aes_key` in `/config.txt`
- Controller side: `AES_KEY[16]` in `include/config.h` (as of writing, LoRa868Stick uses a compile-time constant)

**Default key (do NOT use for production):**

```
000102030405060708090a0b0c0d0e0f
```

To set a real shared secret, generate 16 random bytes and encode as hex:

```bash
openssl rand -hex 16
# e.g. a1b2c3d4e5f60718293a4b5c6d7e8f90
```

Update both ends to the same value. Mismatched keys manifest on the switch as:

```
Decrypt failed (cipher=32 bytes, wrong key?)
```

---

## 6. Controller Reference Implementation

See the stick's TX path for the canonical encrypt-and-frame implementation:

- Encrypt + build frame: [LoRa868Stick/src/main.cpp — `loraSendEncrypted()`](https://github.com/Guru-RF/LoRa868Stick/blob/main/src/main.cpp)
- CBC + PKCS#7: [`encryptMessage()`](https://github.com/Guru-RF/LoRa868Stick/blob/main/src/main.cpp) in the same file

Switch RX + ACK TX path:

- Decrypt + dispatch: [`handleSwitchRequest()`](src/main.cpp)
- CBC decrypt + PKCS#7 validation: [`aesCbcDecrypt()`](src/main.cpp)
- ACK TX: [`sendAck()`](src/main.cpp)

---

## 7. Failure Modes

| Symptom on switch | Meaning |
|-------------------|---------|
| `Received short/unknown packet` | < 4 bytes received; framing header can't even be checked. |
| `Received an unknown packet` | Header bytes 0..2 did not match `3C AA 01`. Another LoRa device on the band. |
| `Decrypt failed (cipher=N bytes, wrong key?)` | Header matched, but AES decrypt produced invalid PKCS#7 padding. Key mismatch or the sender isn't using the same protocol. |
| `Received another switch port req packet: <payload>` | Decrypt succeeded and parsed, but `name` didn't match `config.name`. Packet was addressed to a different switch. |
| `Name mismatch: got 'X' (len=N), expected 'Y' (len=M) - no ACK` | Same as above with more detail. No ACK is sent. |
| `PORT REQ: Turned port N on` + `[ack] tx 'ACK:...'` | Happy path. |

---

## 8. Change Log

- **v1.0** — initial public protocol (AES-128 CBC, 3-byte header, `fast`/`slow` profiles, plaintext ACK).
