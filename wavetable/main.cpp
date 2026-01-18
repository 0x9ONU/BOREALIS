#include "daisysp.h"
#include "daisy_seed.h"
#include "wavetable.h"
#include <array>

using namespace daisysp;
using namespace daisy;

static DaisySeed  hw;
Wavetable wt;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        out[0][i] = out[1][i] = wt.Process(440, 0, 0);
    }
}

int main(void)
{
    // create the wavetable here
    int num_waves = 4;
    std::array<std::array<float, 2048>, 8>& waves;

    // begin initialization
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();
    wt.Init(sample_rate, waves, num_waves);

    float amp[7] = {.15f, 0.15f, 0.0f, 0.33f, 0.0f, 0.15f, 0.15f};
    for(int i = 0; i < 3; i++)
    {
        osc[i].Init(sample_rate);
        osc[i].SetFreq(freqs[i] * .5f);
        osc[i].SetAmplitudes(amp);
    }

    tick.Init(5.f, sample_rate);

    // set adenv parameters
    env.Init(sample_rate);
    env.SetTime(ADENV_SEG_ATTACK, 0.01);
    env.SetTime(ADENV_SEG_DECAY, 0.35);
    env.SetMin(0.f);
    env.SetMax(.3f);
    env.SetCurve(0.f); // linear

    hw.StartAudio(AudioCallback);
    while(1) {}
}
