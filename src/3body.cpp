#include <Arduino.h>
#include <stdint.h>
#include <string.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <cmath>
#include <algorithm>
 
#include "pio.h"
#include "gfx.h"
#include "FFT.h"
#include "palette.h"
#include "scene.h"
#include "wave.h"
#include "3body.h"

#define NUM_BODIES 3
#define G  12.0f  //12.0f
#define dt 0.0008f  //0.0008f 
#define scale 18.0f
#define maxTrailLength 130
#define SIM_RANGE 5.2f

// === STRUCTS ===
struct Vec2 {
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float x_, float y_) : x(x_), y(y_) {}
    Vec2 operator+(const Vec2& v) const { return {x + v.x, y + v.y}; }
    Vec2 operator-(const Vec2& v) const { return {x - v.x, y - v.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    float mag() const { return sqrt(x * x + y * y); }
    Vec2 normalized() const { float m = mag(); return m > 0 ? *this * (1.0f / m) : Vec2{0, 0}; }
};

Vec2 lerpVec(const Vec2& a, const Vec2& b, float t) {
    return a * (1.0f - t) + b * t;
}

struct Body {
    Vec2 pos, vel, acc;
    float mass = 1.0f;
};

// === GLOBALS ===
std::vector<Body> bodies;
Vec2 trails[NUM_BODIES][maxTrailLength];
int trailSizes[NUM_BODIES] = {0};
int currentShape = 0;
int nextShape = 1;
float transition = 3.0f;
std::vector<Vec2> shapePos[30];
std::vector<Vec2> shapeVel[30];

// === TOROIDAL WRAP ===
float wrap(float p, float range) {
    float half = range * 0.5f;
    while (p <= -half) p += range;
    while (p >   half) p -= range;
    return p;
}

// === HSV TO RGB ===
void hsvToRgb(float h, float s, float v, uint8_t& r, uint8_t& g, uint8_t& b) {
    h = fmod(h, 360.0f);
    float c = v * s;
    float x = c * (1.0f - fabs(fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float rf, gf, bf;

    if (h < 60)      { rf = c; gf = x; bf = 0; }
    else if (h < 120) { rf = x; gf = c; bf = 0; }
    else if (h < 180) { rf = 0; gf = c; bf = x; }
    else if (h < 240) { rf = 0; gf = x; bf = c; }
    else if (h < 300) { rf = x; gf = 0; bf = c; }
    else              { rf = c; gf = 0; bf = x; }

    r = (rf + m) * 255;
    g = (gf + m) * 255;
    b = (bf + m) * 255;
}

///*
// === CALCULATE AUDIO AMPLITUDE ===
float getAudioAmplitude() {
    float sum = 0;
    // Sample across frequency spectrum
    for (int i = 0; i < SAMPLES; i += 4) {
        sum += fabs(vRealOriginal[i]);
    }
    float avg = sum / (SAMPLES / 4);
    // Normalize and scale
    float amplitude = avg / 32768.0f;
    return constrain(amplitude, 0.0f, 1.0f);
}
   // */

/*
// === GET HIGH FREQUENCY BEAT AMPLITUDE ===
float getHighBeatAmplitude() {
    // Focus on high frequencies for sharp, reactive movements
    float sum = 0;
    int highFreqStart = SAMPLES / 4;  // Skip low/mid frequencies
    int count = 0;
    
    // Sample high frequency range
    for (int i = highFreqStart; i < SAMPLES / 2; i += 2) {
        sum += fabs(vReal[i]);  // Use FFT data for frequency-specific response
        count++;
    }
    
    if (count == 0) return 0.0f;
    
    float avg = sum / count;
    // Normalize and amplify
    float amplitude = avg / 16384.0f;  // Lower divisor = more sensitive
    
    // Apply exponential curve for sharper response to peaks
    amplitude = powf(amplitude, 0.7f);  // Makes it more reactive
    
    return constrain(amplitude * 1.5f, 0.0f, 1.0f);  // Boost and cap
}

// === SMOOTH HIGH BEAT FOR NATURAL MOTION ===
float smoothedHighBeat = 0.0f;

float getAudioAmplitude() {
    float rawBeat = getHighBeatAmplitude();
    
    // Fast attack, slow decay for punchy response
    if (rawBeat > smoothedHighBeat) {
        smoothedHighBeat += (rawBeat - smoothedHighBeat) * 0.35f;  // Fast attack
    } else {
        smoothedHighBeat *= 0.88f;  // Slow decay
    }
    
    return smoothedHighBeat;
}

*/





////////////////////////////////////////*
// ===================================================================
// PIXEL INTERACTION SYSTEM - Pixels see and react to neighbors
// ===================================================================

enum InteractionMode {
    INTERACT_NONE = 0,       // No interaction (default)
    INTERACT_CONTRAST,       // Draw opposite colors for contrast
    INTERACT_BLEND,          // Blend with surroundings
    INTERACT_RIPPLE,         // Create ripple effect from neighbors
    INTERACT_RADIAL_GLOW,    // Radial color spread
    INTERACT_ENERGY_FLOW,    // Energy flows through neighboring pixels
    INTERACT_KALEIDOSCOPE    // Mirror and multiply patterns
};

InteractionMode currentInteraction = INTERACT_RADIAL_GLOW;


struct NeighborCache {
    uint8_t avgR, avgG, avgB;
    float energy;
    uint32_t lastUpdate;
};

// Cache grid (1/4 resolution for speed)
#define CACHE_W (WIDTH / 4)
#define CACHE_H (HEIGHT / 4)
NeighborCache pixelCache[CACHE_W * CACHE_H];
uint32_t cacheFrameCount = 0;

// Fast cache lookup
inline NeighborCache* getCachedNeighborhood(int x, int y) {
    int cx = (x / 4) % CACHE_W;
    int cy = (y / 4) % CACHE_H;
    return &pixelCache[cy * CACHE_W + cx];
}

// Update cache (call once per frame before drawing)
void updatePixelCache() {
    cacheFrameCount++;
    
    // Only update cache every 2nd frame for speed
    if (cacheFrameCount % 2 != 0) return;
    
    for (int cy = 0; cy < CACHE_H; cy++) {
        for (int cx = 0; cx < CACHE_W; cx++) {
            int x = cx * 4 + 2;  // Sample center of 4x4 block
            int y = cy * 4 + 2;
            
            if (x >= WIDTH || y >= HEIGHT) continue;
            
            // Fast 3x3 neighborhood sample
            uint32_t sumR = 0, sumG = 0, sumB = 0;
            int count = 0;
            
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;
                    
                    if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) continue;
                    
                    uint16_t color = GFX::getPixel(nx, ny);
                    sumR += ((color >> 11) & 0x1F) << 3;
                    sumG += ((color >> 5) & 0x3F) << 2;
                    sumB += (color & 0x1F) << 3;
                    count++;
                }
            }
            
            NeighborCache* cache = &pixelCache[cy * CACHE_W + cx];
            if (count > 0) {
                cache->avgR = sumR / count;
                cache->avgG = sumG / count;
                cache->avgB = sumB / count;
                cache->energy = (cache->avgR + cache->avgG + cache->avgB) / 3.0f / 255.0f;
            } else {
                cache->avgR = cache->avgG = cache->avgB = 0;
                cache->energy = 0;
            }
            cache->lastUpdate = cacheFrameCount;
        }
    }
}

