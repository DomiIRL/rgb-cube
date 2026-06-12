// ============================================================
// LED Cube Electronics Enclosure
// Houses: ESP32 DevKit V1 + USB-C PD power board
// Cube (6×6×6, 25mm pitch, 125×125mm footprint) rests on top
// Open bottom faces DOWN; lid screws onto bottom rim (4× M3)
// Printer: Bambu Lab A1 Mini (180×180mm bed)
// Print orientation: SOLID TOP FACE DOWN ON BED
// ============================================================

// ============================================================
// PARAMETERS
// ============================================================

part = "body";  // "body" or "lid"

// Outer shell
width    = 140.0;  // mm — 125mm cube footprint + ~7.5mm margin each side
depth    = 140.0;  // mm
height   =  45.0;  // mm
wall     =   2.5;  // mm
floor_t  =   2.5;  // mm — solid top plate
corner_r =   5.0;  // mm — outer corner radius

// 4 corner fixation holes in top plate
// Thread a short wire through each hole + around a corner LED lead to tie cube down.
fix_hole_d = 3.0;   // mm
fix_corner = 57.0;  // mm — slightly inward from cube corner (62.5mm) to stay clear of corner fills

// 45° corner fills — solid triangular prisms in each inner cavity corner.
// Much stronger than cylinders: the fill bonds to both adjacent walls and
// the ceiling, and the M3 screw threads directly into the solid PLA mass.
corner_fill = 15.0; // mm — leg length of the right-angle triangle (per side)

// M3 lid screws — pilot hole into the corner fill (self-tap in PLA)
lid_screw_pilot = 2.5;   // mm — M3 pilot: screw cuts thread into PLA
lid_screw_clear = 3.3;   // mm — M3 clearance in lid plate
lid_screw_depth = 12.0;  // mm — pilot depth (more = better thread engagement)

// USB-C port cutout — back face (−Y), stadium profile (r = h/2)
usbc_w  =  9.5;  // mm — port opening width
usbc_h  =  3.5;  // mm — port opening height (USB-C connector ~2.56 mm + tolerance)

// ESP32 DevKit V1 standoffs — centred at (+18, +15), clear of corners
// Board: 51.5×28.6mm, 4× M3 holes ⌀3mm at 46.5×23.4mm spacing (source: mischianti.org)
esp_ox  =  18.0;  // mm — pattern centre offset X
esp_oy  =  15.0;  // mm — pattern centre offset Y
esp_mx  =  46.5;  // mm — mounting hole spacing X (DevKit V1, long axis)
esp_my  =  23.4;  // mm — mounting hole spacing Y (DevKit V1, short axis)
esp_od  =   4.0;  // mm — standoff outer diameter (slim to clear PCB components when upside-down)
esp_h   =   5.0;  // mm — standoff height
esp_sd  =   2.5;  // mm — M3 self-tap pilot in PLA (board holes are ⌀3mm → M3 screws)

// USB-C PD board standoffs — same height as ESP32, near back face
// Measured: hole ⌀2 mm, c-c spacing 5 mm, hole centre 28 mm from PCB edge,
// connector protrudes 1 mm past PCB edge (tip is 29 mm from hole centres).
// Board PCB edge flush with inner wall (Y=−67.5 mm) → hole centres at Y=−39.5 mm.
usbc_bmx =   5.0;  // mm — mounting hole spacing X (measured)
usbc_boy = -39.5;  // mm — board centre Y (hole centres, 28 mm from PCB back edge)
usbc_bod =   3.5;  // mm — standoff outer ⌀ (must be < 5 mm c-c spacing)
usbc_bsd =   1.6;  // mm — M2 self-tap pilot in PLA (board holes ⌀2 mm → M2 screws)

// Lid
lid_t = 2.5;  // mm

// ============================================================
// DERIVED
// ============================================================
inner_w = width  - 2*wall;   // 125 mm
inner_d = depth  - 2*wall;   // 125 mm
inner_h = height - floor_t;  // 42.5 mm
ceil_z  = height - floor_t;  // 42.5 mm

// Both boards hang from ceiling on standoffs of height esp_h
board_z = ceil_z - esp_h;    // 37.5 mm — PCB surface height from Z=0
usbc_z  = board_z + 2.0;     // 39.5 mm — USB-C port centre (adjust to board)

// Corner fill screw hole: at the centroid of each triangular fill
// centroid is corner_fill/3 inward from each wall face
screw_cx = inner_w/2 - corner_fill/3;   // ≈ 55.8 mm from centre
screw_cy = inner_d/2 - corner_fill/3;

eps = 0.01;

// ============================================================
// MODULE: USB-C stadium profile (2D, XY plane, centred at origin)
// ============================================================
module usbc_stadium() {
    r = usbc_h / 2;
    hull() {
        translate([-(usbc_w/2 - r), 0]) circle(r=r, $fn=32);
        translate([ (usbc_w/2 - r), 0]) circle(r=r, $fn=32);
    }
}

