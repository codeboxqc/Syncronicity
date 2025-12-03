# Syncronicity
ESP32-HUB75 Audio Visualizer
Real-time Music-Reactive LED Matrix Art Engine

60+ FPS | 
Stunning Visual Modes | Audio-Reactive Waveforms | Smooth Cross-Fade Transitions | Dynamic Palette Shifts



Feature Description  
Unique ScenesPlasma, Fire, Anime Zoom, Aurora, Forests, Oceans, Cities & moreAudio Reactive
INMP441 I2S Mic → FFT → 15+ Waveform VisualizersSmooth Transitions5-second cross-fade between modes (except Fire)Dynamic Palettes30-sec hold + 30-sec smooth color morphing No Framebuffer FlickerDouble-buffered via ESP32-HUB75-MatrixPanel-I2S-DMAMode 1: Fire3-minute clean-cut showcase with instant palette rotate

Hardware

i use 
Freenove Breakout Board  Terminal Block Shield HAT, 5V 3.3V Power Outputs, GPIO Status LED
Freenove ESP32-WROOM Wireless Board, Dual-core 32-bit 240 MHz Microcontroller

no soldered need 

![111](https://github.com/user-attachments/assets/820171cc-6371-43d1-88a2-e48e7133f288)
![222](https://github.com/user-attachments/assets/5bc32afd-b9b1-4ecb-9fa7-40da1c69b0dc)

freenove.com


INMP441 Omnidirectional Microphone Module MEMS High Precision Low Power I2S Interface Support ESP32
Module (allready soldered)

![123](https://github.com/user-attachments/assets/a7c07e1a-4085-45a3-8443-3ef769b0d908)


20Pin Jumper Wire Dupont Line 20cm Male To Male+Female 
Wired

![123](https://github.com/user-attachments/assets/63d4e053-e659-4f2b-8334-d9defaf3f89a)


![Untitled](https://github.com/user-attachments/assets/974a3752-34d5-48f3-8bb1-434b6bd0465e)


Visual Modes
0.  Anime Plasma       9.  Ocean Depths
1.  Flame (3 min)     10. Mystical Swamp
2.  Radial Glow       11. Anime Mountain Zoom
3.  Bio Forest        12. Floating Islands
4.  Classic Plasma    13. Cherry Blossom
5.  Goo Glow          14. Magical Forest Path
6.  Organic Landscape 15. Anime City Skyline
7.  Aurora Sky        16. Green Aurora Orion
8.  Volcanic Landscape




Audio Waveforms
0. Circular Wave     8. Spiral Wave
1. Line Wave         9. VU Circle
2. Short Wave       10. Orbit Rings
3. Bar Wave         11. Heartbeat Line
4. Old Bar          12. Starburst
5. Neon Bars        13. DNA Helix
6. Liquid Bars      14. EQ Arc
7. Fire Bars


[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA
    FastLED




Credits

ESP32-HUB75-MatrixPanel-I2S-DMA – Display driver
FFT logic 
Palette system based on FastLED CRGBPalette16


MIT License - Feel free to fork, modify, and share!









https://github.com/user-attachments/assets/82b5dbd9-d31b-4034-b058-4c52d2ee04c6






my pio wire config

#define  WIDTH  64
#define  HEIGHT 64


#define PANEL_CHAIN 1  one panel


 
// I2S configuration
#define I2S_WS   16

#define I2S_SD   9

#define I2S_SCK  14

#define I2S_PORT I2S_NUM_0



// HUB75 cable wiring
#define gpior1   17

#define gpiob1   8

#define gpior2   3

#define gpiob2   10

#define gpioa    15

#define gpioc    7

#define gpioclk  5

#define gpiooe   12

#define gpiog1   18

#define gpiog2   2

#define gpioe    13

#define gpiob    11

#define gpiod    4

#define gpiolat  6

