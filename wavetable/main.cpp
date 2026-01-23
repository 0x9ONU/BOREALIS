
#include "daisysp.h"
#include "daisy_seed.h"
#include "wavetable.h"
#include <array>

using namespace daisysp;
using namespace daisy;
using namespace daisy::seed;

constexpr float PI = 3.14159265358979323846f;

static DaisySeed hw;
Wavetable wt;
volatile int state = 0;
float storagevar[10];


void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    wt.SetIndex(0, 2);
    wt.SetFreq(0, 3*220);

    for(size_t i = 0; i < size; i++)
    {
        out[0][i] = out[1][i] = wt.Process(0); // 440Hz, wave 0, oscillator 0
    }
}

int main(void)
{
    // Begin Initialization
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(64);
    hw.StartLog(true);
    float sample_rate = hw.AudioSampleRate();
    
    hw.PrintLine("Start wavetable creation");
    // create the wavetable here, currently 3 waves, sine, triangle, square
    int num_waves = 4;
    std::array<std::array<float, 2048>, 8> waves;
    for(int i = 0; i < 2048; i++){
        float phase = (float)i / 2048.0f;   // 
        
        waves[0][i] = sinf(2.0f * PI * phase);
        hw.PrintLine("My Float: " FLT_FMT(6), FLT_VAR(6, waves[0][i]));

        float tri;
        if (phase < 0.25f)
            tri = 4.0f * phase;
        else if (phase < 0.75f)
            tri = 2.0f - 4.0f * phase;
        else
            tri = -4.0f + 4.0f * phase;

        waves[1][i] = tri;

        waves[2][i] = (phase < 0.5f) ? 1.0f : -1.0f;
    }

    wt.Init(sample_rate, waves, num_waves);
    hw.PrintLine("Wavetable initialized");

    //Configure and initialize button
    Switch button1;
    //Set button to pin 28, to be updated at a 1kHz  samplerate
    button1.Init(D21, 1000);
    hw.PrintLine("Button initialized");

    hw.StartAudio(AudioCallback);
    hw.PrintLine("Audio Callback Started, begin main loop");

    // Main Loop
    while(1) {
        // Read Inputs, doesnt function right now, was using to update a variable to change frequency and wave shape
        System::Delay(1);
    }
}
