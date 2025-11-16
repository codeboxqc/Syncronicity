
#pragma once

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
 
#include "pio.h"
#include "gfx.h"
#include "FFT.h"
#include "palette.h"
#include "math.h"


#define W WIDTH  
#define H HEIGHT  
 
typedef struct {
    uint8_t* body;            // Plasma buffer
    uint8_t palette[257][3];  // 256-color palette
    int cosinus[256];         // Cosine table
    int width;
    int height;
} Plasma;

//extern Plasma plasma;

void initPlasma(Plasma* plasma);
void drawPlasma(Plasma* plasma);
void updatePlasma(Plasma* plasma);


 

struct AnimePlasma {
    uint8_t body[256 * 256];   // pre-allocated index buffer
    uint8_t cosTable[256];     // pre-computed cosinus -> 0-60 (scaled later)
    uint8_t p1, p2, p3, p4;    // phase counters
    uint8_t offset[4];         // random start offset (makes it unique)
};

void initAnimePlasma(AnimePlasma* p);
void updateAnimePlasma(AnimePlasma* p);
void drawAnimePlasma(const AnimePlasma* p);


///////////////////////////////////////////////
/////////////////////////////////////////
// Struct definition fire
struct Flame {
    uint8_t* buffer;        // main fire buffer
    uint8_t* fire_buffer;   // temp buffer for propagation
    uint16_t width, height;
};

void initFlame(Flame* f);
void updateFlame(Flame* f);
void drawFlame(const Flame* f);
 

///////////////////////////////////////////////////

 
  void drawFilledRect(int x, int y, int w, int h, uint8_t paletteIndex);
  float easeInOutExpo(float x);
  




void RadialGlow(uint32_t time);
void BioForest(float t);
void OrganicLandscape(float t);
void FloatingIslands(float t);
void MysticalSwamp(float t);
void OceanDepths(float t);
void VolcanicLandscape(float t);
void AuroraSky(float t);
void GooGlow(float t);
void AnimeMountainZoom(float t);
void CherryBlossomGarden(float t);
void MagicalForestPath(float t);
void AnimeCitySkyline(float t);
void GreenAuroraOrionNight(float t) ;