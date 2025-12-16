#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
 
#include "pio.h"
#include "gfx.h"
#include "FFT.h"
#include "palette.h"
#include "math.h"

#include "scene.h"
 


#include <math.h>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

 
#define ALTER  33768 

#define  norm(beatStrength) \
    (uint8_t)(fmaxf(0.0f, fminf(1.0f, (beatStrength))) * 64)


// Normalization function that returns beat modulation value
uint8_t normalizeBeat(float beatStrength) {
    float normalizedBeat = fmaxf(0.0f, fminf(1.0f, beatStrength));
    return (uint8_t)(normalizedBeat * 64); // 0-64 range


  
}


uint8_t valueToPaletteIndex(float value, bool fadeFromBlack = true) {
    if (fadeFromBlack && value < 0.05f) return 0;  // Force black
    
    // Map to palette range, avoiding index 0
    uint8_t idx = 1 + (uint8_t)(value * (GFX_PALETTE_SIZE - 2));
    return idx;
}

void initAnimePlasma(AnimePlasma* p) {

    memset(p->body, 0, 256*256);

    p->p1 = p->p2 = p->p3 = p->p4 = 0;

    // ---- same cosine table you had (0-60) ----
    for (int i = 0; i < 256; ++i) {
        float angle = i * (M_PI / 128.0f);
        int val = (int)(30.0f * cosf(angle));      // -30 .. +30
        p->cosTable[i] = (uint8_t)(val + 30);      // 0 .. 60
    }

    // ---- random start offsets (never repeats exactly) ----
    p->offset[0] = rand() & 0xFF;
    p->offset[1] = rand() & 0xFF;
    p->offset[2] = rand() & 0xFF;
    p->offset[3] = rand() & 0xFF;
}


 
 

void updateAnimePlasma(AnimePlasma* p) {
    uint8_t beatMod = normalizeBeat(vReal[0]);
    
    // Original code completely untouched
    uint8_t t1 = p->p1 + p->offset[0];
    uint8_t t2 = p->p2 + p->offset[1];

    for (int y = 0; y < 256; ++y) {
        uint8_t t3 = p->p3 + p->offset[2];
        uint8_t t4 = p->p4 + p->offset[3];
        uint8_t* row = p->body + y * 256;

        for (int x = 0; x < 256; ++x) {
           

  // === 4 cosine waves (your original) ===
      //      uint16_t sum = p->cosTable[t1] + 
       //                    p->cosTable[t2] + 
       //                    p->cosTable[t3] + 
       //                    p->cosTable[t4];   // 0–240
 ////////////////////////////////////////////////////////  + (beatMod / 32)) & 0xFF] +
         uint16_t sum = 
               p->cosTable[t1] +  // Add to t1
               p->cosTable[t2] +    
               p->cosTable[t3+ (beatMod / 30) & 0xFF] +
               p->cosTable[t4  ] ;    
 ////////////////////////////////////////////////////////               



            uint8_t idx = (uint8_t)((sum * 255UL) / 240UL);
            row[x] = idx;

            t3 = (t3 + 1) & 0xFF;
            t4 = (t4 + 3) & 0xFF;
        }
        t1 = (t1 + 2) & 0xFF;
        t2 = (t2 + 1) & 0xFF;
    }

    // Original phase increments completely unchanged
    p->p1 = (p->p1 + 1) & 0xFF;
    p->p2 = (p->p2 + 254) & 0xFF;
    p->p3 = (p->p3 + 255) & 0xFF;
    p->p4 = (p->p4 + 3) & 0xFF;
}
 

void drawAnimePlasma(const AnimePlasma* p) {
    // No beat modulation in drawing - keep it pure
    const uint32_t stepX = (256UL << 16) / W;
    const uint32_t stepY = (256UL << 16) / H;

    uint32_t srcY = 0;
    for (uint16_t y = 0; y < H; ++y, srcY += stepY) {
        uint16_t y0 = srcY >> 16;
        uint16_t y1 = (y0 + 1) & 0xFF;
        uint8_t fy = (srcY >> 8) & 0xFF;

        uint32_t srcX = 0;
        for (uint16_t x = 0; x < W; ++x, srcX += stepX) {
            uint16_t x0 = srcX >> 16;
            uint16_t x1 = (x0 + 1) & 0xFF;
            uint8_t fx = (srcX >> 8) & 0xFF;

            uint8_t a = p->body[y0 * 256 + x0];
            uint8_t b = p->body[y0 * 256 + x1];
            uint8_t c = p->body[y1 * 256 + x0];
            uint8_t d = p->body[y1 * 256 + x1];

            // Original interpolation - no beat effect
            uint8_t top = a + (((b - a) * fx) >> 8);
            uint8_t bot = c + (((d - c) * fx) >> 8);
            uint8_t idx = top + (((bot - top) * fy) >> 8);

            GFX::setPixel(x, y, idx);
        }
    }
}



//.....................................................................
 


// ====================================================
// Plasma (GFX Palette Optimized)
// ====================================================
 
