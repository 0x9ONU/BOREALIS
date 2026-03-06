
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

int num_waves = 4;
const int table_size = 256;
std::array<std::array<float, table_size>, 8> DSY_SDRAM_BSS waves;

//Testing variables
uint32_t start_test;

uint32_t test_high = 0;
uint32_t test_low = 0xFFFFFFFF;
float test_avg = 0;
uint32_t test_sum = 0;
uint32_t test_n = 0;

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
    start_test = System::GetUs();

    for(size_t i = 0; i < size; i++){   // Process each sample
        mix = 0;                  // variable for sum of all signals
        oscillators_out = wt.Process();
        
        for(int j = 0; j < 8; j++){     // Process each oscillator
            mix = mix + (oscillators_out[j] * 0.125);
        }
        
        out[0][i] = oscillators_out[0];
        out[1][i] = mix;
    }  

    uint32_t test_time = System::GetUs() - start_test; // calculate time for callback
    if(test_time > test_high){test_high = test_time;} // update high time
    if(test_time < test_low){test_low = test_time;} // update low time
    test_sum += test_time; // add to time counter
    test_n++; // increment number of blocks processed
}

void GenerateWavetable(){
    // create the wavetable here, currently 3 waves, sine, triangle, square
    for(int i = 0; i < table_size; i++){
        float phase = (float)i / (float)table_size;   // 
        
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
}

int main(void){
    // Begin Initialization
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(64);
    hw.StartLog(true);
    float sample_rate = hw.AudioSampleRate();
    
    hw.PrintLine("Start wavetable creation");

    GenerateWavetable();
    
    // Initialize the WT
    wt.Init(sample_rate, waves, num_waves);
    for(int i = 0; i < 8; i++){
        wt.SetIndex(i, 1.5);
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
    
    // max load for now
    wt.AddNote(71); // testing
    wt.AddNote(69); // testing
    wt.AddNote(66); // testing
    wt.AddNote(62); // testing
    wt.AddNote(59); // testing
    wt.AddNote(56); // testing
    wt.AddNote(53); // testing
    wt.AddNote(49); // testing
    
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

        /** Every 2 minutes we'll print the test results */
        /** Currently, all test results are integers, should probably format as floats */
        if(now - log_time > 120000){
            test_avg = (float)test_sum / (float)test_n;
            char outstr[128];
            sprintf(outstr,
                    "\nLatency Results"
                    "\nSum:%lu"
                    "\nN: %lu"
                    "\nHigh:%lu"
                    "\nLow: %lu",
                    test_sum,
                    test_n,
                    test_high,
                    test_low);
            hw.PrintLine(outstr);
            
            FixedCapStr<16> str("\nAvg: ");
            str.AppendFloat(test_avg, 3); // 3 decimal places
            hw.PrintLine(str);
            test_sum = test_n = test_high = test_avg = 0;
            test_low = 0xFFFFFFFF;
            log_time = now;
        }
    }
}
