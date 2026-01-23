
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
volatile int state = 0;
float storagevar[10];
MidiUartHandler midi;

/** FIFO to hold messages as we're ready to print them */
FIFO<MidiEvent, 128> event_log;

void GetMidiTypeAsString(MidiEvent& msg, char* str)
{
    switch(msg.type)
    {
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
    //wt.SetFreq(0, 3*220);

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
    wt.SetIndex(0, 2);
    wt.SetFreq(0, 660);
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

        now = System::GetNow();

        /** Process MIDI in the background */
        midi.Listen();
        /** Loop through any MIDI Events */
        while(midi.HasEvents())
        {
            hw.PrintLine("I got an event!");
            MidiEvent msg = midi.PopEvent();

            /** Handle messages as they come in 
             *  See DaisyExamples for some examples of this
             */
            switch(msg.type)
            {
                case NoteOn:
                    // Do something on Note On events
                    {
                        uint8_t bytes[3] = {0x90, 0x00, 0x00};
                        bytes[1] = msg.data[0];
                        bytes[2] = msg.data[1];
                        midi.SendMessage(bytes, 3);
                        //Set frequency of wavetable oscillator
                        wt.SetFreq(0, 440.0f * powf(2.0f, (msg.data[0] - 57) / 12.0f));
                    }
                    break;
                case NoteOff:
                    {
                      uint8_t bytes[3] = {0x90, 0x00, 0x00};
                        bytes[1] = msg.data[0];
                        bytes[2] = msg.data[1];
                        midi.SendMessage(bytes, 3);
                        
                        wt.SetFreq(0, 0);
                    }
                default: break;
            }

            /** Regardless of message, let's add the message data to our queue to output */
            event_log.PushBack(msg);
        }

        /** Now separately, every 5ms we'll print the top message in our queue if there is one */
        if(now - log_time > 5)
        {
            log_time = now;
            if(!event_log.IsEmpty())
            {
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
