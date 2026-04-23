# 8PswitchLORA – 8-Port RF Switch & Attenuator, LoRa Controlled

The **RF.Guru 8-Port LoRa Switch** is a remote-controlled 8-way RF switch with a built-in 6-bit step attenuator (0 – 31.5 dB in 0.5 dB steps). Commands arrive over an encrypted LoRa link on the 868 MHz band from a [LoRa868Stick](https://github.com/Guru-RF/LoRa868Stick) (or any other controller speaking the same [protocol](PROTOCOL.md)), and the switch selects one of eight RF ports and an attenuation level accordingly.

Typical uses:

- Shared-antenna lab benches where multiple test setups contend for one antenna
- Remote attenuator for receive-path calibration
- Multi-radio hamshack antenna distribution
- Field day / contest station where operators route between several antennas from the logging PC

When ordered, your 8-port switch comes with the **latest firmware pre-installed**.

Buy here [8-Port LoRa RF Switch (868 MHz)](https://shop.rf.guru/)

---

## 🧠 How it Works

Power the device over USB-C. It boots, reads `/config.txt` off its internal flash, restores the **last known port + attenuator state** (or falls back to `default_port` on first boot), and then listens continuously for LoRa packets addressed to its configured `name`.

When a matching command arrives, the switch:

1. Turns off all eight ports (break-before-make)
2. Programs the requested attenuation level (or bypasses the attenuator for `0` dB)
3. Activates the requested port
4. Transmits a plaintext **ACK** back over LoRa so the controller knows the command landed

While the device is plugged into a computer it also presents as a **USB mass storage drive** labelled `LORA8PSW`. Drop in a new `config.txt`, save, and eject — the firmware reboots and comes up with the new settings.

---

## 💡 Status Output

The switch has no onboard status LED by design — the active RF port itself tells you which path is live. Everything else is reported over USB-CDC serial at 115 200 baud.

On serial you'll see:

- `[boot] ...` — trace through startup
- `[hb] up=N s heap=N drv=yes|no` — 10-second heartbeat
- `[<uptime>] RX N bytes rssi=<dBm> snr=<dB> hex=[...]` — every LoRa frame received
- `[<uptime>] PORT REQ: Name: sw0 Port: 1 Attenuator: 3.5` — parsed command
- `[ack] tx 'ACK:sw0/1/3.5' (...)` — ACK transmitted

---

## 🛠 Configuration – `config.txt`

All settings live in `/config.txt` on the USB drive. Delete it and eject to regenerate a fresh default. If the filesystem itself is corrupted, the firmware prints `formatting (5s to abort via reset)...` on serial before auto-reformatting — giving you a window to pull power if you didn't intend that.

### Default `config.txt`

```yaml
# ============================================================
# RF.Guru 8-Port LoRa Switch Configuration
# Edit this file and eject the USB drive to apply.
# ============================================================

# Device name - this switch listens for packets addressed to
# this name from the controller.
name: "sw0"

# Default active port at boot (1-8, 0 = all off)
default_port: 1

# LoRa RX frequency in MHz
lora_frequency: 868.000

# LoRa TX power in dBm (2-23) - used for ACK transmission.
# Lower = less RFI into USB / less range.
tx_power: 2

# LoRa modem profile (MUST match the transmitter):
#   fast - SF7 / BW125k / CR4-5  (default, ~50ms airtime)
#   slow - SF12 / BW125k / CR4-8 (+10dB range, ~1.3s airtime)
lora_mode: "fast"

# AES-128 CBC key as 32 hex chars (must match the transmitter)
aes_key: "000102030405060708090a0b0c0d0e0f"
```

### Fields

| Key              | Type / range | Purpose |
|------------------|--------------|---------|
| `name`           | 1..15 chars  | Device identifier; commands must be addressed to this name. |
| `default_port`   | 0..8         | Which port is active at boot. `0` = all off. |
| `lora_frequency` | MHz          | Carrier frequency (default 868.000 for EU ISM). |
| `tx_power`       | 2..23 dBm    | ACK TX power. Lower it to reduce RFI into the USB CDC. |
| `lora_mode`      | `fast`/`slow` | Modem profile. Must match the controller. |
| `aes_key`        | 32 hex chars | Shared AES-128 key. Must match the controller. |

---

## 📡 Over-the-Air Protocol

See [PROTOCOL.md](PROTOCOL.md) for the full specification. Quick summary:

- **Frequency**: 868 MHz
- **Modem**: SF7/BW125k/CR4-5 (fast) or SF12/BW125k/CR4-8 (slow), sync word 0x12, CRC on
- **Framing**: 3-byte header `0x3C 0xAA 0x01` followed by the payload
- **Command**: AES-128 CBC encrypted (IV || ciphertext), plaintext format `name/port[/att]`
- **ACK**: plaintext `ACK:<name>/<port>/<att>` with the same 3-byte header

Minimum on-air frame is 35 bytes (3 header + 16 IV + 16 ciphertext block).

---

## 🎛 Attenuator

The switch has a **6-bit step attenuator** on the RF output, 0 – 31.5 dB in 0.5 dB steps (via `GP6 / GP5 / GP26 / GP27 / GP28 / GP29` driving the attenuator DAC, enabled by `GP25`).

The attenuator has a fixed **~1.5 dB insertion loss** built into the signal path. The firmware compensates: your `att` value is the *total* attenuation you want (insertion loss + pad), and the DAC is driven accordingly.

| Requested `att` | Result |
|-----------------|--------|
| `0`             | Attenuator **bypassed** (0 dB net, 1.5 dB insertion loss not applied) |
| `1.5`           | 0 dB on the DAC → 1.5 dB total |
| `6.0`           | 4.5 dB on the DAC → 6 dB total |
| `33.0` and up   | Clamped to 33 (max DAC = 31.5 dB) |

---

## 🗣 Serial CLI

While the switch is connected via USB-CDC, a live command line is available (115 200 baud). Type `help` for the full list. Highlights:

| Command | Effect |
|---------|--------|
| `help` | List all CLI commands |
| `status` | Print config + runtime state (name, active port, RSSI, heap, uptime) |
| `ls` | List files on the drive with sizes |
| `cat` | Print `/config.txt` |
| `port <0..8>` | Manually activate a port (0 = all off) |
| `att <dB>` | Manually set attenuator in dB (or `-` to disable) |
| `freq <MHz>` | Retune the LoRa RX live |
| `power <2..23>` | Change ACK TX power |
| `rxtest [sec]` | Actively listen for LoRa packets for N seconds and dump raw bytes |
| `ack [text]` | Manually TX a plaintext ACK (debug TX path) |
| `free` | Print uptime + free heap |
| `formatdisk` | **Wipe** the filesystem (3 s warning, then reboots — everything gone) |
| `reboot` | Software reset |
| `bootloader` | Jump to UF2 bootloader mode |

The CLI echoes what you type and handles backspace, so it works as a real shell.

---

## 🔁 Firmware Update

**Pre‑built UF2 files are attached to every [GitHub release](https://github.com/Guru-RF/8PswitchLORA/releases).** You don't need to build the firmware yourself — grab `8PswitchLORA-vX.Y.Z.uf2` from the latest release's **Assets** section.

To flash it:

1. Open `config.txt` on the `LORA8PSW` drive
2. Replace its contents with the single word `firmwareupdate`
3. Save the file
4. **Eject or safely remove** the `LORA8PSW` drive
5. The device will reboot into **UF2 bootloader mode** and appear as a drive named `RPI-RP2`
6. Drag the downloaded `8PswitchLORA-*.uf2` file onto the `RPI-RP2` drive
7. The firmware will update and reboot automatically
   ✅ Your configuration (`config.txt`) remains untouched

Alternatively, from the serial CLI type `bootloader` — same effect.

### Building from source (optional)

If you want to build your own firmware, you need [PlatformIO](https://platformio.org/install) (`pip install platformio`):

```bash
pio run -e pico                 # build firmware → .pio/build/pico/firmware.uf2
pio run -e pico -t upload       # build + flash via USB (device must be in UF2 mode)
```

Every push to `main` and every tag is built automatically via GitHub Actions ([`.github/workflows/build.yml`](.github/workflows/build.yml)). Pushing a tag like `v1.2.3` triggers an attached UF2 on the matching release.

---

## 🔄 Restoring Default Configuration

Delete `config.txt` from the `LORA8PSW` drive and eject. On next boot the firmware writes a fresh default file.

For a completely clean start, use `formatdisk` from the serial CLI — it wipes the whole filesystem, then reboots and regenerates the default `config.txt`.

---

## 📻 RFI Considerations

Transmitting an ACK while the USB CDC is live can couple RF into the USB data lines on some PCB builds, temporarily disturbing the serial console. To minimise this:

- Keep `tx_power: 2` (minimum for PA_BOOST) unless you genuinely need more range
- Use a **ferrite choke** on the USB cable near the device
- Keep the USB cable short
- Terminate the antenna output into **50 Ω** (dummy load) during bench testing — this eliminates the issue entirely and confirms it's RFI, not firmware

The *switching* itself is not affected by this — port changes happen in the main loop regardless of CDC state. Only the ASCII trace on the serial console may drop lines around a TX event.

---

## 📦 Features Summary

- 8 RF ports switched by LoRa commands on 868 MHz
- 6-bit step attenuator 0 – 31.5 dB in 0.5 dB steps, with insertion-loss compensation
- **Non-volatile state**: last port + attenuator setting is stored in flash (EEPROM-emulated) and restored after power loss or reboot
- AES-128 CBC encrypted command payload with PKCS#7 padding
- Two modem profiles: `fast` (SF7, ~50 ms airtime) and `slow` (SF12, +10 dB range)
- Plaintext ACK transmission for end-to-end command confirmation
- USB Mass Storage drive `LORA8PSW` for config editing
- YAML-style `config.txt` with live reload on eject
- Auto-cleanup of macOS metadata every boot (no more `.Trashes` filling the drive)
- Live serial CLI: status, `ls`/`cat`, manual port/att control, `rxtest`, `formatdisk`, `bootloader`
- `firmwareupdate` keyword in `config.txt` triggers UF2 bootloader for drag-and-drop update
- GitHub Actions CI builds a UF2 on every push to `main` and attaches one to every release tag
- Watchdog-protected main loop (8 s) — a locked-up receive path self-heals in < 10 s

---

## 📃 License

This project is licensed under the [MIT License](LICENSE).

---

## 🧃 Credits

Developed by [ON6URE – RF.Guru](https://rf.guru)
Firmware, schematics, and documentation licensed under MIT.
