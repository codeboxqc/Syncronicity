#include <Arduino.h>
#include <driver/i2s.h>
#include <ArduinoFFT.h>
#include <ctime>

#include "pio.h"
#include "FFT.h"


float vReal[SAMPLES];
float vImag[SAMPLES];

float vRealOriginal[SAMPLES];
float currentEnergy[3] = {0};
bool currentBeats[3] = {false};
 

 AudioFilter amplitudeFilter;
 
  
 BeatDetector beatDetector;
//ArduinoFFT FFT = ArduinoFFT(vReal, vImag, SAMPLES, SAMPLING_FREQUENCY);
ArduinoFFT<float> FFT = ArduinoFFT<float>(vReal, vImag, SAMPLES, SAMPLING_FREQUENCY);


  
 void readMicrophone() {
  size_t bytesRead;
  for (int i = 0; i < SAMPLES; i++) {
    int32_t sample = 0;
    i2s_read(I2S_NUM_0, &sample, sizeof(sample), &bytesRead, portMAX_DELAY);
   vReal[i] = sample >> 8; // Adjusted scaling
   
  }
}


 
Wave WaveData(int x, float time) {
    Wave data;
    
    float normalizedX = (float)x / WIDTH;
    int sampleIndex = (int)(normalizedX * SAMPLES);
    sampleIndex = constrain(sampleIndex, 0, SAMPLES - 1);
    
    // Use ORIGINAL waveform data for visualization
    data.amplitude = (float)vReal[sampleIndex] / 32768.0f;
    data.filteredAmplitude = max(amplitudeFilter.process(fabs(data.amplitude)), 0.05f);
    
    // Use pre-calculated energy and beat data
    data.energy[0] = currentEnergy[0];
    data.energy[1] = currentEnergy[1];
    data.energy[2] = currentEnergy[2];
    data.lowBeat = currentBeats[0];
    data.midBeat = currentBeats[1];
    data.highBeat = currentBeats[2];
    
    // Color calculation with beat enhancement
    float colorIntensity = 0.5f + data.filteredAmplitude * 0.9f;
    if (data.lowBeat) colorIntensity = min(colorIntensity * 1.3f, 1.0f);
    
    int colorIndex = (x + (int)(time * 20)) % 5;
    uint16_t baseColor = 0xF800 + colorIndex * 0x0841;
    
    data.r = ((baseColor >> 11) & 0x1F) << 3;
    data.g = ((baseColor >> 5) & 0x3F) << 2;
    data.b = (baseColor & 0x1F) << 3;
    
    data.r = constrain(data.r * colorIntensity, 0, 255);
    data.g = constrain(data.g * colorIntensity, 0, 255);
    data.b = constrain(data.b * colorIntensity, 0, 255);
    
    data.y = (int)((1.0f - (0.5f + 0.3f * data.filteredAmplitude)) * HEIGHT);
    
    return data;
}


// Compute FFT on the audio samples
void computeFFT() {
    // Clear imaginary part
    for (int i = 0; i < SAMPLES; i++) {
        vImag[i] = 0;
    }
    
    // Perform FFT
    FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    FFT.compute(FFTDirection::Forward);
    FFT.complexToMagnitude();
}

// Calculate energy in different frequency bands
void calculateBandEnergies(float energy[3]) {
    energy[0] = 0; // Low (bass)
    energy[1] = 0; // Mid
    energy[2] = 0; // High
    
    float binWidth = (float)SAMPLING_FREQUENCY / SAMPLES;
    
    for (int i = 1; i < SAMPLES / 2; i++) {
        float freq = i * binWidth;
        float magnitude = vReal[i];
        
        if (freq < LOW_FREQ_MAX) {
            energy[0] += magnitude;
        } else if (freq >= MID_FREQ_MIN && freq < MID_FREQ_MAX) {
            energy[1] += magnitude;
        } else if (freq >= HIGH_FREQ_MIN) {
            energy[2] += magnitude;
        }
    }
    
    // Normalize energies
    energy[0] = energy[0] / (LOW_FREQ_MAX / binWidth);
    energy[1] = energy[1] / ((MID_FREQ_MAX - MID_FREQ_MIN) / binWidth);
    energy[2] = energy[2] / ((SAMPLING_FREQUENCY / 2 - HIGH_FREQ_MIN) / binWidth);
}
 

// Call this in your main loop after readMicrophone()
void processAudio() {
    readMicrophone();
    
    // Copy to original for waveform display
    memcpy(vRealOriginal, vReal, sizeof(vReal));
    
    // Perform FFT analysis (modifies vReal)
    computeFFT();
    
    // Extract frequency band data
    calculateBandEnergies(currentEnergy);
    
    // Detect beats
    currentBeats[0] = beatDetector.detectBeat(currentEnergy[0], 0);
    currentBeats[1] = beatDetector.detectBeat(currentEnergy[1], 1);
    currentBeats[2] = beatDetector.detectBeat(currentEnergy[2], 2);
    beatDetector.updateIndex();
    
    // Restore original waveform for visualization
    memcpy(vReal, vRealOriginal, sizeof(vReal));
}