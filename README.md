# RGB Cube

A **6×6×6 RGB LED matrix cube** (216 WS2812B-compatible LEDs) that plays 3D
animations, driven by an ESP32. Firmware, 3D-printable parts, and build notes
all live in this repo.

> Built for the **"Rechnerarchitektur"** class at **Nordakademie**.

> [!WARNING]
> **The firmware is heavily "vibecoded."**
> Almost all of my time on this project went into the *physical* build —
> soldering 216 LEDs, printing jigs, bending legs, and wiring the cube took
> *really* long. To actually get animations running on the finished cube, the
> ESP32 code in [`code/`](code/) was written largely with AI assistance. It
> works on the real hardware, but don't read it as a model of clean,
> hand-crafted embedded C++.

## Hardware

| | |
|---|---|
| **LEDs** | 8 mm through-hole RGB, integrated WS2812B-compatible controller |
| **Grid** | 6 × 6 × 6 = 216 LEDs, 25 mm pitch, ~125 mm cube |
| **Wiring** | Single-wire WS2812B serpentine daisy-chain |
| **MCU** | ESP32 DevKit V1 (data on GPIO 14, mode button on GPIO 34) |
| **Power** | USB-C PD board (~5 A @ 5 V) |

Full geometry, electronics and enclosure specs: [`CLAUDE.md`](CLAUDE.md).

## Repository layout

```
rgb-cube/
├── code/      ESP32 firmware (PlatformIO)
├── models/    3D-printable parts (OpenSCAD sources + STLs + previews)
├── docs/      design notes (plans / specs)
├── BUILD.md   firmware build & dev guide
└── CLAUDE.md  hardware / geometry / model reference
```

## Firmware — [`code/`](code/)

PlatformIO project (`esp32dev` env, Arduino framework, Adafruit NeoPixel).
A hardware button cycles through animation modes; the active mode is persisted
in NVS across reboots.

**Build & flash** (easiest via the VSCode PlatformIO extension):

```bash
cd code
pio run -e esp32dev            # build
pio run -e esp32dev -t upload  # flash
```

See [`BUILD.md`](BUILD.md) for the Cube/Mode API and how to add a mode.

## 3D models — [`models/`](models/)

OpenSCAD sources with exported STLs and a render preview, one folder per part.
Printed on a Bambu Lab A1 Mini in PLA.

| Folder | Part |
|---|---|
| [`layer_jig/`](models/layer_jig/) | Soldering jig — holds one 6×6 LED layer flat while soldering |
| [`electronics_enclosure/`](models/electronics_enclosure/) | Base enclosure (body + lid) that hides the ESP32 and PD board; the cube sits on top |
| [`led_pin_bender/`](models/led_pin_bender/) | Tool to bend LED legs to a consistent shape |
| [`tests/`](models/tests/) | Throwaway calibration prints (LED hole fit, standoff fit) |

## Status

The cube is built and running animations. Open items (firmware tuning, final
wiring scheme) are tracked at the bottom of [`CLAUDE.md`](CLAUDE.md).
