// ============================================================
// LED Hole Fit Test — 2×2 sample of the layer jig hole pattern
// Print this before printing the full layer_jig to verify
// the 8.3 mm holes fit the actual LED bodies.
// Printer: Bambu Lab A1 Mini
// Print orientation: flat side on bed
// ============================================================

// ============================================================
// PARAMETERS — must match layer_jig.scad exactly
// ============================================================
led_body_d    = 8.0;   // mm — LED body diameter
led_clearance = 0.3;   // mm — same clearance as full jig

pitch         = 25.0;  // mm — center-to-center (same as full jig)
test_grid     = 2;     // holes per side (2×2 = 4 LEDs to test)
board_t       = 3.0;   // mm — same thickness as full jig
margin        = 10.0;  // mm — board edge beyond outermost hole centres
corner_r      = 3.0;   // mm

// ============================================================
// DERIVED
// ============================================================
hole_d     = led_body_d + led_clearance;                      // 8.3 mm
board_side = (test_grid - 1) * pitch + 2 * margin;           // 45 mm
grid_start = -(test_grid - 1) * pitch / 2;                   // −12.5 mm

// ============================================================
// MODEL
// ============================================================
difference() {
    // Rounded-corner plate
    translate([0, 0, board_t / 2])
    minkowski() {
        cube([board_side - 2*corner_r,
              board_side - 2*corner_r,
              board_t - 0.001], center=true);
        cylinder(r=corner_r, h=0.001, $fn=32);
    }

    // 2×2 LED holes
    for (row = [0 : test_grid - 1]) {
        for (col = [0 : test_grid - 1]) {
            translate([grid_start + col * pitch,
                       grid_start + row * pitch,
                       -0.01])
            cylinder(d=hole_d, h=board_t + 0.02, $fn=32);
        }
    }
}

// ============================================================
// KEY DIMENSIONS
// plate     : 45 × 45 × 3 mm
// hole_d    : 8.3 mm (8.0 mm LED body + 0.3 mm clearance)
// pitch     : 25 mm
// grid      : 2×2 = 4 holes
// ============================================================