void initPlasma(Plasma* plasma) {
    plasma->width = 256;
    plasma->height = 256;
    plasma->body = (uint8_t*)malloc(256 * 256);
    memset(plasma->body, 0, 256 * 256);
    
    // Precompute cosinus table [-30,30] → [0,255] for indexing
    for (int i = 0; i < 256; i++) {
        float angle = i * (PI / 128.0f);
        int val = (int)(30 * cosf(angle));
        plasma->cosinus[i] = (uint8_t)(val + 30);  // 0..60 → perfect for palette
    }
    
  
}

void updatePlasma(Plasma* plasma) {
    static uint8_t p1 = 0, p2 = 0, p3 = 0, p4 = 0;
    uint8_t t1, t2, t3, t4;
    
    t1 = p1; t2 = p2;
    for (int y = 0; y < plasma->height; y++) {
        t3 = p3; t4 = p4;
        for (int x = 0; x < plasma->width; x++) {
            // Sum 4 cosines → [0,60] perfect palette range
            int col = plasma->cosinus[t1] + plasma->cosinus[t2] +
                      plasma->cosinus[t3] + plasma->cosinus[t4];
            plasma->body[y * plasma->width + x] = (uint8_t)(col & 0xFF); // 0-60 → palette
            
            t3 = (t3 + 1) & 0xFF;
            t4 = (t4 + 3) & 0xFF;
        }
        t1 = (t1 + 2) & 0xFF;
        t2 = (t2 + 1) & 0xFF;
    }
    p1 = (p1 + 1) & 0xFF;
    p2 = (p2 - 2) & 0xFF;
    p3 = (p3 - 1) & 0xFF;
    p4 = (p4 + 3) & 0xFF;
}

void drawPlasma(Plasma* plasma) {
    for (int y = 0; y < HEIGHT; y++) {
        int iy = (y * (plasma->height - 1)) / HEIGHT;  // Smooth scaling
        for (int x = 0; x < WIDTH; x++) {
            int ix = (x * (plasma->width - 1)) / WIDTH;
            uint8_t index = plasma->body[iy * plasma->width + ix];
             
            GFX::setPixel(x, y, index); 
        }
    }
}






// ====================================================
// fire flame
// ====================================================




void initFlame(Flame* f) {
    f->width  = WIDTH;
    f->height = HEIGHT;

    // ---- Allocate two buffers (not PROGMEM!) ----
    f->buffer      = (uint8_t*)malloc(WIDTH * HEIGHT);
    f->fire_buffer = (uint8_t*)malloc(WIDTH * HEIGHT);

    // ---- Clear them ----
    memset(f->buffer,      0, WIDTH * HEIGHT);
    memset(f->fire_buffer, 0, WIDTH * HEIGHT);
}

void updateFlame(Flame* f) {
    // === 1. Add random coals at bottom ===
    for (int x = 2; x < f->width - 2; x += random(4, 7)) {
        if (random(3) == 0) {
            for (int dx = -5; dx <= 5; ++dx) {        // wider spark
                int px = x + dx;
                if (px >= 0 && px < f->width) {
                    f->buffer[(f->height - 1) * f->width + px] = 255;
                }
            }
        }
    }

    // === 2. Propagate upward (blur + decay) ===
    for (int y = 1; y < f->height; ++y) {
        for (int x = 1; x < f->width - 1; ++x) {
            int idx = y * f->width + x;

            // Average of self + left + right + below
            int avg = (f->buffer[idx] +
                       f->buffer[idx - 1] +
                       f->buffer[idx + 1] +
                       f->buffer[idx - f->width]) >> 2;

           // int newVal = (avg > 2) ? avg - 2 : 0;
            int newVal = (avg > 1.4) ? avg - 1.4 : 0;
            f->fire_buffer[(y - 1) * f->width + x] = newVal;
        }
    }

    // === 3. Swap buffers ===
    memcpy(f->buffer, f->fire_buffer, WIDTH * HEIGHT);
    memset(f->fire_buffer, 0, WIDTH * HEIGHT);
}

void drawFlame2(const Flame* f) {
    for (int y = 0; y < f->height; ++y) {
        for (int x = 0; x < f->width; ++x) {
            uint8_t v = f->buffer[y * f->width + x];

           
           // uint8_t index = v;   // direct mapping!
            
            // === FORCE BLACK when v == 0 ===
            uint8_t index = (v == 0) ? 0 : ((v + 80) % 256);

            // === Use your existing palette system ===
            GFX::setPixel(x, y, index);
        }
    }
}

 

 
 

 void drawFlame(const Flame* f) {
    // Build simple histogram
    uint16_t histogram[256] = {0};
    for (int i = 0; i < f->width * f->height; i++) {
        histogram[f->buffer[i]]++;
    }
    
    // Find meaningful heat range (ignore extremes)
    uint8_t min_heat = 255, max_heat = 0;
    for (int i = 1; i < 256; i++) {
        if (histogram[i] > 0) {
            min_heat = i < min_heat ? i : min_heat;
            max_heat = i > max_heat ? i : max_heat;
        }
    }
    
    float range = max_heat - min_heat;
    if (range < 1.0f) range = 1.0f;
    
    for (int y = 0; y < f->height; y++) {
        for (int x = 0; x < f->width; x++) {
            uint8_t v = f->buffer[y * f->width + x];
            
            if (v < min_heat) {
                GFX::setPixel(x, y, 0);
            } else {
                // Map to palette excluding black (index 0)
                float normalized = (float)(v - min_heat) / range;
                uint8_t index = 1 + (uint8_t)(normalized * (GFX_PALETTE_SIZE - 2));
                GFX::setPixel(x, y, index);
            }
        }
    }
}

 /////////////////////////////////////////////////////////



 
