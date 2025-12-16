#include <Arduino.h>
#include <stdint.h>
#include <string.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

// gfx.cpp
#include "gfx.h"
#include "pio.h"


  
// -----------------------------------------------------------------
// 1. Static member definitions (exactly once!)
// -----------------------------------------------------------------
uint8_t          GFX::backbuf[GFX_WIDTH * GFX_HEIGHT];
GFX::RGB888      GFX::palette[GFX_PALETTE_SIZE];
MatrixPanel_I2S_DMA* GFX::dma_display = nullptr;



// -----------------------------------------------------------------
// 2. Init
// -----------------------------------------------------------------
void GFX::init(MatrixPanel_I2S_DMA* display) {
    dma_display = display;
    memset(backbuf, 0, sizeof(backbuf));

   
}
 
 

// -----------------------------------------------------------------
// 3. Palette helpers
// -----------------------------------------------------------------
void GFX::setPalette(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
    if (idx < GFX_PALETTE_SIZE) {

      
 
         palette[idx].r = r;
       palette[idx].g = g;
       palette[idx].b = b;
        
       
    }
}
void GFX::getPalette(uint8_t idx, uint8_t* r, uint8_t* g, uint8_t* b) {
    if (idx < GFX_PALETTE_SIZE) {
        *r = palette[idx].r;
        *g = palette[idx].g;
        *b = palette[idx].b;
    } else { *r = *g = *b = 0; }
}

void GFX::fadeToBlack(uint8_t amount) {
    uint8_t* buf = backbuf;
    for (size_t i = 0; i < GFX_WIDTH * GFX_HEIGHT; i++) {
        uint8_t idx = buf[i];
        if (idx > amount) {
            buf[i] = idx - amount;
        } else {
            buf[i] = 0;
        }
    }
}
// -----------------------------------------------------------------
// 4. Primitive implementations (inline in header already, keep stubs)
// -----------------------------------------------------------------
void GFX::clear(uint8_t col) {
    memset(backbuf, col, sizeof(backbuf));
}




// -----------------------------------------------------------------
// 6. Line & Circle (Bresenham) – keep the bodies you already have
// -----------------------------------------------------------------

// Fast line using Bresenham
void GFX::line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t col) {
    int16_t dx = x1 - x0;
    int16_t dy = y1 - y0;
    
    // Use optimized functions for straight lines
    if (dx == 0) {
        if (dy < 0) { int16_t t = y0; y0 = y1; y1 = t; dy = -dy; }
        vline(x0, y0, dy + 1, col);
        return;
    }
    if (dy == 0) {
        if (dx < 0) { int16_t t = x0; x0 = x1; x1 = t; dx = -dx; }
        hline(x0, y0, dx + 1, col);
        return;
    }
    
    // Bresenham for diagonal lines
    int16_t sx = dx > 0 ? 1 : -1;
    int16_t sy = dy > 0 ? 1 : -1;
    dx = dx > 0 ? dx : -dx;
    dy = dy > 0 ? dy : -dy;
    
    int16_t err = dx - dy;
    
    while (true) {
        setPixel(x0, y0, col);
        
        if (x0 == x1 && y0 == y1) break;
        
        int16_t e2 = err << 1;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

// Fast circle outline using Bresenham
void GFX::circle(int16_t x0, int16_t y0, int16_t r, uint8_t col) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    setPixel(x0, y0 + r, col);
    setPixel(x0, y0 - r, col);
    setPixel(x0 + r, y0, col);
    setPixel(x0 - r, y0, col);
    
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        setPixel(x0 + x, y0 + y, col);
        setPixel(x0 - x, y0 + y, col);
        setPixel(x0 + x, y0 - y, col);
        setPixel(x0 - x, y0 - y, col);
        setPixel(x0 + y, y0 + x, col);
        setPixel(x0 - y, y0 + x, col);
        setPixel(x0 + y, y0 - x, col);
        setPixel(x0 - y, y0 - x, col);
    }
}

// Fast filled circle using horizontal lines
void GFX::fillCircle(int16_t x0, int16_t y0, int16_t r, uint8_t col) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    vline(x0, y0 - r, 2 * r + 1, col);
    
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        hline(x0 - x, y0 + y, 2 * x + 1, col);
        hline(x0 - x, y0 - y, 2 * x + 1, col);
        hline(x0 - y, y0 + x, 2 * y + 1, col);
        hline(x0 - y, y0 - x, 2 * y + 1, col);
    }
}



