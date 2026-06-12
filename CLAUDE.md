# LED Cube Project

University project: 6×6×6 RGB LED matrix cube for 3D animations.

---

## Hardware

### LEDs — 8mm through-hole RGB with integrated controller
- **Type:** 8mm through-hole RGB LED with integrated WS2812B-compatible controller
- **Protocol:** Single-wire (WS2812B), daisy-chained via DIN → DO
- **Pins:** DIN (data in), VDD (5V), GND, DO (data out)
- **Body diameter:** 8.0 mm

### Cube geometry
- **Grid:** 6 × 6 × 6 = 216 LEDs total
- **LED pitch:** 25 mm center-to-center (17 mm gap between 8 mm LED bodies — comfortable for soldering)
- **Layer footprint:** 125 × 125 mm (5 gaps × 25 mm; corner LEDs at ±62.5 mm from cube center)
- **Overall cube volume:** 125 × 125 × 125 mm
- **Wiring:** Serial WS2812B daisy-chain (single data line), all layers chained together

### Electronics
- **Microcontroller:** ESP32 DevKit V1 (PCB 51.5 × 28.6 mm, 4× M3 mounting holes ⌀3mm at 46.5 × 23.4 mm spacing, source: mischianti.org)
- **Power:** USB-C PD board — APKLVSR B0CQNCFYGV, QC/AFC/PD2.0/PD3.0, PCB ~30 × 20 mm, 2 mounting holes at back (hole spacing **TBD — measure the board**)
- **Power budget:** 216 LEDs × up to 60 mA = ~13 A max at 5 V (plan for ≥5 A USB-C PD)
- **IR receiver:** Elegoo IR receiver module (VS1838B, NEC protocol). Signal → **GPIO34** (input-only pin — fine here, the receiver drives the line push-pull so no pull-up is needed), GND → GND, VCC → **3.3 V** (power at 3.3 V, NOT 5 V — its output idles at VCC and the ESP32 is only 3.3 V tolerant). Full remote mapped in `code/src/main.cpp` (`handleIrCommand`): ▲/▼ next/prev mode, number keys (multi-digit, "1"+"0" → mode 10), `►►|`/`|◄◄` faster/slower animation, VOL+/- brightness, EQ reset brightness, `►||` pause, Power blank, FUNC/STOP restart, ST/REPT auto-cycle (cancelled by any manual mode change). Skip-button NEC bytes `0x43`/`0x44`; firmware logs every received code over serial for remapping.

---

## 3D Models

### Toolchain
- **Printer:** Bambu Lab A1 Mini, 180 × 180 mm bed, 0.4 mm nozzle, PLA
- **CAD:** OpenSCAD 2021.01 — CadQuery not usable (nlopt has no aarch64 pip wheel on Fedora; system Python is 3.14 but project venv is 3.12)
- **Preview:** `PYOPENGL_PLATFORM=egl .venv/bin/python ~/.claude/skills/parametric-3d-printing/preview.py <stl> <png> --views multi`
- **All files:** `models/`

---

### Model 1 — `layer_jig.scad` / `layer_jig.stl`

**Purpose:** Temporary assembly jig. Hold one 6×6 layer of LEDs flat while soldering. LEDs insert from the top (lens up), leads hang below for wiring. Pop out when done.  
**Usage:** Print 1 copy, reuse for all 6 layers, discard after cube is assembled.

| Parameter | Value |
|---|---|
| Board size | 145 × 145 × 3 mm |
| LED hole ⌀ | 8.3 mm (8.0 + 0.3 clearance) |
| Pitch | 25 mm |
| Grid | 6 × 6 = 36 holes |
| Corner radius | 3 mm |

**Print:** PLA, 0.2 mm layer, 3 walls, 20% infill, no supports. Flat side on bed.

---

### Model 2 — `electronics_enclosure.scad`

**Purpose:** Base enclosure the cube sits on top of. Houses ESP32 + USB-C PD board. Only the cube is visible to the user — all electronics are hidden inside.