// Easing function: easeInOutExpo
float easeInOutExpo(float x) {
    if (x <= 0.0f) return 0.0f;
    if (x >= 1.0f) return 1.0f;
    return (x < 0.5f)
        ? (powf(2.0f, 20.0f * x - 10.0f) / 2.0f)
        : ((2.0f - powf(2.0f, -20.0f * x + 10.0f)) / 2.0f);
}
 
// Helper function to draw filled rectangle optimized
void drawFilledRect(int x, int y, int w, int h, uint8_t paletteIndex) {
    if (w <= 0 || h <= 0) return;
    int x0 = max(0, x);
    int y0 = max(0, y);
    int x1 = min(WIDTH, x + w);
    int y1 = min(HEIGHT, y + h);
    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            GFX::setPixel(px, py, paletteIndex);
        }
    }
}


  
 float  SmoothA() 
 {

    
     float currentAmplitude = fabs(vReal[0]) / ALTER;  
     return    0.75f + currentAmplitude * 0.04f;
      
 }

  
void RadialGlow(uint32_t time) {
    static uint32_t startTime = 0;
    if (startTime == 0) startTime = time;

    float t = (time - startTime) * 0.002f;  // slow time

    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            // === 1. Vertical gradient (smooth fade down) ===
            float fade = (float)y / HEIGHT;  // 0.0 (top) → 1.0 (bottom)

            // === 2. Horizontal wave (gentle ripple) ===
            float wave = sinf(x * 0.15f + t) * 0.5f + 0.5f;  // 0.0 → 1.0

            // === 3. Combine: top = bright, bottom = dark, with wave ===
            float value = (1.0f - fade) * 0.7f + wave * 0.3f;  // 0.0 → 1.0

 

            // === 4. Map to palette index ===
            uint8_t index = (uint8_t)(value * 255.0f);

            GFX::setPixel(x, y, index);
        }
    }
}


