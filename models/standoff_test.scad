// ============================================================
// Standoff + Port Test Piece — USB-C Board + ESP32
//
// Section 1 (left)  : ESP32 — 4× M3 standoffs, 46.5 × 23.4 mm
// Section 2 (middle): USB-C board — 2× M2 standoffs, 5 mm apart
// Section 3 (right) : USB-C port L-piece — simulates enclosure wall
//     • push USB-C board onto the 2 standoffs (connector toward wall)
//     • PCB edge should sit flush with wall inner face
//     • connector should fit through the stadium hole
//
// Print flat on bed (base down). No supports needed.
// ============================================================

// ---- Parameters shared with electronics_enclosure.scad ----
esp_mx  = 46.5;  // ESP32 hole spacing X
esp_my  = 23.4;  // ESP32 hole spacing Y
esp_od  =  6.0;  // ESP32 standoff outer ⌀
esp_sd  =  2.5;  // M3 self-tap pilot

usbc_bmx =  5.0;  // USB-C board hole spacing
usbc_bod =  3.5;  // USB-C standoff outer ⌀
usbc_bsd =  1.6;  // M2 self-tap pilot

usbc_w   =  9.5;  // USB-C port opening width
usbc_h   =  3.5;  // USB-C port opening height (stadium: r = h/2 = 1.75)

// USB-C board geometry (measured)
usbc_hole_to_edge = 28.0;  // standoff centre → PCB edge (connector side)
usbc_pcb_to_hole  =  2.0;  // connector centre above PCB surface (adjust if off)

// ---- Test piece params ----
base_t = 2.0;   // base plate thickness
so_h   = 4.0;   // standoff height above base
mg     = 4.0;   // margin around patterns
gap    = 5.0;   // gap between sections
wall_t = 2.5;   // enclosure wall thickness (simulated)

eps = 0.01;

// ---- Derived ----
esp_bw = esp_mx + 2*mg;           // 54.5 mm
esp_bd = esp_my + 2*mg;           // 31.4 mm

usbc_bw = usbc_bmx + 2*mg;        // 13.0 mm
usbc_bd = 2*mg;                   //  8.0 mm
usbc_ox = esp_bw + gap;           // 59.5 mm
usbc_oy = (esp_bd - usbc_bd) / 2; // 11.7 mm (centred on ESP32 Y)

l_ox = usbc_ox + usbc_bw + gap;   // 77.5 mm (X start of L-piece)

// L-piece: wall at the far Y edge, board area between wall and front
//   wall inner face at Y = l_base_d
//   standoffs 28 mm in from inner face → Y = l_base_d - usbc_hole_to_edge
l_base_d = usbc_hole_to_edge + mg; // 32.0 mm (depth of base plate)
l_total_w = 20.0;                  // width (X) — enough for 9.5 mm hole
l_so_y    = mg;                    // = l_base_d - usbc_hole_to_edge = 4.0 mm
l_so_x1   = l_total_w/2 - usbc_bmx/2;  // 7.5 mm
l_so_x2   = l_total_w/2 + usbc_bmx/2;  // 12.5 mm

l_hole_z  = base_t + so_h + usbc_pcb_to_hole;  // 8.0 mm
l_wall_h  = l_hole_z + usbc_h/2 + 2.0;         // 11.75 mm

// ============================================================
// Stadium profile (USB-C shape) — 2D, XY plane, centred at origin
// ============================================================
module usbc_stadium() {
    r = usbc_h / 2;
    hull() {
        translate([-(usbc_w/2 - r), 0]) circle(r=r, $fn=32);
        translate([ (usbc_w/2 - r), 0]) circle(r=r, $fn=32);
    }
}

