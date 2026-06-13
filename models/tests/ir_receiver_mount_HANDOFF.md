# IR Receiver Mount — Handoff

**Status:** v4 (slide-in tray + CLOSED head hole) generated, **awaiting print + fit feedback from user.**
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
- **v3** — **slide-in tray, no flexing parts:** round hole → head **channel**
  (stadium slot open at the +X mouth) the head rides along to its −X seat; board
  edges captured in a groove down each long side. **Failed in hand:** the board
  fit was **too tight to slide in**, and the user wants the head opening to be a
  **closed hole** (the slot was open to its +X side — the real enclosure top
  plate will have a closed hole, so the test piece must replicate that).
- **v4 (current)** — **same slide-in tray, fixed fit + closed head hole:**
  - **Looser fit** so the PCB actually slides: `slide_fit` 0.4 → 0.6 (Y/X),
    `slide_z` 0.3 → 0.5 (Z). Grooves/rails unchanged otherwise.
  - **Head opening is now a CLOSED stadium hole** — fully surrounded by plate, no
    edge opening (case replica). Still stretched along X by `head_slide` = 5 mm so
    the head can be **tilt-inserted at the +X end** and slid −X to its seat
    (X = +1). Plate extended in +X (now to x = +15.6) to enclose the hole's +X end
    with 3 mm margin.
  - Still uses the 6 mm gap for head poke-out (1.5 mm) / pin clearance (1.0 mm).
  - **Insertion changed** from v3's flat horizontal slide (which needed the open
    slot) to **tilt-in-then-slide**: drop the head into the +X hole end from
    below, then slide −X to seat — because a closed hole can't accept the head by
    horizontal slide-from-mouth.

---

## Key geometry (v4)

| | Value |
|---|---|
| Test plate | 32.9 × 29.6 × 2.5 mm (extended +X to enclose the hole) |
| Tray height | to z = 11.2 mm |
| Mount gap | 6 mm → head pokes out **1.5 mm**, pins clear plate by **1.0 mm** |
| Head hole | ⌀13.2 **CLOSED** stadium; −X round seat/stop at X = +1, +X round end at X = +6 (closed, 3 mm plate margin to the edge) |
| Tray inner width | 15.6 mm (board 15 + 0.6 slide fit) |
| Slide slack | `slide_fit` 0.6 (X/Y), `slide_z` 0.5 (Z) — loosened from v3 |
| Side groove grip | 1.2 mm per side (board edge captured ~1 mm) |
| Back wall (−X stop) | X = −10.3 |

All driven by parameters at the top of the `.scad` (board dims, `gap`,
`slide_fit`, `slide_z`, `head_slide`, `groove`, etc.). The poke-out /
pin-clearance trade-off is set by `gap`: poke_out = head_h − gap − ceil_t;
pin_clear = gap − pin_h. The closed-hole stretch is set by `head_slide` (X room
to tilt the head in before sliding to the seat); the plate auto-extends in +X to
keep `hole_margin` (3 mm) of material around the hole's closed +X end.

---

## Print

PLA, 0.2 mm layer, 2 walls, 15% infill, **no supports**.
Orientation: **plate flat on bed, exterior face down, tray walls up.** The head
hole pokes out the bottom — flip to inspect. (The closed stadium hole bridges
≤13 mm with no support; fine for PLA at 0.2 mm.)

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

1. **Slide fit** — now loosened (0.6 / 0.5). Does the PCB slide in freely now
   (still too tight? gone too loose?)? (tune `slide_fit`, `slide_z`)
2. **Closed hole OK?** Is the closed stadium hole what you want to replicate in
   the case top plate, or do you want it shorter/longer? (tune `head_slide`)
3. **Tilt-in works?** Can the head be tilted into the +X hole end and slid to the
   seat without the closed hole getting in the way? (tune `head_slide`, `head_cx`)
4. **Head poke-out & alignment** — reaches the −X seat, pokes ~1.5 mm, centred?
   (tune `gap`, `head_cx`, `head_clear`)
5. **Pins clear** the plate? (tune `gap`)
6. **Retention** — no detent yet; only friction. If it feels loose after the
   looser fit, add a small click-stop bump.

---

## Next steps (after fit confirmed)

- Graft the tray into `electronics_enclosure.scad` top plate at the **front-centre
  between-LED cell ≈ (0, +50)** — a 25×25 cell bounded by LEDs at (±12.5, 37.5)
  and (±12.5, 62.5); nearest LED ~17.7 mm away, ~7.7 mm clear of the ⌀12 head, so
  the head hole fits with margin. Confirm clearance to the closed head hole
  (stadium ~18.2 mm long × 13.2 mm wide — note it grew with `head_slide`) against
  LED bodies/leads at that cell before committing; shrink `head_slide` if tight.
- Verify it clears the ESP32 (~+18, +15) and the USB-C board (back, Y ≈ −39.5).
- Add the firmware-side note is already in `CLAUDE.md`; update the enclosure
  section there once the mount is integrated.