// ===================================================================
// FAST INTERACTION MODES (using cache)
// ===================================================================

// FAST: Radial glow from cache
void applyRadialGlowCached(int x, int y, uint8_t& r, uint8_t& g, uint8_t& b) {
    NeighborCache* cache = getCachedNeighborhood(x, y);
    
    if (cache->energy > 0.15f) {
        // Blend with cached neighborhood
        float strength = cache->energy * 0.6f;
        r = min(255, (int)(r + cache->avgR * strength));
        g = min(255, (int)(g + cache->avgG * strength));
        b = min(255, (int)(b + cache->avgB * strength));
    }
}

// FAST: Energy flow from cache
void applyEnergyFlowCached(int x, int y, uint8_t& r, uint8_t& g, uint8_t& b, 
                           float audioAmp) {
    // Get cached neighborhood
    NeighborCache* cache = getCachedNeighborhood(x, y);
    
    if (cache->energy > 0.2f) {
        // Flow energy from cache to current pixel
        float flowStrength = 0.5f + audioAmp * 0.5f;
        r = min(255, (int)(cache->avgR * flowStrength + r * (1.0f - flowStrength)));
        g = min(255, (int)(cache->avgG * flowStrength + g * (1.0f - flowStrength)));
        b = min(255, (int)(cache->avgB * flowStrength + b * (1.0f - flowStrength)));
        
        // Audio boost
        float boost = 1.0f + audioAmp * 0.4f;
        r = min(255, (int)(r * boost));
        g = min(255, (int)(g * boost));
        b = min(255, (int)(b * boost));
    }
}