// ============================================================
// SECTION 1 — ESP32 standoffs
// ============================================================
difference() {
    cube([esp_bw, esp_bd, base_t]);
    for (sx = [-1,1]) for (sy = [-1,1])
        translate([esp_bw/2 + sx*esp_mx/2, esp_bd/2 + sy*esp_my/2, -eps])
            cylinder(d=esp_sd, h=base_t + eps*2, $fn=20);
}
for (sx = [-1,1]) for (sy = [-1,1]) {
    px = esp_bw/2 + sx*esp_mx/2;
    py = esp_bd/2 + sy*esp_my/2;
    difference() {
        translate([px, py, base_t])
            cylinder(d=esp_od, h=so_h, $fn=24);
        translate([px, py, -eps])
            cylinder(d=esp_sd, h=base_t + so_h + eps*2, $fn=20);
    }
}

// ============================================================
// Bridge 1 — ESP32 → USB-C standoff section
// ============================================================
bridge_d = 5.0;
translate([esp_bw - eps, (esp_bd - bridge_d)/2, 0])
    cube([gap + 2*eps, bridge_d, base_t]);

// ============================================================
// SECTION 2 — USB-C board standoffs
// ============================================================
translate([usbc_ox, usbc_oy, 0]) {
    difference() {
        cube([usbc_bw, usbc_bd, base_t]);
        for (sx = [-1,1])
            translate([usbc_bw/2 + sx*usbc_bmx/2, usbc_bd/2, -eps])
                cylinder(d=usbc_bsd, h=base_t + eps*2, $fn=20);
    }
    for (sx = [-1,1]) {
        px = usbc_bw/2 + sx*usbc_bmx/2;
        py = usbc_bd/2;
        difference() {
            translate([px, py, base_t])
                cylinder(d=usbc_bod, h=so_h, $fn=24);
            translate([px, py, -eps])
                cylinder(d=usbc_bsd, h=base_t + so_h + eps*2, $fn=20);
        }
    }
}

// ============================================================
// Bridge 2 — USB-C standoff section → L-piece
// ============================================================
translate([usbc_ox + usbc_bw - eps, (esp_bd - bridge_d)/2, 0])
    cube([gap + 2*eps, bridge_d, base_t]);

// ============================================================
// SECTION 3 — USB-C port L-piece
//
// Base plate covers the board area.
// Wall stands at the far Y edge — simulates the enclosure back wall.
// Hole is cut through the wall at the correct height.
//
// Usage:
//   Orient the USB-C board with connector pointing toward the wall (+Y).
//   Place mounting holes onto the 2 standoffs.
//   → PCB edge should be flush with wall inner face
//   → connector should pass through the hole
//   If height is off, adjust usbc_pcb_to_hole in both SCAD files.
// ============================================================
translate([l_ox, 0, 0]) {
    difference() {
        union() {
            // Base plate (board area)
            cube([l_total_w, l_base_d, base_t]);
            // Standing wall at far Y (inner face at Y = l_base_d)
            translate([0, l_base_d, 0])
                cube([l_total_w, wall_t, l_wall_h]);
        }
        // USB-C stadium hole — through wall in +Y direction
        // Start just inside inner face, extrude through full wall thickness
        translate([l_total_w/2, l_base_d - eps, l_hole_z])
        rotate([-90, 0, 0])
        linear_extrude(height = wall_t + 2*eps)
            usbc_stadium();
    }

    // 2 standoffs for USB-C board (usbc_hole_to_edge mm from wall inner face)
    for (px = [l_so_x1, l_so_x2]) {
        difference() {
            translate([px, l_so_y, base_t])
                cylinder(d=usbc_bod, h=so_h, $fn=24);
            translate([px, l_so_y, -eps])
                cylinder(d=usbc_bsd, h=base_t + so_h + eps*2, $fn=20);
        }
    }
}

// ============================================================
// Key dimensions
// Section 1 — ESP32     : 46.5 × 23.4 mm, ⌀6 mm boss, M3 pilot ⌀2.5 mm
// Section 2 — USB-C SO  : 5 mm apart, ⌀3.5 mm boss, M2 pilot ⌀1.6 mm
// Section 3 — port wall : 9.5 × 3.5 mm stadium, z = 8 mm, wall = 2.5 mm thick
//             standoffs 28 mm from wall inner face, board flush when correct
// Total footprint        : ~97.5 × 34.5 mm
// ============================================================
