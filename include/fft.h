#pragma once

#include <Arduino.h>
#include <driver/i2s.h>
#include <ArduinoFFT.h>

// Constraint function for clamping values
template<typename T>
T constrain(T value, T min_val, T max_val) {
    return (value < min_val) ? min_val : (value > max_val) ? max_val : value;
}

 
#include "FFT.h"
 

// FFT Configuration
//#define SAMPLES 1024
#define SAMPLES 512
#define SAMPLING_FREQUENCY 16000

#define LOW_FREQ_MAX 250      // Bass frequencies (Hz)
#define MID_FREQ_MIN 250      
#define MID_FREQ_MAX 2000     // Mid frequencies (Hz)
#define HIGH_FREQ_MIN 2000    // High frequencies (Hz)

// Beat detection thresholds
#define BEAT_THRESHOLD 1.5f   // Energy must be 1.5x the average


 // Beat detection class
class BeatDetector {
private:
    float history[3][10] = {{0}}; // Store last 10 values for low, mid, high
    int historyIndex = 0;
    
public:
    bool detectBeat(float currentEnergy, int band) {
        // Calculate average energy from history
        float avgEnergy = 0;
        for (int i = 0; i < 10; i++) {
            avgEnergy += history[band][i];
        }
        avgEnergy /= 10.0f;
        
        // Update history
        history[band][historyIndex] = currentEnergy;
        
        // Detect beat if current energy is significantly higher than average
        return (currentEnergy > avgEnergy * BEAT_THRESHOLD && avgEnergy > 0.01f);
    }
    
    void updateIndex() {
        historyIndex = (historyIndex + 1) % 10;
    }
};


// WaveData structure declaration
struct Wave {
    float amplitude;
    float filteredAmplitude;
    uint8_t r, g, b;
    int y;
    bool lowBeat;
    bool midBeat;
    bool highBeat;
    float energy[3]; // low, mid, high energy levels
};




// Audio filter class
class AudioFilter {
private:
    float value = 0;
    float alpha = 0.0f;
   
public:
    float process(float input) {
        value = value * (1 - alpha) + input * alpha;
        return value;
    }
    void setSmoothness(float smoothness) { 
        alpha = constrain(smoothness, 0.01f, 0.3f); 
    }
};


extern AudioFilter amplitudeFilter;

extern float vRealOriginal[SAMPLES];  // Original waveform
extern float currentEnergy[3];        // Store energy levels globally
extern bool currentBeats[3];   
extern float vReal[SAMPLES];
extern float vImag[SAMPLES];
 

void readMicrophone();
 Wave WaveData(int x, float time);


 extern BeatDetector beatDetector;

// Function to compute FFT and extract frequency band 
void computeFFT();
void calculateBandEnergies(float energy[3]); 
void processAudio();






 

 
 