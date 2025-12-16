#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
 
#include "pio.h"
#include "gfx.h"
#include "FFT.h"
#include "palette.h"
#include "scene.h"
#include "wave.h"
#include "3body.h"
  


Plasma plasma  ;
AnimePlasma plasma2;
Flame flame;

#define MORE 4
 
 // -----------------------------------------------------------------
     const uint8_t TOTAL_MODES = 17;  
     const uint16_t MODE_DURATIONS  =  60 * MORE;// 60 ;
     const uint16_t BLEND_FRAMES = 15;           // 150 frames ≈ 5 s
     // uint8_t  visualizerMode   = 0;          // current mode
      uint8_t visualizerMode=random(0, TOTAL_MODES);
      time_t   lastChange       = 0;          // when we entered the mode
      uint16_t blendFrame       = 0;          // 0 … BLEND_FRAMES-1
      uint8_t  oldBackbuf[GFX_WIDTH * GFX_HEIGHT]; // previous frame
     unsigned long now = 0;
    // -----------------------------------------------------------------
  
     
     // globals
    time_t pallastChange = 0;
     int currentPaletteIndex = 0;
     int      currentPal     =  0;
     bool     inBlend        = false;      // cross-fading?

     unsigned long lastwaveTime = 0;   // initialize once
     int Bw = 0;                       // current mode
     const int BAR =13;
     unsigned long TIMEbarwave = 30000 ; // 30 seconds


     // ---- Wave-only  -------------------------------------------------
bool     waveOnlyMode   = false;          // are we in "wave-only" now?
unsigned long waveOnlyStart = 0;          // when we entered wave-only
const unsigned long WAVE_ONLY_DURATION = 30000UL ; //* (MORE/2);   // 30 s
unsigned long nextWaveOnlyTrigger = 0;    // when the next random pause will fire
const unsigned long WAVE_ONLY_MIN_GAP = 50000UL;  // change anime bar
const unsigned long WAVE_ONLY_MAX_GAP = 100000UL;  // 1 min
bool     waveOnlyJustStarted = false;

int BD3=0;

void visualizeFFTOnMatrix(float* fftData, size_t dataLength);
void changepal();
void Visualizer();
void Bwave(int currentMode);
 
extern void logo( );

void setup() {
    Serial.begin(115200);
    Serial.println("Starting...");
    
    // Initialize display first
     hub75();     
    // Initialize microphone
    i2sInit();
   

    
    
initmath();
initPlasma(&plasma);
initAnimePlasma(&plasma2);
initFlame(&flame);
  init3Body();
  
generatePalette(currentPal, targetRGB);
for (int i = 0; i < GFX_PALETTE_SIZE; i++) {
        setPalEntry(i, targetRGB[i][0], targetRGB[i][1], targetRGB[i][2]);
    }
    setPalEntry(0, 0, 0, 0);
 /*********Graphics end**************** */

   
 logo();
 delay(6000);
 
 
 lastChange = millis();
pallastChange = millis();
nextWaveOnlyTrigger = millis() + random(WAVE_ONLY_MIN_GAP, WAVE_ONLY_MAX_GAP);

Visualizer();
delay(20);
GFX::fliper();
 
 
 
}




void debug2(int bug)
{

dma_display->setCursor(0,0);
dma_display->printf("%d",bug);
delay(200);

}
 
 


