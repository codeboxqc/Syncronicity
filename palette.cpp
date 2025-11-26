#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
 
#include "pio.h"
#include "gfx.h"
#include "FFT.h"

#include "palette.h"
#include "math.h"

 


 

uint8_t currentRGB[GFX_PALETTE_SIZE][3];
uint8_t targetRGB [GFX_PALETTE_SIZE][3];
bool    transitioning = false;
unsigned long transitionStart = 0;






 

uint8_t rgbToPaletteIndex(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t target = (r << 16) | (g << 8) | b;
    uint8_t bestIdx = 0;
    uint32_t bestDiff = UINT32_MAX;

    for (int i = 0; i < GFX_PALETTE_SIZE; ++i) {
        uint8_t pr = currentRGB[i][0];
        uint8_t pg = currentRGB[i][1];
        uint8_t pb = currentRGB[i][2];

        // Fast Euclidean distance squared (no sqrt)
        uint32_t dr = r - pr;
        uint32_t dg = g - pg;
        uint32_t db = b - pb;
        uint32_t diff = dr*dr + dg*dg + db*db;

        if (diff < bestDiff) {
            bestDiff = diff;
            bestIdx = i;
        }
    }
    return bestIdx;
}



void setPalEntry(int i, uint8_t r, uint8_t g, uint8_t b)
{
    // ---- 1. Write to the *real* HUB75 palette ----
    GFX::setPalette(i, r, g, b);          // <-- your library call

    // ---- 2. Keep the software cache in sync ----
    currentRGB[i][0] = r;
    currentRGB[i][1] = g;
    currentRGB[i][2] = b;

    
}

void updatePalette()
{
    if (!transitioning) return;

    unsigned long now = millis();
    unsigned long elapsed = now - transitionStart;

    // -----------------------------------------------------------------
    // 1. Fade finished → SNAP to target (and force black at 0)
    // -----------------------------------------------------------------
    if (elapsed >= TRANSITION_DURATION) {
        for (int i = 0; i < GFX_PALETTE_SIZE; ++i) {
             
               
            
                setPalEntry(i,
                            targetRGB[i][0],
                            targetRGB[i][1],
                            targetRGB[i][2]);
              // setPalEntry(0, 0, 0, 0);  // force black
             
        }
        transitioning = false;
        return;
    }

    // -----------------------------------------------------------------
    // 2. Linear interpolation (0.0 → 1.0), skip i=0
    // -----------------------------------------------------------------
    float t = (float)elapsed / TRANSITION_DURATION;

    // Force palette[0] = black during fade
   // setPalEntry(0, 0, 0, 0);

    for (int i = 1; i < GFX_PALETTE_SIZE; ++i) {  // start from 1
        uint8_t r = lerp(currentRGB[i][0], targetRGB[i][0], t);
        uint8_t g = lerp(currentRGB[i][1], targetRGB[i][1], t);
        uint8_t b = lerp(currentRGB[i][2], targetRGB[i][2], t);
        setPalEntry(i, r, g, b);
    }
}



 

// ------------------------------------------------------------------
// Rotate current palette by N steps (0-255)
// Positive = forward, Negative = backward
// ------------------------------------------------------------------
void rotatePaletteNow(int steps) {
    steps = ((steps % 256) + 256) % 256;  // wrap to 0-255

    if (steps == 0) return;

    // Temp copy of the whole palette
    uint8_t temp[GFX_PALETTE_SIZE][3];
    memcpy(temp, currentRGB, sizeof(temp));

    // Apply rotation immediately
    for (int i = 0; i < GFX_PALETTE_SIZE; ++i) {
        int src = (i + steps) % 256;
        currentRGB[i][0] = temp[src][0];
        currentRGB[i][1] = temp[src][1];
        currentRGB[i][2] = temp[src][2];
    }

    // Force index 0 = black (optional but safe)
    currentRGB[0][0] = 0;
    currentRGB[0][1] = 0;
    currentRGB[0][2] = 0;
}




// ------------------------------------------------------------------
// INSTANT Palette Rotation + Full Screen Redraw
// Call this for animation FX (e.g. button press, beat drop)
// ------------------------------------------------------------------
void rotatePaletteFX(int steps, EffectFunc drawEffect) {
    steps = ((steps % 256) + 256) % 256;
    if (steps == 0) return;

    // Rotate palette
    uint8_t temp[GFX_PALETTE_SIZE][3];
    memcpy(temp, currentRGB, sizeof(temp));

    for (int i = 0; i < GFX_PALETTE_SIZE; ++i) {
        int src = (i + steps) % 256;
        currentRGB[i][0] = temp[src][0];
        currentRGB[i][1] = temp[src][1];
        currentRGB[i][2] = temp[src][2];
    }

    currentRGB[0][0] = currentRGB[0][1] = currentRGB[0][2] = 0;  // black

    // Redraw screen
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            uint8_t idx = GFX::getPixel(x, y);
            GFX::setPixel(x, y, idx);
        }
    }

    // Call effect with current time
    if (drawEffect) drawEffect(millis());
}


