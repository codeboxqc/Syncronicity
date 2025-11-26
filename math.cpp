

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
 
#include "pio.h"
#include "gfx.h"
#include "FFT.h"
#include "palette.h"
#include "math.h"



float sinLUT[256];
uint32_t  RandomState = 123456789;

static float sin_table[360];
static bool sin_table_init = false;

void  initmath() {
     
    
    // Precompute sine LUT
    for (int i = 0; i < 256; i++) {
        sinLUT[i] = sin((i / 256.0f) * FM_2PI);
    }
     

     for (int i = 0; i < 360; i++) {
        sin_table[i] = sinf(i * 0.0174533f); // PI/180
    }
    sin_table_init = true;

}




 


// -------------------------------------------------------------------
// Internal Color Math (standalone — no external engine needed)
// -------------------------------------------------------------------
 uint8_t saturate(float x) {
    if (x < 0.0f) return 0;
    if (x > 255.0f) return 255;
    return (uint8_t)x;
}

 uint16_t rgbToColor(uint8_t r, uint8_t g, uint8_t b) {
    // Convert 8-bit RGB888 to 16-bit RGB565
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}


 


 void hsvToRgb888(float h, float s, float v, uint8_t &r, uint8_t &g, uint8_t &b) {
    h = fmod(h, 1.0f);
    if (h < 0) h += 1.0f;
    float i = floor(h * 6.0f);
    float f = h * 6.0f - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);

    float R, G, B;
    switch ((int)i % 6) {
        case 0: R = v; G = t; B = p; break;
        case 1: R = q; G = v; B = p; break;
        case 2: R = p; G = v; B = t; break;
        case 3: R = p; G = q; B = v; break;
        case 4: R = t; G = p; B = v; break;
        case 5: R = v; G = p; B = q; break;
    }

    r = (uint8_t)(R * 255);
    g = (uint8_t)(G * 255);
    b = (uint8_t)(B * 255);
}

// Optional helpers for math palettes
 float lerp(float a, float b, float t) { return a + (b - a) * t; }
 float Pow2(float x) { return x * x; }
 float Log2(float x) { return logf(x) / logf(2.0f); }
 float wrap(float x, float minVal, float maxVal) {
    float range = maxVal - minVal;
    return minVal + fmodf((x - minVal), range);
}


 float clamp(float x, float lower, float upper) {
    return fmin(fmax(x, lower), upper);
}

 float smoothstep(float edge0, float edge1, float x) {
    x = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}


// Fast sine approximation
float fastSin(float x) {
    int idx = (int)(x * 57.2958f) % 360; // 180/PI
    if (idx < 0) idx += 360;
    return sin_table[idx];
}

float  Sin(float x) {
     
    
    // Normalize to [0, 2PI]
    x = fmod(x, FM_2PI);
    if (x < 0) x += FM_2PI;
    
    // Convert to LUT index
    int index = (int)((x / FM_2PI) * 256) & 255;
    return sinLUT[index];
}

float  Cos(float x) {
    return  Sin(x + FM_PI_2);
}




// Fast inverse square root (Quake III algorithm)
float FastInvSqrt(float x) {
    if (x <= 0.0f) return 0.0f;
    
    float xhalf = 0.5f * x;
    int i = *(int*)&x;          // Get bits for floating value
    i = 0x5f3759df - (i >> 1);  // Give initial guess y0
    x = *(float*)&i;            // Convert bits back to float
    x = x * (1.5f - xhalf * x * x); // Newton step, repeating increases accuracy
    return x;
}

float FastSqrt(float x) {
    if (x <= 0.0f) return 0.0f;
    return x * FastInvSqrt(x);
}

// Fast power of 2 approximation
float FastPow2(float x) {
    if (x < -126.0f) return 0.0f;
    if (x > 128.0f) return INFINITY;
    
    int integerPart = (int)x;
    float fractionalPart = x - integerPart;
    
    // Use bit shift for integer part, approximation for fractional
    int result = 1 << (integerPart + 127);
    float approx = 1.0f + fractionalPart * (0.6956f + fractionalPart * 0.2262f);
    
    return *(float*)&result * approx;
}

