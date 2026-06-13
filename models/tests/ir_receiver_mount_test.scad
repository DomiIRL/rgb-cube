// ============================================================
// IR Receiver Mount — Fit Test (v4, SLIDE-IN tray + CLOSED head hole)
//
// Changes from v3 (per user, 2026-06-13):
//   • v3's board fit was TOO TIGHT — the PCB wouldn't slide in. Loosened:
//     slide_fit 0.4→0.6, slide_z 0.3→0.5.
//   • v3's head opening was a stadium slot OPEN at the +X (mouth) edge. The
//     real enclosure top plate will have a CLOSED hole, so this test piece
//     must replicate that. The head opening is now a CLOSED, stretched
//     (stadium) hole fully surrounded by plate — still elongated along X so
//     the head can be tilt-inserted at the +X end and slid to its seat.
//
// Still a SLIDE-IN tray — NO flexing, NO press-fit:
//   • A groove down each long side captures the board edges in Z (can't drop
//     out); the board slides in to mount, back out to remove.
//   • The CLOSED stadium head hole: its −X round end is the seat/stop that
//     locates the head at X=+1; its +X round end is closed (a hole, like the
//     case) but offset by `head_slide` so the head has room to enter + slide.
//   • Board hangs in a 6 mm gap below the plate so the 10 mm head pokes
//     through (and ~1.5 mm out) while the 5 mm pins clear the plate.
//
// MEASURED (2026-06-12): board 20 × 15 × 1.0; head ⌀12 (r6) × 10 tall; pins
// 5 tall, same face as the head, overhanging a 15 mm edge by 4 mm. Head is
// centred across the width and 11 mm from the no-pin edge (→ seats at +1 X).
//
// ---- Print ----
// PLA, 0.2 mm layer, 2 walls, 15% infill, NO supports.
// Orientation: plate flat on bed (exterior face down), tray walls UP. The
// head hole pokes out the bottom — flip the piece over to view it.
//
// ---- How to test & report ----
// 1. From below/the mouth, tilt the board in: drop the head DOWN into the
//    +X (open) end of the closed stadium hole, board edges entering the groove
//    mouth, pins/wires trailing off the +X side. Then slide it in (−X) until
//    the head reaches the hole's closed −X seat and stops. Should slide
//    smoothly, not press.
// 2. Flip the piece: the head should poke out the flat exterior face ~1.5 mm,
//    sitting in the −X round seat of the now-CLOSED hole (no edge opening).
// 3. Slide it back out (+X) and lift the head free — should come without prying.
// Report: does it slide easily (too loose / too tight)?  does the head reach
//         the seat / poke out right?  do the pins clear?  is the closed hole
//         representative of what you want in the case top plate?
// ============================================================

// ============================================================
// PARAMETERS
// ============================================================

// ---- IR receiver PCB (MEASURED) ----
pcb_l = 20.0;   // mm — board length (X, slide axis)
pcb_w = 15.0;   // mm — board width (Y)
pcb_t =  1.0;   // mm — board thickness

// ---- TSOP head / dome (MEASURED) ----
head_d  = 12.0; // mm — head diameter (radius 6)
head_h  = 10.0; // mm — head height above the PCB face
head_cx =  1.0; // mm — head seat X: +1 toward the pin (+X) edge (11mm / 9mm)

// ---- Pins (MEASURED) — same face as head, at the +X (15 mm) edge ----
pin_h        = 5.0;  // mm — pin height above the PCB face
pin_overhang = 4.0;  // mm — pins extend this far past the +X board edge

// ---- Enclosure top plate (ceiling) ----
ceil_t = 2.5;   // mm — top-plate thickness (= floor_t in the enclosure)

// ---- Mount gap (board face below the ceiling interior) ----
gap = 6.0;      // mm — head pokes out (head_h−gap−ceil_t)=1.5, pins clear=1.0

// ---- Slide tray / groove ----
slide_fit  = 0.6;  // mm — clearance along the slide (X) and across (Y)  [v4: 0.4→0.6, was too tight]
slide_z    = 0.5;  // mm — vertical slack so the board slides freely     [v4: 0.3→0.5, was too tight]
groove     = 1.2;  // mm — how far each side groove grips the board edge (Y)
rail_t     = 1.2;  // mm — thickness of the groove's upper lip (Z)
wall       = 2.0;  // mm — outer wall thickness
head_clear = 0.6;  // mm — radial clearance added to the head hole
border     = 5.0;  // mm — flat plate margin (back & sides)