void BioForest (float t) {
    
    const float scale = 10.0f;
    const float inv_H = 1.0f / H;
    
    float t_factor = t * 0.015f;


   
     
    
    // Gentle forest movement
    float swayX = fastSin(t * 0.0007f) * 1.2f;
    float swayY = fastSin(t * 0.0004f) * 0.6f;
    float breath = 1.0f + fastSin(t * 0.08f) * SmoothA(); //SmoothA() ;//0.05f;
    
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float px = (x * inv_H * scale - 5.0f) * breath + swayX;
            float py = (y * inv_H * scale - 5.0f) * breath + swayY;
            
            // Tree trunk/vertical patterns
            float tree = fastSin(px * 8.0f) * 0.5f;
            float ground = fastSin(px * 3.0f + py * 2.0f + t_factor) * 0.3f;
            
            // Bioluminescent particles
            float particles = 0.0f;
            for (int i = 0; i < 4; i++) {
                float freq = 5.0f + i * 3.0f;
                particles += fastSin(px * freq + py * (freq * 0.7f) + t_factor * (i + 1)) * (0.2f / (i + 1));
            }
            
            // Combine elements
            float density = tree + ground + particles;
            
            // Animate glowing colors
            float glow_phase = t * 0.002f;
             float color_val = density * 2.0f + glow_phase + py * 0.7f;
          
            
            // Map to palette with pulsing effect
            uint8_t pulse = (uint8_t)((fastSin(t * 0.1f) * 0.3f + 0.7f) * 255);
            uint8_t index = 1 + (uint8_t)(color_val * 33) % (GFX_PALETTE_SIZE - 2);
            
            //////////////aaaa
           //  density = (tree + ground + particles + 2.0f) / 4.0f;
            //density = fmax(0.0f, fmin(1.0f, density));
           //  index = valueToPaletteIndex(density, true);
            //////////////
            GFX::setPixel(x, y, index);
        }
    }
}
void OrganicLandscape(float t) {
   
    const float scale = 8.0f;
    const float inv_H = 1.0f / H;
    
    float t_factor = t * 0.02f;
    int centerX = W / 2;
    Wave soundData = WaveData(centerX, t);

    // Sound-reactive parameters
    float bassEnergy = soundData.energy[0];      // Low frequencies (0-1 range)
    float midEnergy = soundData.energy[1];       // Mid frequencies  
    float trebleEnergy = soundData.energy[2];    // High frequencies
    float overallAmp = soundData.filteredAmplitude; // Overall volume


  
    
    // Terrain movement
    float moveX = fastSin(t * 0.0008f) + bassEnergy  * SmoothA() ;//1.5f;
    float moveY = fastSin(t * 0.0003f) +  (midEnergy/2)  * SmoothA() ;//0.8f;
    float pulse = 1.0f + fastSin(t * 0.05f) * SmoothA() ;//0.1f;
    
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            // Normalized coordinates with perspective
            float px = (x * inv_H * scale - 4.0f) * pulse + moveX;
            float py = (y * inv_H * scale - 3.0f) * pulse + moveY;
            
            // Layered organic patterns
            float height = 0.0f;
            
            // Base terrain (mountains/hills)
            height += fastSin(px * 0.7f + t_factor * 0.1f) * 0.8f;
            height += fastSin(px * 1.3f + py * 0.5f + t_factor * 0.2f) * 0.4f;
            height += fastSin(px * 2.1f + py * 1.2f + t_factor * 0.3f) * 0.2f;
            
            // River/valley pattern
            float river = fastSin(py * 3.0f + t_factor * 0.5f) * 0.3f;
            height -= river * fastSin(px * 2.0f) * 0.4f;
            
            // Animate the "sunset" colors based on height
            float color_time = t * 0.001f;
            float hue_base = height * 2.0f + color_time;
            
            // Map to palette (height determines color zone)
            uint8_t index;
            if (height < -0.3f) {
                // Deep water/valley - cool colors
                index = 1 + (uint8_t)((height + 1.0f) * 20) % 40;
            } else if (height < 0.2f) {
                // Mid-range - warm earth tones
                index = 40 + (uint8_t)((height + 0.3f) * 100) % 80;
            } else {
                // Peaks - hot colors
                index = 120 + (uint8_t)((height - 0.2f) * 60) % 100;
            }


            ///////////////////aaaa
            //float normalizedHeight = (height + 1.5f) / 3.0f;  // Adjust range
           // normalizedHeight = fmax(0.0f, fmin(1.0f, normalizedHeight));
            // index = valueToPaletteIndex(normalizedHeight, true);
            ////////////////////////////////////////
            
            GFX::setPixel(x, y, index);
        }
    }
}
void GooGlow(float t) {
  
    const float scale = 9.0f;
    const float inv_H = 1.0f / H;
    
    float t_factor = t * 0.03f;
    
    // Add movement animation
    float moveX = fastSin(t * 0.0015f) *  SmoothA() ;//2.0f;      // Horizontal drift
    float moveY = fastSin(t * 0.0001f) * SmoothA() ;//2.0f;       // Vertical drift
    float zoom = 1.0f + fastSin(t * 0.08f) * 0.03f; // Pulsing zoom
    float rotate = t * SmoothA() ;//0.001f;                      // Gentle rotation
    
    // Pre-calculate rotation
    float cosR = fastSin(rotate + 0.5308f);//1.1308f); // cos = sin(x + π/2)
    float sinR = fastSin(rotate);
    
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            // Center coordinates
            float px = (x * inv_H * scale - 3.5f) * zoom;
            float py = (y * inv_H * scale - 3.5f) * zoom;
            
            // Apply rotation
            float rotX = px * cosR - py * sinR;
            float rotY = px * sinR + py * cosR;
            
            // Apply movement
            px = rotX + moveX;
            py = rotY + moveY;

            // Swirl iterations
            for (int i = 0; i < 3; ++i) {
                px += fastSin(py + i + t_factor);
                
                float newX = px * 0.35f - py;
                float newY = px + py * 0.35f;
                px = newX;
                py = newY;
            }

            // Color with time-based hue shift
            float px_color = px * 1.1f + t * 1.1f;
            uint8_t R = (uint8_t)((fastSin(px_color) * 0.2f + 0.5f) * 255.0f);
            uint8_t G = (uint8_t)((fastSin(px_color + 1.0f) * 0.2f + 0.5f) * 255.0f);
            uint8_t B = (uint8_t)((fastSin(px_color + 2.0f) * 0.2f + 0.5f) * 255.0f);

            uint8_t index = rgbToPaletteIndex(R, G, B);

             ////////////////////////aaaa
           // float colorValue = (fastSin(px * 1.1f + t * 1.1f) * 0.5f + 0.5f);
            // index = valueToPaletteIndex(colorValue, true);
            //////////////////////////
            GFX::setPixel(x, y, index);
        }
    }
}