// FAST: Contrast from cache
void applyContrastCached(int x, int y, uint8_t& r, uint8_t& g, uint8_t& b) {
    NeighborCache* cache = getCachedNeighborhood(x, y);
    
    if (cache->energy > 0.1f) {
        // Invert cached colors for contrast
        r = 255 - cache->avgR;
        g = 255 - cache->avgG;
        b = 255 - cache->avgB;
        
        // Boost
        float boost = 1.4f;
        r = min(255, (int)(r * boost));
        g = min(255, (int)(g * boost));
        b = min(255, (int)(b * boost));
    }
}

// ===================================================================
// FAST PIXEL PROCESSOR (uses cache)
// ===================================================================
inline void processPixelInteractionFast(int x, int y, uint8_t& r, uint8_t& g, uint8_t& b,
                                        float audioAmp) {
    switch (currentInteraction) {
        case INTERACT_RADIAL_GLOW:
            applyRadialGlowCached(x, y, r, g, b);
            break;
            
        case INTERACT_ENERGY_FLOW:
            applyEnergyFlowCached(x, y, r, g, b, audioAmp);
            break;
            
        case INTERACT_CONTRAST:
            applyContrastCached(x, y, r, g, b);
            break;
            
        case INTERACT_BLEND: {
            // Simple 4-neighbor blend (no cache needed)
            uint16_t c1 = (x > 0) ? GFX::getPixel(x-1, y) : 0;
            uint16_t c2 = (x < WIDTH-1) ? GFX::getPixel(x+1, y) : 0;
            uint8_t r1 = ((c1 >> 11) & 0x1F) << 3;
            uint8_t r2 = ((c2 >> 11) & 0x1F) << 3;
            r = (r + r1 + r2) / 3;
            
            uint8_t g1 = ((c1 >> 5) & 0x3F) << 2;
            uint8_t g2 = ((c2 >> 5) & 0x3F) << 2;
            g = (g + g1 + g2) / 3;
            
            uint8_t b1 = (c1 & 0x1F) << 3;
            uint8_t b2 = (c2 & 0x1F) << 3;
            b = (b + b1 + b2) / 3;
            break;
        }
            
        case INTERACT_NONE:
        default:
            break;
    }
}

/*****************************************************************/















