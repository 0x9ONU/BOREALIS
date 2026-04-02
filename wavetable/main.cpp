#include "daisysp.h"
#include "daisy_seed.h"
#include "wavetable.h"
#include <array>

using namespace daisysp;
using namespace daisy;
using namespace daisy::seed;

enum AdcChannel {
   VOLUME = 0,
   WT_INDEX = 1,
   ATTACK = 2,
   DECAY = 3,
   SUSTAIN = 4,
   RELEASE = 5
};

constexpr float PI = 3.14159265358979323846f;

static DaisySeed hw;
MidiUartHandler midi;
Wavetable wt;
Adsr adsr[8];
bool notes_on[8] = {0,0,0,0,0,0,0,0}; // bool to store if a note is currently being played

volatile float volume = 0.0f;      // Volume knob/pot
volatile float wt_index = 0.0f;    // wavetable index knob/pot
volatile float attack = 0.0;       // ADSR values
volatile float decay = 0.0f;       // wavetable index knob/pot
volatile float sustain = 0.0f;     // wavetable index knob/pot
volatile float release = 0.0f;     // wavetable index knob/pot

/* max ADSR times */
const float MAX_A = 7;
const float MAX_D = 2.5;
const float MAX_R = 1.75;

uint32_t now, log_time = System::GetNow(); // for periodic events

FIFO<MidiEvent, 128> event_log; // Midi message FIFO for printing

std::array<float, 8> oscillators_out;
float mix = 0;

void GetMidiTypeAsString(MidiEvent& msg, char* str){
    switch(msg.type){
        case NoteOff: strcpy(str, "NoteOff"); break;
        case NoteOn: strcpy(str, "NoteOn"); break;
        case PolyphonicKeyPressure: strcpy(str, "PolyKeyPres."); break;
        case ControlChange: strcpy(str, "CC"); break;
        case ProgramChange: strcpy(str, "Prog. Change"); break;
        case ChannelPressure: strcpy(str, "Chn. Pressure"); break;
        case PitchBend: strcpy(str, "PitchBend"); break;
        case SystemCommon: strcpy(str, "Sys. Common"); break;
        case SystemRealTime: strcpy(str, "Sys. Realtime"); break;
        case ChannelMode: strcpy(str, "Chn. Mode"); break;
        default: strcpy(str, "Unknown"); break;
    }
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++){   // Process each sample
        oscillators_out = wt.Process();     // Process each oscillator
        for(int j = 0; j < 8; j++){ 
            mix = mix + (oscillators_out[j] * adsr[j].Process(notes_on[j]) * 0.20); // sum the oscillators together
        }
        out[0][i] = out[1][i] = mix * volume; // output
        mix = 0; // reset
    }    
}