void OceanDepths(float t) {
     
    const float scale = 11.0f;
    const float inv_H = 1.0f / H;
    
    float t_factor = t * 0.018f;
    
    // Ocean currents
    float currentX = fastSin(t * 0.0006f) * 1.8f;
    float currentY = fastSin(t * 0.0009f) * 1.2f;
    float swell = 1.0f + fastSin(t * 0.02f) * SmoothA() ; //0.08f; //alteration here
    
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float px = (x * inv_H * scale - 5.5f) * swell + currentX;
            float py = (y * inv_H * scale - 5.5f) * swell + currentY;
            
            // Water caustics
            float caustics = 0.0f;
            for (int i = 0; i < 4; i++) {
                float freq = 2.0f + i * 1.5f;
                caustics += fastSin(px * freq + py * freq * 0.6f + t_factor * (i + 1)) * (0.12f / (i + 1));
            }
            
            // Deep currents
            float currents = fastSin(px * 0.8f + py * 1.2f + t_factor * 0.5f) * 0.4f;
            
            // Marine snow/particles
            float particles = fastSin(px * 15.0f + t * 0.02f) * fastSin(py * 12.0f + t * 0.015f) * 0.2f;
            
            // Depth gradient
            float depth = (py + 5.5f) * 0.3f;
            
            float ocean = caustics + currents + particles - depth;
            
            // Map to blue/green palette range
            uint8_t index = 1 + (uint8_t)((ocean + 1.0f) * 40) % (GFX_PALETTE_SIZE - 2);
            
            GFX::setPixel(x, y, index);
        }
    }
}

void MysticalSwamp(float t) {
     
    const float scale = 8.0f;
    const float inv_H = 1.0f / H;
    
    float t_factor = t * 0.022f;
    
    // Swamp mist movement
    float mistX = fastSin(t * 0.0005f) * 1.0f;
    float mistY = fastSin(t * 0.0007f) * 0.5f;
    float haze = 1.0f + fastSin(t * 0.06f) * SmoothA() ;//0.07f;
    
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float px = (x * inv_H * scale - 4.0f) * haze + mistX;
            float py = (y * inv_H * scale - 4.0f) * haze + mistY;
            
            // Swamp gas/mist patterns
            float mist = 0.0f;
            mist += fastSin(px * 1.2f + t_factor * 0.3f) * 0.5f;
            mist += fastSin(px * 2.8f + py * 1.5f + t_factor * 0.7f) * 0.3f;
            
            // Water ripples
            float ripples = fastSin(px * 6.0f + py * 4.0f + t_factor * 2.0f) * 0.4f;
            
            // Glowing fungi/mushrooms
            float glow = fastSin(px * 10.0f + t * 0.03f) * fastSin(py * 8.0f + t * 0.025f) * 0.3f;
            
            // Will-o-wisp floating lights
            float wisps = fastSin(px * 3.0f + t * 0.01f) * 0.2f;
            
            float swamp = mist + ripples + glow + wisps;
            
            // Map to mystical green/purple palette
            uint8_t index = 1 + (uint8_t)((swamp + 1.0f) * 35 + t * 0.1f) % (GFX_PALETTE_SIZE - 2);
            
            GFX::setPixel(x, y, index);
        }
    }
}

void VolcanicLandscape(float t) {
     
    const float scale = 9.0f;
    const float inv_H = 1.0f / H;
    
    float t_factor = t * 0.025f;
    
    // Volcanic activity
    float rumble = fastSin(t * 0.002f) * (vReal[1]/ALTER);  // 0.3f;
    float lava_flow = t * 0.001f;
    float heat_pulse = 1.0f + fastSin(t * 0.1f) * SmoothA() ;   // 0.2f;
    
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float px = (x * inv_H * scale - 4.5f) + rumble;
            float py = (y * inv_H * scale - 4.5f) * heat_pulse;
            
            // Rocky terrain
            float rock = 0.0f;
            rock += fastSin(px * 1.5f) * 0.6f;
            rock += fastSin(px * 3.2f + py * 1.8f) * 0.3f;
            rock += fastSin(px * 6.7f + py * 3.4f) * 0.1f;
            
            // Lava rivers
            float lava = fastSin(px * 4.0f + lava_flow) * 0.4f;
            lava += fastSin(py * 3.0f + lava_flow * 1.3f) * 0.3f;
            
            // Volcanic vents
            float vents = fastSin(px * 8.0f) * fastSin(py * 8.0f) * 0.5f;
            
            // Combine with eruption pulses
            float eruption = fastSin(t * 0.05f + px * 2.0f) * 0.4f;
            
            float volcanic = rock + lava + vents + eruption;
            
            // Map to fire palette
            uint8_t index;
            if (volcanic < 0.3f) {
                // Cool rock
                index = 1 + (uint8_t)(volcanic * 60) % 40;
            } else if (volcanic < 0.7f) {
                // Warm rock/lava
                index = 40 + (uint8_t)((volcanic - 0.3f) * 100) % 60;
            } else {
                // Hot lava core
                index = 100 + (uint8_t)((volcanic - 0.7f) * 120) % (GFX_PALETTE_SIZE - 101);
            }
            
            ///////////////aaaa
            // volcanic = (rock + lava + vents + eruption + 3.0f) / 6.0f;
           // volcanic = fmax(0.0f, fmin(1.0f, volcanic));
            // index = valueToPaletteIndex(volcanic, true);
            ////////////////
            GFX::setPixel(x, y, index);
        }
    }
}

