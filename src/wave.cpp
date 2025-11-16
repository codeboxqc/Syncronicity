#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
 
#include "pio.h"
#include "gfx.h"
#include "FFT.h"
#include "palette.h"
#include "scene.h"
#include "wave.h"


 
#include "gfx.h"
#include "FFT.h"
#include "palette.h"
#include "math.h"
 
#define  PANEL_RES_X WIDTH 
#define  PANEL_RES_Y HEIGHT

 

///////////////////////////////////////////////////////////////
float catmullRom(float v0, float v1, float v2, float v3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;

    return 0.5f * (
        (2.0f * v1) +
        (-v0 + v2) * t +
        (2.0f * v0 - 5.0f * v1 + 4.0f * v2 - v3) * t2 +
        (-v0 + 3.0f * v1 - 3.0f * v2 + v3) * t3
    );
}

// ---------------------------------------------------
// 1. Extract RGB565 → 8-bit
// ---------------------------------------------------
inline void rgb565To888(uint16_t c, uint8_t &r, uint8_t &g, uint8_t &b) {
    r = ((c >> 11) & 0x1F) << 3;
    g = ((c >>  5) & 0x3F) << 2;
    b =  (c        & 0x1F) << 3;
}

// ---------------------------------------------------
// 2. Euclidean distance squared (fast)
// ---------------------------------------------------
inline uint32_t colorDiffSq(uint8_t r1,uint8_t g1,uint8_t b1,
                           uint8_t r2,uint8_t g2,uint8_t b2) {
    int dr = (int)r1-r2, dg = (int)g1-g2, db = (int)b1-b2;
    return dr*dr + dg*dg + db*db;
}

