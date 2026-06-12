#pragma once
#include "../Mode.h"
#include <Arduino.h>
#include <math.h>
#include <string.h>

// ModeTetris — an ambient "Tetris Effect" cascade for the 6x6x6 cube.
//
// This is eye-candy, not a game: the cube auto-plays itself forever. The seven
// classic tetrominoes fall in their iconic colours (I-cyan, O-yellow, T-purple,
// S-green, Z-red, J-blue, L-orange), each lying flat in the X-Z plane so a piece
// drops down the vertical Y axis and adds four cells to one horizontal layer. A
// lightweight greedy placer chooses where each piece lands — packing them low,
// tight against neighbours and walls, and avoiding holes — so the bottom layers
// keep filling up. Whenever a full 6x6 layer completes it flashes white and the
// stack above collapses down into the gap, exactly like Tetris line-clears, so
// the stack stays low and clears keep coming. Over the settled blocks a slow
// brightness wave rises through the cube for the hypnotic, breathing feel.
//
// Animation niceties: falling pieces use a float Y with vertical anti-aliasing
// (brightness split between the two nearest layers) so they glide smoothly
// instead of snapping cell-to-cell, while the X-Z block edges stay crisp. Locks
// give a quick white "pop", clears flash and then the upper stack glides down
// over a short eased collapse.
//
// State is a colour-id per voxel (0 = empty, 1..7 = piece colour) plus a per-
// column height cache — a few hundred bytes total. The placer runs once per
// piece (not per frame); rendering is a full repaint every ~33 ms like the other
// 3D modes, so the global power cap in Cube handles any bright frames.
//
// NB: the phase enumerators are PH_-prefixed on purpose — Arduino #defines
// FALLING / RISING / CHANGE as interrupt-edge macros, so a bare `FALLING`
// enumerator would be silently rewritten by the preprocessor and fail to build.
class ModeTetris : public Mode {
public:
    explicit ModeTetris(float fallPerFrame = 0.16f,
                        uint32_t frameMs = 30,
                        float shimmerSpeed = 0.010f)
        : _fallPerFrame(fallPerFrame),
          _frameMs(frameMs),
          _shimmerSpeed(shimmerSpeed),
          _lastMs(0),
          _shimmer(0.0f) {
    }

    void onEnter(Cube& cube) override {
        resetGrid();
        _shimmer  = 0.0f;
        _bagPos   = NUM_PIECES;   // forces a fresh shuffle on the first spawn
        _phase    = PH_SPAWN;
        _phaseT   = 0;
        _lastMs   = 0;
        cube.clear();
        cube.show();
    }

    void update(Cube& cube, uint32_t ms) override {
        if (ms - _lastMs < _frameMs) {
            return;
        }
        _lastMs = ms;

        // advance the shimmer wave every frame, regardless of phase
        _shimmer += _shimmerSpeed;
        if (_shimmer > 1000.0f) {
            _shimmer -= 1000.0f;
        }

        switch (_phase) {
            case PH_SPAWN:    stepSpawn();    break;
            case PH_FALLING:  stepFalling();  break;
            case PH_LOCKED:   stepLocked();   break;
            case PH_FLASH:    stepFlash();    break;
            case PH_COLLAPSE: stepCollapse(); break;
        }

        render(cube);
        cube.show();
    }

private:
    // ── piece library ───────────────────────────────────────────────────
    static constexpr int NUM_PIECES = 7;
    static constexpr int PLANE_CELLS = CUBE_X * CUBE_Z;   // cells in one Y-layer

    enum Phase { PH_SPAWN, PH_FALLING, PH_LOCKED, PH_FLASH, PH_COLLAPSE };

    // phase durations (ms)
    static constexpr uint32_t LOCK_MS     = 110;
    static constexpr uint32_t FLASH_MS    = 170;
    static constexpr uint32_t COLLAPSE_MS = 150;

    // The seven tetrominoes as flat footprints in the X-Z plane (offsets from a
    // local origin). Each occupies four distinct (x,z) columns, one cell each.
    static const int8_t (*basePiece(int type))[2] {
        static const int8_t B[NUM_PIECES][4][2] = {
            {{0, 0}, {1, 0}, {2, 0}, {3, 0}},   // I
            {{0, 0}, {1, 0}, {0, 1}, {1, 1}},   // O
            {{0, 0}, {1, 0}, {2, 0}, {1, 1}},   // T
            {{1, 0}, {2, 0}, {0, 1}, {1, 1}},   // S
            {{0, 0}, {1, 0}, {1, 1}, {2, 1}},   // Z
            {{0, 0}, {1, 0}, {2, 0}, {0, 1}},   // J
            {{0, 0}, {1, 0}, {2, 0}, {2, 1}},   // L
        };
        return B[type];
    }