void AuroraSky(float t) {
    
    const float scale = 12.0f;
    const float inv_H = 1.0f / H;
    
    float t_factor = t *  0.01f;
    
    // Aurora movement
    float drift = t *  0.0002f;
    float wave = fastSin(t * 0.003f) *  (vReal[1]/ALTER);//2.0f;
    
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float px = x * inv_H * scale - (vReal[1]/ALTER);//6.0f;
            float py = y * inv_H * scale - (vReal[2]/ALTER);//3.0f;
            
            // Aurora curtain effect
            float curtain = fastSin(px * 0.5f + drift) * 2.0f;
            float bands = fastSin(px * 2.0f + py * 4.0f + t_factor) *   0.7f;
            
            // Rippling waves through aurora
            float ripple = 0.0f;
            for (int i = 0; i < 3; i++) {
                float freq = 3.0f + i * 2.0f;
                ripple += fastSin(px * freq + py * freq * 0.3f + t_factor * (i + 1) * 0.5f) * (0.15f / (i + 1));
            }
            
            // Vertical gradient for sky
            float sky_gradient = (py + 1.0f) * SmoothA() ; //0.4f;
            
            // Combine with starfield
            float stars = fastSin(px * 50.0f) * fastSin(py * 16.0f)  * 0.1f;
            
            float aurora = curtain + bands + ripple + sky_gradient + stars;
            
            // Map to cool end of palette
            uint8_t index;
            if (aurora < 0.3f) {
                // Deep space
                index = 1 + (uint8_t)(aurora * 40) % 15;
            } else {
                // Aurora colors
                index = 30 + (uint8_t)((aurora - 0.5f) * 150) % (GFX_PALETTE_SIZE - 31);
            }
            
            GFX::setPixel(x, y, index);
        }
    }
}

void AnimeMountainZoom(float t) {
    
    const float scale = 8.0f;
    const float inv_H = 1.0f / H;
    
    // Dynamic zoom with smooth in/out
    float zoom = 1.0f + fastSin(t * 0.0008f) * SmoothA() ;//0.5f; // Gentle zoom cycle
    float panX = fastSin(t * 0.0005f) * 2.0f; // Horizontal pan
    float panY =  Cos(t * 0.0003f) * 1.0f; // Vertical pan
    
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            // Apply zoom and pan
            float px = (x * inv_H * scale - 4.0f) * zoom + panX;
            float py = (y * inv_H * scale - 3.0f) * zoom + panY;
            
            // Anime-style layered mountains
            float height = 0.0f;
            
            // Distant mountains (low frequency)
            height += fastSin(px * 0.3f + t * 0.0001f) * 0.4f;
            height += fastSin(px * 0.7f - py * 0.2f + t * 0.0002f) * 0.3f;
            
            // Mid-range hills
            height += fastSin(px * 1.5f + py * 0.8f + t * 0.0004f) * 0.5f;
            
            // Foreground details
            height += fastSin(px * 4.0f + py * 2.0f + t * 0.001f) * 0.2f;
            
            // Anime color zones
            uint8_t index;
            if (height < -0.2f) {
                // Sky - blues and purples
                index = 1 + (uint8_t)((height + 1.0f) * 15) % 30;
            } else if (height < 0.1f) {
                // Distant mountains - desaturated
                index = 30 + (uint8_t)((height + 0.2f) * 80) % 40;
            } else if (height < 0.4f) {
                // Forest/mid-range - greens
                index = 70 + (uint8_t)((height - 0.1f) * 90) % 50;
            } else {
                // Snow peaks - whites and light blues
                index = 120 + (uint8_t)((height - 0.4f) * 40) % 30;
            }
            
            GFX::setPixel(x, y, index);
        }
    }
}

void FloatingIslands(float t) {
     
    const float scale = 7.0f;
    const float inv_H = 1.0f / H;
    
    // Bouncing zoom for dreamy effect
    float zoom = 1.0f + fastSin(t * 0.001f) * 0.9f;
    float floatY = fastSin(t * 0.06f) * SmoothA() ;//1.2f; // Floating motion
    //float floatX =  Cos(t * 0.06f) * 0.8f;
    float floatX = fastSin(t * 0.06f) * SmoothA() ;
    
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float px = (x * inv_H * scale - 3.5f) * zoom + floatX;
            float py = (y * inv_H * scale - 3.5f) * zoom + floatY;
            
            // Island shapes with smooth edges
            float island = 0.0f;
            
            // Main island body
            island += fastSin(px * 2.0f + t * 0.0003f) * 0.6f;
            island += fastSin(px * 3.5f + py * 1.5f + t * 0.0005f) * 0.4f;
            
            // Smaller floating rocks
            island += fastSin(px * 8.0f + t * 0.002f) * 0.2f;
            island += fastSin(py * 6.0f - t * 0.0015f) * 0.15f;
            
            // Cloud base
            float clouds = fastSin(px * 0.8f + py * 0.7f + t * 0.08f) * 0.3f;
            
            float scene = island + clouds;
            
            // Anime fantasy colors
            uint8_t index;
            if (scene < 0.2f) {
                // Sky background
                index = 1 + (uint8_t)(py * 20) % 25;
            } else if (scene < 0.5f) {
                // Cloud wisps
                index = 25 + (uint8_t)((scene - 0.1f) * 60) % 30;
            } else if (scene < 0.8f) {
                // Island vegetation
                index = 55 + (uint8_t)((scene - 0.4f) * 80) % 45;
            } else {
                // Island tops/rock
                index = 100 + (uint8_t)((scene - 0.7f) * 50) % 40;
            }
            
            GFX::setPixel(x, y, index);
        }
    }
}