void SomeFix() {

  for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    setPalEntry(i, targetRGB[i][0], targetRGB[i][1], targetRGB[i][2]);
}
setPalEntry(0, 0, 0, 0);
}

 int debugmem=0;
void debug()
{

dma_display->setCursor(0,0);
dma_display->printf("%d",debugmem);
//delay(200);

}

////////////////////////////////////////////////////////////////////////////////////
//
//
//
///////////////////////////////////////////////////////////////////////////////////
void generatePalette(int paletteIndex, uint8_t pal[GFX_PALETTE_SIZE][3]) { 



 debugmem =  paletteIndex;


switch(paletteIndex) {
    
     
case PALETTE_SAKURA: // Pink cherry blossom theme
  for (int i = 0; i < 64; i++) {

     

        pal[i][0] = i*4; pal[i][1] = 0; pal[i][2] = 0;           // Dark red to bright red
        pal[64+i][0] = 255; pal[64+i][1] = i*4; pal[64+i][2] = 0;      // Red to yellow
        pal[128+i][0] = 255; pal[128+i][1] = 255; pal[128+i][2] = i*4;   // Yellow to white
    }
   
  break;

case PALETTE_KIMONO: for (int i = 0; i < 64; i++) {

   

    pal[i][0] = 80 + i*2; pal[i][1] = 20 + i; pal[i][2] = 40 + i*2;     // Deep pink to medium pink
    pal[128+i][0] = 180 + i; pal[128+i][1] = 60 + i*2; pal[128+i][2] = 200 + i;  // Pink to light pink
    pal[64+i][0] = 255; pal[64+i][1] = 180 + i; pal[64+i][2] = 100 + i;      // Light pink to pale pink
  }
  break;

case PALETTE_ANIME_SKY: 
 static float plasmaTime = 0.0f;
plasmaTime += 0.01f; // Plasma animation speed

for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / (GFX_PALETTE_SIZE - 1);
    
    
    
    // Plasma-style multiple sine waves for rainbow effect
    float plasma1 = Sin(t * 8.0f + plasmaTime);
    float plasma2 = Sin(t * 12.0f + plasmaTime * 1.3f);
    float plasma3 = Sin(t * 6.0f + plasmaTime * 0.7f);
    
    // Combine waves for complex plasma pattern
    float plasma = (plasma1 + plasma2 + plasma3) / 3.0f;
    
    // Rainbow hue rotation through entire spectrum
    float h = fmod(t + plasmaTime * 0.2f + plasma * 0.3f, 1.0f);
    
    // Full saturation for vibrant rainbow colors
    float s = 1.0f;
    
    // Brightness with plasma modulation and fade from black
    float fadeIn = (float)i / (GFX_PALETTE_SIZE * 0.15f);
    fadeIn = fadeIn > 1.0f ? 1.0f : fadeIn;
    float v = 0.9f + 0.1f * plasma * fadeIn;
    
    uint8_t r, g, b;
    hsvToRgb888(h, s, v, r, g, b);
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
     pal[0][0] = 0; pal[0][1] = 0; pal[0][2] = 0;
}
  break;


case PALETTE_COTTON_CANDY: // Soft cotton candy
  for (int i = 0; i < GFX_PALETTE_SIZE; i++) {

    

    float t = (float)i / GFX_PALETTE_SIZE;
    float h = 0.83f + 0.2f * Sin(t * FM_2PI * 2.0f);
    float s = 0.6f;
    float v = 0.9f + 0.1f * cos(t * FM_2PI * 3.0f);

    uint8_t r, g, b;
    hsvToRgb888(h, s, v, r, g, b);
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
  }
  break;

case PALETTE_MAGICAL_GIRL: // Magical girl transformation
  for (int i = 0; i < GFX_PALETTE_SIZE; i++) {

    
    float t = (float)i / GFX_PALETTE_SIZE;
    float h = t * 0.7f + 0.3f * Sin(t * FM_2PI * 5.0f);
    float s = 0.8f + 0.2f * cos(t * FM_2PI * 4.0f);
    float v = 0.9f + 0.1f * Sin(t * FM_2PI * 6.0f);

    uint8_t r, g, b;
    hsvToRgb888(h, s, v, r, g, b);
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
  }
  break;

case PALETTE_GENSHIN: // Genshin Impact style

static float plasmaTime2 = 0.0f;
plasmaTime2 += 0.012f;

for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / (GFX_PALETTE_SIZE - 1);
    
    
    
    // Plasma waves
    float p1 = Sin(t * 9.0f + plasmaTime2);
    float p2 = cos(t * 6.0f + plasmaTime2 * 1.4f);
    float plasma = (p1 + p2 * 0.8f) / 1.8f;
    
    // Cyan to magenta range
    float h = fmod(0.5f + plasma * 0.3f + plasmaTime2 * 0.08f, 1.0f);
    
    // High saturation for electric look
    float s = 0.9f + 0.1f * Sin(t * 4.0f + plasmaTime);
    
    // Bright with fade from black
    float fadeIn = smoothstep(0.0f, 0.08f, t);
    float v = (0.9f + 0.1f * plasma) * fadeIn;
    
    uint8_t r, g, b;
    hsvToRgb888(h, s, v, r, g, b);
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
}
 
  break;



