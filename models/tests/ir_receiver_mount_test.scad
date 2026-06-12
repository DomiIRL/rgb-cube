// ============================================================
// IR Receiver Mount — Fit Test (v2, measured board)
//
// Holds the Elegoo IR receiver board flat in a GAP below the enclosure top
// plate. The TSOP head (⌀12, 10 mm tall) pokes UP through a hole and out a
// little; the 3 pins (5 mm, SAME side as the head) point up into the gap and
// clear the ceiling. Board hangs in the gap because head + pins share a face.
//
// MEASURED (2026-06-12): board 20 × 15 × 1.0, head ⌀12 (r6) × 10 tall, pins
// 5 tall on the same side as the head, overhanging a 15 mm edge by 4 mm.
// Head: centred across the 15 mm width; along the 20 mm length it sits 11 mm
// from the no-pin edge / 9 mm from the pin edge → centre +1 mm toward pins.
// (Your 5 mm = gap to the head's near edge; 9.5 mm = to the head centre.)
//
// ---- Geometry trade-off (set by `gap`) ----
//   head poke-out  = head_h − gap − ceil_t   (gap 6 → 1.5 mm out)
//   pin clearance  = gap − pin_h             (gap 6 → 1.0 mm below ceiling)
// Bigger gap = more pin/wire room but less poke-out (head_h−ceil_t = 7.5 max).
//
// ---- Print ----
// PLA, 0.2 mm layer, 2 walls, 15% infill, NO supports.
// Orientation: plate flat on bed, cradle UP (real top-plate orientation,
// exterior face down). The head hole pokes out the bottom — flip to view.
//
// ---- How to test & report ----
// 1. Drop the board in from the top, head over the round hole, pins/wires out
//    the OPEN end. It rests on the inner ledges; the 2 snap lips hold it down.
// 2. Flip the piece: the head should poke out the flat face ~1.5 mm.
// 3. The pins should hang ~1 mm above the plate's top (= the ceiling clearance).
// Report: board fit (tight/loose)?  head aligned with hole?  poke-out amount?
//         pins clear?  lips hold?
// ============================================================

// ============================================================
// PARAMETERS
// ============================================================

// ---- IR receiver PCB (MEASURED, except thickness) ----
pcb_l = 20.0;   // mm — board length (X)
pcb_w = 15.0;   // mm — board width (Y); pins overhang one 15 mm (short) edge
pcb_t =  1.0;   // mm — board thickness (MEASURED)
fit   =  0.4;   // mm — total clearance around PCB (0.2/side)

// ---- TSOP head / dome (MEASURED) ----
head_d = 12.0;  // mm — head diameter (radius 6)
head_h = 10.0;  // mm — head height above the PCB face
head_cx = 1.0;  // mm — head centre X: +1 toward the pin (+X) edge (11mm/9mm)
head_cy = 0.0;  // mm — head centre Y: centred across the width (MEASURED)
head_hole_clear = 1.0;  // mm — added to head_d for the hole (→ ⌀13.0; also
                        //      absorbs ~0.5 mm head-position uncertainty)

// ---- Pins (MEASURED) — same side as head, at the +X (15 mm) edge ----
pin_h        = 5.0;  // mm — pin height above the PCB face
pin_overhang = 4.0;  // mm — pins extend this far past the +X board edge

// ---- Enclosure top plate (ceiling) ----
ceil_t = 2.5;   // mm — top-plate thickness (= floor_t in the enclosure)

// ---- Mount gap (board face below the ceiling interior) ----
gap = 6.0;      // mm — sets poke-out (1.5) and pin clearance (1.0)

// ---- Cradle ----
wall    = 2.0;  // mm — wall thickness
ledge   = 1.0;  // mm — inward rest ledge the board sits on
lip     = 1.0;  // mm — snap-lip inward overhang
lip_h   = 1.2;  // mm — snap-lip thickness (Z)
lip_len = 8.0;  // mm — snap-lip length along the board
border  = 5.0;  // mm — flat plate margin

eps = 0.01;

// ============================================================
// DERIVED
// ============================================================
pocket_l = pcb_l + fit;            // 20.4
pocket_w = pcb_w + fit;            // 15.4
outer_l  = pocket_l + 2*wall;      // 24.4
outer_w  = pocket_w + 2*wall;      // 19.4
rest_z   = ceil_t + gap;           // 8.5 — board head-side face rests here
top_z    = rest_z + pcb_t + lip_h; // 11.3 — cradle top
hole_d   = head_d + head_hole_clear;          // 12.6
plate_l  = pcb_l + 2*pin_overhang + 2*border; // 38
plate_w  = pcb_w + 2*wall + 2*border;         // 29
poke_out = head_h - gap - ceil_t;  // 1.5 (info)
pin_clear = gap - pin_h;           // 1.0 (info)

// ============================================================
// MODULE: cradle (U-shape, +X end open for pins/wires)
// ============================================================
module cradle() {
    difference() {
        // outer box
        translate([-outer_l/2, -outer_w/2, ceil_t])
            cube([outer_l, outer_w, top_z - ceil_t]);
        // upper pocket — board drops in (z >= rest_z)
        translate([-pocket_l/2, -pocket_w/2, rest_z])
            cube([pocket_l, pocket_w, top_z - rest_z + eps]);
        // lower cavity — head passes through (z ceil_t..rest_z); ledge juts in
        translate([-(pocket_l/2 - ledge), -(pocket_w/2 - ledge), ceil_t - eps])
            cube([pocket_l - 2*ledge, pocket_w - 2*ledge, gap + 2*eps]);
        // open the +X end (remove +X wall + ledge) for pins and wires
        translate([pocket_l/2 - ledge, -outer_w, ceil_t - eps])
            cube([outer_l, 2*outer_w, top_z - ceil_t + 2*eps]);
    }
}

// ============================================================
// MODEL
// ============================================================
difference() {
    union() {
        // ceiling plate (a patch of the top plate; covers board + pin overhang)
        translate([-plate_l/2, -plate_w/2, 0])
            cube([plate_l, plate_w, ceil_t]);

        cradle();

        // snap lips on the two long walls (hold the board down on the ledges)
        for (sy = [-1, 1]) {
            yy = (sy > 0) ? pocket_w/2 - lip : -pocket_w/2;
            translate([-lip_len/2, yy, rest_z + pcb_t])
                cube([lip_len, lip, lip_h]);
        }
    }

    // head hole through the plate
    translate([head_cx, head_cy, -eps])
        cylinder(d=hole_d, h=ceil_t + 2*eps, $fn=64);
}

// ============================================================
// KEY DIMENSIONS  (v2 — measured board)
// plate        : 38 × 29 × 2.5 mm, cradle to z=10.7 mm
// board pocket : 20.4 × 15.4 mm (PCB 20 × 15 + 0.4 fit), thickness 1.0
// head hole    : ⌀13.0 mm (head ⌀12 + 1.0), centre +1 mm toward pins
// mount gap    : 6 mm  → head pokes out 1.5 mm, pins clear ceiling by 1.0 mm
// pins         : 5 mm tall, 4 mm past the +X edge → +X cradle end left open
// rest ledges  : 1 mm inward on −X / +Y / −Y; 2 snap lips on the long walls
// ============================================================
