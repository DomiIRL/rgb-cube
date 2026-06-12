// ============================================================
// USB-C PD Mount — Minimal Fit Test
//
// Reproduces ONLY the USB-C board mount from electronics_enclosure.scad,
// at the CORRECTED port height (measured: top of port 5 mm above PCB bottom):
//   • 2× M2 standoffs, 5 mm apart, 5 mm tall — the board screws onto these
//   • back wall with the USB-C stadium cutout at the corrected height
//   • a short "ceiling lip" above the cutout = the enclosure top plate, so
//     the port sits flush under it exactly like in the real enclosure
//
// TEST: screw the USB-C PD board onto the 2 standoffs, connector facing the
// wall. The connector should poke cleanly through the stadium hole, its top
// edge meeting the underside of the ceiling lip. If the port is still too
// low/high, change usbc_port_top_above_board here AND in the enclosure.
//
// Printer: Bambu Lab A1 Mini. Print flat (base on bed), no supports.
// ============================================================

// ---- Parameters mirrored from electronics_enclosure.scad ----
usbc_w   =  9.5;   // mm — port opening width
usbc_h   =  3.5;   // mm — port opening height (stadium r = h/2 = 1.75)
usbc_port_top_above_board = 5.0;  // mm — MEASURED: top of port above PCB bottom face

usbc_bmx =  5.0;   // mm — board mounting hole spacing
usbc_bod =  3.5;   // mm — standoff outer ⌀
usbc_bsd =  1.6;   // mm — M2 self-tap pilot

so_h     =  5.0;   // mm — standoff height (= esp_h in enclosure)
wall_t   =  2.5;   // mm — enclosure wall thickness
ceil_lip =  2.5;   // mm — top-plate thickness above the cutout (= floor_t)

usbc_hole_to_edge = 28.0;  // mm — standoff centre → connector edge (= wall inner face)

// ---- Test-piece params ----
base_t   =  2.0;   // mm — base plate thickness
margin   =  6.0;   // mm — base material in front of the standoffs
total_w  = 24.0;   // mm — X width (room for 9.5 mm hole + 5 mm standoffs + walls)

eps = 0.01;

// ---- Derived ----
base_d   = usbc_hole_to_edge + margin;                        // 34 mm — base depth (Y)
board_z  = base_t + so_h;                                     // 7 mm — PCB seating plane
hole_z   = board_z + usbc_port_top_above_board - usbc_h/2;    // 10.25 mm — cutout centre
wall_h   = hole_z + usbc_h/2 + ceil_lip;                      // 14.5 mm — wall total height

so_y     = margin;                                            // standoff Y (= base_d − hole_to_edge)
so_x1    = total_w/2 - usbc_bmx/2;                            // 9.5 mm
so_x2    = total_w/2 + usbc_bmx/2;                            // 14.5 mm

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
// MODEL
// ============================================================
difference() {
    union() {
        // Base plate (board area)
        cube([total_w, base_d, base_t]);
        // Back wall at the far +Y edge (inner face at Y = base_d)
        translate([0, base_d, 0])
            cube([total_w, wall_t, wall_h]);
    }
    // USB-C stadium hole through the wall (+Y) at the corrected height
    translate([total_w/2, base_d - eps, hole_z])
    rotate([-90, 0, 0])
    linear_extrude(height = wall_t + 2*eps)
        usbc_stadium();
}

// 2 standoffs for the USB-C board (usbc_hole_to_edge from wall inner face)
for (px = [so_x1, so_x2]) {
    difference() {
        translate([px, so_y, base_t])
            cylinder(d=usbc_bod, h=so_h, $fn=24);
        translate([px, so_y, -eps])
            cylinder(d=usbc_bsd, h=base_t + so_h + eps*2, $fn=20);
    }
}

// ============================================================
// KEY DIMENSIONS
// footprint   : 24 × 36.5 × 14.5 mm (incl. wall)
// standoffs   : 5 mm apart, 5 mm tall, ⌀3.5 mm boss, M2 pilot ⌀1.6 mm
// board plane : z = 7 mm (top of standoffs)
// cutout      : 9.5 × 3.5 mm stadium, centre z = 10.25 mm, top z = 12 mm
//               (= board plane + 5 mm; 2.5 mm ceiling lip above)
// wall        : 2.5 mm thick, 28 mm behind the standoffs
// ============================================================