    // iconic Tetris colours, packed as Adafruit Color() does: (r<<16)|(g<<8)|b.
    // index = colour-id - 1, in piece order I,O,T,S,Z,J,L.
    static uint32_t pieceColor(uint8_t id) {
        static const uint32_t COLORS[NUM_PIECES] = {
            0x00FFFFu,   // I cyan
            0xFFFF00u,   // O yellow
            0xA000F0u,   // T purple
            0x00FF00u,   // S green
            0xFF0000u,   // Z red
            0x0000FFu,   // J blue
            0xFF8C00u,   // L orange
        };
        return COLORS[(id - 1) % NUM_PIECES];
    }

    // Rotate a piece's footprint k quarter-turns in the X-Z plane and normalise
    // it so the smallest offset on each axis is 0. (a,b) -> (b,-a) per turn.
    static void footprint(int type, int rot, int out[4][2]) {
        const int8_t (*base)[2] = basePiece(type);
        int p[4][2];
        for (int i = 0; i < 4; i++) {
            p[i][0] = base[i][0];
            p[i][1] = base[i][1];
        }
        for (int r = 0; r < rot; r++) {
            for (int i = 0; i < 4; i++) {
                int a = p[i][0], b = p[i][1];
                p[i][0] = b;
                p[i][1] = -a;
            }
        }
        int minA = p[0][0], minB = p[0][1];
        for (int i = 1; i < 4; i++) {
            if (p[i][0] < minA) minA = p[i][0];
            if (p[i][1] < minB) minB = p[i][1];
        }
        for (int i = 0; i < 4; i++) {
            out[i][0] = p[i][0] - minA;
            out[i][1] = p[i][1] - minB;
        }
    }

    // ── grid helpers ────────────────────────────────────────────────────
    void resetGrid() {
        memset(_cell, 0, sizeof(_cell));
        memset(_h, 0, sizeof(_h));
        memset(_planeCount, 0, sizeof(_planeCount));
    }

    // Recompute the per-column surface heights and per-layer occupancy from the
    // voxel grid. Cheap (a few hundred reads); run once when a piece spawns.
    void recomputeDerived() {
        for (int x = 0; x < CUBE_X; x++) {
            for (int z = 0; z < CUBE_Z; z++) {
                int h = 0;
                for (int y = CUBE_Y - 1; y >= 0; y--) {
                    if (_cell[x][y][z]) { h = y + 1; break; }
                }
                _h[x][z] = (uint8_t)h;
            }
        }
        for (int y = 0; y < CUBE_Y; y++) {
            int c = 0;
            for (int x = 0; x < CUBE_X; x++) {
                for (int z = 0; z < CUBE_Z; z++) {
                    if (_cell[x][y][z]) c++;
                }
            }
            _planeCount[y] = (uint16_t)c;
        }
    }

    // a neighbour cell counts as "contact" if it is a wall or already filled
    int contactAt(int x, int y, int z) const {
        if (x < 0 || x >= CUBE_X || z < 0 || z >= CUBE_Z) return 1;  // wall
        if (y < 0) return 1;                                          // floor
        if (y >= CUBE_Y) return 0;
        return _cell[x][y][z] ? 1 : 0;
    }

    // ── 7-bag randomiser ────────────────────────────────────────────────
    int nextPieceType() {
        if (_bagPos >= NUM_PIECES) {
            for (int i = 0; i < NUM_PIECES; i++) {
                _bag[i] = (uint8_t)i;
            }
            for (int i = NUM_PIECES - 1; i > 0; i--) {
                int j = random(0, i + 1);
                uint8_t t = _bag[i]; _bag[i] = _bag[j]; _bag[j] = t;
            }
            _bagPos = 0;
        }
        return _bag[_bagPos++];
    }

    // ── phase steps ─────────────────────────────────────────────────────

