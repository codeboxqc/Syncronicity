#pragma once

// gfx.h
 
#include <Arduino.h>
#include <stdint.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#include "pio.h"


#define GFX_WIDTH WIDTH
#define GFX_HEIGHT HEIGHT
#define GFX_PALETTE_SIZE 256




#define hack 0          // 0 → use fliper(), 1 → use flip()/copyRow()

// forward declaration (the library already does this, keep for clarity)
class MatrixPanel_I2S_DMA;

class GFX {
public:
    // -----------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------
    static void init(MatrixPanel_I2S_DMA* display = nullptr);
    static void fadeToBlack(uint8_t amount);
    static void setPalette(uint8_t idx, uint8_t r, uint8_t g, uint8_t b);
    static void getPalette(uint8_t idx, uint8_t* r, uint8_t* g, uint8_t* b);

    // pixel access
    static inline void setPixel(int16_t x, int16_t y, uint8_t col) {
        if (x < 0 || x >= GFX_WIDTH || y < 0 || y >= GFX_HEIGHT) return;
        backbuf[y * GFX_WIDTH + x] = col;
    }
    static inline void pixel(int16_t x, int16_t y, uint8_t col) { setPixel(x, y, col); }
    static inline uint8_t getPixel(int16_t x, int16_t y) {
        if (x < 0 || x >= GFX_WIDTH || y < 0 || y >= GFX_HEIGHT) return 0;
        return backbuf[y * GFX_WIDTH + x];
    }

    static MatrixPanel_I2S_DMA* getDisplay() { return dma_display; }

    // primitive drawing
    static   void line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t col);
    static   void vline(int16_t x, int16_t y, int16_t h, uint8_t col);
    static   void hline(int16_t x, int16_t y, int16_t w, uint8_t col);
    static   void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t col);
    static   void rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t col);
    static   void circle(int16_t x0, int16_t y0, int16_t r, uint8_t col);
    static   void fillCircle(int16_t x0, int16_t y0, int16_t r, uint8_t col);
    static   void clear(uint8_t col = 0);

#if (hack == 1)
    static void flip();
    static void flipMemcpy();
    static void copyRow(int y, uint8_t* src);
#else
    static void fliper();
#endif

    // direct buffer access (advanced)
    static uint8_t* getBackBuffer() { return backbuf; }

private:
    // -----------------------------------------------------------------
    // Private data – only static members
    // -----------------------------------------------------------------
    struct RGB888 { uint8_t r, g, b; };

    static uint8_t    backbuf[GFX_WIDTH * GFX_HEIGHT];   // indexed frame buffer
    static RGB888     palette[GFX_PALETTE_SIZE];        // 256-entry palette
    static MatrixPanel_I2S_DMA* dma_display;            // HUB75 driver
};