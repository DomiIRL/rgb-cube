// ============================================================
// IR Receiver Mount — Fit Test (v3, SLIDE-IN tray)
//
// Fixes v2's problems: the head was captive in a round hole (couldn't slide)
// and the snap-lips were a press-fit too tight to insert or remove in PLA.
//
// v3 is a SLIDE-IN tray — NO flexing, NO press-fit:
//   • The board slides in flat from the +X (mouth) end, pins/wires trailing.
//   • A groove on each long side captures the board edges in Z (can't drop
//     out), so it just slides — in to mount, back out to remove.
//   • The round head hole becomes a CHANNEL (slot) the TSOP head rides along.
//     The slot's closed −X end is the stop that locates the head at its seat.
//   • Board still hangs in a 6 mm gap below the plate so the 10 mm head pokes
//     through (and ~1.5 mm out) while the 5 mm pins clear the plate.
//
// MEASURED (2026-06-12): board 20 × 15 × 1.0; head ⌀12 (r6) × 10 tall; pins
// 5 tall, same face as the head, overhanging a 15 mm edge by 4 mm. Head is
// centred across the width and 11 mm from the no-pin edge (→ seats at +1 X).
//
// ---- Print ----
// PLA, 0.2 mm layer, 2 walls, 15% infill, NO supports.
// Orientation: plate flat on bed (exterior face down), tray walls UP. The
// head channel pokes out the bottom — flip the piece over to view it.
//
// ---- How to test & report ----
// 1. Hold the board flat at the +X mouth, head DOWN, pins/wires hanging off
//    the +X side. Slide it in (−X) until the head reaches the channel's
//    closed end and stops. It should slide smoothly, not press.
// 2. Flip the piece: the head should poke out the flat face ~1.5 mm, sitting
//    in the round end of the channel.
// 3. Slide it back out — should come free without prying.
// Report: does it slide easily (too loose / too tight)?  does the head reach
//         the channel end / poke out right?  do the pins clear?  held firmly
//         enough, or does it need a detent so it can't slide back out?
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
slide_fit  = 0.4;  // mm — clearance along the slide (X) and across (Y)
slide_z    = 0.3;  // mm — vertical slack so the board slides freely
groove     = 1.2;  // mm — how far each side groove grips the board edge (Y)
rail_t     = 1.2;  // mm — thickness of the groove's upper lip (Z)
wall       = 2.0;  // mm — outer wall thickness
head_clear = 0.6;  // mm — radial clearance added to the head channel
border     = 5.0;  // mm — flat plate margin (back & sides; mouth is flush)

eps = 0.01;

// ============================================================
// DERIVED
// ============================================================
pocket_w = pcb_w + slide_fit;        // 15.4 — tray inner width
rail_in  = pocket_w/2 - groove;      // 6.5  — inner edge of the side rails
rest_z   = ceil_t + gap;             // 8.5  — board lower face rests here
groove_top = rest_z + pcb_t + slide_z; // 9.8 — underside of the upper lip
tray_top = groove_top + rail_t;      // 11.0 — top of the tray
head_r   = head_d/2 + head_clear;    // 6.6  — head channel radius

back_x   = -pcb_l/2 - slide_fit/2;   // -10.2 — back wall inner face (−X stop)
mouth_x  =  pcb_l/2 + slide_fit/2;   //  10.2 — open mouth (+X)
outer_back = back_x - wall;          // -12.2
outer_w  = pocket_w + 2*wall;        // 19.4

// plate extents: borders on back/sides, flush at the mouth
plate_x0 = outer_back - border;      // -17.2
plate_x1 = mouth_x;                  //  10.2 (flush — board/pins slide out here)
plate_y  = outer_w/2 + border;       //  14.8

poke_out  = head_h - gap - ceil_t;   // 1.5 (info)
pin_clear = gap - pin_h;             // 1.0 (info)

// ============================================================
// MODULE: head channel (2D) — stadium from the seat to past the mouth.
// The closed −X round end (at head_cx) is the X-stop that locates the head.
// ============================================================
module head_channel() {
    hull() {
        translate([head_cx, 0]) circle(r=head_r, $fn=64);   // round seat (stop)
        translate([mouth_x + head_r, 0]) circle(r=head_r, $fn=64); // open at mouth
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
// KEY DIMENSIONS  (v3 — slide-in tray)
// plate        : 27.4 × 29.6 × 2.5 mm, tray to z=11.0 mm
// slide tray   : 15.4 mm inner width, board edges captured in 1.2 mm grooves
// head channel : ⌀13.2 stadium, round end (seat/stop) at x=+1, open at mouth
// mount gap    : 6 mm → head pokes out 1.5 mm, pins clear plate by 1.0 mm
// insertion    : slide in flat from +X mouth; back wall at x=−10.2 (−X stop)
// retention    : Z-capture by grooves (no flex); slides back out to remove
// ============================================================