case PALETTE_STUDIO_GHIBLI:  

 for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    uint8_t r = 150 + (uint8_t)(105 * Sin(t * FM_2PI * 6.0f));
    uint8_t g = 128 + (uint8_t)(100 * Sin(t * FM_2PI * 5.0f + 1.8f));
    uint8_t b = 100 + (uint8_t)(80 * Sin(t * FM_2PI * 7.0f + 3.6f));
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
}

  break;

case PALETTE_KAIJU: // Monster/Kaiju theme
   for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    uint8_t r = 120 + (uint8_t)(100 * Sin(t * FM_2PI * 8.0f + 1.0f));
    uint8_t g = 110 + (uint8_t)(90 * Sin(t * FM_2PI * 7.0f + 2.5f));
    uint8_t b = 140 + (uint8_t)(115 * Sin(t * FM_2PI * 9.0f + 4.0f));
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
}
  break;

case PALETTE_CYBER_ANIME: // Cyberpunk anime
  for (int i = 0; i < GFX_PALETTE_SIZE; i++) {

    

    float t = (float)i / GFX_PALETTE_SIZE;
    float h = 0.75f + 0.2f * Sin(t * FM_2PI * 4.0f);
    float s = 0.9f;
    float v = 0.7f + 0.3f * pow(t * 1.5f, 2);

    uint8_t r, g, b;
    hsvToRgb888(h, s, v, r, g, b);
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
  }
  break;

case PALETTE_MOONLIGHT: // Romantic moonlight – now with more energy!
{
    const float freqHue   = 2.5f;   // how many hue cycles
    const float freqSat   = 5.0f;   // saturation pulses
    const float freqVal   = 7.0f;   // value flicker
    const float ampSat    = 0.35f;  // saturation swing
    const float ampVal    = 0.30f;  // value swing
    const float noiseAmp  = 0.07f;  // tiny per-entry jitter (remove if unwanted)

    for (int i = 0; i < GFX_PALETTE_SIZE; ++i) {
        float t = (float)i / (GFX_PALETTE_SIZE - 1);  
        
        

        // ----- Hue: purple → indigo → cyan -----
        float h = 0.68f + 0.22f * sinf(t * FM_PI * freqHue);      // 0.68 .. 0.90

        // ----- Saturation: rapid multi-wave pulse -----
        float s = 0.55f + ampSat * (0.5f + 0.5f *
                    sinf(t * FM_PI * freqSat + FM_PI * 0.33f));   // 0.20 .. 0.90

        // ----- Value: base ramp + high-freq flicker -----
        float v = 0.55f + 0.35f * t                               // linear ramp
                + ampVal * sinf(t * FM_PI * freqVal);             // fast flicker

        // ----- Optional per-entry noise (makes it shimmer) -----
        float noise = noiseAmp * (2.0f * rand()/(float)RAND_MAX - 1.0f);
        v = fminf(1.0f, fmaxf(0.0f, v + noise));

        uint8_t r, g, b;
        hsvToRgb888(h, s, v, r, g, b);
        pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
    }
    break;
}


case PALETTE_RETRO_ANIME: // 90s anime vibe
  for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;

     

    uint8_t r = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 3.0f));
    uint8_t g = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 5.0f + 1.0f));
    uint8_t b = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 7.0f + 2.0f));
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
  }
  break;

case PALETTE_OCEAN_BREEZE: // Fresh ocean colors

    
 for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    uint8_t r = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 7.0f));
    uint8_t g = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 7.0f + FM_2PI/2.0f)); // 90° out of phase
    uint8_t b = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 3.5f)); // Half frequency
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
}

  break;