// -----------------------------------------------------------------
// 4. Fast horizontal and vertical lines
// -----------------------------------------------------------------

void GFX::hline(int16_t x, int16_t y, int16_t w, uint8_t col) {
    if (y < 0 || y >= GFX_HEIGHT) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > GFX_WIDTH) w = GFX_WIDTH - x;
    if (w <= 0) return;

    uint8_t* dst = &backbuf[y * GFX_WIDTH + x];
    memset(dst, col, w);
}

void GFX::vline(int16_t x, int16_t y, int16_t h, uint8_t col) {
    if (x < 0 || x >= GFX_WIDTH) return;
    if (y < 0) { h += y; y = 0; }
    if (y + h > GFX_HEIGHT) h = GFX_HEIGHT - y;
    if (h <= 0) return;

    uint8_t* dst = &backbuf[y * GFX_WIDTH + x];
    while (h--) {
        *dst = col;
        dst += GFX_WIDTH;
    }
}

// -----------------------------------------------------------------
// Rectangle primitives
// -----------------------------------------------------------------

void GFX::rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t col) {
    hline(x, y, w, col);
    hline(x, y + h - 1, w, col);
    vline(x, y, h, col);
    vline(x + w - 1, y, h, col);
}

void GFX::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t col) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > GFX_WIDTH)  w = GFX_WIDTH - x;
    if (y + h > GFX_HEIGHT) h = GFX_HEIGHT - y;
    if (w <= 0 || h <= 0) return;

    uint8_t* dst = &backbuf[y * GFX_WIDTH + x];
    while (h--) {
        memset(dst, col, w);
        dst += GFX_WIDTH;
    }
}






// -----------------------------------------------------------------
// 5. Flip functions
// -----------------------------------------------------------------

/*#ifndef NO_FAST_FUNCTIONS

 move to public
   virtual  void updateMatrixDMABuffer(uint16_t x, uint16_t y, uint8_t red, uint8_t green, uint8_t blue);
   virtual  void updateMatrixDMABuffer(uint8_t red, uint8_t green, uint8_t blue);
*/
 

#if (hack == 1)  
void IRAM_ATTR GFX::flip() {
    if (!dma_display) return;

    uint8_t *src = backbuf;
    for (int y = 0; y < GFX_HEIGHT; y++) {
        for (int x = 0; x < GFX_WIDTH; x++) {
            RGB888 &color = palette[*src++];
            dma_display->updateMatrixDMABuffer(x, y, color.r, color.g, color.b);
            
        }
    }

    dma_display->flipDMABuffer();  // swap front/back
}


//////////////////////////////////////////////////////


void IRAM_ATTR GFX::copyRow(int y, uint8_t* src) {

    static GFX::RGB888 rowBuf[GFX_WIDTH];
    // Expand indexed row into RGB888 rowBuf
    for (int x = 0; x < GFX_WIDTH; x++) {
        rowBuf[x] = GFX::palette[*src++];
    }

    // Blast rowBuf into DMA buffer using public API
    for (int x = 0; x < GFX_WIDTH; x++) {
        GFX::RGB888 &c = rowBuf[x];
         dma_display->updateMatrixDMABuffer(x, y, c.r, c.g, c.b);
    }
}

void IRAM_ATTR GFX::flipMemcpy() {

    if (!dma_display) return;
    
    uint8_t *src = backbuf;
    for (int y = 0; y < GFX_HEIGHT; y++) {
        copyRow(y, src);
        src += GFX_WIDTH;
    }

    dma_display->flipDMABuffer();
}
//////////////////////////////////////////////////////
 

 #else  
// CRITICAL: Fast page flip with palette lookup + direct HUB75 RGB888 output
// gfx.cpp  (replace the whole function)
void IRAM_ATTR GFX::fliper() {
    if (! dma_display) return;

    uint8_t* src = backbuf;
    for (int y = 0; y < GFX_HEIGHT; ++y) {
        for (int x = 0; x < GFX_WIDTH; ++x) {
            const RGB888& c = palette[*src++];
             dma_display->drawPixelRGB888(x, y, c.r, c.g, c.b);
        }
    }

     dma_display->flipDMABuffer();   // <<< THIS LINE WAS MISSING
}

#endif