int main(void){
    /** Begin Initialization */ 
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(64);
    hw.StartLog(false); // set this to true to wait for terminal connection
    float sample_rate = hw.AudioSampleRate();
    
    // 1. Initialize Wavetables
    // make wavetables  
    int num_waves = 4;
    std::array<std::array<float, 2048>, 8> waves;
    for(int i = 0; i < 2048; i++){
        float phase = (float)i / 2048.0f;
        
        waves[0][i] = sinf(2.0f * PI * phase);

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
    for(int i = 0; i < 8; i++){
        wt.SetIndex(i, 0);
    }

    // 2. Initalize Midi and Start receiving
    MidiUartHandler::Config midi_config;
    midi.Init(midi_config);
    midi.StartReceive();

    // 3. ADC Config 
    // volume on pin 15/channel 0
    // wt index on pin 16/channel 1
    // ADSR is on pins 17 18 19 20/ channels 2 3 4 5
    AdcChannelConfig adc_config[6];
    adc_config[0].InitSingle(A0); // volume
    adc_config[1].InitSingle(A1); // wt index
    adc_config[2].InitSingle(A2); // a
    adc_config[3].InitSingle(A3); // d
    adc_config[4].InitSingle(A4); // s
    adc_config[5].InitSingle(A5); // r

    hw.adc.Init(adc_config, 6);
    hw.adc.Start();

    // 4. ADSR config
    // currently using arbitrary values for testing
    for(int i = 0; i < 8; i++){
        adsr[i].Init(sample_rate);
        adsr[i].SetAttackTime(0.15F, 0.75F);
        adsr[i].SetDecayTime(0.5F);
        adsr[i].SetSustainLevel(0.25F);
        adsr[i].SetReleaseTime(0.15F);
    }

    hw.StartAudio(AudioCallback);

    int idx = wt.AddNote(69);               // testing
    notes_on[idx] = 1;                      // tells the ADSR that this oscillator is on
    adsr[idx].Retrigger(0);                 // retriggers the ADSR in case of overflow

    // Main Loop
    while(1) {
        System::Delay(1);
        now = System::GetNow();
        // Look for new MIDI events
        midi.Listen();
        /** Loop through any MIDI Events */
        while(midi.HasEvents()){
            MidiEvent msg = midi.PopEvent();
            // Handle messages as they come in 
            switch(msg.type){
                case NoteOn:
                {
                    // Debugging, disable later
                    char outstr[128];
                    sprintf(outstr, "Add note:%d\n", msg.data[0]);
                    hw.PrintLine(outstr);

                    int osc_ = wt.AddNote(msg.data[0]);     // Adds note to oscillators
                    if(osc_ < 9){
                        notes_on[osc_] = 1;                 // tells the ADSR that this oscillator is on
                        adsr[osc_].Retrigger(0);            // retriggers the ADSR in case of overflow
                    }
                    break;
                }
                case NoteOff:
                {
                    // Debugging, disable later
                    char outstr[128];
                    sprintf(outstr, "Remove note:%d\n", msg.data[0]);
                    hw.PrintLine(outstr);

                    int osc_ = wt.RemoveNote(msg.data[0]);  // removes the note from oscillators
                    if(osc_<9){ notes_on[osc_] = 0; }       // tells the ADSR that this oscillator is off
                    break;
                }  
                default: break;
            }
            /* Regardless of message, add the message data to output queue */
            event_log.PushBack(msg);
        }
        
        /* 200Hz or 5ms code block */
        if(now - log_time > 5){
            log_time = now;
            
            /* Read potentiometers and scale to correct values if needed*/
            volume = hw.adc.GetFloat(VOLUME);
            wt_index = hw.adc.GetFloat(WT_INDEX) * 8;
            attack = hw.adc.GetFloat(ATTACK) * MAX_A;
            decay = hw.adc.GetFloat(DECAY) * MAX_D;
            sustain = hw.adc.GetFloat(SUSTAIN);
            release = hw.adc.GetFloat(RELEASE) * MAX_R;

            // update each oscillator's wt index and envelope
            for(int i = 0; i < 8; i++){
                wt.SetIndex(i,wt_index);
                adsr[i].SetAttackTime(attack, 0.75F);
                adsr[i].SetDecayTime(decay);
                adsr[i].SetSustainLevel(sustain);
                adsr[i].SetReleaseTime(release);
            }
        

            /* Start debugging/logs, disable later */
            char outstr[128];

            /* input debugging*/
            sprintf(outstr, "Volume: %d\tIndex: %d\tA: %d\tD: %d\tS: %d\tR: %d\n", (int)volume, (int)wt_index, (int)attack, (int)decay, (int)sustain, (int)release);
            hw.PrintLine(outstr);
            
            /* MIDI debugging*/
            if(!event_log.IsEmpty()){
                auto msg = event_log.PopFront();
                char outstr[128];
                char type_str[16];
                GetMidiTypeAsString(msg, type_str);
                sprintf(outstr,
                        "time:\t%ld\ttype: %s\tChannel:  %d\tData MSB: "
                        "%d\tData LSB: %d\n",
                        now,
                        type_str,
                        msg.channel,
                        msg.data[0],
                        msg.data[1]);
                hw.PrintLine(outstr);
            }
        }
    }
}