void CherryBlossomGarden(float t) {
    
    const float scale = 10.0f;
    const float inv_H = 1.0f / H;
    
    // Camera follows waves
    float zoom = 1.0f + fastSin(t * 0.007f) * SmoothA() ;//0.4f;
    float wavePan = fastSin(t * 0.0009f) * 2.0f;
    float bobY = fastSin(t * 0.0012f) * 0.5f;
    
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float px = (x * inv_H * scale - 5.0f) * zoom + wavePan;
            float py = (y * inv_H * scale - 5.0f) * zoom + bobY;
            
            // Ocean waves
            float ocean = 0.0f;
            ocean += fastSin(px * 1.2f + t * 0.001f) * 0.6f; // Large swells
            ocean += fastSin(px * 3.5f + py * 1.8f + t * 0.003f) * 0.4f; // Detail waves
            ocean += fastSin(px * 8.0f + t * 0.008f) * 0.2f; // Foam
            
            // Beach/sand
            float sand = fastSin(px * 0.8f - py * 2.0f) * 0.3f;
            
            // Cliff/coastline
            float cliffs = fastSin(px * 4.0f) * 0.5f;
            
            float coast = ocean + sand + cliffs;
            
            // Anime ocean colors
            uint8_t index;
            if (coast < 0.2f) {
                // Deep ocean
                index = 1 + (uint8_t)((coast + 1.0f) * 12) % 25;
            } else if (coast < 0.5f) {
                // Wave crests and medium depth
                index = 25 + (uint8_t)((coast - 0.2f) * 70) % 40;
            } else if (coast < 0.7f) {
                // Shallow water/foam
                index = 65 + (uint8_t)((coast - 0.5f) * 50) % 25;
            } else {
                // Sand and cliffs
                index = 90 + (uint8_t)((coast - 0.7f) * 60) % 45;
            }
            
            GFX::setPixel(x, y, index);
        }
    }
}
void MagicalForestPath(float t) {
   
    const float scale = 8.0f;
    const float inv_H = 1.0f / H;
    
    // Camera moving along path
    float zoom = 1.0f + fastSin(t * 0.0005f) * SmoothA() ;//0.3f;
    float pathMove = t * 0.0008f;
    float sway = fastSin(t * 0.0006f) * SmoothA() ;//1.0f;
    
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float px = (x * inv_H * scale - 4.0f) * zoom + sway;
            float py = (y * inv_H * scale - 4.0f) * zoom + pathMove;
            
            // Forest density
            float forest = 0.0f;
            forest += fastSin(px * 1.5f) * 0.5f; // Tree lines
            forest += fastSin(px * 3.0f + py * 2.0f) * 0.4f; // Undergrowth
            
            // Path through forest
            float path = fastSin(py * 4.0f) * 0.6f;
            
            // Magical glowing elements
            float magic = fastSin(px * 6.0f + py * 3.0f + t * 0.005f) * 0.3f;
            magic += fastSin(px * 10.0f + t * 0.008f) * 0.2f;
            
            float scene = forest - path + magic;
            
            // Anime magical forest colors
            uint8_t index;
            if (scene < 0.1f) {
                // Path and open areas
                index = 1 + (uint8_t)(scene * 80) % 20;
            } else if (scene < 0.4f) {
                // Forest shadows
                index = 20 + (uint8_t)((scene - 0.1f) * 60) % 30;
            } else if (scene < 0.7f) {
                // Mid-tone foliage
                index = 50 + (uint8_t)((scene - 0.4f) * 70) % 40;
            } else {
                // Magical glow and highlights
                index = 90 + (uint8_t)((scene - 0.7f) * 80 + t * 0.01f) % 50;
            }
            
            GFX::setPixel(x, y, index);
        }
    }
}
 