**Two parts, export separately:**
```bash
openscad -D 'part="body"' -o electronics_enclosure_body.stl electronics_enclosure.scad
openscad -D 'part="lid"'  -o electronics_enclosure_lid.stl  electronics_enclosure.scad
```

#### Body (`electronics_enclosure_body.stl`)

| Parameter | Value |
|---|---|
| Outer size | 140 × 140 × 45 mm |
| Wall / top plate | 2.5 mm |
| Inner cavity | 135 × 135 × 42.5 mm, open bottom |
| Corner radius (outer) | 5 mm |
| Corner filling (inner) | Solid 45° triangular prism, 15 mm leg, full-height — bonds to both walls + ceiling |
| Lid screw holes | M3 pilot ⌀2.5 mm, 12 mm deep into each corner fill (self-tap in PLA) |
| Cube fixation holes (×4) | ⌀3 mm at ±57 mm on top plate — inward from cube corners (±62.5 mm) to stay clear of corner fills |
| USB-C port cutout | 9.5 × 3.5 mm stadium on back face (−Y), centre z = 40.75 mm (top of opening flush with inner ceiling at 42.5 mm). Derived from measured `usbc_port_top_above_board` = 5 mm above PCB bottom face |
| ESP32 standoffs (×4) | ⌀4 mm, M3 ⌀2.5 mm pilot, 5 mm tall, 46.5 × 23.4 mm pattern centred at (+18, +15) |
| USB-C board standoffs (×2) | ⌀3.5 mm, M2 ⌀1.6 mm pilot, 5 mm tall, **5 mm spacing measured** |
| Board surface height | 37.5 mm from bottom rim — both boards level |

**Print:** PLA, 0.2 mm layer, 3 walls, 20% infill, no supports. **Solid top face DOWN on bed** (open bottom faces up during printing).

#### Lid (`electronics_enclosure_lid.stl`)

| Parameter | Value |
|---|---|
| Size | 140 × 140 × 2.5 mm |
| Screw holes (×4) | M3 clearance ⌀3.3 mm, positions match body corner centroids (±60.8 mm from centre) |

**Print:** PLA, 0.2 mm layer, 2 walls, 15% infill, no supports. Flat on bed.

#### Assembly notes
- Cube sits on the solid top face. Thread short wires through the 4 corner fixation holes and around the corner LED leads to hold the cube down.
- Lid screws onto the open bottom with 4× M3 screws (self-tapping into PLA corner fills).
- USB-C PD board and ESP32 both hang from the ceiling on 5 mm standoffs — both boards at the same level, use M2.5 screws.
- Before printing: measure exact USB-C PD board and adjust `usbc_bmx` (hole spacing), `usbc_boy` (Y position), `usbc_w`/`usbc_h` (port opening), `usbc_z` (port centre height).

---

## Key parameters to adjust for future sessions

| What | File | Variable(s) |
|---|---|---|
| LED pitch (if changed) | both files | `pitch` in `layer_jig.scad`; `fix_corner` = pitch×2.5 in `electronics_enclosure.scad` |
| LED body diameter | `layer_jig.scad` | `led_body_d` (currently 8.0 mm) |
| USB-C port opening size/height | `electronics_enclosure.scad` | `usbc_w`, `usbc_h`, `usbc_z` (hole spacing + Y-pos now fixed from measurement) |
| Cube fixation hole size | `electronics_enclosure.scad` | `fix_hole_d` |
| M3 self-tap tightness | `electronics_enclosure.scad` | `lid_screw_pilot` (2.5 mm = tight; 2.7 mm = easier) |
| Enclosure height | `electronics_enclosure.scad` | `height` (currently 45 mm) |

---

## Remaining work

- [x] Get exact USB-C PD board model → update enclosure SCAD parameters (done 2026-06-07)
- [ ] Verify USB-C port cutout height (`usbc_z`, `usbc_h`) by dry-fitting board in enclosure before final print
- [ ] Decide wiring scheme: how layers connect in series (snake pattern vs. straight columns)
- [ ] Firmware for ESP32 (WS2812B library, animation engine)
- [ ] Design Model 3 if needed: inter-layer structural spacer / frame to keep cube rigid