case PALETTE_SUNSET_ANIME: // Anime sunset
  

    for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
        float t = (float)i / (GFX_PALETTE_SIZE - 1); // 0.0 to 1.0

        

        // --- Simulate radial distance in a circle (center = bright, edge = soft) ---
        float radial = fabsf(sinf(t * FM_PI)); // 0 (edge) → 1 (center) → 0 (edge)

        // --- Circular hue: full 360° orbit (warm → hot → warm) ---
        float angle = t * FM_PI * 2.0f * 3.0f; // 3 full hue rotations
        float h = 0.05f + 0.12f * (0.5f + 0.5f * cosf(angle)); // Orange → Yellow → Pink → Orange

        // --- Saturation: high in core, fades slightly outward ---
        float s = 0.95f - 0.25f * (1.0f - radial); // 0.70 at edge → 0.95 at center

        // --- Value: bright core + radiant pulses + micro-shimmer ---
        float v = 0.75f + 0.25f * radial;                    // base radial glow
        v += 0.12f * sinf(t * FM_PI * 8.0f);                 // fast energy ripples
        v += 0.08f * sinf(t * FM_PI * 20.0f + 1.8f);         // shimmer
        v = fminf(1.0f, v);

        // --- Golden sparkle (high-frequency highlight bursts) ---
        float sparkle = sinf(t * FM_PI * 47.0f) * sinf(t * FM_PI * 31.0f);
        if (sparkle > 0.7f) v = fminf(1.0f, v + 0.1f * (sparkle - 0.7f) * 10.0f);

        uint8_t r, g, b;
        hsvToRgb888(h, s, v, r, g, b);
        pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
    }
    break;



case PALETTE_NEON_ANIME: // Bright anime neon
  for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    float h = t + 0.2f * Sin(t * FM_2PI * 6.0f);
    float s = 1.0f;
    float v = 0.9f + 0.1f * cos(t * FM_2PI * 8.0f);

    
    uint8_t r, g, b;
    hsvToRgb888(h, s, v, r, g, b);
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
  }
  break;

case PALETTE_FANTASY: // Fantasy RPG colors

    for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
        float t = (float)i / (GFX_PALETTE_SIZE - 1); // 0.0 to 1.0

        // --- Radial "shockwave" distance (center = blast core) ---
        float radial = fabsf(sinf(t * FM_PI)); // 0 (edge) → 1 (center) → 0

        // --- Hue: toxic cycle (green → cyan → purple → orange → white) ---
        float angle = t * FM_PI * 2.0f * 4.0f; // 4 full hue rotations
        float h = fmodf(
            0.3f +                                          // base: toxic green
            0.1f * sinf(angle) +                            // green ↔ cyan
            0.15f * sinf(angle * 2.1f + 1.3f) +             // purple pulses
            0.08f * sinf(angle * 3.7f + 2.1f), 1.0f);       // orange spikes

        // --- Saturation: oversaturated in core, sickly at edges ---
        float s = 0.85f + 0.15f * radial;                    // 0.85 → 1.0
        s -= 0.3f * (1.0f - radial);                         // edge desaturation
        s = fminf(1.0f, fmaxf(0.4f, s));

        // --- Value: blinding core + shock rings + flicker ---
        float v = 0.4f + 0.6f * radial;                      // base radial glow
        v += 0.18f * sinf(t * FM_PI * 12.0f);                // fast shockwaves
        v += 0.12f * sinf(t * FM_PI * 28.0f + 0.9f);          // gamma flicker

        // --- Geiger-like random bursts (simulated with high-freq sine) ---
        float burst = sinf(t * FM_PI * 51.0f) * sinf(t * FM_PI * 37.0f + 1.4f);
        if (burst > 0.75f) {
            v = fminf(1.0f, v + 0.25f);
            s = fminf(1.0f, s + 0.15f);
            h = fmodf(h + 0.05f, 1.0f); // brief color spike
        }

        // --- Fallout ash fade at very edges ---
        if (radial < 0.2f || radial > 0.8f) {
            s *= 0.7f;
            v *= 0.8f;
        }

        v = fminf(1.0f, v);

        uint8_t r, g, b;
        hsvToRgb888(h, s, v, r, g, b);
        pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
    }
    break;

