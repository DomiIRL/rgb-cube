# IR Receiver Mount — Handoff

**Status:** v5 (screw-mount via the board's 2 mounting holes) generated, **awaiting print + fit feedback from user.**
**Last updated:** 2026-06-13
**Files:** `ir_receiver_mount_test.scad` / `.stl` / `_preview.png` (this folder)

---

## Goal

Mount an **Elegoo IR receiver board** (TSOP-1838, NEC remote) inside the LED-cube
electronics enclosure so the sensor **head pokes UP through the top plate into the
air gap between four LEDs** (the cube is an open lattice — ~70% air — so the dome
hides among the LEDs). Firmware side is already noted in `CLAUDE.md`
(signal → GPIO34, VCC → 3.3 V, NEC).

The files in this folder are a **minimal fit-test piece**, not the final part — a
small patch of the top plate + the mount, printed standalone so the user can
validate fit before it's grafted into `electronics_enclosure.scad`.

---

## Measured board

| Property | Value | Source |
|---|---|---|
| PCB length | 20 mm (X) | 2026-06-12 |
| PCB width | 15 mm (Y) | 2026-06-12 |
| PCB thickness | 1.0 mm | 2026-06-12 |
| Head (dome) diameter | 12 mm | 2026-06-12 |
| Head height above PCB face | 10 mm | 2026-06-12 |
| Head position (along length) | **closer to the NO-PIN edge** (≈9 mm from it) → `head_cx = −1` | user 2026-06-13 |
| Head position (across width) | centred (y = 0) | 2026-06-12 |
| Pins | 3-pin header, **same face as the head**, 5 mm tall, overhanging the **other** short (15 mm) edge by 4 mm | 2026-06-12 |
| **Mounting holes** | **2 ×, ⌀2.0 mm, 10 mm apart (centre-centre), 2.5 mm from the NO-PIN edge (centre→edge), centred on width (y = ±5)** | **user 2026-06-13** |

The two mounting holes are on the **no-pin** short edge — the same end the head
sits closest to. So at that end the head and the two holes are crowded together
(see the head↔standoff note below). Pins/wires hang off the **opposite** edge.

---

## Design evolution

- **v1** — round dome hole + drop-in pocket with snap-lips. Guessed dims, didn't fit.
- **v2** — corrected dims, board hung in a 6 mm gap. **Failed:** head captive in a
  round hole (couldn't insert) + snap-lips too tight for PLA.
- **v3** — slide-in tray with side grooves + a stadium head slot open at the mouth.
  **Failed:** board fit too tight to slide; head slot was open to its side.
- **v4** — looser slide-in tray + a **closed** stadium head hole (case replica,
  tilt-in-then-slide). **Superseded** before printing — see v5.
- **v5 (current)** — **SCREW-MOUNT, no sliding at all** (user: "vergiss alles mit
  sliding"). The board has **two ⌀2 mm mounting holes** → use them. The board
  screws onto **two ⌀3.5 mm standoffs** (M2 ⌀1.6 self-tap pilot) that hang from
  the top plate; the head pokes through a plain **round** hole; pins/wires hang
  off into the enclosure. Two screws fully constrain the board (X, Y, rotation);
  the standoff height sets the 6 mm gap (poke-out 1.5 mm, pin clearance 1.0 mm).

---

## Key geometry (v5)

| | Value |
|---|---|
| Test plate | 30 × 25 × 2.5 mm |
| Standoffs | ⌀3.5 × 6 mm tall, M2 ⌀1.6 pilot, at **x = −7.5, y = ±5** (10 mm apart, 2.5 mm from the no-pin edge) |
| Board fixing | 2× M2 self-tapping screws through the ⌀2 board holes into the pilots |
| Head hole | plain **round ⌀13.2** at (x = −1, y = 0), through the plate |
| Mount gap | 6 mm → head pokes out **1.5 mm**, pins clear plate by **1.0 mm** |

All driven by parameters at the top of the `.scad` (`pcb_*`, `head_*`, `mh_*`
mounting holes, `so_*` standoffs, `gap`, etc.). The poke-out / pin-clearance
trade-off is set by `gap`: poke_out = head_h − gap − ceil_t; pin_clear = gap − pin_h.

**Head ↔ standoff proximity (watch this):** the head hole rim and the standoff
bases are ~0.15 mm apart (centre distance 8.20 vs radii 6.6 + 1.75 = 8.35) —
they nearly touch because the real board crowds the head and both holes at the
same end. If they print merged/weak, reduce `head_clear` (e.g. 0.4) or `so_d`
(e.g. 3.0), or nudge `head_cx`.

---

## Print

PLA, 0.2 mm layer, 2 walls, 15% infill, **no supports**.
Orientation: **plate flat on bed, exterior (head-hole) face down, standoffs up.**
The round head hole bridges ⌀13.2 with no support — fine for PLA at 0.2 mm.

Regenerate:
```bash
cd models/tests
openscad -o ir_receiver_mount_test.stl ir_receiver_mount_test.scad
# preview (from repo root):
PYOPENGL_PLATFORM=egl .venv/bin/python ~/.claude/skills/parametric-3d-printing/preview.py \
  models/tests/ir_receiver_mount_test.stl models/tests/ir_receiver_mount_test_preview.png --views multi
```

---

## Open questions / pending user feedback

1. **Standoff spacing/position** — do the two standoffs line up with the board's
   holes (10 mm apart, 2.5 mm from the no-pin edge, centred on width)? (tune
   `mh_pitch`, `mh_edge`)
2. **Screw fit** — does an M2 self-tapping screw bite the ⌀1.6 pilot without
   stripping or splitting the standoff? (tune `so_pilot`, `so_d`)
3. **Standoff height / gap** — board sits level, head pokes ~1.5 mm, pins clear
   ~1 mm? (tune `gap`)
4. **Head centred?** Is the round hole centred on the head when the board is
   screwed down? `head_cx = −1` is inferred from "closer to the no-pin edge" —
   confirm/adjust. (tune `head_cx`, `head_clear`)
5. **Head/standoff merge** — did the hole rim and standoff bases print fused? If
   so, see the proximity note above.

---

## Next steps (after fit confirmed)

- Graft the plate + 2 standoffs + head hole into `electronics_enclosure.scad`
  top plate at the **front-centre between-LED cell ≈ (0, +50)** — a 25×25 cell
  bounded by LEDs at (±12.5, 37.5) and (±12.5, 62.5); nearest LED ~17.7 mm away,
  so the ⌀13.2 head hole + the two standoffs fit with margin. Confirm the
  standoff footprint (x = −7.5, y = ±5 relative to the head) clears LED
  bodies/leads at that cell before committing.
- Verify it clears the ESP32 (~+18, +15) and the USB-C board (back, Y ≈ −39.5).
- Firmware-side note already in `CLAUDE.md`; update the enclosure section there
  once the mount is integrated.