// === DEFINE 30 SHAPES ===
void defineShapes() {
    shapePos[0]  = { Vec2(-1, 0), Vec2(1, 0), Vec2(0, 0) };
    shapeVel[0]  = { Vec2(0, 0.5), Vec2(0, -0.5), Vec2(0.5, 0) };

    shapePos[1]  = { Vec2(-1, -0.577), Vec2(1, -0.577), Vec2(0, 1) };
    shapeVel[1]  = { Vec2(0, 0.5), Vec2(0, 0.5), Vec2(0, -1) };

    shapePos[2]  = { Vec2(-1.5, 0), Vec2(0, 0), Vec2(1.5, 0) };
    shapeVel[2]  = { Vec2(0, 0.4), Vec2(0, 0), Vec2(0, -0.4) };

    shapePos[3]  = { Vec2(-0.5, -0.289), Vec2(0.5, -0.289), Vec2(0, 0.577) };
    shapeVel[3]  = { Vec2(0, 0.3), Vec2(0, 0.3), Vec2(0, -0.6) };

    shapePos[4]  = { Vec2(-1.5, 0), Vec2(1.5, 0), Vec2(0, 0.5) };
    shapeVel[4]  = { Vec2(0, 0.6), Vec2(0, -0.6), Vec2(0.6, 0) };

    shapePos[5]  = { Vec2(-1, -0.3), Vec2(0.8, -0.2), Vec2(0, 0.8) };
    shapeVel[5]  = { Vec2(0, 0.4), Vec2(0, 0.4), Vec2(0, -0.8) };

    shapePos[6]  = { Vec2(-1.2, 0.2), Vec2(1.2, 0.2), Vec2(0, -0.4) };
    shapeVel[6]  = { Vec2(0, 0.5), Vec2(0, -0.5), Vec2(0.5, 0) };

    shapePos[7]  = { Vec2(-0.8, -0.4), Vec2(0.8, -0.4), Vec2(0, 0.8) };
    shapeVel[7]  = { Vec2(0, 0.5), Vec2(0, 0.5), Vec2(0, -1) };

    shapePos[8]  = { Vec2(0, -1), Vec2(0, 0), Vec2(0, 1) };
    shapeVel[8]  = { Vec2(0.4, 0), Vec2(0, 0), Vec2(-0.4, 0) };

    shapePos[9]  = { Vec2(-1.2, -0.7), Vec2(1.2, -0.7), Vec2(0, 1.4) };
    shapeVel[9]  = { Vec2(0, 0.6), Vec2(0, 0.6), Vec2(0, -1.2) };

    shapePos[10] = { Vec2(-1.1, 0.1), Vec2(1.1, 0.1), Vec2(0, -0.5) };
    shapeVel[10] = { Vec2(0.1, 0.5), Vec2(-0.1, -0.5), Vec2(0.4, 0) };

    shapePos[11] = { Vec2(-1.3, 0), Vec2(1.3, 0), Vec2(0, 0.3) };
    shapeVel[11] = { Vec2(0, 0.6), Vec2(0, -0.6), Vec2(0.5, 0) };

    shapePos[12] = { Vec2(-0.9, -0.5), Vec2(0.9, -0.5), Vec2(0, 1.0) };
    shapeVel[12] = { Vec2(0.2, 0.4), Vec2(-0.2, 0.4), Vec2(0, -0.8) };

    shapePos[13] = { Vec2(-1.0, -0.2), Vec2(1.0, -0.2), Vec2(0, 0.6) };
    shapeVel[13] = { Vec2(0, 0.3), Vec2(0, 0.3), Vec2(0, -0.6) };

    shapePos[14] = { Vec2(-1.4, 0.2), Vec2(1.4, 0.2), Vec2(0, -0.6) };
    shapeVel[14] = { Vec2(0.1, 0.5), Vec2(-0.1, -0.5), Vec2(0.6, 0) };

    shapePos[15] = { Vec2(-0.6, -0.3), Vec2(0.6, -0.3), Vec2(0, 0.7) };
    shapeVel[15] = { Vec2(0, 0.4), Vec2(0, 0.4), Vec2(0, -0.8) };

    shapePos[16] = { Vec2(-1.5, -0.1), Vec2(1.5, -0.1), Vec2(0, 0.4) };
    shapeVel[16] = { Vec2(0, 0.5), Vec2(0, -0.5), Vec2(0.5, 0) };

    shapePos[17] = { Vec2(-1.0, -0.6), Vec2(1.0, -0.6), Vec2(0, 1.2) };
    shapeVel[17] = { Vec2(0.3, 0.5), Vec2(-0.3, 0.5), Vec2(0, -1.0) };

    shapePos[18] = { Vec2(-1.2, -0.4), Vec2(1.2, -0.4), Vec2(0, 0.8) };
    shapeVel[18] = { Vec2(0, 0.4), Vec2(0, 0.4), Vec2(0, -0.8) };

    shapePos[19] = { Vec2(-1.3, 0.3), Vec2(1.3, 0.3), Vec2(0, -0.6) };
    shapeVel[19] = { Vec2(0.2, 0.4), Vec2(-0.2, -0.4), Vec2(0.5, 0) };

    shapePos[20] = { Vec2(-1.3, -0.8), Vec2(0.7, 0.9), Vec2(0.6, -0.4) };
    shapeVel[20] = { Vec2(0.3, 0.6), Vec2(-0.4, -0.3), Vec2(0.2, 0.5) };

    shapePos[21] = { Vec2(-1.6, 0.3), Vec2(1.4, -0.5), Vec2(0.2, 1.1) };
    shapeVel[21] = { Vec2(0.1, 0.7), Vec2(-0.3, 0.4), Vec2(0.4, -0.9) };

    shapePos[22] = { Vec2(-0.7, 1.2), Vec2(1.3, -0.3), Vec2(-0.4, -0.9) };
    shapeVel[22] = { Vec2(0.5, -0.2), Vec2(-0.6, 0.5), Vec2(0.3, 0.4) };

    shapePos[23] = { Vec2(-1.1, -1.0), Vec2(0.9, 0.8), Vec2(0.3, 0.2) };
    shapeVel[23] = { Vec2(0.6, 0.3), Vec2(-0.2, -0.6), Vec2(0.4, 0.5) };

    shapePos[24] = { Vec2(-0.7, -0.7), Vec2(0.7, -0.7), Vec2(0, 1.0) };
    shapeVel[24] = { Vec2(0.3, 0.5), Vec2(-0.3, 0.5), Vec2(0, -0.7) };

    shapePos[25] = { Vec2(-1.0, 0.3), Vec2(0.5, -0.9), Vec2(0.5, 0.6) };
    shapeVel[25] = { Vec2(0.4, 0.3), Vec2(-0.2, 0.6), Vec2(-0.3, -0.5) };

    shapePos[26] = { Vec2(-0.6, 0), Vec2(0.6, 0), Vec2(0, -0.8) };
    shapeVel[26] = { Vec2(0.2, 0.6), Vec2(-0.2, 0.6), Vec2(0, -0.8) };

    shapePos[27] = { Vec2(-1.2, 0.5), Vec2(0, -0.8), Vec2(1.2, 0.3) };
    shapeVel[27] = { Vec2(0.5, 0.2), Vec2(0, 0.7), Vec2(-0.5, 0.3) };

    shapePos[28] = { Vec2(-0.8, 0.6), Vec2(0.8, 0.6), Vec2(0, -0.9) };
    shapeVel[28] = { Vec2(0.3, -0.4), Vec2(-0.3, -0.4), Vec2(0, 0.8) };

    shapePos[29] = { Vec2(-0.9, 0), Vec2(0.45, 0.78), Vec2(0.45, -0.78) };
    shapeVel[29] = { Vec2(0, 0.6), Vec2(-0.52, -0.3), Vec2(0.52, -0.3) };

    // === INIT BODIES FROM SHAPE 0 ===
    bodies.clear();
    for (int i = 0; i < NUM_BODIES; i++) {
        Body b;
        b.pos = shapePos[0][i];
        b.vel = shapeVel[0][i];
        b.acc = Vec2(0, 0);
        b.mass = 1.0f;
        bodies.push_back(b);
    }
}