case PALETTE_FIRE: {                     // Warm fire gradient (black to red to yellow to white)
    for (int i = 0; i < GFX_PALETTE_SIZE; ++i) {
        float t = (float)i / (GFX_PALETTE_SIZE - 1);   // 0.0 to 1.0

        float h = 0.0f, s = 1.0f, v = 0.0f;            // HSV for this index

        /* --------------------------------------------------------------
           Three flame zones
           -------------------------------------------------------------- */
        if (t < 0.35f) {                     // deep red embers (base)
            h = 0.0f;                         // pure red
            s = 1.0f;
            v = 0.4f + 0.4f * (t / 0.35f);    // 0.4 to 0.8
        }
        else if (t < 0.75f) {                // orange inferno (core)
            float local = (t - 0.35f) / 0.4f; // 0 to 1
            h = 0.055f * local;              // red to orange
            s = 1.0f - 0.1f * local;
            v = 0.8f + 0.15f * local;        // 0.8 to 0.95
        }
        else {                               // yellow-white tips (peak)
            float local = (t - 0.75f) / 0.25f; // 0 to 1
            h = 0.055f + 0.045f * local;     // orange to yellow
            s = 0.9f - 0.9f * local;         // full to white
            v = 0.95f + 0.05f * local;       // near-max
        }

        /* --------------------------------------------------------------
           Heat shimmer (fast flicker)
           -------------------------------------------------------------- */
        float shimmer = sinf(t * FM_PI * 18.0f) *
                        sinf(t * FM_PI * 11.0f + 1.3f);
        v += 0.08f * shimmer;                // plus/minus 8 percent flicker

      
        float spark = sinf(t * FM_PI * 47.0f) *
                      sinf(t * FM_PI * 29.0f + 2.1f);
        if (spark > 0.8f) {
            v = fminf(1.0f, v + 0.18f);
            s = fminf(1.0f, s + 0.10f);
        }

        /* --------------------------------------------------------------
           Micro-turbulence in the orange zone
           -------------------------------------------------------------- */
        if (t > 0.35f && t < 0.75f) {
            float turbulence = 0.05f * sinf(t * FM_PI * 31.0f + 0.7f);
            h = fmodf(h + turbulence, 1.0f);
        }

        /* --------------------------------------------------------------
           Clamp value (never go below 0.3 so the fire never looks dead)
           -------------------------------------------------------------- */
        v = fminf(1.0f, fmaxf(0.3f, v));

        /* --------------------------------------------------------------
           Convert HSV to RGB to target buffer
           -------------------------------------------------------------- */
        uint8_t r, g, b;
        hsvToRgb888(h, s, v, r, g, b);
        pal[i][0] = r;
        pal[i][1] = g;
        pal[i][2] = b;
    }
    break;
}

 

case PALETTE_OCEAN:  
for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    uint8_t r = 100 + (uint8_t)(90 * Sin(t * FM_2PI * 6.0f));
    uint8_t g = 140 + (uint8_t)(115 * Sin(t * FM_2PI * 7.0f + 1.2f));
    uint8_t b = 160 + (uint8_t)(95 * Sin(t * FM_2PI * 8.0f + 2.4f));
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
}
    break;

case PALETTE_FOREST: // Earthy greens
   
     for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    uint8_t r = 110 + (uint8_t)(80 * Sin(t * FM_2PI * 4.5f));
    uint8_t g = 120 + (uint8_t)(70 * Sin(t * FM_2PI * 5.5f + 1.7f));
    uint8_t b = 130 + (uint8_t)(60 * Sin(t * FM_2PI * 6.5f + 3.4f));
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
}
    break;



case PALETTE_SUNSET: 

static float acidTime = 0.0f;
acidTime += 0.02f; // Fast animation for trippy effect

for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / (GFX_PALETTE_SIZE - 1);
    
    if (i == 0) {
        pal[0][0] = 0; pal[0][1] = 0; pal[0][2] = 0;
        continue;
    }
    
    // Multiple overlapping waves for intense patterns
    float wave1 = Sin(t * 15.0f + acidTime * 3.0f);
    float wave2 = cos(t * 23.0f + acidTime * 4.2f);
    float wave3 = Sin(t * 31.0f + acidTime * 2.7f);
    float wave4 = cos(t * 47.0f + acidTime * 5.1f);
    float wave5 = Sin(t * 59.0f + acidTime * 1.8f);
    
    // Complex combination with different weights
    float pattern = (wave1 + wave2 * 1.3f + wave3 * 0.8f + wave4 * 1.7f + wave5 * 0.5f) / 5.3f;
    
    // Rapid hue cycling through entire spectrum
    float h = fmod(pattern * 2.0f + t * 1.5f + acidTime * 0.8f, 1.0f);
    
    // Maximum saturation for intense colors
    float s = 1.0f;
    
    // Pulsating brightness for strobing effect
    float v = 0.9f + 0.3f * Sin(t * 25.0f + acidTime * 6.0f);
    
    // Quick fade from black
    float fadeIn = smoothstep(0.0f, 0.03f, t);
    v *= fadeIn;
    
    uint8_t r, g, b;
    hsvToRgb888(h, s, v, r, g, b);
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
}
  break;

case PALETTE_NEON: // Bright 6-segment neon
 static float tripTime = 0.0f;
