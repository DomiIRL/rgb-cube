#pragma once
#include "../Mode.h"
#include <Arduino.h>
#include <math.h>
#include <string.h>

// ModeTetris — a self-playing 3D Tetris ("Tetris Effect" vibe) for the 6x6x6 cube.
//
// Legit Tetris, played by the cube itself. The seven tetrominoes fall in their
// iconic colours (I-cyan, O-yellow, T-purple, S-green, Z-red, J-blue, L-orange),
// rotate in FULL 3D (they can lie flat OR stand upright), and land by real
// gravity/collision. A horizontal 6x6 layer clears ONLY when it is genuinely,
// completely full — then it flashes white and the stack above collapses into the
// gap. Nothing is ever force-cleared.
//
// A greedy bot places each piece using an El-Tetris-style score: take real
// clears, never bury holes, keep the stack low and flat. Standing pieces let it
// fill narrow wells, so it survives a long time. If it is ever genuinely boxed
// in — no orientation of the next piece fits under the ceiling — that is a real
// top-out: it plays a short game-over dissolve and starts a fresh game (the
// rules, not a cheat). A slow brightness wave breathes over the settled stack.
//
// NB: phase enumerators are PH_-prefixed on purpose — Arduino #defines FALLING /
// RISING / CHANGE as interrupt-edge macros that would clobber a bare `FALLING`.
class ModeTetris : public Mode {
public:
    explicit ModeTetris(float fallPerFrame = 0.18f,
                        uint32_t frameMs = 30,
                        float shimmerSpeed = 0.010f)
        : _fallPerFrame(fallPerFrame), _frameMs(frameMs),
          _shimmerSpeed(shimmerSpeed), _lastMs(0), _shimmer(0.0f) {
    }

    void onEnter(Cube& cube) override {
        generateOrients();
        resetGrid();
        _shimmer = 0.0f;
        _bagPos  = NUM_PIECES;   // force a fresh shuffle on the first spawn
        _phase   = PH_SPAWN;
        _phaseT  = 0;
        _lastMs  = 0;
        cube.clear();
        cube.show();
    }

    void update(Cube& cube, uint32_t ms) override {
        if (ms - _lastMs < _frameMs) {
            return;
        }
        _lastMs = ms;
        _shimmer += _shimmerSpeed;
        if (_shimmer > 1000.0f) _shimmer -= 1000.0f;

        switch (_phase) {
            case PH_SPAWN:    stepSpawn();    break;
            case PH_FALLING:  stepFalling();  break;
            case PH_LOCKED:   stepLocked();   break;
            case PH_FLASH:    stepFlash();    break;
            case PH_COLLAPSE: stepCollapse(); break;
            case PH_GAMEOVER: stepGameOver(); break;
        }
        render(cube);
        cube.show();
    }

private:
    static constexpr int NUM_PIECES  = 7;
    static constexpr int PLANE_CELLS = CUBE_X * CUBE_Z;
    static constexpr int MAX_ORI     = 24;   // max distinct 3D rotations of a tetromino

    enum Phase { PH_SPAWN, PH_FALLING, PH_LOCKED, PH_FLASH, PH_COLLAPSE, PH_GAMEOVER };

    static constexpr uint32_t LOCK_MS     = 100;
    static constexpr uint32_t FLASH_MS    = 170;
    static constexpr uint32_t COLLAPSE_MS = 150;
    static constexpr uint32_t GAMEOVER_MS = 1100;

    // a normalised orientation: 4 cells (offsets) + per-axis extents
    struct Orient { int8_t c[4][3]; uint8_t ex, ey, ez; };

    // iconic Tetris colours, packed as Adafruit Color(): (r<<16)|(g<<8)|b
    static uint32_t pieceColor(uint8_t id) {
        static const uint32_t COLORS[NUM_PIECES] = {
            0x00FFFFu, 0xFFFF00u, 0xA000F0u, 0x00FF00u, 0xFF0000u, 0x0000FFu, 0xFF8C00u
        };
        return COLORS[(id - 1) % NUM_PIECES];
    }

