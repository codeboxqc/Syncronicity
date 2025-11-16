#pragma once

 
 
#include <Arduino.h>
#include <stdint.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "pio.h"
#include "gfx.h"
#include "FFT.h"
#include "palette.h"
#include "math.h"

 

 #define TRANSITION_DURATION   5000UL    
 #define PALETTE_HOLD_TIME     5000UL    

// External declarations for palette transition system
extern uint8_t currentRGB[GFX_PALETTE_SIZE][3];
extern uint8_t oldRGB[GFX_PALETTE_SIZE][3];
extern uint8_t targetRGB[GFX_PALETTE_SIZE][3];
extern bool transitioning;
extern unsigned long transitionStart;
 
void debug();

// Helper to set palette entry and update currentRGB cache
void setPalEntry(int i, uint8_t r, uint8_t g, uint8_t b);

// Update palette during transition (call every loop)
void updatePalette();

// Generate target palette into buffer
void generatePalette(int paletteIndex, uint8_t pal[GFX_PALETTE_SIZE][3]);

uint8_t rgbToPaletteIndex(uint8_t r, uint8_t g, uint8_t b) ;
void rotatePaletteNow(int steps);

void SomeFix();

/////////////////////////////////////
//typedef void (*EffectFunc)();
typedef void (*EffectFunc)(uint32_t);  // NOW ACCEPTS uint32_t
void rotatePaletteFX(int steps, EffectFunc drawEffect = nullptr);
//////////////////////////////////////////////

enum PalettePreset {
    PALETTE_SAKURA=0,
    PALETTE_KIMONO,
    PALETTE_ANIME_SKY,
    PALETTE_COTTON_CANDY,
    PALETTE_MAGICAL_GIRL,
    PALETTE_GENSHIN,
    PALETTE_STUDIO_GHIBLI,
    PALETTE_KAIJU,
    PALETTE_CYBER_ANIME,
    PALETTE_MOONLIGHT,
    PALETTE_RETRO_ANIME,
    PALETTE_OCEAN_BREEZE,
    PALETTE_SUNSET_ANIME,
    PALETTE_NEON_ANIME,
    PALETTE_FANTASY,
    PALETTE_FIRE  ,
    PALETTE_OCEAN,
    PALETTE_FOREST,
    PALETTE_SUNSET,
    PALETTE_NEON,
    PALETTE_PASTEL,
    PALETTE_RAINBOW,
    PALETTE_ICE,
    PALETTE_HEAT,
    PALETTE_AURORA,
    PALETTE_cosMIC,
    PALETTE_PSYCHEDELIC,
    PALETTE_BLACK_AND_WHITE,
    PALETTE_GRAYSCALE,
    PALETTE_ELECTRIC_STORM,
    PALETTE_AMIGA,
    PALETTE_C64,
    PALETTE_LAVA_FLOW,
    PALETTE_RETRO,
    PALETTE_RGB,
    PALETTE_TOXIC_GLOW,
    PALETTE_DEEP_SPACE,
    PALETTE_GLITCH_CORE,
    PALETTE_BLOOD_MOON,
    PALETTE_FROZEN_VOID,
    PALETTE_SPECTRUM,
    PALETTE_NUCLEAR_PULSE,
    PALETTE_LAST,
    NUM_PALETTES = PALETTE_LAST
};