void loop() {
     
    
    unsigned long currentwaveTime = millis();
  
    
     // GFX::clear(0);

      

     ///////////////////////////////////////////////////////
     //  wave-only mode -----
    if (!waveOnlyMode && now >= nextWaveOnlyTrigger) {
        waveOnlyMode   = true;
        waveOnlyStart  = now;

        waveOnlyJustStarted = true;
        Bw = random(0, BAR);
        lastwaveTime = now;                 // force immediate draw
    }
 
    if (now - lastwaveTime >= TIMEbarwave) {
        Bw = random(0, BAR);
        lastwaveTime = now;
    }
   ///////////////////////////////////////////////////

   

    Visualizer();   
    //  debug();
 
  
    if (currentwaveTime - lastwaveTime >= TIMEbarwave) {
        Bw = random(0, BAR);       // pick new mode
        lastwaveTime = currentwaveTime;
    }


    //GFX::clear(0);     debug2(Bw);    Bwave(Bw);


    processAudio();
    //Bwave(Bw);
    changepal();

    
    // Flip buffer to display
     GFX::fliper();

   // GFX::fliper();
   //FX::flipMemcpy();
   // GFX::flip();

}

 

 void Visualizer()
{
    
     
    now = millis();


   if (waveOnlyMode && (now - waveOnlyStart >= WAVE_ONLY_DURATION)) {
        waveOnlyMode = false;
        inBlend=true;
        nextWaveOnlyTrigger = now + random(WAVE_ONLY_MIN_GAP, WAVE_ONLY_MAX_GAP);
    }



    // -----------------------------------------------------------------
    // 4. SWITCH TO A NEW MODE WHEN TIME IS UP
    // -----------------------------------------------------------------
    if (!waveOnlyMode && !inBlend &&
        (now - lastChange >= (unsigned long)MODE_DURATIONS * 1000UL)) {
        
        uint8_t nextMode;
        do { nextMode = random(TOTAL_MODES); } while (nextMode == visualizerMode);

        
        if (nextMode == 1) { rotatePaletteNow(random(200));
            
        }

        // ---- save the frame we are about to leave (for blending) ----
       memcpy(oldBackbuf, GFX::getBackBuffer(), GFX_WIDTH * GFX_HEIGHT);


        visualizerMode = nextMode;
        lastChange     = now;
        inBlend        = true;          // start cross-fade **unless** we go to mode 1
        blendFrame     = 0;

        // If the *new* mode is the fire (case 1) we **do NOT blend** – we want a clean cut
        if (visualizerMode == 1) inBlend = false;
    }


    //visualizerMode=5;

   if (waveOnlyMode) {
  
        if (waveOnlyJustStarted) {
            
               
           for (int fade = 0; fade < 10; fade++) {  // 12 steps to fade out
            GFX::fadeToBlack(10);
            GFX::fliper();
            delay(30);
           }
           GFX::clear(0);
           // current back buffer
            memset(oldBackbuf, 0, GFX_WIDTH * GFX_HEIGHT);      // previous-frame buffer
            waveOnlyJustStarted = false;   // run only once
        } else {
            GFX::clear(0);
             body3( BD3);
            Bwave(Bw);                     // draw the audio wave

        }

    } else {
     
       

    switch (visualizerMode) {
        case 0:  updateAnimePlasma(&plasma2); drawAnimePlasma(&plasma2); break;
        case 1:  updateFlame(&flame);         drawFlame(&flame);         break;
        case 2:  RadialGlow(millis());                     break;
        case 3:  BioForest(millis());                                break;
        case 4:  updatePlasma(&plasma);       drawPlasma(&plasma);       break;
        case 5:  GooGlow( millis()); break;
        case 6:  OrganicLandscape(millis()); break;

        case 7:  AuroraSky(millis()); break;///////<- ?

        case 8:  VolcanicLandscape(millis()); break;

        case 9:  OceanDepths(millis()); break;
        case 10:  MysticalSwamp(millis()); break;
        case 11:   AnimeMountainZoom(millis()); break;
        case 12:  FloatingIslands(millis()); break;
        case 13: CherryBlossomGarden(millis()); break;
        case 14: MagicalForestPath(millis()); break;
        case 15: AnimeCitySkyline(millis()); break;
        case 16: GreenAuroraOrionNight(millis()); break;

       // case 6: body3(0);               break;   
        //case 7: Hyperdimensional();     break;

      //rotatePaletteNow(random(200)); 
     //rotatePaletteFX(128,RadialGlow);


        
        default: visualizerMode=random(0, TOTAL_MODES); break;
    }
  }

    if (!inBlend && !waveOnlyMode) {
         body3( BD3);
        Bwave(Bw);   
    }
     
    // -----------------------------------------------------------------
    // 6. CROSS-FADE (only when we just left a mode)
    // -----------------------------------------------------------------
    if (inBlend) {
        

        float alpha = (float)blendFrame / BLEND_FRAMES;   // 0 → 1
        uint8_t *dst = GFX::getBackBuffer();   
        uint8_t *old = oldBackbuf;

        for (size_t i = 0; i < GFX_WIDTH * GFX_HEIGHT; ++i) {
            uint8_t a = old[i];
            uint8_t b = dst[i];
            // linear interpolation of palette indices
            dst[i] = (uint8_t)(a * (1.0f - alpha) + b * alpha + 0.5f);
        }

        ++blendFrame;
        if (blendFrame >= BLEND_FRAMES) {
            inBlend    = false;
            blendFrame = 0;
        }
    }

     
}

 


void Bwave(int cMode)
{

 if (inBlend) return ;


 //cMode=5;

switch (cMode) {

   
        case 0: CircularWave(random(2)); break;
        case 1: linewave(); break;
        case 2: shortwave(); break;
        case 3: dnaHelix(); break;
        case 4: oldbar(); break;
        case 5: orbitalBars(); break;
        case 6: liquidBars(); break;//
        case 7: fireBars(); break;
        case 8: spiralWave(); break;
        case 9: eqArc(); break;
        case 10: orbitRings(); break;
        case 11: heartbeatLine(); break;
        case 12: starburst(); break;


        default:  CircularWave(random(2)); break;

       // barwave(); break;
         
    }


}
 
 // -----------------------------------------------------------------
//  Palette change
// -----------------------------------------------------------------
void changepal()
{

  updatePalette();
   GFX::setPalette(0, 0, 0, 0);

    // ---- 2. Time to switch? (30 s hold + 30 s fade) ----
    unsigned long now2 = millis();
    if (!transitioning && (now2 - pallastChange >= PALETTE_HOLD_TIME + TRANSITION_DURATION)) {
        // pick next palette (wrap around)
       // currentPal = (currentPal + 1) % NUM_PALETTES;

        currentPal = random(0, NUM_PALETTES+1);

        // fill *target* buffer with the new colours
        generatePalette(currentPal, targetRGB);
        
        // start the 30-second cross-fade
        transitionStart = now2;
        transitioning   = true;
        pallastChange   = now2;          // reset timer
        // body3(random(0, 30));
        BD3=random(0, 30);
        
    }

}
////////////////////////////////////////////////////////////////////








  