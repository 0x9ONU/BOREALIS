
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
    for(int i = 0; i < 8; i++){
        notes[i] = 255;
    }
}

// Processes the selected oscillator by 1 sample at a target frequency and target wavetable index
std::array<float, 8> Wavetable::Process(){
    std::array<float, 8> out;                              
    // calculate every oscillator's next sample
    for(int osc = 0; osc < 8; osc++){
        float dt = frequencies[osc] * 2048.0 / fs;          // time difference between samples

        // finds indicies and weights 
        uint8_t wave1 = (int)indicies[osc];                 // closest wave to the left
        uint8_t wave2 = wave1 + 1;                          // closest wave to the right
        if (wave2 >= num_waves){ wave2 = 0; }               // wrap to beginning
        float weight = daisysp::fastmod1f(indicies[osc]);   // weight between the two waves
        
        // for interpolating between samples in a wave
        float phase = phases[osc];                          // Which oscillator are we using 
        uint16_t i = (int)phase;                            // closest sample to left
        uint16_t i_next = i + 1;                            // closest sample to the right
        float i_weight = daisysp::fastmod1f(phase);         // distance from the left sample
        if(i_next >= 2048){ i_next -= 2048;}                // wrap if needed

        float fl1 = waves[wave1][i];    
        float ce1 = waves[wave1][i_next];
        float fl2 = waves[wave2][i];        
        float ce2 = waves[wave2][i_next];

        float fl = (1-weight) * fl1 + weight * fl2;
        float ce = (1-weight) * ce1 + weight * ce2;

        out[osc] = (1 - i_weight) * fl + i_weight * ce;

        phase += dt;
        if(phase >= 2048.0f){ phase -= 2048.0f; }
        phases[osc] = phase;
    }

    return out;
}

void Wavetable::SetFreq(int osc, float freq){
    frequencies[osc] = freq;
}

void Wavetable::SetIndex(int osc, float index){
    indicies[osc] = index;
}

int Wavetable::AddNote(uint8_t note){
    /* Find a target oscillator */
    uint8_t target = 0;                             // keep track of the oldest oscillator
    for(int i = 0; i < 8; i++){                     // search through all oscillators
        //if(notes[i] == note){ return; }             // return if we are playing the note
        if(notes[i] == 255){                        // if oscillator is empty
            target = i;
            break;
        }                        
        if(ages[i] > ages[target]){ target = i; }   // update oldest
    }

    /* Update target oscillator */
    notes[target] = note;                           // assign oscillator
    frequencies[target] = midi_to_freq[note];       // update the oscillator's frequency (could probably merge with the above line)
    for(int j = 0; j < 8; j++){                     // increment every active oscillator's age
        if(notes[j] != 255){ ages[j]++; }
    }
    ages[target] = 1;                               // oldest is now youngest
    phases[target] = 0;                             // reset phase
    return target;
}

int Wavetable::RemoveNote(uint8_t note){
    /* Start by finding the oscillator to turn off */
    for(int i = 0; i < 8; i++){                     // search through all oscillators
        if(notes[i] == note){                       // if oscillator is target
            notes[i] = 255;                         // turn it off

            for(int j = 0; j < 8; j++){             // decrement every active oscillator's age older than the removed
                if(ages[j] > ages[i]){ ages[j]--; }
            }
            ages[i] = 0;                            // clear the target oscillator's age
            return i;
        }
    }

    /* Note to turn off is not currently being played */
    return 9;
}

std::array<uint8_t, 8> Wavetable::ReadNotes(){
    return notes;
}
std::array<float, 8> Wavetable::ReadFreqs(){
    return frequencies;
}
std::array<float, 8> Wavetable::ReadPhases(){
    return phases;
}
std::array<float, 8> Wavetable::ReadIndicies(){
    return indicies;
}