// Fast log2 approximation
float FfastLog2(float x) {
    if (x <= 0.0f) return -INFINITY;
    
    int i = *(int*)&x;
    return (((i >> 23) & 0xFF) - 127) + (i & 0x7FFFFF) / (float)0x7FFFFF;
}

// Fast exponential approximation
float FastExp(float x) {
    x = 1.0f + x / 256.0f;
    x *= x; x *= x; x *= x; x *= x;
    x *= x; x *= x; x *= x; x *= x;
    return x;
}

float Smoothstep(float edge0, float edge1, float x) {
    x = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

float Smootherstep(float edge0, float edge1, float x) {
    x = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
}

 

float RandomFloat(float min, float max) {
    return lerp(min, max, RandomFloat());
}

int RandomInt(int min, int max) {
    return min + (int)(RandomFloat() * (max - min + 1));
}

float Distance(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return FastSqrt(dx * dx + dy * dy);
}

float Length(float x, float y) {
    return FastSqrt(x * x + y * y);
}

void Normalize(float& x, float& y) {
    float len = Length(x, y);
    if (len > 0.0f) {
        float invLen = 1.0f / len;
        x *= invLen;
        y *= invLen;
    }
}

 
 

float LinearToDb(float linear) {
    if (linear <= 0.0f) return -INFINITY;
    return Log2i(linear) * 6.0f;
}

float Rms(const float* samples, int count) {
    float sum = 0.0f;
    for (int i = 0; i < count; i++) {
        sum += samples[i] * samples[i];
    }
    return FastSqrt(sum / count);
}

float Peak(const float* samples, int count) {
    float maxVal = 0.0f;
    for (int i = 0; i < count; i++) {
        float absVal = fabs(samples[i]);
        if (absVal > maxVal) maxVal = absVal;
    }
    return maxVal;
}

// Simple noise functions for visual effects
float Noise1D(float x) {
    return fmod(sin(x * 12.9898f) * 43758.5453f, 1.0f);
}



float Noise2D(float x, float y) {
    return fmod(sin(x * 12.9898f + y * 78.233f) * 43758.5453f, 1.0f);
}

float Fbm1D(float x, int octaves) {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    
    for (int i = 0; i < octaves; i++) {
        value += Noise1D(x * frequency) * amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    
    return value;
}

float Fbm2D(float x, float y, int octaves) {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    
    for (int i = 0; i < octaves; i++) {
        value += Noise2D(x * frequency, y * frequency) * amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    
    return value;
}

void RandomSeed(uint32_t seed) {
    RandomState = seed;
    
    // Ensure the seed is non-zero and has good distribution
    if (RandomState == 0) {
        RandomState = 123456789;
    }
    
    // Run the PRNG a few times to mix the seed properly
    for (int i = 0; i < 10; i++) {
        RandomFloat();
    }
}

 
float FAbs(float x) {
    return (x < 0.0f) ? -x : x;
}

// Integer versions
int Min(int a, int b) {
    return (a < b) ? a : b;
}

int  Max(int a, int b) {
    return (a > b) ? a : b;
}

int Log2i(float x) {
        union { float f; uint32_t i; } vx = { x };
        int exponent = ((vx.i >> 23) & 0xFF) - 127;
        return exponent;
    }



 uint8_t fade(uint8_t t) {
    return (t * t * t * (t * (t * 6 - 15) + 10)) >> 8;   // 8-bit fixed-point
}
 uint8_t lerp8(uint8_t a, uint8_t b, uint8_t t) {
    return a + ((b - a) * t >> 8);
}
 uint8_t grad(uint8_t hash, uint8_t x) {
    uint8_t h = hash & 7;
    uint8_t g = (h < 4) ? x : -x;
    return (h & 1) ? -g : g;
}

 uint8_t noise1D(uint8_t* table, uint8_t x) {
    uint8_t X = x;
    uint8_t i0 = X, i1 = (X + 1) & 0xFF;
    uint8_t xf = fade(X << 0);               // 0..255
    uint8_t a = grad(table[i0], X);
    uint8_t b = grad(table[i1], X + 1);
    return lerp8(a + 128, b + 128, xf) - 128;   // -128..127
}
 

 
