
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
MidiUartHandler midi;

volatile float volume = 0.0f;       // Volume knob/pot
volatile float wt_index = 0.0f;     // wavetable index knob/pot

uint32_t now, log_time = System::GetNow(); // for periodic debug outputs

FIFO<MidiEvent, 128> event_log; // Midi message FIFO for printing

std::array<float, 8> oscillators_out;
float mix;

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
        mix = 0;
        oscillators_out = wt.Process();
        
        for(int j = 0; j < 8; j++){ mix = mix + (oscillators_out[j] * 0.125); } // Process each oscillator
        
        out[0][i] = out[1][i] = volume * mix ; // CHANGE THIS LATER, need to scale volume to 0-1 range.
    }    
}

int main(void){
    /** Begin Initialization */ 
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(64);
    hw.StartLog(true);
    float sample_rate = hw.AudioSampleRate();
    
    // 1. Initialize Wavetables
    // make wavetables  
    int num_waves = 4;
    std::array<std::array<float, 2048>, 8> waves;
    for(int i = 0; i < 2048; i++){
        float phase = (float)i / 2048.0f;   // 
        
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
        wt.SetIndex(i, 1.5);
    }

    // 2. Initalize Midi and Start receiving
    MidiUartHandler::Config midi_config;
    midi.Init(midi_config);
    midi.StartReceive();

    // 3. ADC Config (Pins 15, 16, 17, 18)
    // adc_config[0]: volume knob temp
    // adc_config[1]: wavetable index knob temp
    // adc_config[2]: 
    // adc_config[3]: 
    // adc_config[4]: 

    AdcChannelConfig adc_config[4];
    adc_config[0].InitSingle(hw.GetPin(15));
    adc_config[1].InitSingle(hw.GetPin(16));
    adc_config[2].InitSingle(hw.GetPin(17));
    adc_config[3].InitSingle(hw.GetPin(18));
    hw.adc.Init(adc_config, 4);
    hw.adc.Start();

    hw.StartAudio(AudioCallback);

    wt.AddNote(69); // testing

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
                    // add a note to the wavetable, automatically assigned to an oscillator
                    char outstr[128];
                    sprintf(outstr, "Add note:%d\n", msg.data[0]);
                    hw.PrintLine(outstr);
                    wt.AddNote(msg.data[0]);
                    break;
                }
                case NoteOff:
                {
                    // remove the note
                    char outstr[128];
                    sprintf(outstr, "Remove note:%d\n", msg.data[0]);
                    hw.PrintLine(outstr);
                    wt.RemoveNote(msg.data[0]);   
                    break;
                }  
                default: break;
            }
            /** Regardless of message, let's add the message data to our queue to output */
            event_log.PushBack(msg);
        }
        
        /** 200Hz or 5ms code block */
        if(now - log_time > 5){
            
            /** Read potentiometers */
            volume = hw.adc.GetFloat(0); // read the volume knob (0-??)
            wt_index = hw.adc.GetFloat(1); // read the wavetable index (0-??)
            for(int i = 0; i < 8; i++){ wt.SetIndex(i, 1.5); }

            log_time = now;

            char outstr[128];
            sprintf(outstr, "Volume: %d\tWT Index: %d\n", (int)volume, (int)wt_index);
            hw.PrintLine(outstr);
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
