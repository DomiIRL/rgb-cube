// ============================================================
// LED Pin Bending Jig — single 8mm through-hole RGB LED
//
// Usage: insert LED lens-first from above (pins pointing UP).
// The collar/flange rests on the flat board top surface.
// The dome hangs below the board in the leg clearance.
// Bend pins outward against the elevated tabs.
//
// Printer: Bambu Lab A1 Mini
// Print orientation: legs down on bed (same as layer_jig)
// ============================================================

// ============================================================
// PARAMETERS
// ============================================================

// LED body
led_body_d    = 8.0;   // mm — LED cylinder / dome diameter
led_clearance = 0.3;   // mm — hole oversize for smooth insertion
collar_d      = 9.5;   // mm — collar/flange diameter (MEASURE YOUR LED!)
                        //      Must be > hole_d (8.3) or LED will fall through.

// Board (flat plate — same style as layer_jig)
board_w  = 40.0;   // mm — board width  (X)
board_l  = 40.0;   // mm — board length (Y)
board_t  = 3.0;    // mm — board thickness
corner_r = 3.0;    // mm — outer corner radius

// Legs — elevate board to clear the inverted LED dome (~4mm radius)
leg_h = 8.0;   // mm — leg height (> LED dome radius; 8mm gives 4mm margin)
leg_d = 6.0;   // mm — leg diameter

// Bending tabs — one per side, inner face clears the LED collar
tab_gap    = 1.5;   // mm — gap between collar edge and inner tab face
tab_w      = 5.0;   // mm — tab span (perpendicular to outward direction)
tab_l      = 4.0;   // mm — tab depth (outward from hole centre)
tab_h_low  = 2.0;   // mm — height above board top on ±X sides
tab_h_high = 4.0;   // mm — height above board top on ±Y sides

// ============================================================
// DERIVED
// ============================================================
hole_d     = led_body_d + led_clearance;    // 8.3 mm
tab_offset = collar_d / 2 + tab_gap;        // inner face of tab from centre
leg_pos    = board_w / 2 - leg_d / 2 - 1;  // leg centre (inset from corners)
board_z    = leg_h;                          // Z of board bottom face
eps        = 0.01;

// ============================================================
// MODULES
// ============================================================

// One bending wall — runs from tab_offset to the board edge,
// spanning the full board width in the perpendicular direction.
// Rotated into place for each of the four sides.
module bend_tab(h) {
    wall_len = board_w / 2 - tab_offset;   // inner face → board edge
    translate([tab_offset, -board_l / 2, board_z + board_t])
    cube([wall_len, board_l, h]);
}

// ============================================================
// MODEL  —  Z=0 on print bed, board top at Z = leg_h + board_t
// ============================================================

// --- Corner legs (same pattern as layer_jig) ---
for (sx = [-1, 1]) for (sy = [-1, 1]) {
    translate([sx * leg_pos, sy * leg_pos, 0])
    cylinder(d=leg_d, h=leg_h + eps, $fn=24);
}

// --- Board (flat plate elevated on legs) ---
translate([0, 0, board_z])
difference() {
    // Rounded-corner board (minkowski of box + flat cylinder)
    translate([0, 0, board_t / 2])
    minkowski() {
        cube([board_w - 2*corner_r,
              board_l - 2*corner_r,
              board_t - 0.001], center=true);
        cylinder(r=corner_r, h=0.001, $fn=32);
    }

    // LED body through-hole — collar rests on flat top surface,
    // dome hangs below into leg clearance
    translate([0, 0, -eps])
    cylinder(d=hole_d, h=board_t + 2*eps, $fn=48);
}

// --- Bending tabs on board top ---
// Two opposing sides at tab_h_low (2 mm): ±X
for (a = [0, 180]) rotate([0, 0, a]) bend_tab(tab_h_low);

// Two opposing sides at tab_h_high (4 mm): ±Y
for (a = [90, 270]) rotate([0, 0, a]) bend_tab(tab_h_high);

// ============================================================
// KEY DIMENSIONS
// board          : 40 × 40 × 3 mm   (bottom at z=8, top at z=11)
// legs           : ⌀6 mm, 8 mm tall — 4 mm dome clearance below board
// hole_d         : 8.3 mm   (8.0 + 0.3 clearance)
// collar_d       : 9.5 mm   (MEASURE AND ADJUST — must be > 8.3 mm)
// tab inner face : 6.25 mm from centre   (collar edge + 1.5 mm gap)
// low tabs  (±X) : 2 mm above board top
// high tabs (±Y) : 4 mm above board top
// ============================================================
