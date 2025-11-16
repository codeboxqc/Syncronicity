#pragma once

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
 
#include "pio.h"
#include "gfx.h"
#include "FFT.h"
#include "palette.h"
#include "math.h"


#define FM_PI 3.14159265359f
#define FM_2PI 6.28318530718f
#define FM_PI_2 1.57079632679f
#define FM_INV_PI 0.31830988618f
#define FM_INV_2PI 0.15915494309f



uint8_t saturate(float x);
 uint16_t rgbToColor(uint8_t r, uint8_t g, uint8_t b);
  void hsvToRgb888(float h, float s, float v, uint8_t &r, uint8_t &g, uint8_t &b);

void  initmath();
  float lerp(float a, float b, float t) ;
 float Pow2(float x) ;
 float Log2(float x) ;
 float wrap(float x, float minVal, float maxVal);
  float clamp(float x, float lower, float upper) ;
 float smoothstep(float edge0, float edge1, float x) ;


 float  Sin(float x);
 float  Cos(float x);
 float fastSin(float x);

 
 
float FastInvSqrt(float x) ;
float FastSqrt(float x) ;
float FastPow2(float x) ;
float FfastLog2(float x) ;
float FastExp(float x) ;
float Smoothstep(float edge0, float edge1, float x) ;
float Smootherstep(float edge0, float edge1, float x); 
float RandomFloat() ;
float RandomFloat(float min, float max) ;
int RandomInt(int min, int max) ;
float Distance(float x1, float y1, float x2, float y2) ;
float Length(float x, float y) ;
void Normalize(float& x, float& y) ;
float LinearToDb(float linear);
float Rms(const float* samples, int count) ;
float Peak(const float* samples, int count); 
float Noise1D(float x) ;
float Noise2D(float x, float y) ;
float Fbm1D(float x, int octaves) ;
float Fbm2D(float x, float y, int octaves) ;
void RandomSeed(uint32_t seed); 
float FAbs(float x)  ;
int Min(int a, int b) ; 
int  Max(int a, int b) ;
int  Log2i(float x) ; 


 uint8_t fade(uint8_t t)  ;
 uint8_t lerp8(uint8_t a, uint8_t b, uint8_t t) ;
 uint8_t grad(uint8_t hash, uint8_t x) ;
 uint8_t noise1D(uint8_t* table, uint8_t x)  ;