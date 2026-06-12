# IR Receiver Mount — Handoff

**Status:** v3 (slide-in tray) generated, **awaiting print + fit feedback from user.**
**Last updated:** 2026-06-13
**Files:** `ir_receiver_mount_test.scad` / `.stl` / `_preview.png` (this folder)

---

## Goal

Mount an **Elegoo IR receiver board** (TSOP-1838, NEC remote) inside the LED-cube
electronics enclosure so the sensor **head pokes UP through the top plate into the
air gap between four LEDs** (the cube is an open lattice — ~70% air — so the dome
hides among the LEDs). Firmware side is already noted in `CLAUDE.md`
(signal → GPIO15, VCC → 3.3 V, NEC).

The files in this folder are a **minimal fit-test piece**, not the final part — a
small patch of the top plate + the mount, printed standalone so the user can
validate fit before it's grafted into `electronics_enclosure.scad`.

---

## Measured board (from the user, 2026-06-12)

| Property | Value |
|---|---|
| PCB length | 20 mm (slide axis = X) |
| PCB width | 15 mm (Y) |
| PCB thickness | 1.0 mm |
| Head (dome) diameter | 12 mm (radius 6) |
| Head height above PCB face | 10 mm |
| Head position (along length) | 11 mm from the no-pin edge / 9 mm from the pin edge → **seats at X = +1** (1 mm toward pins from board centre) |
| Head position (across width) | centred |
| Pins | 3-pin header, **same face as the head**, 5 mm tall, overhanging one **15 mm (short) edge** by 4 mm |

Note on the head-position numbers: user gave "5 mm from the no-pin edge" and
"9.5 mm from the pin edge". These reconcile if 5 mm = gap to the head's *near
edge* (→ 11 mm to centre) and 9.5 ≈ 9 mm = to the head *centre*. Both → centre at
X ≈ +1. The head hole has extra clearance to absorb the residual ~0.5 mm
uncertainty.

---

## Design evolution

- **v1** — round dome hole + drop-in pocket with snap-lips. Used wrong/guessed
  dims (⌀5.5 hole). Head didn't fit.
- **v2** — corrected to measured dims: ⌀13 head hole, board hung in a 6 mm gap so
  the 10 mm head pokes through while the 5 mm same-side pins clear the plate.
  **Failed in hand:** head was captive in the round hole so the board couldn't
  slide in, and the inward snap-lips were a press-fit too tight for PLA to flex
  into (and impossible to remove).
- **v3 (current)** — **slide-in tray, no flexing parts:**
  - Round hole → **head channel** (stadium slot); its closed −X round end is the
    stop that locates the head at its seat (X = +1).
  - Board edges captured in a **groove down each long side** → slides in flat from
    the +X mouth, can't drop out, slides back out to remove. No press-fit.
  - Still uses the 6 mm gap for head poke-out / pin clearance.

---

## Key geometry (v3)

| | Value |
|---|---|
| Test plate | 27.4 × 29.6 × 2.5 mm |
| Tray height | to z = 11.0 mm |
| Mount gap | 6 mm → head pokes out **1.5 mm**, pins clear plate by **1.0 mm** |
| Head channel | ⌀13.2 stadium, round seat/stop at X = +1, open at the +X mouth |
| Tray inner width | 15.4 mm (board 15 + 0.4 slide fit) |
| Side groove grip | 1.2 mm per side (board edge captured ~1 mm) |
| Back wall (−X stop) | X = −10.2 |

All driven by parameters at the top of the `.scad` (board dims, `gap`,
`slide_fit`, `groove`, etc.). The poke-out / pin-clearance trade-off is set by
`gap`: poke_out = head_h − gap − ceil_t; pin_clear = gap − pin_h.

---

## Print

PLA, 0.2 mm layer, 2 walls, 15% infill, **no supports**.
Orientation: **plate flat on bed, exterior face down, tray walls up.** The head
channel pokes out the bottom — flip to inspect.

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

1. **Slide fit** — too loose / too tight? (tune `slide_fit`, `slide_z`)
2. **Head poke-out & alignment** — reaches the channel end, pokes ~1.5 mm, centred? (tune `gap`, `head_cx`, `head_clear`)
3. **Pins clear** the plate? (tune `gap`)
4. **Retention** — no detent yet; only friction stops it sliding back out the
   mouth. In the real (horizontal-slide) orientation gravity won't pull it out,
   but if it feels loose, add a small click-stop bump.
5. **Service trade-off decided?** Slide-in means the head withdraws sideways, so
   the IR board is inserted/removed with the cube **off**. If straight-down removal
   with the cube on is wanted instead, switch to a drop-down clip (reintroduces a
   flexing part — the reason v3 avoided it).

---

## Next steps (after fit confirmed)

- Graft the tray into `electronics_enclosure.scad` top plate at the **front-centre
  between-LED cell ≈ (0, +50)** — a 25×25 cell bounded by LEDs at (±12.5, 37.5)
  and (±12.5, 62.5); nearest LED ~17.7 mm away, ~7.7 mm clear of the ⌀12 head, so
  the head channel fits with margin. Confirm clearance to the head channel slot
  (slot ~15.7 × 13 mm) against LED bodies/leads at that cell before committing.
- Verify it clears the ESP32 (~+18, +15) and the USB-C board (back, Y ≈ −39.5).
- Add the firmware-side note is already in `CLAUDE.md`; update the enclosure
  section there once the mount is integrated.