// === UPDATE PHYSICS (unchanged - same orbits) ===
void updateBodies() {
    if (currentShape >= 30 || nextShape >= 30) return;

    if (transition < 0.1f) {
        float t = transition / 0.1f;
        for (int i = 0; i < NUM_BODIES; i++) {
            bodies[i].pos = lerpVec(shapePos[currentShape][i], shapePos[nextShape][i], t);
            bodies[i].vel = lerpVec(shapeVel[currentShape][i], shapeVel[nextShape][i], t);
            bodies[i].pos.x = wrap(bodies[i].pos.x, SIM_RANGE);
            bodies[i].pos.y = wrap(bodies[i].pos.y, SIM_RANGE);
        }
    } else {
        // Gravity
        for (int i = 0; i < NUM_BODIES; i++) {
            bodies[i].acc = Vec2(0, 0);
            for (int j = 0; j < NUM_BODIES; j++) {
                if (i == j) continue;
                Vec2 r = bodies[j].pos - bodies[i].pos;
                r.x = wrap(r.x, SIM_RANGE);
                r.y = wrap(r.y, SIM_RANGE);
                float d = r.mag();
                if (d > 0.01f) {
                    float f = G / (d * d);
                    bodies[i].acc = bodies[i].acc + r.normalized() * f;
                }
            }
        }
        // Update
        for (int i = 0; i < NUM_BODIES; i++) {
            bodies[i].vel = bodies[i].vel + bodies[i].acc * dt;
            bodies[i].pos = bodies[i].pos + bodies[i].vel * dt;
            bodies[i].pos.x = wrap(bodies[i].pos.x, SIM_RANGE);
            bodies[i].pos.y = wrap(bodies[i].pos.y, SIM_RANGE);
        }
    }
}

