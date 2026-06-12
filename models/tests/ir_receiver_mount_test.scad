// ============================================================
// IR Receiver Mount — Fit Test
//
// Tests the cradle that holds the Elegoo IR receiver board flat against
// the underside of the enclosure top plate, with the TSOP-1838 dome poking
// UP through a hole into the air gap between four LEDs.
//
// ASSUMES the dome faces PERPENDICULAR to the PCB (points up when the board
// lies flat) — standard KY-022 / Elegoo layout. If the dome looks off an
// EDGE instead, tell me and I'll rotate the mount 90°.
//
// All board dims below are WEB-SOURCED DEFAULTS — adjust after measuring
// your board, then reprint. We iterate this like the USB-C port test.
//
// ---- Print ----
// PLA, 0.2 mm layer, 2 walls, 15% infill, NO supports.
// Orientation: plate flat on the bed, cradle walls UP (this is the real
// top-plate print orientation: exterior face down).
//
// ---- How to test ----
// 1. After printing, drop the IR board into the cradle, sensor/dome end over
//    the round hole, header end at the OPEN/notched wall (for the wires).
// 2. The two snap lips on the long walls should hold the board down flat.
// 3. Flip the piece over: the dome should poke out the flat (exterior) face
//    by a few mm — that's what sticks up between the LEDs.
// 4. Tell me: does the board fit (too tight / too loose)? Does the dome poke
//    out a good amount (too little / too much)? Do the lips hold it?
// ============================================================

// ============================================================
// PARAMETERS
// ============================================================

// ---- IR receiver PCB (Elegoo module — WEB DEFAULTS, measure & adjust) ----
pcb_l = 18.5;   // mm — board length (sensor end → header end)
pcb_w = 15.6;   // mm — board width
pcb_t =  1.2;   // mm — board thickness
fit   =  0.4;   // mm — total clearance around PCB in the pocket (0.2/side)

// ---- TSOP-1838 dome (perpendicular to board) ----
dome_d        = 5.5;   // mm — dome hole ⌀ (≈5.0 dome + clearance)
dome_from_end = 6.0;   // mm — dome centre, measured from the SENSOR-end edge
dome_protrude = 5.0;   // mm — (info) how far the dome stands above the PCB face

// ---- Enclosure top plate (ceiling) ----
ceil_t = 2.5;   // mm — top-plate thickness (= floor_t in the enclosure)

// ---- Cradle ----
wall    = 2.0;  // mm — pocket wall thickness
lip     = 1.0;  // mm — snap-lip inward overhang over the board
lip_h   = 1.2;  // mm — snap-lip thickness (Z)
lip_len = 6.0;  // mm — snap-lip length along the board
border  = 4.0;  // mm — flat plate margin beyond the cradle walls
wire_notch = 10.0;  // mm — gap in the header-end wall for wires/pins

eps = 0.01;

// ============================================================
// DERIVED
// ============================================================
poc_l = pcb_l + fit;        // pocket inner length
poc_w = pcb_w + fit;        // pocket inner width
out_l = poc_l + 2*wall;     // cradle outer length
out_w = poc_w + 2*wall;     // cradle outer width
plate_l = out_l + 2*border; // flat plate length
plate_w = out_w + 2*border; // flat plate width

wall_h = pcb_t + lip_h;     // cradle wall height above the ceiling interior face
dome_x = -pcb_l/2 + dome_from_end;  // dome centre X (sensor end at −X)

// Z layout: z=0 = exterior (on bed), z=ceil_t = interior face, cradle above
int_z = ceil_t;             // interior face of the ceiling

// ============================================================
// MODEL
// ============================================================
difference() {
    union() {
        // ---- Ceiling plate (a patch of the enclosure top plate) ----
        translate([-plate_l/2, -plate_w/2, 0])
            cube([plate_l, plate_w, ceil_t]);

        // ---- Cradle frame on the interior side ----
        translate([0, 0, int_z])
        difference() {
            translate([-out_l/2, -out_w/2, 0])
                cube([out_l, out_w, wall_h]);
            // hollow the pocket
            translate([-poc_l/2, -poc_w/2, -eps])
                cube([poc_l, poc_w, wall_h + 2*eps]);
            // wire notch in the header-end (+X) wall
            translate([out_l/2 - wall - eps, -wire_notch/2, -eps])
                cube([wall + 2*eps, wire_notch, wall_h + 2*eps]);
        }
    }

    // ---- Dome hole through the plate ----
    translate([dome_x, 0, -eps])
        cylinder(d=dome_d, h=ceil_t + 2*eps, $fn=48);
}

// ---- Snap lips on the two long walls (hold the board down flat) ----
for (sy = [-1, 1]) {
    yy = (sy > 0) ? poc_w/2 - lip : -poc_w/2;
    translate([-lip_len/2, yy, int_z + pcb_t])
        cube([lip_len, lip, lip_h]);
}

// ============================================================
// KEY DIMENSIONS  (defaults — refine after measuring the board)
// plate        : 30.9 × 28.0 × 2.5 mm  + 2.4 mm cradle walls = 4.9 mm tall
// pocket       : 18.9 × 16.0 mm (PCB 18.5 × 15.6 + 0.4 fit)
// dome hole    : ⌀5.5 mm, 3.25 mm toward sensor end from pocket centre
// dome poke-out: dome_protrude − ceil_t = 5.0 − 2.5 = ~2.5 mm past exterior
// snap lips    : 1 mm overhang, on both long walls
// wire exit    : 10 mm notch in the header-end wall
// ============================================================