    // Pick the next piece and the best place to drop it, then start it falling.
    void stepSpawn() {
        recomputeDerived();

        // Endless-run relief valve. A 6x6 layer can't always be tiled exactly by
        // the random tetromino stream, so a few un-fillable gaps can stall the
        // natural line-clears and let the stack creep to the top. To keep the
        // cascade endless (instead of filling solid and resetting), once every
        // column has built up near the top, clear the lowest occupied layer and
        // collapse — it reuses the normal flash+collapse, so it just reads as a
        // line-clear. Tune the trigger with the CUBE_Y - 2 margin below.
        if (CUBE_Y >= 4) {
            int minH = CUBE_Y;
            for (int x = 0; x < CUBE_X; x++) {
                for (int z = 0; z < CUBE_Z; z++) {
                    if (_h[x][z] < minH) minH = _h[x][z];
                }
            }
            if (minH >= CUBE_Y - 2 && forceClearLowest()) {
                return;
            }
        }

        int type = nextPieceType();

        long bestScore = 0;
        bool found = false;
        int bestCells[4][2];
        int bestLanding = 0;

        int fp[4][2];
        for (int rot = 0; rot < 4; rot++) {
            footprint(type, rot, fp);
            int maxA = 0, maxB = 0;
            for (int i = 0; i < 4; i++) {
                if (fp[i][0] > maxA) maxA = fp[i][0];
                if (fp[i][1] > maxB) maxB = fp[i][1];
            }
            for (int ox = 0; ox + maxA < CUBE_X; ox++) {
                for (int oz = 0; oz + maxB < CUBE_Z; oz++) {
                    // landing height = highest column surface under the footprint
                    int landing = 0;
                    for (int i = 0; i < 4; i++) {
                        int hh = _h[ox + fp[i][0]][oz + fp[i][1]];
                        if (hh > landing) landing = hh;
                    }
                    if (landing > CUBE_Y - 1) continue;   // no room: cell would be off the top

                    int holes = 0, contact = 0;
                    for (int i = 0; i < 4; i++) {
                        int cx = ox + fp[i][0];
                        int cz = oz + fp[i][1];
                        holes += landing - _h[cx][cz];          // empty cells trapped below
                        contact += contactAt(cx - 1, landing, cz);
                        contact += contactAt(cx + 1, landing, cz);
                        contact += contactAt(cx, landing, cz - 1);
                        contact += contactAt(cx, landing, cz + 1);
                        contact += contactAt(cx, landing - 1, cz);   // support underneath
                    }
                    int newCount  = _planeCount[landing] + 4;
                    int completed = (newCount >= PLANE_CELLS) ? 1 : 0;

                    long score = 100000L * completed;
                    score += (long)newCount * (CUBE_Y - landing) * 40L;  // pack low layers full
                    score += 12L * contact;                              // hug walls/neighbours
                    score -= 500L * holes;                               // never trap gaps
                    score -= 80L * landing;                              // stay low
                    score += random(0, 6);                               // break ties organically

                    if (!found || score > bestScore) {
                        found = true;
                        bestScore = score;
                        bestLanding = landing;
                        for (int i = 0; i < 4; i++) {
                            bestCells[i][0] = ox + fp[i][0];
                            bestCells[i][1] = oz + fp[i][1];
                        }
                    }
                }
            }
        }

        if (!found) {            // no legal placement anywhere — relieve, never freeze
            if (!forceClearLowest()) {
                resetGrid();     // degenerate (empty grid) — restart cleanly
            }
            return;
        }

        for (int i = 0; i < 4; i++) {
            _fcells[i][0] = bestCells[i][0];
            _fcells[i][1] = bestCells[i][1];
        }
        _fcolor   = (uint8_t)(type + 1);
        _landingY = bestLanding;
        _pieceY   = (float)CUBE_Y - 0.5f;       // slide in from just above the top
        if (_pieceY < (float)_landingY) {
            _pieceY = (float)_landingY;
        }
        _phase  = PH_FALLING;
        _phaseT = 0;
    }

    void stepFalling() {
        _pieceY -= _fallPerFrame;
        if (_pieceY <= (float)_landingY) {
            _pieceY = (float)_landingY;
            for (int i = 0; i < 4; i++) {       // lock the four cells into the stack
                _cell[_fcells[i][0]][_landingY][_fcells[i][1]] = _fcolor;
            }
            _phase  = PH_LOCKED;
            _phaseT = 0;
        }
    }

    void stepLocked() {
        _phaseT += _frameMs;
        if (_phaseT >= LOCK_MS) {
            detectClears();
            _phase  = (_clearCount > 0) ? PH_FLASH : PH_SPAWN;
            _phaseT = 0;
        }
    }

    void stepFlash() {
        _phaseT += _frameMs;
        if (_phaseT >= FLASH_MS) {
            buildCollapse();
            _phase  = PH_COLLAPSE;
            _phaseT = 0;
        }
    }