    // ── 3D orientation generation ───────────────────────────────────────
    // 90° right-handed rotations about each axis.
    static void rotX(int c[3]) { int y = c[1], z = c[2]; c[1] = -z; c[2] =  y; }
    static void rotY(int c[3]) { int x = c[0], z = c[2]; c[0] =  z; c[2] = -x; }
    static void rotZ(int c[3]) { int x = c[0], y = c[1]; c[0] = -y; c[1] =  x; }

    static void normalize(int s[4][3]) {
        int mnx = s[0][0], mny = s[0][1], mnz = s[0][2];
        for (int i = 1; i < 4; i++) {
            if (s[i][0] < mnx) mnx = s[i][0];
            if (s[i][1] < mny) mny = s[i][1];
            if (s[i][2] < mnz) mnz = s[i][2];
        }
        for (int i = 0; i < 4; i++) { s[i][0] -= mnx; s[i][1] -= mny; s[i][2] -= mnz; }
    }

    // order-independent key for a normalised shape (coords 0..3 → 2 bits each)
    static uint32_t shapeKey(const int s[4][3]) {
        int v[4];
        for (int i = 0; i < 4; i++) v[i] = (s[i][0] << 4) | (s[i][1] << 2) | s[i][2];
        for (int a = 0; a < 3; a++)
            for (int b = 0; b < 3 - a; b++)
                if (v[b] > v[b + 1]) { int t = v[b]; v[b] = v[b + 1]; v[b + 1] = t; }
        return ((uint32_t)v[0] << 18) | ((uint32_t)v[1] << 12)
             | ((uint32_t)v[2] << 6)  | (uint32_t)v[3];
    }

    // BFS the rotation group from each canonical shape, deduping by key.
    void generateOrients() {
        static const int8_t BASE[NUM_PIECES][4][3] = {
            {{0,0,0},{1,0,0},{2,0,0},{3,0,0}},  // I
            {{0,0,0},{1,0,0},{0,1,0},{1,1,0}},  // O
            {{0,0,0},{1,0,0},{2,0,0},{1,1,0}},  // T
            {{1,0,0},{2,0,0},{0,1,0},{1,1,0}},  // S
            {{0,0,0},{1,0,0},{1,1,0},{2,1,0}},  // Z
            {{0,0,0},{1,0,0},{2,0,0},{0,1,0}},  // J
            {{0,0,0},{1,0,0},{2,0,0},{2,1,0}},  // L
        };
        for (int t = 0; t < NUM_PIECES; t++) {
            _norient[t] = 0;
            uint32_t keys[MAX_ORI];
            int nk = 0;
            int base[4][3];
            for (int i = 0; i < 4; i++) {
                base[i][0] = BASE[t][i][0]; base[i][1] = BASE[t][i][1]; base[i][2] = BASE[t][i][2];
            }
            normalize(base);
            addOrient(t, base, keys, nk);
            for (int qi = 0; qi < _norient[t]; qi++) {       // closure over the orientation list
                for (int r = 0; r < 3; r++) {
                    int s[4][3];
                    for (int i = 0; i < 4; i++) {
                        s[i][0] = _orient[t][qi].c[i][0];
                        s[i][1] = _orient[t][qi].c[i][1];
                        s[i][2] = _orient[t][qi].c[i][2];
                    }
                    for (int i = 0; i < 4; i++) {
                        if (r == 0) rotX(s[i]); else if (r == 1) rotY(s[i]); else rotZ(s[i]);
                    }
                    normalize(s);
                    addOrient(t, s, keys, nk);
                }
            }
        }
    }

    void addOrient(int t, const int s[4][3], uint32_t* keys, int& nk) {
        uint32_t k = shapeKey(s);
        for (int i = 0; i < nk; i++) if (keys[i] == k) return;
        if (_norient[t] >= MAX_ORI) return;
        keys[nk++] = k;
        Orient& o = _orient[t][_norient[t]];
        int ex = 0, ey = 0, ez = 0;
        for (int i = 0; i < 4; i++) {
            o.c[i][0] = (int8_t)s[i][0]; o.c[i][1] = (int8_t)s[i][1]; o.c[i][2] = (int8_t)s[i][2];
            if (s[i][0] > ex) ex = s[i][0];
            if (s[i][1] > ey) ey = s[i][1];
            if (s[i][2] > ez) ez = s[i][2];
        }
        o.ex = (uint8_t)ex; o.ey = (uint8_t)ey; o.ez = (uint8_t)ez;
        _norient[t]++;
    }