tripTime += 0.025f;

for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    if (i == 0) {
        pal[0][0] = 0; pal[0][1] = 0; pal[0][2] = 0;
        continue;
    }
    
    float h = (float)i / GFX_PALETTE_SIZE;
    
    // Different speeds AND phase offsets
    float r = Sin(h * FM_2PI * 7.0f + tripTime * 0.3f + 0.0f) * 0.5f + 0.5f;
    float g = Sin(h * FM_2PI * 13.0f + tripTime * 0.9f + 2.0f) * 0.5f + 0.5f;
    float b = Sin(h * FM_2PI * 23.0f + tripTime * 1.5f + 4.0f) * 0.5f + 0.5f;
    
    pal[i][0] = (uint8_t)(r * 255);
    pal[i][1] = (uint8_t)(g * 255);
    pal[i][2] = (uint8_t)(b * 255);
}
  break;


case 20:

for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    // Exponential mapping for longer subtle sections
    float exp_t = 1.0f - exp(-t * 3.0f);
    
    float r = Sin(exp_t * FM_2PI * 2.0f) * 0.5f + 0.5f;
    float g = Sin(exp_t * FM_2PI * 2.3f + 1.0f) * 0.5f + 0.5f;
    float b = Sin(exp_t * FM_2PI * 2.6f + 2.0f) * 0.5f + 0.5f;
    pal[i][0] = (uint8_t)(r * 255); 
    pal[i][1] = (uint8_t)(g * 255); 
    pal[i][2] = (uint8_t)(b * 255);
}

  break;

  case 21:
  for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    // Cubic easing for ultra-smooth transitions
    float smooth_t = t * t * (3.0f - 2.0f * t);
    
    float r = Sin(smooth_t * FM_2PI * 1.5f) * 0.5f + 0.5f;
    float g = Sin(smooth_t * FM_2PI * 2.0f + FM_2PI/3.0f) * 0.5f + 0.5f;
    float b = Sin(smooth_t * FM_2PI * 2.5f + 2.0f*FM_2PI/3.0f) * 0.5f + 0.5f;
    pal[i][0] = (uint8_t)(r * 255); 
    pal[i][1] = (uint8_t)(g * 255); 
    pal[i][2] = (uint8_t)(b * 255);
}
    break;

 case 22:
 for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    // Very close frequencies create slow "beating" patterns
    float r = Sin(t * FM_2PI * 1.0f) * 0.5f + 0.5f;
    float g = Sin(t * FM_2PI * 1.1f) * 0.5f + 0.5f;
    float b = Sin(t * FM_2PI * 1.2f) * 0.5f + 0.5f;
    pal[i][0] = (uint8_t)(r * 255); 
    pal[i][1] = (uint8_t)(g * 255); 
    pal[i][2] = (uint8_t)(b * 255);
}
    break;

case PALETTE_HEAT: // Black → red → yellow → white
  for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / (GFX_PALETTE_SIZE - 1);
    float s = 1.0f;
    float v = t;
    if (t > 0.66f) s = 1.0f - (t - 0.66f) * 3.0f;
    uint8_t r, g, b;
    hsvToRgb888(0.0f, s, v, r, g, b);
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
  }
  break;






case PALETTE_AURORA: // Northern lights shimmer
  for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    float h = 0.55f + 0.15f * Sin(t * FM_2PI * 4.0f);
    float s = 0.7f + 0.3f * Sin(t * FM_2PI * 3.0f);
    float v = 0.8f + 0.2f * Sin(t * FM_2PI * 3.5f);
    uint8_t r, g, b;
    hsvToRgb888(h, s, v, r, g, b);
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
  }
  break;

case PALETTE_cosMIC: // Deep-space nebula
  for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = max(0.0001f, (float)i / (GFX_PALETTE_SIZE - 1));
    float logt = Log2(t);
    float r = 80.0f * Pow2(0.3f * logt) + 50.0f * Sin(t * FM_2PI * 5.0f);
    float g = 60.0f * Pow2(0.5f * logt) + 40.0f * Sin(t * FM_2PI * 4.0f + 1.0f);
    float b = 255.0f * Pow2(0.8f * logt) + 30.0f * Sin(t * FM_2PI * 6.0f + 2.0f);
    pal[i][0] = (uint8_t)saturate(r); pal[i][1] = (uint8_t)saturate(g); pal[i][2] = (uint8_t)saturate(b);
  }
  break;

case PALETTE_PSYCHEDELIC:
 for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    // Sub-1.0 frequencies for very slow evolution
    uint8_t r = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 0.7f));
    uint8_t g = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 0.9f + 1.57f));
    uint8_t b = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 1.2f + 3.14f));
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
}
break;