    void stepCollapse() {
        _phaseT += _frameMs;
        if (_phaseT >= COLLAPSE_MS) {
            resetGrid();                        // rebuild the grid from settled cells
            for (int i = 0; i < _mcCount; i++) {
                _cell[_mc[i].x][_mc[i].toY][_mc[i].z] = _mc[i].color;
            }
            _phase  = PH_SPAWN;
            _phaseT = 0;
        }
    }

    // mark every completely-filled Y-layer for clearing
    void detectClears() {
        _clearCount = 0;
        for (int y = 0; y < CUBE_Y; y++) {
            int c = 0;
            for (int x = 0; x < CUBE_X; x++) {
                for (int z = 0; z < CUBE_Z; z++) {
                    if (_cell[x][y][z]) c++;
                }
            }
            _clearPlane[y] = (c == PLANE_CELLS);
            if (_clearPlane[y]) _clearCount++;
        }
    }

    // snapshot every surviving cell with the layer it will fall to, so the
    // collapse can be animated smoothly before the grid is rebuilt
    void buildCollapse() {
        _mcCount = 0;
        for (int x = 0; x < CUBE_X; x++) {
            for (int z = 0; z < CUBE_Z; z++) {
                for (int y = 0; y < CUBE_Y; y++) {
                    if (!_cell[x][y][z] || _clearPlane[y]) continue;
                    int drop = 0;
                    for (int k = 0; k < y; k++) {
                        if (_clearPlane[k]) drop++;
                    }
                    _mc[_mcCount].x     = (uint8_t)x;
                    _mc[_mcCount].z     = (uint8_t)z;
                    _mc[_mcCount].fromY = (uint8_t)y;
                    _mc[_mcCount].toY   = (uint8_t)(y - drop);
                    _mc[_mcCount].color = _cell[x][y][z];
                    _mcCount++;
                }
            }
        }
    }

    // Relief valve (see stepSpawn): mark the lowest occupied layer for clearing
    // and enter the flash→collapse pipeline. Clearing a partial layer is fine —
    // buildCollapse drops everything above it regardless. Returns false only if
    // the cube is empty (nothing to clear) so the caller can fall back.
    bool forceClearLowest() {
        for (int y = 0; y < CUBE_Y; y++) {
            _clearPlane[y] = false;
        }
        _clearCount = 0;
        for (int y = 0; y < CUBE_Y; y++) {
            bool any = false;
            for (int x = 0; x < CUBE_X && !any; x++) {
                for (int z = 0; z < CUBE_Z && !any; z++) {
                    if (_cell[x][y][z]) any = true;
                }
            }
            if (any) {
                _clearPlane[y] = true;
                _clearCount = 1;
                break;
            }
        }
        if (_clearCount == 0) {
            return false;   // empty cube — nothing to relieve
        }
        _phase  = PH_FLASH;
        _phaseT = 0;
        return true;
    }

    // ── rendering ───────────────────────────────────────────────────────

    static float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
    static float smoothstep(float p) { return p * p * (3.0f - 2.0f * p); }

    // slow brightness wave rising through the cube (0.62 .. 1.0)
    float shimmerFactor(int x, int y, int z) const {
        float phase = _shimmer * 6.2831853f - (float)y * 0.85f + (float)(x + z) * 0.20f;
        return 0.62f + 0.38f * (0.5f + 0.5f * sinf(phase));
    }

    static uint32_t scaleColor(uint32_t c, float f) {
        if (f < 0.0f) f = 0.0f; else if (f > 1.0f) f = 1.0f;
        uint8_t r = (uint8_t)(((c >> 16) & 0xFF) * f);
        uint8_t g = (uint8_t)(((c >> 8) & 0xFF) * f);
        uint8_t b = (uint8_t)((c & 0xFF) * f);
        return Cube::colorRGB(r, g, b);
    }

    static uint32_t whiteBlend(uint32_t c, float amt) {
        amt = clamp01(amt);
        uint8_t r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = c & 0xFF;
        r = (uint8_t)(r + (255 - r) * amt);
        g = (uint8_t)(g + (255 - g) * amt);
        b = (uint8_t)(b + (255 - b) * amt);
        return Cube::colorRGB(r, g, b);
    }