// distance² (fast)
inline uint32_t diffSq(uint8_t a1,uint8_t b1,uint8_t c1,
                       uint8_t a2,uint8_t b2,uint8_t c2) {
    int da=a1-a2, db=b1-b2, dc=c1-c2;
    return da*da + db*db + dc*dc;
}
/////////////////////////////////////////////////////////////////


 
 

 
//////////////////////////////////////////////////////////////////////////////////////
void drawSmoothLine(int x0,int y0,int x1,int y1,
                    uint8_t r,uint8_t g,uint8_t b)   // beat colour
{
    int dx = abs(x1-x0), sx = x0<x1?1:-1;
    int dy = abs(y1-y0), sy = y0<y1?1:-1;
    int err = dx-dy;

    while (true) {
        if (x0>=0 && x0<PANEL_RES_X && y0>=0 && y0<PANEL_RES_Y) {

            // ---- 1. read background ----
            uint16_t bg565 = GFX::getPixel(x0, y0);
            uint8_t bgR, bgG, bgB;
            rgb565To888(bg565, bgR, bgG, bgB);

            // ---- 2. contrast check ----
            const uint32_t THRESH = 11000;               // tweak 8000-15000
            bool low = diffSq(r,g,b, bgR,bgG,bgB) < THRESH;

            // ---- 3. darken background ONLY if needed ----
            if (low) {
                uint8_t darkR = bgR * 25 / 100;          // 25% brightness
                uint8_t darkG = bgG * 25 / 100;
                uint8_t darkB = bgB * 25 / 100;
                 GFX::setPixel(x0, y0, rgbToColor(darkR,darkG,darkB));
            }

            // ---- 4. draw bright line (always) ----
            GFX::setPixel(x0, y0, rgbToColor(r,g,b));
             
        }

        if (x0==x1 && y0==y1) break;
        int e2 = 2*err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}



void linewave() {
    
    float t = millis() * 0.001f;

    const int centerY = PANEL_RES_Y / 2;
    const float scaleY = (PANEL_RES_Y / 2.0f) * 0.88f;

    // === 1. DC Removal + Heavy Smoothing ===
    static float smoothed[SAMPLES] = {0};
    static bool firstRun = true;

    if (firstRun) {
        memcpy(smoothed, vRealOriginal, sizeof(smoothed));
        firstRun = false;
    }

    float dc = 0;
    for (int i = 0; i < SAMPLES; i++) dc += vRealOriginal[i];
    dc /= SAMPLES;

    for (int i = 0; i < SAMPLES; i++) {
        float raw = vRealOriginal[i] - dc;
        raw = constrain(raw, -26000, 26000);
        smoothed[i] = smoothed[i] * 0.85f + raw * 0.15f;  // Strong smoothing
    }

    // === 2. OVERSAMPLE: Draw 4x more points ===
    const int steps = PANEL_RES_X * 4;  // 4x resolution
    int prevX = 0, prevY = centerY;
    bool first = true;

    for (int s = 0; s <= steps; s++) {
        float fx = (float)s / steps;  // 0.0 to 1.0
        float sampleIdx = fx * (SAMPLES - 1);

        // === CUBIC INTERPOLATION (4-point) ===
        int i = (int)sampleIdx;
        float frac = sampleIdx - i;

        // Clamp indices
        int i0 = max(i - 1, 0);
        int i1 = i;
        int i2 = min(i + 1, SAMPLES - 1);
        int i3 = min(i + 2, SAMPLES - 1);

        float v0 = smoothed[i0] / 32768.0f;
        float v1 = smoothed[i1] / 32768.0f;
        float v2 = smoothed[i2] / 32768.0f;
        float v3 = smoothed[i3] / 32768.0f;

        // Catmull-Rom spline (smooth!)
        float yNorm = catmullRom(v0, v1, v2, v3, frac);

        int y = centerY + (int)(yNorm * scaleY);
        y = constrain(y, 1, PANEL_RES_Y - 2);

        int x = (int)(fx * (PANEL_RES_X - 1));

        // === Color ===
        Wave w = WaveData(x, t);
        uint8_t r = constrain(w.r * 1.7f, 0, 255);
        uint8_t g = constrain(w.g * 1.7f, 0, 255);
        uint8_t b = constrain(w.b * 1.7f, 0, 255);

        // === Draw anti-aliased line ===
        if (!first) {
            drawSmoothLine(prevX, prevY, x, y, r, g, b);
        }
        first = false;
        prevX = x;
        prevY = y;
    }

 
}
//////////////////////////////////////////////////////////////////////////////////////






///////////////////////////////////////////////

void drawLine(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        if (x0 >= 0 && x0 < PANEL_RES_X && y0 >= 0 && y0 < PANEL_RES_Y) {
            GFX::setPixel(x0, y0, rgbToColor(r, g, b));
             
        }
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

 void shortwave() {
    float t = millis() * 0.001f;                     // time for colour
    float rot = millis() * 0.0002f;                  // **slow rotation** (360° in ~31 s)

    const int centerX = PANEL_RES_X / 2;
    const int centerY = PANEL_RES_Y / 2;
    const int lineLen = 32;                          // **32 pixels long**
    const float halfLen = lineLen / 2.0f;

    // Pre-smooth + DC removal (same as before)
    static float smoothed[SAMPLES] = {0};
    static bool firstRun = true;
    if (firstRun) { memcpy(smoothed, vRealOriginal, sizeof(smoothed)); firstRun = false; }

    float dc = 0;
    for (int i = 0; i < SAMPLES; i++) dc += vRealOriginal[i];
    dc /= SAMPLES;

    for (int i = 0; i < SAMPLES; i++) {
        float raw = vRealOriginal[i] - dc;
         raw = constrain(raw, -28000, 28000);
        smoothed[i] = smoothed[i] * 0.75f + raw * 0.25f;
    }

    // === Build the 32-pixel waveform (centered) ===
    int waveY[32];
    uint8_t waveR[32], waveG[32], waveB[32];

    for (int i = 0; i < lineLen; i++) {
        // Map i (0..31) → sample index (centered in buffer)
        float norm = (i - halfLen + 0.5f) / halfLen;   // -1.0 … +1.0
        int sampleIdx = (int)((norm + 1.0f) * 0.5f * (SAMPLES - 1));
        sampleIdx = constrain(sampleIdx, 0, SAMPLES - 1);

        float sample = smoothed[sampleIdx] / 32768.0f;
        waveY[i] = centerY + (int)(sample * (PANEL_RES_Y / 2.0f) * 0.85f);
        waveY[i] = constrain(waveY[i], 1, PANEL_RES_Y - 2);

        // Colour from WaveData (use center X for beat sync)
        Wave w = WaveData(centerX, t);
        if(random(0,2)==1) {
        waveR[i] = constrain(w.r * 1.6f, 1, 250);
        waveG[i] = constrain(w.g * 1.6f, 1, 250);
        waveB[i] = constrain(w.b * 1.6f, 1, 250);
        }
        else {
        waveR[i] = constrain(w.r * 1.6f, 0, 255);
        waveG[i] = constrain(w.g * 1.6f, 0, 255);
        waveB[i] = constrain(w.b * 1.6f, 0, 255);
        }
    }

    // === Rotate and draw the 32-pixel line ===
    float cx = cos(rot);
    float sy = sin(rot);

    int prevX = 0, prevY = 0;
    bool first = true;

    for (int i = 0; i < lineLen; i++) {
        // Local offset from center: -16 … +15
        float localX = i - halfLen + 0.5f;
        float localY = waveY[i] - centerY;

        // Rotate
        int screenX = centerX + (int)(localX * cx - localY * sy);
        int screenY = centerY + (int)(localX * sy + localY * cx);

        if (screenX >= 0 && screenX < PANEL_RES_X && screenY >= 0 && screenY < PANEL_RES_Y) {
            if (!first) {
                drawLine(prevX, prevY, screenX, screenY,
                         waveR[i], waveG[i], waveB[i]);
            }
            first = false;
            prevX = screenX;
            prevY = screenY;
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////
 


 void barwave() {
    float t = millis() * 0.001f;  // Time for color animation

    const int centerY = PANEL_RES_Y / 2;  // Exact middle of panel
    const float maxHeight = (PANEL_RES_Y / 2.0f) * 0.80f;  // Use 80% of height for bars (up/down)

    // -------------------------------------------------
    // 1. DC removal + heavy smoothing + global peak hold
    // -------------------------------------------------
    static float smoothed[SAMPLES] = {0};  // Persistent smoothing buffer
    static float globalPeak = 0.0f;        // Global peak for beat emphasis
    static bool firstRun = true;

    if (firstRun) {
        memcpy(smoothed, vRealOriginal, sizeof(smoothed));
        firstRun = false;
    }

    // Compute DC offset (average)
    float dc = 0.0f;
    for (int i = 0; i < SAMPLES; i++) {
        dc += vRealOriginal[i];
    }
    dc /= SAMPLES;

    // Process each sample: clip, smooth, track max abs
    float maxAbs = 0.0f;
    for (int i = 0; i < SAMPLES; i++) {
        float raw = vRealOriginal[i] - dc;
        raw = constrain(raw, -28000.0f, 28000.0f);  // Prevent spikes

        // Heavy low-pass filter for smooth motion
        smoothed[i] = smoothed[i] * 0.94f + raw * 0.06f;

        float absVal = fabs(smoothed[i]);
        if (absVal > maxAbs) maxAbs = absVal;
    }

    // Update global peak with fast rise, slow decay (beat "hold")
    if (maxAbs > globalPeak) {
        globalPeak = maxAbs;  // Quick response to beat
    } else {
        globalPeak *= 0.96f;  // Slower decay (~1 sec)
    }

    // -------------------------------------------------
    // 2. Draw symmetric bars from center (Winamp-style)
    // -------------------------------------------------
    for (int x = 0; x < PANEL_RES_X; x++) {
        // Map x to smoothed sample
        int idx = (int)((float)x / (PANEL_RES_X - 1) * (SAMPLES - 1));
        idx = constrain(idx, 0, SAMPLES - 1);

        // Get absolute height (symmetric up/down)
        float norm = fabs(smoothed[idx]) / 32768.0f;

        // Add subtle beat boost from global peak
        float boost = globalPeak / 32768.0f;
        norm = norm * 0.75f + boost * 0.25f;  // Balanced reaction
        norm = min(norm, 1.0f);                // Clamp to max

        int barHeight = (int)(norm * maxHeight);

        // Compute top/bottom Y positions
        int yTop = centerY - barHeight;
        int yBot = centerY + barHeight;

        yTop = max(yTop, 0);
        yBot = min(yBot, PANEL_RES_Y - 1);

        // Get beat-reactive color
        Wave w = WaveData(x, t);
        uint8_t r = constrain(w.r * 1.6f, 0, 255);
        uint8_t g = constrain(w.g * 1.6f, 0, 255);
        uint8_t b = constrain(w.b * 1.6f, 0, 255);

        // Draw the bar (solid fill from center)
        for (int y = yTop; y <= yBot; y++) {
            GFX::setPixel(x, y, rgbToColor(r, g, b));
        }

        // Soft glow at ends (subtle, not overwhelming)
        float glow = 0.3f;
        if (yTop > 0) GFX::setPixel(x, yTop - 1, rgbToColor(r * glow, g * glow, b * glow));
        if (yBot < PANEL_RES_Y - 1) GFX::setPixel(x, yBot + 1, rgbToColor(r * glow, g * glow, b * glow));
    }
}


void oldbar() {
    float t = millis() * 0.01f;

    const int bottomY = PANEL_RES_Y - 1;           // Bottom of panel
    const float maxBarHeight = PANEL_RES_Y * 1.3f; // Max 90% height

    // -------------------------------------------------
    // 1. DC removal + smoothing + per-bar peak tracking
    // -------------------------------------------------
    static float smoothed[SAMPLES] = {0};
    static float barPeak[PANEL_RES_X] = {0};       // One peak per bar
    static bool firstRun = true;

    if (firstRun) {
        memcpy(smoothed, vRealOriginal, sizeof(smoothed));
        firstRun = false;
    }

    // DC offset
    float dc = 0;
    for (int i = 0; i < SAMPLES; i++) dc += vRealOriginal[i];
    dc /= SAMPLES;

    // Process samples
    for (int i = 0; i < SAMPLES; i++) {
        float raw = vRealOriginal[i] - dc;
        raw = constrain(raw, -28000.0f, 28000.0f);

        smoothed[i] = smoothed[i] * 0.90f + raw * 0.10f;  // Smooth
    }

    // -------------------------------------------------
    // 2. Update per-bar height with fast rise, slow fall
    // -------------------------------------------------
    for (int x = 0; x < PANEL_RES_X; x++) {
        int idx = (int)((float)x / (PANEL_RES_X - 1) * (SAMPLES - 1));
        idx = constrain(idx, 0, SAMPLES - 1);

        float sample = fabs(smoothed[idx]);
        float level = sample / 32768.0f;
        level = min(level, 1.0f);

        // Fast rise, slow fall (degrade)
        if (level > barPeak[x]) {
            barPeak[x] = level;           // Instant jump on beat
        } else {
            barPeak[x] *= 0.93f;          // Slow fall (~1.2 sec)
            if (barPeak[x] < 0.01f) barPeak[x] = 0;
        }

        int barHeight = (int)(barPeak[x] * maxBarHeight);
        int yTop = bottomY - barHeight;
        yTop = max(yTop, 0);

        // -------------------------------------------------
        // 3. Unique color per bar (rainbow gradient)
        // -------------------------------------------------
        float hue = (float)x / PANEL_RES_X;  // 0.0 to 1.0
        uint8_t r, g, b;
        hsvToRgb888(hue, 1.0f, 1.0f, r, g, b);  // Full saturation & value

        // Optional: dim slightly for glow
        uint8_t rGlow = r * 0.3f;
        uint8_t gGlow = g * 0.3f;
        uint8_t bGlow = b * 0.3f;

        // -------------------------------------------------
        // 4. Draw bar from bottom up
        // -------------------------------------------------
        for (int y = bottomY; y >= yTop; y--) {
            GFX::setPixel(x, y, rgbToColor(r, g, b));
        }

        // Soft glow at top of bar
        if (yTop > 0) {
            GFX::setPixel(x, yTop - 1, rgbToColor(rGlow, gGlow, bGlow));
        }
    }
}



void fireBars() {
    float t = millis() * 0.001f;
    const int bottom = PANEL_RES_Y - 1;
    static float height[PANEL_RES_X] = {0};

    float dc = 0; for (int i = 0; i < SAMPLES; i++) dc += vRealOriginal[i]; dc /= SAMPLES;
    for (int x = 0; x < PANEL_RES_X; x++) {
        int i = x * SAMPLES / PANEL_RES_X;
        float raw = constrain(vRealOriginal[i] - dc, -28000, 28000);
        float level = fabs(raw) / 32768.0f;
        height[x] += (level * 0.9f - height[x]) * 0.12f;
    }

    int scan = (int)(t * 30) % PANEL_RES_Y;

    for (int x = 0; x < PANEL_RES_X; x++) {
        int h = (int)(height[x] * (PANEL_RES_Y * 0.85f));
        int y0 = bottom - h;

        for (int y = bottom; y >= max(y0, 0); y--) {
            float depth = (float)(bottom - y) / h;
            uint8_t b = 200 + (uint8_t)(55 * sin(depth * 10));
            uint8_t g = 100;
            uint8_t r = 50;

            if (abs(y - scan) < 2) { r = g = b = 255; }  // Scanline

            GFX::setPixel(x, y, rgbToColor(r, g, b));
        }
    }
}

void liquidBars() {
    float t = millis() * 0.001f;
    const int bottom = PANEL_RES_Y - 1;

    static float height[PANEL_RES_X] = {0};

    float dc = 0; for (int i = 0; i < SAMPLES; i++) dc += vRealOriginal[i]; dc /= SAMPLES;
    for (int x = 0; x < PANEL_RES_X; x++) {
        int i = x * SAMPLES / PANEL_RES_X;
        float raw = constrain(vRealOriginal[i] - dc, -28000, 28000);
        float target = fabs(raw) / 32768.0f * (PANEL_RES_Y * 0.8f);
        height[x] += (target - height[x]) * 0.15f;  // Liquid easing
    }

    for (int x = 0; x < PANEL_RES_X; x++) {
        int h = (int)height[x];
        int y0 = bottom - h;

        // Chrome gradient
        float shine = sin(t * 3 + x * 0.2f) * 0.5f + 0.5f;
        uint8_t base = 180 + (uint8_t)(75 * shine);
        uint8_t r = base, g = base, b = base + 40;

        for (int y = bottom; y >= max(y0, 0); y--) {
            float norm = (float)(bottom - y) / h;
            uint8_t brightness = 200 + (uint8_t)(55 * sin(norm * 3.14f));
            GFX::setPixel(x, y, rgbToColor(brightness, brightness * 0.9f, brightness * 0.7f));
        }
        if (y0 > 0) GFX::setPixel(x, y0-1, rgbToColor(255, 255, 200));
    }
}


void orbitRings() {
    float t = millis() * 0.001f;
    const int cx = PANEL_RES_X / 2;
    const int cy = PANEL_RES_Y / 2;
    const int maxR = min(cx, cy) - 2;

    static float ringLevel[8] = {0};  // 8 rings
    float dc = 0; for (int i=0;i<SAMPLES;i++) dc += vRealOriginal[i]; dc /= SAMPLES;

    for (int i=0; i<8; i++) {
        int idx = i * SAMPLES / 8;
        float raw = fabs(vRealOriginal[idx] - dc);
        float level = raw / 32768.0f;
        if (level > ringLevel[i]) ringLevel[i] = level;
        else ringLevel[i] *= 0.96f;
    }

    for (int r=1; r<=maxR; r++) {
        int ring = r * 8 / maxR;
        float intensity = ringLevel[ring] * 1.5f;
        if (intensity < 0.1f) continue;

        float hue = t * 0.3f + ring * 0.1f;
        uint8_t rr, gg, bb; hsvToRgb888(hue, 1.0f, intensity, rr, gg, bb);

        // Draw circle
        for (int a=0; a<360; a+=8) {
            float rad = a * 3.14159f / 180.0f;
            int x = cx + (int)(r * cos(rad));
            int y = cy + (int)(r * sin(rad));
            if (x>=0 && x<PANEL_RES_X && y>=0 && y<PANEL_RES_Y) {

             if(random(0,3)==1)   GFX::setPixel(x, y, rgbToColor(rr, gg, bb));
            }
        }
    }
}


void eqArc() {
    float t = millis() * 0.001f;
    int cx = PANEL_RES_X/2, cy = PANEL_RES_Y;
    int radius = PANEL_RES_Y * 0.7f;

    static float bar[16] = {0};
    float dc = 0; for (int i=0;i<SAMPLES;i++) dc += vRealOriginal[i]; dc /= SAMPLES;

    for (int b=0; b<16; b++) {
        int start = b * SAMPLES / 16;
        int end = (b+1) * SAMPLES / 16;
        float sum = 0;
        for (int i=start; i<end; i++) sum += fabs(vRealOriginal[i] - dc);
        float level = sum / (end-start) / 32768.0f;
        if (level > bar[b]) bar[b] = level;
        else bar[b] *= 0.95f;
    }

    for (int b=0; b<16; b++) {
        float angle = (b / 16.0f) * 180.0f - 90.0f;
        float rad = angle * 3.14159f / 180.0f;
        int x0 = cx + (int)((radius - 10) * cos(rad));
        int y0 = cy + (int)((radius - 10) * sin(rad));
        int len = (int)(bar[b] * radius * 0.8f);

        for (int i=0; i<len; i++) {
            int x = cx + (int)((radius - 10 - i) * cos(rad));
            int y = cy + (int)((radius - 10 - i) * sin(rad));
            if (x>=0 && x<PANEL_RES_X && y>=0 && y<PANEL_RES_Y) {
                float hue = t * 0.2f + b * 0.06f;
                uint8_t rr, gg, bb; hsvToRgb888(hue, 1.0f, 1.0f, rr, gg, bb);
                GFX::setPixel(x, y, rgbToColor(rr, gg, bb));
            }
        }
    }
}


void starburst() {
    float t = millis() * 0.001f;
    int cx = PANEL_RES_X/2, cy = PANEL_RES_Y/2;

    float dc = 0; for (int i=0;i<SAMPLES;i++) dc += vRealOriginal[i]; dc /= SAMPLES;
    float kick = 0;
    for (int i=0;i<SAMPLES/4;i++) {
        float s = fabs(vRealOriginal[i] - dc) / 32768.0f;
        if (s > kick) kick = s;
    }

    static float burst = 0;
    if (kick > 0.6f) burst = 1.0f;
    else burst *= 0.92f;

    for (int a=0; a<360; a+=3) {
        float rad = a * 3.14159f / 180.0f;
        int len = (int)(burst * min(cx, cy) * 0.8f);
        float hue = t + a * 0.01f;
        uint8_t rr, gg, bb; hsvToRgb888(hue, 1.0f, 1.0f, rr, gg, bb);

        for (int d=0; d<len; d++) {
            int x = cx + (int)(d * cos(rad));
            int y = cy + (int)(d * sin(rad));
            if (x>=0 && x<PANEL_RES_X && y>=0 && y<PANEL_RES_Y) {
              if(random(0,5)==1)   GFX::setPixel(x, y, rgbToColor(rr, gg, bb));
            }
        }
    }
}


void dnaHelix() {
    float t = millis() * 0.001f;
    float dc = 0; for (int i=0;i<SAMPLES;i++) dc += vRealOriginal[i]; dc /= SAMPLES;

    for (int x=0; x<PANEL_RES_X; x++) {
        int i = x * SAMPLES / PANEL_RES_X;
        float level = fabs(vRealOriginal[i] - dc) / 32768.0f;
        int h = (int)(level * PANEL_RES_Y * 0.4f);

        float phase = t + x * 0.1f;
        int y1 = PANEL_RES_Y/2 - h/2 + (int)(sin(phase) * 8);
        int y2 = PANEL_RES_Y/2 - h/2 + (int)(sin(phase + 3.14f) * 8);

        uint8_t r1 = 100, g1 = 200, b1 = 255;
        uint8_t r2 = 255, g2 = 100, b2 = 100;

        for (int y=y1; y<y1+h; y++) if (y>=0 && y<PANEL_RES_Y) GFX::setPixel(x, y, rgbToColor(r1,g1,b1));
        for (int y=y2; y<y2+h; y++) if (y>=0 && y<PANEL_RES_Y) GFX::setPixel(x, y, rgbToColor(r2,g2,b2));
    }
}

void heartbeatLine() {
    float t = millis() * 0.001f;
    static float yPos[PANEL_RES_X] = {0};
    int cy = PANEL_RES_Y / 2;

    float dc = 0; for (int i=0;i<SAMPLES;i++) dc += vRealOriginal[i]; dc /= SAMPLES;
    for (int x=0; x<PANEL_RES_X; x++) {
        int i = x * SAMPLES / PANEL_RES_X;
        float raw = (vRealOriginal[i] - dc) / 32768.0f;
        float target = cy + raw * (PANEL_RES_Y * 0.4f);
        yPos[x] += (target - yPos[x]) * 0.3f;
    }

    bool flash = false;
    if (yPos[PANEL_RES_X/2] > cy + 20 && yPos[PANEL_RES_X/2-5] < cy) flash = true;

    for (int x=0; x<PANEL_RES_X-1; x++) {
        uint8_t r=255, g=50, b=50;
        if (flash && x > PANEL_RES_X/3 && x < PANEL_RES_X*2/3) { r=g=b=255; }
        drawLine(x, (int)yPos[x], x+1, (int)yPos[x+1], r, g, b);
    }
}

 

void spiralWave() {
    float t = millis() * 0.001f;
    float rot = t * 0.8f;
    float dc = 0; for (int i=0;i<SAMPLES;i++) dc += vRealOriginal[i]; dc /= SAMPLES;

    int cx = PANEL_RES_X / 2, cy = PANEL_RES_Y / 2;

    for (float a=0; a<12*3.14159f; a+=0.1f) {
        float r = a * 3.0f;
        float sampleIdx = (a / (12*3.14159f)) * SAMPLES;
        int idx = (int)sampleIdx % SAMPLES;
        float amp = fabs(vRealOriginal[idx] - dc) / 32768.0f;
        r += amp * 2;

        float x = cx + r * cos(a + rot);
        float y = cy + r * sin(a + rot);

        if (x>=0 && x<PANEL_RES_X && y>=0 && y<PANEL_RES_Y) {
            float hue = a * 0.1f + t * 0.5f;
            uint8_t rr, gg, bb; hsvToRgb888(hue, 1.0f, 1.0f, rr, gg, bb);
            GFX::setPixel((int)x, (int)y, rgbToColor(rr, gg, bb));
        }
    }
}


 
void orbitalBars() { 
 static float peak[PANEL_RES_X] = {0};
    static float smooth[SAMPLES] = {0};
    
    // Process audio with DC removal
    float dc = 0;
    for(int i = 0; i < SAMPLES; i++) dc += vRealOriginal[i];
    dc /= SAMPLES;
    
    for(int i = 0; i < SAMPLES; i++) {
        float raw = constrain(vRealOriginal[i] - dc, -28000, 28000);
        smooth[i] = smooth[i] * 0.85f + raw * 0.15f;
    }
    
    // Quantum-inspired bars with particle effects
    for(int x = 0; x < PANEL_RES_X; x++) {
        int idx = map(x, 0, PANEL_RES_X-1, 0, SAMPLES-1);
        float level = fabs(smooth[idx]) / 32768.0f;
        
        // Quantum tunneling effect - bars can jump higher occasionally
        if(random(100) < 3) level *= 1.8f;
        
        // Fast attack, slow decay
        peak[x] = (level > peak[x]) ? level : peak[x] * 0.92f;
        
        int height = peak[x] * PANEL_RES_Y * 1.4f;
        int topY = PANEL_RES_Y - height;
        
        // Holographic color with audio modulation
        float hue = (x * 0.03f + millis() * 0.001f);
        uint8_t r, g, b;
        hsvToRgb888(hue, 0.9f, 0.8f + peak[x] * 0.4f, r, g, b);
        
        // Draw shimmering bar
        for(int y = PANEL_RES_Y-1; y >= topY; y--) {
            float sparkle = (random(100) < peak[x] * 15) ? 1.3f : 1.0f;
            GFX::setPixel(x, y, rgbToColor(r * sparkle, g * sparkle, b * sparkle));
        }
        
        // Energy particles above bar
        if(topY > 2 && random(100) < peak[x] * 20) {
            GFX::setPixel(x, topY-1, rgbToColor(255, 255, 255));
        }
    }
}


/////////////////////////////////////////////////////////////
void dl1(int x1, int y1, int x2, int y2, 
             uint8_t r1, uint8_t g1, uint8_t b1,
             uint8_t r2, uint8_t g2, uint8_t b2) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int steps = max(dx, dy);
    
    if (steps == 0) steps = 1;

    for (int i = 0; i <= steps; i++) {
        float ratio = (float)i / steps;
        int x = x1 + (x2 - x1) * ratio;
        int y = y1 + (y2 - y1) * ratio;
        
        uint8_t r = r1 + (r2 - r1) * ratio;
        uint8_t g = g1 + (g2 - g1) * ratio;
        uint8_t b = b1 + (b2 - b1) * ratio;

        // Center pixel - full brightness
        if (x >= 0 && x < PANEL_RES_X && y >= 0 && y < PANEL_RES_Y) {
            uint16_t existingColor = GFX::getPixel(x, y);
            uint8_t oldR = ((existingColor >> 11) & 0x1F) << 3;
            uint8_t oldG = ((existingColor >> 5) & 0x3F) << 2;
            uint8_t oldB = (existingColor & 0x1F) << 3;
            
            // Additive blending for bright overlaps
            uint8_t finalR = min(255, oldR + r);
            uint8_t finalG = min(255, oldG + g);
            uint8_t finalB = min(255, oldB + b);
            
            GFX::setPixel(x, y, rgbToColor(0, 0, 0));
        }

        // Glow pixels around center
        for (int gy_offset = -1; gy_offset <= 1; gy_offset++) {
            for (int gx_offset = -1; gx_offset <= 1; gx_offset++) {
                if (gx_offset == 0 && gy_offset == 0) continue; // Skip center (already drawn)
                
                int gx = x + gx_offset;
                int gy = y + gy_offset;
                
                if (gx >= 0 && gx < PANEL_RES_X && gy >= 0 && gy < PANEL_RES_Y) {

                    uint16_t existingColor = GFX::getPixel(gx, gy);
                    uint8_t oldR = ((existingColor >> 11) & 0x1F) << 3;
                    uint8_t oldG = ((existingColor >> 5) & 0x3F) << 2;
                    uint8_t oldB = (existingColor & 0x1F) << 3;
                    
                    // Glow is 50% brightness
                    float glowFade = 0.2;
                    uint8_t newR = r * glowFade;
                    uint8_t newG = g * glowFade;
                    uint8_t newB = b * glowFade;
                    
                    // Additive blending for glow
                    uint8_t finalR = min(252, oldR + newR);
                    uint8_t finalG = min(252, oldG + newG);
                    uint8_t finalB = min(252, oldB + newB);
                    
                    GFX::setPixel(gx, gy, rgbToColor(finalR , finalG , finalB ));
                }
            }
        }
    }
}

void dl2(int x1, int y1, int x2, int y2, 
             uint8_t r1, uint8_t g1, uint8_t b1,
             uint8_t r2, uint8_t g2, uint8_t b2) {

    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int steps = max(dx, dy);
    
    if (steps == 0) steps = 1; // Prevent division by zero

    for (int i = 0; i <= steps; i++) {
        float ratio = (float)i / steps;
        int x = x1 + (x2 - x1) * ratio;
        int y = y1 + (y2 - y1) * ratio;
        
        uint8_t r = r1 + (r2 - r1) * ratio;
        uint8_t g = g1 + (g2 - g1) * ratio;
        uint8_t b = b1 + (b2 - b1) * ratio;

          
       
                int px = x  ;
                int py = y  ;
                
                if (px >= 0 && px < PANEL_RES_X && py >= 0 && py < PANEL_RES_Y) {
                     
                uint16_t existingColor = GFX::getPixel(px, py);

                // Extract RGB565 components
                uint8_t r = (existingColor >> 11) & 0x1F;
                uint8_t g = (existingColor >> 5)  & 0x3F;
                uint8_t b = existingColor         & 0x1F;

                // Invert and scale (ensures visible contrast)
                r = 31 - r;  // 5-bit invert
                g = 63 - g;  // 6-bit invert
                b = 31 - b;  // 5-bit invert

                // Reassemble
                uint16_t contrastColor = (r << 11) | (g << 5) | b;
 
                GFX::setPixel(px, py, rgbToColor(r  , g  , b  ));
       
        }
    }
 
   }

   void CircularWave(int quel) {
 
    float t = millis() * 0.002;

    int cx = PANEL_RES_X / 2;
    int cy = PANEL_RES_Y / 2;
    float baseRadius = min(PANEL_RES_X, PANEL_RES_Y) * 0.3;

    for (int i = 0; i < SAMPLES - 1; i += 2) {
        float angle1 = TWO_PI * i / SAMPLES;
        float angle2 = TWO_PI * (i + 2) / SAMPLES;

        // Use calculateWaveData for everything
        int virtualX1 = (int)((float)i / SAMPLES * PANEL_RES_X);
        int virtualX2 = (int)((float)(i + 2) / SAMPLES * PANEL_RES_X);
        Wave data1 =  WaveData(virtualX1, t);
        Wave  data2 = WaveData(virtualX2, t);

        // Simpler radius using only wave data
        float radius1 = baseRadius + data1.amplitude * 3;//3 15.0;
        float radius2 = baseRadius + data2.amplitude * 3; //3 15.0;

        int x1 = cx + (int)(radius1 * cos(angle1));
        int y1 = cy + (int)(radius1 * sin(angle1));
        int x2 = cx + (int)(radius2 * cos(angle2));
        int y2 = cy + (int)(radius2 * sin(angle2));

         if(quel==0) dl1(x1, y1, x2, y2, data1.r, data1.g, data1.b, data2.r, data2.g, data2.b);
          else dl2(x1, y1, x2, y2, data1.r, data1.g, data1.b, data2.r, data2.g, data2.b);
    }

   
}
///////////////////////////////////////////////////////////
 


 




 