void AnimeCitySkyline(float t) {
     
    const float scale = 12.0f;
    const float inv_H = 1.0f / H;
    
    // Camera pan across city
    float zoom = 1.0f + fastSin(t * 0.0004f) *  0.6f;
    float cityPan = t *  0.0006f;
    float heightVar = fastSin(t * 0.0007f) *  0.3f;
    
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float px = (x * inv_H * scale - 6.0f) * zoom + cityPan;
            float py = (y * inv_H * scale - 6.0f) * zoom;
            
            // Building silhouettes
            float buildings = 0.0f;
            buildings += fastSin(px * 7.0f + heightVar) * SmoothA() ;//0.7f; // Main buildings
            buildings += fastSin(px * 11.0f + heightVar * 2.0f) * SmoothA() ;//0.4f; // Details
            
            // Windows and lights
            float lights = fastSin(px * 25.0f + py * 10.0f + t * 0.01f) * (vReal[2]/ALTER);//0.5f;
            
            // Sunset sky gradient
            float sky = (py + 2.0f) * 0.3f;
            
            float city = buildings + lights + sky;
            
            // Anime city night colors
            uint8_t index;
            if (city < 0.3f) {
                // Sky gradient
                index = 1 + (uint8_t)(city * (vReal[0]/ALTER)) % 5;
            } else if (city < 0.6f) {
                // Building silhouettes
                index = 35 + (uint8_t)((city - 0.3f) * 50) % 5;
            } else {
                // Window lights
                index = 65 + (uint8_t)((city - 0.4f) * 100 + t * 0.02f) % 5;
            }
            
            GFX::setPixel(x, y, index);
        }
    }
}


void GreenAuroraOrionNight(float t) {
     
    const float scale = 12.0f;
    const float inv_H = 1.0f / H;
    
    // Time factors for different animation speeds
    float aurora_time = t * 0.015f;
    float star_twinkle = t * 0.002f;
    
    // Aurora movement parameters
    float drift = t * 0.0003f;
    float wave_sway = fastSin(t * 0.0008f) * SmoothA() ;//1.5f;
    
    // Orion constellation coordinates (normalized to screen space)
    float orion_stars[][2] = {
        {0.5f, 0.2f},  // Betelgeuse
        {0.5f, 0.3f},  // Bellatrix
        {0.45f, 0.4f}, // Alnitak
        {0.48f, 0.42f},// Alnilam
        {0.51f, 0.44f},// Mintaka
        {0.55f, 0.5f}, // Saiph
        {0.6f, 0.55f}  // Rigel
    };
    const int ORION_STAR_COUNT = 7;
    
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            // Normalized coordinates
            float px = x * inv_H * scale - 7.0f;
            float py = y * inv_H * scale - 3.5f;
            
            // GREEN AURORA EFFECT
            float aurora = 0.0f;
            
            // Main aurora curtain (green waves)
            aurora += fastSin(px * 0.4f + drift) * SmoothA() ;//1.8f;
            
            // Secondary aurora layers
            aurora += fastSin(px * 0.8f + py * 0.6f + aurora_time * 0.7f) * 0.9f;
            aurora += fastSin(px * 1.5f + py * 1.2f + aurora_time * 1.3f) * 0.5f;
            
            // Fine aurora details
            for (int i = 0; i < 2; i++) {
                float freq = 3.0f + i * 2.0f;
                aurora += fastSin(px * freq + py * freq * 0.7f + aurora_time * (i + 2)) * (0.2f / (i + 1));
            }
            
            // Vertical gradient for night sky
            float sky_darkness = (py + 3.5f) * 0.01f;
            aurora -= sky_darkness;
            
            // STARFIELD
            float stars = 0.0f;
            
            // Random starfield
            float star_noise = fastSin(px * 25.0f) * fastSin(py * 22.0f);
            if (star_noise > 0.85f) {
                stars = (star_noise - 0.85f) * 6.0f;
            }
            
            // Twinkling effect
            float twinkle = fastSin(px * 30.0f + star_twinkle) * fastSin(py * 28.0f + star_twinkle * 1.3f);
            stars += max(0.0f, twinkle * 0.3f);
            
            // ORION CONSTELLATION
            float orion = 0.0f;
            for (int i = 0; i < ORION_STAR_COUNT; i++) {
                float star_x = orion_stars[i][0] * W;
                float star_y = orion_stars[i][1] * H;
                
                // Calculate distance to this star
                float dx = x - star_x;
                float dy = y - star_y;
                float dist = sqrt(dx * dx + dy * dy);
                
                // Star glow with twinkling
                if (dist < 2.0f) {
                    float star_intensity = (2.0f - dist) / 2.0f;
                    float star_twinkle = 0.7f + fastSin(t * 0.005f + i) * 0.3f;
                    orion = max(orion, star_intensity * star_twinkle);
                }
            }
            
            // COMBINE ALL ELEMENTS
            float final_value = aurora + stars * 0.8f + orion * 1.2f;
            
            // GREEN AURORA COLOR MAPPING
            uint8_t index;
            if (final_value < -0.5f) {
                // Deep space - dark blues
                index = 1 + (uint8_t)((final_value + 1.0f) * 15) % 20;
            } else if (final_value < 0.0f) {
                // Mid space - medium blues
                index = 20 + (uint8_t)((final_value + 0.5f) * 40) % 25;
            } else if (final_value < 0.5f) {
                // Aurora green range
                index = 45 + (uint8_t)(final_value * 80) % 50;
            } else if (final_value < 1.0f) {
                // Bright aurora greens
                index = 95 + (uint8_t)((final_value - 0.5f) * 60) % 40;
            } else {
                // Star white/highlights
                index = 135 + (uint8_t)((final_value - 1.0f) * 30) % 20;
            }
            
            GFX::setPixel(x, y, index);
        }
    }
}
    

     