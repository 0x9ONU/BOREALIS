
#include "daisysp.h"
#include "daisy_seed.h"
#include "wavetable.h"
#include <array>

using namespace daisysp;
using namespace daisy;
using namespace daisy::seed;

constexpr float PI = 3.14159265358979323846f;

//Intalizations
static DaisySeed hw;
Wavetable wt;
MidiUartHandler midi;

/** FIFO to hold messages as we're ready to print them */
FIFO<MidiEvent, 128> event_log;

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
        mix = 0;                  // variable for sum of all signals
        oscillators_out = wt.Process();
        
        for(int j = 0; j < 8; j++){     // Process each oscillator
            mix = mix + (oscillators_out[j] * 0.125);
        }
        
        out[0][i] = out[1][i] = mix;
    }    
}

int main(void){
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

    // Initialize the WT
    wt.Init(sample_rate, waves, num_waves);
    for(int i = 0; i < 4; i++){
        wt.SetIndex(i, 1);
    }
    hw.PrintLine("Wavetable initialized");

    // Initalize Midi and Start receiving
    hw.PrintLine("Start Midi Initalization...");
    MidiUartHandler::Config midi_config;
    midi.Init(midi_config);
    hw.PrintLine("Midi Initalization! Start Receiving.");
    midi.StartReceive();

    //Temp
    uint32_t now      = System::GetNow();
    uint32_t log_time = System::GetNow();

    hw.StartAudio(AudioCallback);
    hw.PrintLine("Audio Callback Started, begin main loop");

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

        /** Now separately, every 5ms we'll print the top message in our queue if there is one */
        if(now - log_time > 5){
            log_time = now;
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