// === DRAW WITH AUDIO REACTION - WAVE DISTORTION ===
/*

void drawOrbitalTrails() {
    float t = millis() * 0.002f;
    
    // Get audio amplitude for wave effect
    float audioAmp = getAudioAmplitude();

    for (int i = 0; i < NUM_BODIES; i++) {
        // Update trail
        if (trailSizes[i] < maxTrailLength) {
            trails[i][trailSizes[i]++] = bodies[i].pos;
        } else {
            memmove(trails[i], trails[i] + 1, (maxTrailLength - 1) * sizeof(Vec2));
            trails[i][maxTrailLength - 1] = bodies[i].pos;
        }

        // Draw trail with wave distortion
        for (int j = 0; j < trailSizes[i]; j++) {
            // Base position
            float baseX = trails[i][j].x * scale + WIDTH / 2;
            float baseY = trails[i][j].y * scale + HEIGHT / 2;
            
            // Apply wave distortion based on audio
            // Create travelling wave along the trail
            float wavePhase = t * 3.0f + j * 0.3f;
            float waveAmplitude = audioAmp * 2.5f; // Amplitude controlled by beat
            
            // Perpendicular wave offset (oscillates perpendicular to trail direction)
            float waveOffset = sin(wavePhase) * waveAmplitude;
            
            // Calculate perpendicular direction (approximate from trail direction)
            float dirX = 0, dirY = 0;
            if (j > 0 && j < trailSizes[i] - 1) {
                // Use neighbors to find trail direction
                dirX = trails[i][j + 1].x - trails[i][j - 1].x;
                dirY = trails[i][j + 1].y - trails[i][j - 1].y;
                float len = sqrt(dirX * dirX + dirY * dirY);
                if (len > 0.01f) {
                    dirX /= len;
                    dirY /= len;
                }
            }
            
            // Perpendicular vector (rotate 90 degrees)
            float perpX = -dirY;
            float perpY = dirX;
            
            // Apply wave offset perpendicular to trail
            float fx = baseX + perpX * waveOffset;
            float fy = baseY + perpY * waveOffset;
            
            int x = (int)fx, y = (int)fy;
            if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) continue;

            float normX = baseX / WIDTH;
            int idx = (int)(normX * (SAMPLES - 1));
            idx = constrain(idx, 0, SAMPLES - 1);
            float amp = fabs(vRealOriginal[idx]) / 32768.0f;

            float glow = 0.7f + 0.3f * amp;
            float hue = fmod(t * 50 + i * 120 + j * 0.1f, 360);
            uint8_t r, g, b;
            hsvToRgb(hue, 1.0f, glow, r, g, b);

            
            // Enhanced brightness on beat
            //if (audioAmp > 0.5f) {
               // r = min(255, (int)(r * (1.0f + audioAmp * 0.3f)));
              //  g = min(255, (int)(g * (1.0f + audioAmp * 0.3f)));
             //   b = min(255, (int)(b * (1.0f + audioAmp * 0.3f)));
            //}

             // Get existing pixel and create contrast
            uint16_t existingColor = GFX::getPixel(x, y);
            uint8_t existingR = ((existingColor >> 11) & 0x1F) << 3;
            uint8_t existingG = ((existingColor >> 5) & 0x3F) << 2;
            uint8_t existingB = (existingColor & 0x1F) << 3;
            
            // Blend with existing pixel for contrast/additive effect
            uint8_t finalR = min(255, (int)r + (int)existingR / 2);
            uint8_t finalG = min(255, (int)g + (int)existingG / 2);
            uint8_t finalB = min(255, (int)b + (int)existingB / 2);

          
            GFX::setPixel(x, y, rgbToColor(finalR , finalG, finalB));
            
        }

        // Draw body with wave distortion
        float baseBodyX = bodies[i].pos.x * scale + WIDTH / 2;
        float baseBodyY = bodies[i].pos.y * scale + HEIGHT / 2;
        
        float bodyWavePhase = t * 3.0f + trailSizes[i] * 0.3f;
        float bodyWaveAmp = audioAmp * 2.5f;
        float bodyWaveOffset = sin(bodyWavePhase) * bodyWaveAmp;
        
        // Use velocity direction for perpendicular wave
        float velLen = bodies[i].vel.mag();
        float perpBodyX = 0, perpBodyY = 0;
        if (velLen > 0.01f) {
            perpBodyX = -bodies[i].vel.y / velLen;
            perpBodyY = bodies[i].vel.x / velLen;
        }
        
        int bx = (int)(baseBodyX + perpBodyX * bodyWaveOffset);
        int by = (int)(baseBodyY + perpBodyY * bodyWaveOffset);
        
        if (bx >= 0 && bx < WIDTH && by >= 0 && by < HEIGHT) {
            int idx = (int)(baseBodyX * (SAMPLES - 1) / WIDTH);
            idx = constrain(idx, 0, SAMPLES - 1);
            float pulse = 200 + 55 * fabs(vRealOriginal[idx]) / 32768.0f;
            
            // Extra pulse on strong beats
            if (audioAmp > 0.6f) {
                pulse = min(255.0f, pulse * (1.0f + audioAmp * 0.4f));
            }

            
            GFX::setPixel(bx, by, rgbToColor((uint8_t)pulse, (uint8_t)pulse, (uint8_t)pulse));
      
        }
    }
}
*/