// ============================================================
// MODULE: one 45° corner fill + screw pilot hole
// sx/sy: sign for which corner (+1 or −1)
// ============================================================
module corner_block(sx, sy) {
    // Absolute inner corner position
    cx = sx * inner_w/2;
    cy = sy * inner_d/2;

    // Screw hole centre
    hx = sx * screw_cx;
    hy = sy * screw_cy;

    difference() {
        // Triangular prism filling the 45° corner — full height so it bonds
        // to both adjacent walls AND the ceiling.
        linear_extrude(height = ceil_z + eps)
        polygon([
            [cx,              cy             ],   // inner corner point
            [cx - sx*corner_fill, cy        ],   // along X wall
            [cx,              cy - sy*corner_fill]    // along Y wall
        ]);

        // M3 pilot hole from open bottom (Z=0) — screw threads into PLA
        translate([hx, hy, -eps])
        cylinder(d=lid_screw_pilot, h=lid_screw_depth + eps*2, $fn=20);
    }
}

// ============================================================
// BODY
// ============================================================
if (part == "body") {

    difference() {
        // ---- Outer solid box ----
        translate([0, 0, height/2])
        minkowski() {
            cube([width  - 2*corner_r,
                  depth  - 2*corner_r,
                  height - eps], center=true);
            cylinder(r=corner_r, h=eps, $fn=48);
        }

        // ---- Hollow interior (open bottom to ceiling) ----
        translate([-inner_w/2, -inner_d/2, -eps])
        cube([inner_w, inner_d, inner_h + eps]);

        // ---- 4 corner fixation holes in top plate ----
        for (sx = [-1, 1]) for (sy = [-1, 1]) {
            translate([sx * fix_corner, sy * fix_corner, ceil_z - eps])
            cylinder(d=fix_hole_d, h=floor_t + eps*2, $fn=20);
        }

        // ---- USB-C port cutout (back face = −Y) — stadium profile ----
        translate([0, -(depth/2) - eps, usbc_z])
        rotate([-90, 0, 0])
        linear_extrude(height = wall + 2*eps)
            usbc_stadium();
    }

    // ---- 45° corner fills with M3 pilot holes ----
    for (sx = [-1, 1]) for (sy = [-1, 1]) {
        corner_block(sx, sy);
    }

    // ---- ESP32 standoffs (hang from ceiling) ----
    for (sx = [-1, 1]) for (sy = [-1, 1]) {
        px = sx * esp_mx/2 + esp_ox;
        py = sy * esp_my/2 + esp_oy;
        difference() {
            translate([px, py, ceil_z - esp_h])
            cylinder(d=esp_od, h=esp_h + eps, $fn=24);
            translate([px, py, ceil_z - esp_h - eps])
            cylinder(d=esp_sd, h=esp_h + eps*3, $fn=20);
        }
    }

    // ---- USB-C PD board standoffs (same height as ESP32) ----
    for (sx = [-1, 1]) {
        px = sx * usbc_bmx / 2;
        py = usbc_boy;
        difference() {
            translate([px, py, ceil_z - esp_h])
            cylinder(d=usbc_bod, h=esp_h + eps, $fn=24);
            translate([px, py, ceil_z - esp_h - eps])
            cylinder(d=usbc_bsd, h=esp_h + eps*3, $fn=20);
        }
    }
}

// ============================================================
// LID — flat plate with 4× M3 clearance holes at corner centroids
// ============================================================
if (part == "lid") {
    difference() {
        translate([0, 0, lid_t/2])
        minkowski() {
            cube([width  - 2*corner_r,
                  depth  - 2*corner_r,
                  lid_t - eps], center=true);
            cylinder(r=corner_r, h=eps, $fn=48);
        }
        for (sx = [-1, 1]) for (sy = [-1, 1]) {
            translate([sx * screw_cx, sy * screw_cy, -eps])
            cylinder(d=lid_screw_clear, h=lid_t + eps*2, $fn=20);
        }
    }
}

// ============================================================
// KEY DIMENSIONS
// outer body      : 140 × 140 × 45 mm, open bottom
// inner cavity    : 135 × 135 × 42.5 mm
// corner fills    : 45°, 15mm leg, full-height — M3 pilot ⌀2.5 mm, 12mm deep
// screw positions : ±62.5 mm from centre (corner centroid, = inner_w/2 − fill/3)
// fixation holes  : ⌀3 mm at ±57 mm (inward from cube corners ±62.5 mm, clear of fills)
// USB-C cutout    : 9.5 × 3.5 mm stadium (r=1.75 mm), back face, centre z≈39.5 mm
// ESP32 standoffs : 46.5×23.4 mm at (+18,+15), 5 mm, M3 (board holes ⌀3mm), boss ⌀4 mm
// USB-C standoffs : 5 mm spacing at (0,−39.5), 5 mm, M2 (⌀3.5 mm boss, ⌀1.6 mm pilot)
// board surface z : 37.5 mm (both boards level)
// lid             : 140 × 140 × 2.5 mm, 4× M3 clearance ⌀3.3 mm
// ============================================================