case 27:

for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    uint8_t r = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 2.7f));
    uint8_t g = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 1.9f + 1.8f));
    uint8_t b = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 3.2f + 3.6f));
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
}
  
  break;

  case 28:
   for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    // Deep blue -> purple -> red -> orange -> yellow -> white
    float r = fmin(1.0f, 0.8f * t + 0.4f * Sin(t * FM_2PI * 3.0f) + 0.2f);
    float g = fmin(1.0f, 0.6f * t + 0.3f * Sin(t * FM_2PI * 2.5f + 1.0f));
    float b = fmax(0.0f, 0.4f - t * 0.6f + 0.2f * Sin(t * FM_2PI * 4.0f + 2.0f));
    pal[i][0] = (uint8_t)(r * 255); 
    pal[i][1] = (uint8_t)(g * 255); 
    pal[i][2] = (uint8_t)(b * 255);
}
  break;

case PALETTE_ELECTRIC_STORM:
  for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    float h = 0.66f + 0.15f * Sin(t * FM_2PI * 2.5f);
    float s = 0.9f;
    float v = 0.4f + 0.6f * Pow2(t * 0.7f);
    uint8_t r, g, b;
    hsvToRgb888(h, s, v, r, g, b);
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
  }
  break;


  case PALETTE_AMIGA:  
   for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    // Green -> yellow -> orange fire (magical/poison flame)
    float r = 0.4f * t + 0.3f * Sin(t * FM_2PI * 2.0f) + 0.2f;
    float g = 0.7f * t + 0.2f * Sin(t * FM_2PI * 2.5f + 1.0f);
    float b = 0.3f * (1.0f - t) + 0.1f * Sin(t * FM_2PI * 3.0f + 2.0f);
    pal[i][0] = (uint8_t)(r * 255); 
    pal[i][1] = (uint8_t)(g * 255); 
    pal[i][2] = (uint8_t)(b * 255);
}
  break;

case PALETTE_C64:  
for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    // Pastel rainbow: pink -> lavender -> blue -> teal -> gold
    float r = 0.7f + 0.3f * Sin(t * FM_2PI * 1.2f);
    float g = 0.5f + 0.3f * Sin(t * FM_2PI * 1.5f + FM_2PI/3.0f);
    float b = 0.8f + 0.2f * Sin(t * FM_2PI * 1.8f + 2.0f*FM_2PI/3.0f);
    pal[i][0] = (uint8_t)(r * 255); 
    pal[i][1] = (uint8_t)(g * 255); 
    pal[i][2] = (uint8_t)(b * 255);
}

  break;


case PALETTE_LAVA_FLOW:
   for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    // Cyan -> blue -> purple with ethereal glow
    float r = 0.3f + 0.4f * Sin(t * FM_2PI * 1.5f + 2.0f);
    float g = 0.5f + 0.3f * Sin(t * FM_2PI * 1.3f + 1.0f);
    float b = 0.8f + 0.2f * Sin(t * FM_2PI * 1.1f);
    pal[i][0] = (uint8_t)(r * 255); 
    pal[i][1] = (uint8_t)(g * 255); 
    pal[i][2] = (uint8_t)(b * 255);
}
  break;


  case PALETTE_RETRO:  
static float casinoTime = 0.0f;
casinoTime += 0.012f;

for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / (GFX_PALETTE_SIZE - 1);
    
    if (i == 0) {
        pal[0][0] = 0; pal[0][1] = 0; pal[0][2] = 0;
        continue;
    }
    
    // Rich casino colors: gold, purple, deep red
    float wave1 = Sin(t * 6.0f + casinoTime * 2.5f);
    float wave2 = cos(t * 15.0f + casinoTime * 4.1f);
    
    // Luxury casino color rotation
    float colorCycle = fmod(t * 3.0f + casinoTime * 0.2f, 3.0f);
    float baseHue;
    
    if (colorCycle < 1.0f) baseHue = 0.08f;  // Rich gold
    else if (colorCycle < 2.0f) baseHue = 0.83f; // Royal purple
    else baseHue = 0.98f;                    // Deep magenta
    
    float h = fmod(baseHue + wave1 * 0.05f, 1.0f);
    
    // High saturation for luxury colors
    float s = 0.9f + 0.1f * wave2;
    
    // Steady bright lighting
    float v = 0.85f;
    
    // Gentle fade from black
    float fadeIn = smoothstep(0.0f, 0.08f, t);
    v *= fadeIn;
    
    uint8_t r, g, b;
    hsvToRgb888(h, s, v, r, g, b);
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
}
break;

case 34:
for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    // Sub-1.0 frequencies for very slow evolution
    uint8_t r = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 0.7f));
    uint8_t g = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 0.9f + 1.57f));
    uint8_t b = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 1.2f + 3.14f));
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
}
break;

