#pragma once


#include <Arduino.h>
#include <driver/i2s.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

 
#define  WIDTH  64
#define  HEIGHT 64


#define PANEL_CHAIN 1

//////////////  micro cable wireds

#define I2S_WS    16 
#define I2S_SD    9
#define I2S_SCK   14
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE   (16000)
#define I2S_SAMPLE_BITS   (16)
#define I2S_READ_LEN      (16 * 1024)
 #define I2S_CHANNEL_NUM   (1)
#define FLASH_RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * RECORD_TIME)
 
 

#define SAMPLE_RATE 44100


///////////////hub 75 cable  wireds
#define gpior1   17
#define gpiob1   8
#define gpior2   3
#define gpiob2   10
#define gpioa  15
#define gpioc   7
#define gpioclk   5
#define gpiooe   12
#define gpiog1   18  
#define gpiog2   2
 #define gpioe   13
#define gpiob   11
#define gpiod   4
#define gpiolat   6


 void i2sInit();
 void hub75();

extern MatrixPanel_I2S_DMA* dma_display;;
 

 