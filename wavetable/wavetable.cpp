
#include "daisysp.h"
#include "wavetable.h"
#include "daisy_seed.h"


// Defailt constructor
Wavetable::Wavetable() 
{
    this->phases.fill(0.0f);
    this->num_waves = 0;
    this->fs = 44100.0f; // or a sensible default
}

// Constructor 
Wavetable::Wavetable(float samp_freq, const std::array<std::array<float, 2048>, 8>& wavesin, int num_waves):
    fs(samp_freq),
    waves(wavesin),
    num_waves(num_waves),
    phases{0.0f}
{}

// Initialize function
void Wavetable::Init(float samp_freq, const std::array<std::array<float, 2048>, 8>& wavesin, int num_waves){
    this->waves = wavesin;      // copy all 8 × 2048 samples
    this->phases.fill(0.0f);    // zero all phases
    this->num_waves = num_waves;
    this->fs = samp_freq;
}

// Processes the selected oscillator by 1 sample at a target frequency and target wavetable index
float Wavetable::Process(uint8_t osc){
    float out;                                      // for the output sample
    float dt = frequencies[osc] * 2048.0 / fs;      // time difference between samples

    // finds indicies and weights 
    uint8_t wave1 = (int)indicies[osc];           // closest wave to the left
    uint8_t wave2 = wave1 + 1;                      // closest wave to the right
    if (wave2 >= num_waves){ wave2 = 0; }           // wrap to beginning
    float weight = daisysp::fastmod1f(indicies[osc]);       // weight between the two waves
    
    // for interpolating between samples in a wave
    float phase = phases[osc];                      // Which oscillator are we using 
    uint16_t i = (int)phase;                      // closest sample to left
    uint16_t i_next = i + 1;                        // closest sample to the right
    float i_weight = daisysp::fastmod1f(phase);     // distance from the left sample
    if(i_next >= 2048){ i_next -= 2048;}            // wrap if needed

    float fl1 = waves[wave1][i];    
    float ce1 = waves[wave1][i_next];
    float fl2 = waves[wave2][i];        
    float ce2 = waves[wave2][i_next];

    float fl = (1-weight) * fl1 + weight * fl2;
    float ce = (1-weight) * ce1 + weight * ce2;

    out = (1 - i_weight) * fl + i_weight * ce;

    phase += dt;
    if(phase >= 2048.0f){ phase -= 2048.0f; }
    phases[osc] = phase;

    return out;
}

void Wavetable::SetFreq(int osc, float freq){
    frequencies[osc] = freq;
}

void Wavetable::SetIndex(int osc, float index){
    indicies[osc] = index;
}