    // ── grid helpers ────────────────────────────────────────────────────
    void resetGrid() { memset(_cell, 0, sizeof(_cell)); }

    int colHeight(int x, int z) const {
        for (int y = CUBE_Y - 1; y >= 0; y--) if (_cell[x][y][z]) return y + 1;
        return 0;
    }
    static int iabs(int v) { return v < 0 ? -v : v; }

    void placeOrient(const Orient& o, int ox, int oz, int baseY, uint8_t color) {
        for (int i = 0; i < 4; i++) _cell[ox + o.c[i][0]][baseY + o.c[i][1]][oz + o.c[i][2]] = color;
    }
    void removeOrient(const Orient& o, int ox, int oz, int baseY) {
        for (int i = 0; i < 4; i++) _cell[ox + o.c[i][0]][baseY + o.c[i][1]][oz + o.c[i][2]] = 0;
    }

    // board quality metrics for the placement heuristic
    struct Metrics { int cleared, holes, agg, bump, maxh; };
    Metrics computeMetrics() const {
        int h[CUBE_X][CUBE_Z];
        Metrics m; m.cleared = 0; m.holes = 0; m.agg = 0; m.bump = 0; m.maxh = 0;
        for (int x = 0; x < CUBE_X; x++) {
            for (int z = 0; z < CUBE_Z; z++) {
                int top = -1;
                for (int y = CUBE_Y - 1; y >= 0; y--) if (_cell[x][y][z]) { top = y; break; }
                int ch = top + 1;
                h[x][z] = ch; m.agg += ch; if (ch > m.maxh) m.maxh = ch;
                for (int y = 0; y < top; y++) if (!_cell[x][y][z]) m.holes++;   // buried empties
            }
        }
        for (int x = 0; x < CUBE_X; x++) {
            for (int z = 0; z < CUBE_Z; z++) {
                if (x + 1 < CUBE_X) m.bump += iabs(h[x][z] - h[x + 1][z]);
                if (z + 1 < CUBE_Z) m.bump += iabs(h[x][z] - h[x][z + 1]);
            }
        }
        for (int y = 0; y < CUBE_Y; y++) {
            bool full = true;
            for (int x = 0; x < CUBE_X && full; x++)
                for (int z = 0; z < CUBE_Z && full; z++)
                    if (!_cell[x][y][z]) full = false;
            if (full) m.cleared++;
        }
        return m;
    }

    int nextPieceType() {
        if (_bagPos >= NUM_PIECES) {
            for (int i = 0; i < NUM_PIECES; i++) _bag[i] = (uint8_t)i;
            for (int i = NUM_PIECES - 1; i > 0; i--) {
                int j = random(0, i + 1);
                uint8_t tmp = _bag[i]; _bag[i] = _bag[j]; _bag[j] = tmp;
            }
            _bagPos = 0;
        }
        return _bag[_bagPos++];
    }

    // ── phases ──────────────────────────────────────────────────────────