    // draw a cell at a fractional Y, splitting its brightness across the two
    // nearest layers so vertical motion reads as a smooth glide
    void drawAA(Cube& cube, int x, float yf, int z, uint32_t color) {
        int y0 = (int)floorf(yf);
        float f = yf - (float)y0;
        if (1.0f - f > 0.0f) cube.setPixel(x, y0,     z, scaleColor(color, 1.0f - f));
        if (f > 0.0f)        cube.setPixel(x, y0 + 1, z, scaleColor(color, f));
    }

    void render(Cube& cube) {
        cube.clear();

        if (_phase == PH_COLLAPSE) {
            float p = smoothstep(clamp01((float)_phaseT / (float)COLLAPSE_MS));
            for (int i = 0; i < _mcCount; i++) {
                float yf = (float)_mc[i].fromY + ((float)_mc[i].toY - (float)_mc[i].fromY) * p;
                uint32_t col = scaleColor(pieceColor(_mc[i].color),
                                          shimmerFactor(_mc[i].x, _mc[i].toY, _mc[i].z));
                drawAA(cube, _mc[i].x, yf, _mc[i].z, col);
            }
            return;
        }

        // settled stack with the shimmer wave (skip cells that are flashing white)
        for (int x = 0; x < CUBE_X; x++) {
            for (int y = 0; y < CUBE_Y; y++) {
                for (int z = 0; z < CUBE_Z; z++) {
                    if (!_cell[x][y][z]) continue;
                    if (_phase == PH_FLASH && _clearPlane[y]) continue;
                    cube.setPixel(x, y, z,
                                  scaleColor(pieceColor(_cell[x][y][z]), shimmerFactor(x, y, z)));
                }
            }
        }

        // completed layers flash white, pulsing brightest mid-flash
        if (_phase == PH_FLASH) {
            float pulse = 0.55f + 0.45f * sinf(clamp01((float)_phaseT / (float)FLASH_MS) * 3.14159f);
            uint8_t w = (uint8_t)(255.0f * clamp01(pulse));
            uint32_t white = Cube::colorRGB(w, w, w);
            for (int y = 0; y < CUBE_Y; y++) {
                if (!_clearPlane[y]) continue;
                for (int x = 0; x < CUBE_X; x++) {
                    for (int z = 0; z < CUBE_Z; z++) {
                        cube.setPixel(x, y, z, white);
                    }
                }
            }
        }

        // the active piece: anti-aliased glide while falling, white pop on lock
        if (_phase == PH_FALLING) {
            uint32_t col = pieceColor(_fcolor);   // full brightness so it stands out
            for (int i = 0; i < 4; i++) {
                drawAA(cube, _fcells[i][0], _pieceY, _fcells[i][1], col);
            }
        } else if (_phase == PH_LOCKED) {
            float pop = 1.0f - clamp01((float)_phaseT / (float)LOCK_MS);
            uint32_t col = whiteBlend(pieceColor(_fcolor), pop * 0.8f);
            for (int i = 0; i < 4; i++) {
                cube.setPixel(_fcells[i][0], _landingY, _fcells[i][1], col);
            }
        }
    }

    // ── tuning ──────────────────────────────────────────────────────────
    float    _fallPerFrame;   // cells the piece drops per rendered frame
    uint32_t _frameMs;        // render interval (≈30 ms → ~33 Hz)
    float    _shimmerSpeed;   // shimmer-wave advance per frame
    uint32_t _lastMs;
    float    _shimmer;

    // ── grid + piece state ──────────────────────────────────────────────
    uint8_t  _cell[CUBE_X][CUBE_Y][CUBE_Z];   // 0 = empty, else colour-id 1..7
    uint8_t  _h[CUBE_X][CUBE_Z];              // surface height (next free y) per column
    uint16_t _planeCount[CUBE_Y];             // occupied cells per Y-layer

    uint8_t  _bag[NUM_PIECES];
    int      _bagPos = NUM_PIECES;

    int      _fcells[4][2];   // active piece's four (x,z) columns
    uint8_t  _fcolor = 1;     // active piece colour-id
    float    _pieceY = 0.0f;  // active piece's fractional height
    int      _landingY = 0;   // layer the active piece locks into

    Phase    _phase  = PH_SPAWN;
    uint32_t _phaseT = 0;     // ms elapsed in the current timed phase

    bool     _clearPlane[CUBE_Y];   // which Y-layers are being cleared
    int      _clearCount = 0;

    struct MovingCell { uint8_t x, z, fromY, toY, color; };
    MovingCell _mc[CUBE_LEDS];      // surviving cells during a collapse
    int        _mcCount = 0;
};
