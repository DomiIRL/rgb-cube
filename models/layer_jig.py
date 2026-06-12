import cadquery as cq

# ============================================================
# PARAMETERS
# ============================================================
# LED specs (APA106 F5 / P9823, 5mm through-hole RGB)
led_body_d     = 5.0   # mm - LED body diameter
led_clearance  = 0.3   # mm - hole oversize for easy insertion/removal

# Grid
grid_n         = 6     # LEDs per side (6×6 cube)
pitch          = 20.0  # mm - center-to-center spacing
                        # 20mm gives 15mm gap between 5mm LED bodies —
                        # enough room to solder and route wires cleanly

# Board
margin         = 10.0  # mm - board edge beyond outermost LED centers
board_t        = 3.0   # mm - board thickness (holds LEDs vertical while soldering)
corner_r       = 3.0   # mm - corner fillet radius

# ============================================================
# DERIVED
# ============================================================
hole_d       = led_body_d + led_clearance   # 5.3 mm
board_side   = (grid_n - 1) * pitch + 2 * margin  # 120 mm

# ============================================================
# PHASE 1 — base board only, no holes yet
# Print orientation: flat side down (this face is the print bed)
# ============================================================
board = (
    cq.Workplane("XY")
    .box(board_side, board_side, board_t, centered=(True, True, False))
)

# ============================================================
# EXPORT
# ============================================================
cq.exporters.export(board, "layer_jig_phase1.stl",
                    tolerance=0.01, angularTolerance=0.1)
print(f"Phase 1 board: {board_side:.0f}x{board_side:.0f}x{board_t:.0f}mm")
print(f"Hole diameter will be: {hole_d:.1f}mm")
print(f"Pitch: {pitch:.0f}mm | Grid: {grid_n}x{grid_n} = {grid_n**2} LEDs")