// ---- Closed head hole (v4 — case replica, NOT open to the side) ----
head_slide  = 5.0; // mm — +X stretch of the closed stadium hole: room to
                   //      tilt the head in at the +X end, then slide to seat
hole_margin = 3.0; // mm — min plate material around the hole's closed +X end

eps = 0.01;

// ============================================================
// DERIVED
// ============================================================
pocket_w = pcb_w + slide_fit;        // 15.4 — tray inner width
rail_in  = pocket_w/2 - groove;      // 6.5  — inner edge of the side rails
rest_z   = ceil_t + gap;             // 8.5  — board lower face rests here
groove_top = rest_z + pcb_t + slide_z; // 9.8 — underside of the upper lip
tray_top = groove_top + rail_t;      // 11.0 — top of the tray
head_r   = head_d/2 + head_clear;    // 6.6  — head hole radius

back_x   = -pcb_l/2 - slide_fit/2;   // -10.3 — back wall inner face (−X stop)
mouth_x  =  pcb_l/2 + slide_fit/2;   //  10.3 — open tray mouth (+X, board entry)
outer_back = back_x - wall;          // -12.3
outer_w  = pocket_w + 2*wall;        // 19.6

// closed head hole: −X seat at head_cx, +X end offset by head_slide (CLOSED)
hole_x1  = head_cx + head_slide + head_r; // 12.6 — hole's +X outer extreme

// plate extents: borders on back/sides; +X reaches past the closed hole so it
// stays surrounded by plate (the case-replica hole, no edge opening)
plate_x0 = outer_back - border;          // -17.3
plate_x1 = max(mouth_x, hole_x1 + hole_margin); // 15.6 — encloses the +X hole end
plate_y  = outer_w/2 + border;           //  14.8

poke_out  = head_h - gap - ceil_t;   // 1.5 (info)
pin_clear = gap - pin_h;             // 1.0 (info)

// ============================================================
// MODULE: head hole (2D) — CLOSED stadium, both ends rounded, fully inside
// the plate (replicates the case top-plate hole — NOT open to the edge).
// The −X round end (at head_cx) is the seat/stop; the +X round end (offset by
// head_slide) is the open-but-enclosed end the head tilts into before sliding.
// ============================================================
module head_channel() {
    hull() {
        translate([head_cx, 0])              circle(r=head_r, $fn=64); // −X seat (stop)
        translate([head_cx + head_slide, 0]) circle(r=head_r, $fn=64); // +X insertion end (CLOSED)
    }
}

// ============================================================
// MODULE: slide tray — side walls + a capture groove down each long side,
// open at the +X mouth, closed by a back wall at −X.
// ============================================================
module tray() {
    difference() {
        // solid block spanning back wall → mouth, full tray height
        translate([outer_back, -outer_w/2, ceil_t])
            cube([mouth_x - outer_back, outer_w, tray_top - ceil_t]);

        // central through-channel: board body + head + pins (open to mouth)
        translate([back_x, -rail_in, ceil_t - eps])
            cube([mouth_x - back_x + eps, 2*rail_in, tray_top - ceil_t + 2*eps]);

        // side grooves the board edges slide into (carved between the rails)
        translate([back_x, -pocket_w/2, rest_z])
            cube([mouth_x - back_x + eps, pocket_w, groove_top - rest_z]);
    }
}

// ============================================================
// MODEL
// ============================================================
difference() {
    union() {
        // ceiling plate (a patch of the enclosure top plate)
        translate([plate_x0, -plate_y, 0])
            cube([plate_x1 - plate_x0, 2*plate_y, ceil_t]);
        tray();
    }
    // head channel through the plate
    translate([0, 0, -eps])
        linear_extrude(height = ceil_t + 2*eps)
            head_channel();
}

// ============================================================
// KEY DIMENSIONS  (v4 — slide-in tray + CLOSED head hole)
// plate        : 32.9 × 29.6 × 2.5 mm, tray to z=11.2 mm
// slide tray   : 15.6 mm inner width (slide_fit 0.6), edges in 1.2 mm grooves
// head hole    : ⌀13.2 CLOSED stadium, −X seat/stop at x=+1, +X end at x=+6
//                (closed, surrounded by plate — replicates the case hole)
// mount gap    : 6 mm → head pokes out 1.5 mm, pins clear plate by 1.0 mm
// insertion    : tilt head into the +X hole end, slide −X to seat; back wall
//                at x=−10.3 (−X stop). Looser than v3 (was too tight to slide)
// retention    : Z-capture by grooves (no flex); slides back out to remove
// ============================================================
