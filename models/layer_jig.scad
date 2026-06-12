// ============================================================
// LED Cube Assembly Jig — one layer (6×6 grid)
// 8mm through-hole RGB LEDs with integrated controller
// Printer: Bambu Lab A1 Mini (180×180mm bed)
// Print orientation: legs down on bed
// ============================================================

// ============================================================
// PARAMETERS
// ============================================================

// LED (8mm body)
led_body_d    = 8.0;   // mm — LED body diameter
led_clearance = 0.3;   // mm — hole oversize for easy insertion/removal

// Grid
grid_n        = 6;     // LEDs per side (6×6 layer)
pitch         = 25.0;  // mm — center-to-center spacing
                       // 25 mm → 17 mm gap between 8 mm LED bodies;
                       // comfortable room for soldering iron + wire

// Board
margin        = 10.0;  // mm — board edge beyond outermost LED centers
board_t       = 3.0;   // mm — board thickness
corner_r      = 3.0;   // mm — corner rounding radius

// Legs — elevate board so LED leads (≤15 mm) hang freely below
leg_d         = 6.0;   // mm — leg diameter
leg_h         = 20.0;  // mm — leg height (longer than 15 mm DO lead)

// ============================================================
// DERIVED
// ============================================================
hole_d      = led_body_d + led_clearance;            // 8.3 mm
board_side  = (grid_n - 1) * pitch + 2 * margin;    // 145 mm
grid_start  = -(grid_n - 1) * pitch / 2;            // −62.5 mm — centre the grid
leg_pos     = board_side / 2 - leg_d / 2 - 0.5;    // 69 mm — just inside board corners
                                                      // (2 mm clear of corner LED holes)

// ============================================================
// MODEL
// ============================================================

// Board — raised on legs so LED leads hang freely in the gap below
translate([0, 0, leg_h])
difference() {
    // Rounded-corner board (minkowski of box + cylinder)
    translate([0, 0, board_t / 2])
    minkowski() {
        cube([board_side - 2*corner_r,
              board_side - 2*corner_r,
              board_t - 0.001], center=true);
        cylinder(r=corner_r, h=0.001, $fn=32);
    }

    // 6×6 grid of LED body holes (through the full board thickness)
    for (row = [0 : grid_n - 1]) {
        for (col = [0 : grid_n - 1]) {
            translate([grid_start + col * pitch,
                       grid_start + row * pitch,
                       -0.01])
            cylinder(d=hole_d, h=board_t + 0.02, $fn=32);
        }
    }
}

// Corner legs (slightly overlap board bottom for a clean union)
for (sx = [-1, 1]) for (sy = [-1, 1]) {
    translate([sx * leg_pos, sy * leg_pos, 0])
    cylinder(d=leg_d, h=leg_h + 0.01, $fn=24);
}

// ============================================================
// KEY DIMENSIONS
// total height   : 23 mm (20 mm legs + 3 mm board)
// board          : 145 × 145 × 3 mm (elevated at z = 20 mm)
// hole_d         : 8.3 mm  (8.0 mm LED + 0.3 mm clearance)
// pitch          : 25 mm   (17 mm gap between LED bodies)
// grid           : 6×6 = 36 LEDs per layer
// legs           : ⌀6 mm, 20 mm tall, centres at ±69 mm
// lead clearance : 20 mm > 15 mm (longest DO lead) ✓
// ============================================================