case PALETTE_TOXIC_GLOW:
  
static float mathTime = 0.0f;
mathTime += 0.006f;

for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / (GFX_PALETTE_SIZE - 1);
    
  
    // Complex plane coordinates
    float x = (t - 0.5f) * 3.0f;
    float y = Sin(t * FM_PI * 2.0f) * 1.5f;
    
    // Mandelbrot iteration: z = z² + c
    float zx = 0.0f, zy = 0.0f;
    float iter = 0.0f;
    
    for (int j = 0; j < 20; j++) {
        float zx2 = zx * zx;
        float zy2 = zy * zy;
        
        // Escape condition
        if (zx2 + zy2 > 4.0f) {
            iter = (float)j / 20.0f;
            break;
        }
        
        // Mandelbrot formula
        float new_zx = zx2 - zy2 + x;
        zy = 2.0f * zx * zy + y;
        zx = new_zx;
    }
    
    // Color based on iteration count and escape time
    float h = fmod(iter * 2.5f + mathTime * 0.1f, 1.0f);
    float s = 0.8f + 0.2f * Sin(iter * 10.0f);
    float v = 0.7f + 0.3f * (1.0f - iter);
    
    float fadeIn = smoothstep(0.0f, 0.1f, t);
    v *= fadeIn;
    
    uint8_t r, g, b;
    hsvToRgb888(h, s, v, r, g, b);
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
}

  break;

case PALETTE_DEEP_SPACE:
   for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    // Electric blue -> pink -> purple (cyberpunk style)
    float r = 0.6f + 0.4f * Sin(t * FM_2PI * 1.8f + 1.57f);
    float g = 0.2f + 0.3f * Sin(t * FM_2PI * 2.2f);
    float b = 0.8f + 0.2f * Sin(t * FM_2PI * 1.6f + 3.14f);
    pal[i][0] = (uint8_t)(r * 255); 
    pal[i][1] = (uint8_t)(g * 255); 
    pal[i][2] = (uint8_t)(b * 255);
}
  break;

case PALETTE_GLITCH_CORE:
  for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    uint8_t r = (uint8_t)(255.0f * wrap(t * 7.0f, 0.0f, 1.0f));
    uint8_t g = (uint8_t)(255.0f * wrap(t * 11.0f, 0.0f, 1.0f));
    uint8_t b = (uint8_t)(255.0f * wrap(t * 13.0f, 0.0f, 1.0f));
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
  }
  break;

case PALETTE_BLOOD_MOON:
   for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    uint8_t r = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 5.5f));
    uint8_t g = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 4.5f + 1.5f));
    uint8_t b = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 6.5f + 3.0f));
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
}
  break;

case PALETTE_FROZEN_VOID:
 for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    uint8_t r = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 9.5f));
    uint8_t g = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 11.5f + 2.2f));
    uint8_t b = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 13.5f + 4.4f));
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
}
  break;

  case PALETTE_SPECTRUM: // ZX Spectrum inspired
  for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    uint8_t r = ((i & 0x1C) << 3);
    uint8_t g = ((i * 5) & 0xC0);
    uint8_t b = ((i * 7) & 0xC0);
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
  }
  break;

case PALETTE_NUCLEAR_PULSE:
for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    // Slow base waves with subtle high-frequency detail
    float r_wave = 0.7f * Sin(t * FM_2PI * 0.8f) + 0.3f * Sin(t * FM_2PI * 5.2f);
    float g_wave = 0.7f * Sin(t * FM_2PI * 1.1f + 1.0f) + 0.3f * Sin(t * FM_2PI * 6.1f + 1.0f);
    float b_wave = 0.7f * Sin(t * FM_2PI * 0.9f + 2.0f) + 0.3f * Sin(t * FM_2PI * 4.8f + 2.0f);
    
    uint8_t r = 128 + (uint8_t)(127 * fmax(-1.0f, fmin(1.0f, r_wave)));
    uint8_t g = 128 + (uint8_t)(127 * fmax(-1.0f, fmin(1.0f, g_wave)));
    uint8_t b = 128 + (uint8_t)(127 * fmax(-1.0f, fmin(1.0f, b_wave)));
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
}
break;

default: // Default rainbow
  for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
    float t = (float)i / GFX_PALETTE_SIZE;
    uint8_t r = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 9.5f));
    uint8_t g = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 11.5f + 2.2f));
    uint8_t b = 128 + (uint8_t)(127 * Sin(t * FM_2PI * 13.5f + 4.4f));
    pal[i][0] = r; pal[i][1] = g; pal[i][2] = b;
}
  break;
}

}








