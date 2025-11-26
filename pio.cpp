#include <Arduino.h>
#include <driver/i2s.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <driver/i2s.h>

 
 #include "pio.h"
 #include "gfx.h"

 
// === pio.cpp ===

#include <Arduino.h>
#include <driver/i2s.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#include "pio.h"
#include "gfx.h"





// Global config — only one!
HUB75_I2S_CFG mxconfig(64, 64, 1);  // Will configure pins below

 MatrixPanel_I2S_DMA*  dma_display = nullptr;  // OK

void i2sInit() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = i2s_bits_per_sample_t(I2S_SAMPLE_BITS),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 1024,
    .use_apll = false,          // Added for stability
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  // Install and check I2S driver
  esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("I2S driver install failed: %d\n", err);
    while (true);
  }

  // Set pins
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  err = i2s_set_pin(I2S_PORT, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("I2S set pin failed: %d\n", err);
    while (true);
  }

  i2s_zero_dma_buffer(I2S_PORT);
  
  Serial.println("I2S initialized successfully");
}




void hub75() {
  Serial.begin(115200);
  Serial.println("ESP32 HUB75 Starting...");

  // === CONFIGURE GLOBAL mxconfig ===
  mxconfig.gpio.r1 = gpior1;
  mxconfig.gpio.g1 = gpiog1;
  mxconfig.gpio.b1 = gpiob1;
  mxconfig.gpio.r2 = gpior2;
  mxconfig.gpio.g2 = gpiog2;
  mxconfig.gpio.b2 = gpiob2;
  mxconfig.gpio.a = gpioa;
  mxconfig.gpio.b = gpiob;
  mxconfig.gpio.c = gpioc;
  mxconfig.gpio.d = gpiod;
  mxconfig.gpio.e = gpioe;
  mxconfig.gpio.lat = gpiolat;
  mxconfig.gpio.oe = gpiooe;
  mxconfig.gpio.clk = gpioclk;

  // Optional: fix OE polarity (common issue)
  // mxconfig.i2s.oe_inverted = true;

  
  // === CREATE DISPLAY ===
   dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  if (! dma_display->begin()) {
    Serial.println("Display init failed!");
    while (true);
  }

  // Initialize GFX with display reference
    GFX::init(dma_display);

  // === INIT GFX ENGINE ===
   init( );  // CRITICAL!

  // Optional: brightness
   dma_display->setBrightness8(120);

  Serial.println("HUB75 Ready!");
}









 


  
  
 


 
 /*
void i2s_adc(void *arg)
{
    
    int i2s_read_len = I2S_READ_LEN;
    int flash_wr_size = 0;
    size_t bytes_read;

    char* i2s_read_buff = (char*) calloc(i2s_read_len, sizeof(char));
    uint8_t* flash_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));

    i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
    i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
    
    Serial.println(" *** Recording Start *** ");
    while (flash_wr_size < FLASH_RECORD_SIZE) {
        //read data from I2S bus, in this case, from ADC.
        i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
        //example_disp_buf((uint8_t*) i2s_read_buff, 64);
        //save original data from I2S(ADC) into flash.
        i2s_adc_data_scale(flash_write_buff, (uint8_t*)i2s_read_buff, i2s_read_len);
        file.write((const byte*) flash_write_buff, i2s_read_len);
        flash_wr_size += i2s_read_len;
        ets_printf("Sound recording %u%%\n", flash_wr_size * 100 / FLASH_RECORD_SIZE);
        ets_printf("Never Used Stack Size: %u\n", uxTaskGetStackHighWaterMark(NULL));
    }
    file.close();

    free(i2s_read_buff);
    i2s_read_buff = NULL;
    free(flash_write_buff);
    flash_write_buff = NULL;
    
    listSPIFFS();
    vTaskDelete(NULL);
}

 */
 