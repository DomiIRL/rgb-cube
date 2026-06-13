// ============================================================
// IR Receiver Mount — Fit Test (v5, SCREW-MOUNT via the board's 2 holes)
//
// Redesign (user, 2026-06-13): forget the slide-in tray / sliding entirely.
// The IR board has TWO mounting holes — use them. The board screws onto two
// standoffs that hang from the enclosure top plate; the TSOP head pokes through
// a round hole in the plate; the pins/wires hang off into the enclosure.
//
// MOUNTING HOLES (measured by user, 2026-06-13):
//   • 2 holes, ⌀2.0 mm
//   • 10.0 mm apart, centre-to-centre
//   • 2.5 mm from the board edge (hole centre → edge), on the NO-PIN short edge
//     — the edge the head sits closest to (pins are on the OPPOSITE short edge)
//   • centred across the 15 mm width → y = ±5.0 mm
//
// BOARD (measured 2026-06-12): 20 × 15 × 1.0; head ⌀12 × 10 tall, centred on
// width and closer to the NO-PIN edge (→ head_cx = −1, ≈9 mm from that edge);
// pins 5 tall, same face as the head, overhanging the OTHER short edge by 4 mm.
//
// Coordinates: X = 20 mm length (NO-PIN edge at −10, PIN edge at +10);
//              Y = 15 mm width (±7.5); Z up, plate exterior face on the bed.
//
// ---- Print ----
// PLA, 0.2 mm layer, 2 walls, 15% infill, NO supports.
// Orientation: plate flat on bed, exterior (head-hole) face DOWN, standoffs UP.
//
// ---- How to test & report ----
// 1. Do the two standoffs line up with the board's two mounting holes
//    (10 mm apart, 2.5 mm from the no-pin edge)? Does the board sit flat on them?
// 2. Screw it down with 2× M2 self-tapping screws (board hole ⌀2 → standoff
//    pilot ⌀1.6). Holds snug without stripping the PLA?
// 3. Flip the piece: does the head poke ~1.5 mm out the flat exterior face,
//    centred in the round hole? Do the pins clear the plate (~1 mm gap)?
// Report: standoff spacing & height right? head centred / pokes out? pins clear?
// ============================================================

// ============================================================
// PARAMETERS
// ============================================================

// ---- IR receiver PCB (MEASURED) ----
pcb_l = 20.0;   // mm — board length (X); no-pin edge at −10, pin edge at +10
pcb_w = 15.0;   // mm — board width (Y)
pcb_t =  1.0;   // mm — board thickness

// ---- TSOP head / dome (MEASURED) ----
head_d  = 12.0; // mm — head diameter
head_h  = 10.0; // mm — head height above the PCB face
head_cx = -1.0; // mm — head seat X: −1 toward the NO-PIN (−X) edge (≈9 mm from it)

// ---- Pins (MEASURED) — same face as head, on the +X (pin) short edge ----
pin_h        = 5.0;  // mm — pin height above the PCB face (info: clearance check)
pin_overhang = 4.0;  // mm — pins extend this far past the +X board edge (info)

// ---- Mounting holes (MEASURED 2026-06-13) ----
mh_d     = 2.0;   // mm — board mounting-hole diameter (→ M2 screw)
mh_pitch = 10.0;  // mm — centre-to-centre spacing (across the width, Y)
mh_edge  = 2.5;   // mm — hole centre → NO-PIN edge (along the length, X)

// ---- Standoffs the board screws onto ----
so_d         = 3.5;  // mm — standoff outer diameter
so_pilot     = 1.6;  // mm — M2 self-tap pilot hole ⌀
so_screw_clr = 0.0;  // (info) board hole is the clearance; screw is M2

// ---- Enclosure top plate (ceiling) ----
ceil_t = 2.5;   // mm — top-plate thickness (= floor_t in the enclosure)

// ---- Mount gap (board face below the ceiling interior) ----
gap = 6.0;      // mm — head pokes out (head_h−gap−ceil_t)=1.5, pins clear=1.0

// ---- Misc ----
head_clear = 0.6;  // mm — radial clearance added to the round head hole
border     = 5.0;  // mm — flat plate margin around the board footprint

eps = 0.01;

// ============================================================
// DERIVED
// ============================================================
mh_x   = -pcb_l/2 + mh_edge;     // -7.5 — mounting holes / standoffs X (no-pin end)
mh_y   =  mh_pitch/2;            //  5.0 — mounting holes / standoffs ±Y

rest_z = ceil_t + gap;          //  8.5 — board lower (component) face height
so_h   = gap;                   //  6.0 — standoff height (top = rest_z)

head_hole_d = head_d + 2*head_clear; // 13.2 — round head hole ⌀

poke_out  = head_h - gap - ceil_t;   // 1.5 (info)
pin_clear = gap - pin_h;             // 1.0 (info)

// plate: board footprint + uniform border (covers the pin overhang at +X)
plate_x0 = -pcb_l/2 - border;   // -15.0
plate_x1 =  pcb_l/2 + border;   //  15.0 (pins reach +14 → covered, clear in Z)
plate_y  =  pcb_w/2 + border;   //  12.5

// head-hole ↔ standoff proximity (info): centre distance, hole vs standoff radii
hs_dist = sqrt((head_cx - mh_x)*(head_cx - mh_x) + mh_y*mh_y); // 8.20
// head_hole_r + so_r = 6.6 + 1.75 = 8.35 → rims nearly touch (real board is
// crowded at this end). If they print merged, drop head_clear or so_d.

// ============================================================
// MODEL
// ============================================================
difference() {
    union() {
        // top-plate patch (a piece of the enclosure ceiling)
        translate([plate_x0, -plate_y, 0])
            cube([plate_x1 - plate_x0, 2*plate_y, ceil_t]);

        // two standoffs the board screws down onto
        for (sy = [-mh_y, mh_y])
            translate([mh_x, sy, ceil_t])
                cylinder(d = so_d, h = so_h, $fn = 48);
    }

    // round head hole through the plate (case-replica: a hole, fully enclosed)
    translate([head_cx, 0, -eps])
        cylinder(d = head_hole_d, h = ceil_t + 2*eps, $fn = 64);

    // M2 self-tap pilot holes down each standoff (not piercing the plate)
    for (sy = [-mh_y, mh_y])
        translate([mh_x, sy, ceil_t])
            cylinder(d = so_pilot, h = so_h + eps, $fn = 32);
}

// ============================================================
// KEY DIMENSIONS  (v5 — screw-mount)
// plate        : 30 × 25 × 2.5 mm
// standoffs    : ⌀3.5 × 6 mm, M2 ⌀1.6 pilot, at x=−7.5, y=±5 (10 mm apart,
//                2.5 mm from the no-pin edge) — board screws on with 2× M2
// head hole    : round ⌀13.2 at (x=−1, y=0), through the plate
// mount gap    : 6 mm → head pokes out 1.5 mm, pins clear plate by 1.0 mm
// board fix    : 2 mounting holes fully constrain X/Y/rotation; standoffs set Z
// ============================================================