void drawOrbitalTrails() {
    float t = millis() * 0.002f;
    float audioAmp = getAudioAmplitude();
    
    // Update cache once per frame
    updatePixelCache();

    for (int i = 0; i < NUM_BODIES; i++) {
        // Update trail
        if (trailSizes[i] < maxTrailLength) {
            trails[i][trailSizes[i]++] = bodies[i].pos;
        } else {
            memmove(trails[i], trails[i] + 1, (maxTrailLength - 1) * sizeof(Vec2));
            trails[i][maxTrailLength - 1] = bodies[i].pos;
        }

        // Draw trail
        for (int j = 0; j < trailSizes[i]; j++) {
            // Position calculation (keep your original)
            float baseX = trails[i][j].x * scale + WIDTH / 2;
            float baseY = trails[i][j].y * scale + HEIGHT / 2;
            
            float wavePhase = t * 3.0f + j * 0.3f;
            float waveAmplitude = audioAmp * 2.5f;
            float waveOffset = sin(wavePhase) * waveAmplitude;
            
            float dirX = 0, dirY = 0;
            if (j > 0 && j < trailSizes[i] - 1) {
                dirX = trails[i][j + 1].x - trails[i][j - 1].x;
                dirY = trails[i][j + 1].y - trails[i][j - 1].y;
                float len = sqrt(dirX * dirX + dirY * dirY);
                if (len > 0.01f) {
                    dirX /= len;
                    dirY /= len;
                }
            }
            
            float perpX = -dirY;
            float perpY = dirX;
            
            int x = (int)(baseX + perpX * waveOffset);
            int y = (int)(baseY + perpY * waveOffset);
            
            if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) continue;

            // Base color (keep your original)
            float normX = baseX / WIDTH;
            int idx = (int)(normX * (SAMPLES - 1));
            idx = constrain(idx, 0, SAMPLES - 1);
            float amp = fabs(vRealOriginal[idx]) / 32768.0f;

            float glow = 0.7f + 0.3f * amp;
            float hue = fmod(t * 50 + i * 120 + j * 0.1f, 360);
            uint8_t r, g, b;
            hsvToRgb(hue, 1.0f, glow, r, g, b);

            // FAST CACHED INTERACTION
            processPixelInteractionFast(x, y, r, g, b, audioAmp);

            GFX::setPixel(x, y, rgbToColor(r, g, b));
        }

        // Body (keep original)
        float baseBodyX = bodies[i].pos.x * scale + WIDTH / 2;
        float baseBodyY = bodies[i].pos.y * scale + HEIGHT / 2;
        
        float bodyWavePhase = t * 3.0f + trailSizes[i] * 0.3f;
        float bodyWaveAmp = audioAmp * 2.5f;
        float bodyWaveOffset = sin(bodyWavePhase) * bodyWaveAmp;
        
        float velLen = bodies[i].vel.mag();
        float perpBodyX = 0, perpBodyY = 0;
        if (velLen > 0.01f) {
            perpBodyX = -bodies[i].vel.y / velLen;
            perpBodyY = bodies[i].vel.x / velLen;
        }
        
        int bx = (int)(baseBodyX + perpBodyX * bodyWaveOffset);
        int by = (int)(baseBodyY + perpBodyY * bodyWaveOffset);
        
        if (bx >= 0 && bx < WIDTH && by >= 0 && by < HEIGHT) {
            int idx = (int)(baseBodyX * (SAMPLES - 1) / WIDTH);
            idx = constrain(idx, 0, SAMPLES - 1);
            float pulse = 200 + 55 * fabs(vRealOriginal[idx]) / 32768.0f;
            
            if (audioAmp > 0.6f) {
                pulse = min(255.0f, pulse * (1.0f + audioAmp * 0.4f));
            }

            GFX::setPixel(bx, by, rgbToColor((uint8_t)pulse, (uint8_t)pulse, (uint8_t)pulse));
        }
    }
}

// === MAIN FUNCTION ===
void body3(int trans) {
    currentShape = trans % 30;
    nextShape = (trans + 1) % 30;

    //////////////
    memset(pixelCache, 0, sizeof(pixelCache));
    currentInteraction = INTERACT_RADIAL_GLOW;
    ///////////////////////////


    updateBodies();  // Physics stays the same
    drawOrbitalTrails();  // Visual reacts to audio

    transition += 0.001f;
    if (transition >= 1.0f) {
        transition = 0.0f;
        currentShape = nextShape;
        nextShape = (nextShape + 1) % 30;
        //for (int i = 0; i < NUM_BODIES; i++) trailSizes[i] = 0;
    }
}

// === INIT ===
void init3Body() {
    defineShapes();
}