    // Pick the next piece and the best legal landing for it (greedy heuristic).
    void stepSpawn() {
        int type = nextPieceType();
        long best = 0; bool found = false;
        int bO = 0, bX = 0, bZ = 0, bY = 0;

        for (int oi = 0; oi < _norient[type]; oi++) {
            const Orient& o = _orient[type][oi];
            for (int ox = 0; ox + o.ex < CUBE_X; ox++) {
                for (int oz = 0; oz + o.ez < CUBE_Z; oz++) {
                    // drop: rest when the tightest cell touches its column surface
                    int need = -1000;
                    for (int i = 0; i < 4; i++) {
                        int H = colHeight(ox + o.c[i][0], oz + o.c[i][2]);
                        int v = H - o.c[i][1];
                        if (v > need) need = v;
                    }
                    int baseY = need < 0 ? 0 : need;
                    if (baseY + o.ey > CUBE_Y - 1) continue;   // piece would poke out the top

                    placeOrient(o, ox, oz, baseY, (uint8_t)(type + 1));
                    Metrics m = computeMetrics();
                    long score = 220L * m.cleared      // take real clears
                               -  45L * m.holes         // never bury holes (deadly)
                               -   5L * m.maxh          // keep the peak low (avoid top-out)
                               -   2L * m.bump          // keep the surface flat
                               -   1L * m.agg           // keep total height down
                               -   3L * baseY           // prefer lower landings
                               + random(0, 4);          // tiny organic tie-break
                    removeOrient(o, ox, oz, baseY);

                    if (!found || score > best) {
                        found = true; best = score; bO = oi; bX = ox; bZ = oz; bY = baseY;
                    }
                }
            }
        }

        if (!found) {                 // genuinely boxed in — a real top-out
            _phase = PH_GAMEOVER; _phaseT = 0;
            return;
        }
        _type = type; _oidx = bO; _ox = bX; _oz = bZ; _baseY = bY; _color = (uint8_t)(type + 1);
        _pieceY = (float)(CUBE_Y - 1 - _orient[type][bO].ey);   // enter at the top, fall to baseY
        if (_pieceY < (float)_baseY) _pieceY = (float)_baseY;
        _phase = PH_FALLING; _phaseT = 0;
    }

    void stepFalling() {
        _pieceY -= _fallPerFrame;
        if (_pieceY <= (float)_baseY) {
            _pieceY = (float)_baseY;
            placeOrient(_orient[_type][_oidx], _ox, _oz, _baseY, _color);
            _phase = PH_LOCKED; _phaseT = 0;
        }
    }

    void stepLocked() {
        _phaseT += _frameMs;
        if (_phaseT >= LOCK_MS) {
            detectClears();
            _phase = (_clearCount > 0) ? PH_FLASH : PH_SPAWN;
            _phaseT = 0;
        }
    }
    void stepFlash() {
        _phaseT += _frameMs;
        if (_phaseT >= FLASH_MS) { buildCollapse(); _phase = PH_COLLAPSE; _phaseT = 0; }
    }
    void stepCollapse() {
        _phaseT += _frameMs;
        if (_phaseT >= COLLAPSE_MS) {
            resetGrid();
            for (int i = 0; i < _mcCount; i++) _cell[_mc[i].x][_mc[i].toY][_mc[i].z] = _mc[i].color;
            _phase = PH_SPAWN; _phaseT = 0;
        }
    }
    void stepGameOver() {
        _phaseT += _frameMs;
        if (_phaseT >= GAMEOVER_MS) { resetGrid(); _phase = PH_SPAWN; _phaseT = 0; }
    }

    void detectClears() {
        _clearCount = 0;
        for (int y = 0; y < CUBE_Y; y++) {
            bool full = true;
            for (int x = 0; x < CUBE_X && full; x++)
                for (int z = 0; z < CUBE_Z && full; z++)
                    if (!_cell[x][y][z]) full = false;
            _clearPlane[y] = full;
            if (full) _clearCount++;
        }
    }
    void buildCollapse() {
        _mcCount = 0;
        for (int x = 0; x < CUBE_X; x++) {
            for (int z = 0; z < CUBE_Z; z++) {
                for (int y = 0; y < CUBE_Y; y++) {
                    if (!_cell[x][y][z] || _clearPlane[y]) continue;
                    int drop = 0;
                    for (int k = 0; k < y; k++) if (_clearPlane[k]) drop++;
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

    // ── rendering ───────────────────────────────────────────────────────
    static float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
    static float smoothstep(float p) { return p * p * (3.0f - 2.0f * p); }

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
    static uint32_t lerpColor(uint32_t a, uint32_t b, float t) {
        t = clamp01(t);
        float ar = (a >> 16) & 0xFF, ag = (a >> 8) & 0xFF, ab = a & 0xFF;
        float br = (b >> 16) & 0xFF, bg = (b >> 8) & 0xFF, bb = b & 0xFF;
        return Cube::colorRGB((uint8_t)(ar + (br - ar) * t),
                              (uint8_t)(ag + (bg - ag) * t),
                              (uint8_t)(ab + (bb - ab) * t));
    }
    static uint32_t whiteBlend(uint32_t c, float a) {
        return lerpColor(c, 0xFFFFFFu, a);
    }
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
                drawAA(cube, _mc[i].x, yf, _mc[i].z,
                       scaleColor(pieceColor(_mc[i].color), shimmerFactor(_mc[i].x, _mc[i].toY, _mc[i].z)));
            }
            return;
        }

        if (_phase == PH_GAMEOVER) {
            // flash toward red, then fade the whole stack to black — a clean defeat
            float p = clamp01((float)_phaseT / (float)GAMEOVER_MS);
            float toRed = clamp01(p * 2.0f);
            float bright = 1.0f - p;
            for (int x = 0; x < CUBE_X; x++)
                for (int y = 0; y < CUBE_Y; y++)
                    for (int z = 0; z < CUBE_Z; z++) {
                        if (!_cell[x][y][z]) continue;
                        cube.setPixel(x, y, z,
                            scaleColor(lerpColor(pieceColor(_cell[x][y][z]), 0xFF0000u, toRed), bright));
                    }
            return;
        }

        // settled stack with the shimmer wave (skip cells flashing white)
        for (int x = 0; x < CUBE_X; x++)
            for (int y = 0; y < CUBE_Y; y++)
                for (int z = 0; z < CUBE_Z; z++) {
                    if (!_cell[x][y][z]) continue;
                    if (_phase == PH_FLASH && _clearPlane[y]) continue;
                    cube.setPixel(x, y, z, scaleColor(pieceColor(_cell[x][y][z]), shimmerFactor(x, y, z)));
                }

        if (_phase == PH_FLASH) {
            float pulse = 0.55f + 0.45f * sinf(clamp01((float)_phaseT / (float)FLASH_MS) * 3.14159f);
            uint8_t w = (uint8_t)(255.0f * clamp01(pulse));
            uint32_t white = Cube::colorRGB(w, w, w);
            for (int y = 0; y < CUBE_Y; y++) {
                if (!_clearPlane[y]) continue;
                for (int x = 0; x < CUBE_X; x++)
                    for (int z = 0; z < CUBE_Z; z++)
                        cube.setPixel(x, y, z, white);
            }
        }

        if (_phase == PH_FALLING) {
            const Orient& o = _orient[_type][_oidx];
            uint32_t col = pieceColor(_color);
            for (int i = 0; i < 4; i++)
                drawAA(cube, _ox + o.c[i][0], _pieceY + (float)o.c[i][1], _oz + o.c[i][2], col);
        } else if (_phase == PH_LOCKED) {
            float pop = 1.0f - clamp01((float)_phaseT / (float)LOCK_MS);
            uint32_t col = whiteBlend(pieceColor(_color), pop * 0.8f);
            const Orient& o = _orient[_type][_oidx];
            for (int i = 0; i < 4; i++)
                cube.setPixel(_ox + o.c[i][0], _baseY + o.c[i][1], _oz + o.c[i][2], col);
        }
    }

    // ── tuning ──────────────────────────────────────────────────────────
    float    _fallPerFrame;
    uint32_t _frameMs;
    float    _shimmerSpeed;
    uint32_t _lastMs;
    float    _shimmer;

    // ── state ───────────────────────────────────────────────────────────
    uint8_t  _cell[CUBE_X][CUBE_Y][CUBE_Z];      // 0 = empty, else colour-id 1..7
    Orient   _orient[NUM_PIECES][MAX_ORI];       // precomputed 3D rotations
    uint8_t  _norient[NUM_PIECES];

    uint8_t  _bag[NUM_PIECES];
    int      _bagPos = NUM_PIECES;

    int      _type = 0, _oidx = 0, _ox = 0, _oz = 0, _baseY = 0;
    uint8_t  _color = 1;
    float    _pieceY = 0.0f;

    Phase    _phase  = PH_SPAWN;
    uint32_t _phaseT = 0;

    bool     _clearPlane[CUBE_Y];
    int      _clearCount = 0;

    struct MovingCell { uint8_t x, z, fromY, toY, color; };
    MovingCell _mc[CUBE_LEDS];
    int        _mcCount = 0;
};
