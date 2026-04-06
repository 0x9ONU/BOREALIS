#include "daisysp.h"
#include "daisy_seed.h"
#include "wavetable.h"
#include <array>

using namespace daisysp;
using namespace daisy;
using namespace daisy::seed;

enum AdcChannel {
   VOLUME   = 0,
   WT_INDEX = 1,
   ATTACK   = 2,
   DECAY    = 3,
   SUSTAIN  = 4,
   RELEASE  = 5
};

constexpr float PI = 3.14159265358979323846f;

// Hardware Definitions
static DaisySeed    hw;
UartHandler         uart;
Encoder             menu_encoder;
Encoder             one_encoder;
Encoder             two_encoder;
Encoder             three_encoder;
Switch              forward_button;
Switch              backward_button;
MidiUartHandler     midi;

// Buffers
uint8_t DMA_BUFFER_MEM_SECTION tx_buffer[64];   // Arduino
FIFO<MidiEvent, 128> event_log;                 // Midi messages
char outstr[128];                               // debugging

// Wavetables (add static WT definitions below, very long arrays, or maybe just put them in another file)
Wavetable wt;

// ADC inputs
volatile float volume = 0.0f;      // Volume knob/pot
volatile float wt_index = 0.0f;    // wavetable index knob/pot
volatile float attack = 0.0;       // ADSR values
volatile float decay = 0.0f;       // wavetable index knob/pot
volatile float sustain = 0.0f;     // wavetable index knob/pot
volatile float release = 0.0f;     // wavetable index knob/pot

// Encoders
volatile int   menu_counter = 0;
volatile int   one_counter = 0;
volatile int   two_counter = 0;
volatile int   three_counter = 0;

// ADSR Variables
Adsr adsr[8];
bool notes_on[8] = {0,0,0,0,0,0,0,0}; // bool to store if a note is currently being played
const float MAX_A = 7;
const float MAX_D = 2.5;
const float MAX_R = 1.75;

// Clock divider for polling
uint32_t now = System::GetNow();
uint32_t log_time = now; 

// Audio output variables
std::array<float, 8> oscillators_out;
float mix = 0;

// Variables
//uint32_t last_send = 0;
uint8_t menu = 0; 
uint8_t loc = 0;
uint8_t menu_atk = 0;
uint8_t menu_dec = 0;
uint8_t menu_sus = 0;
uint8_t menu_rel = 0;
uint8_t preset = 0;
uint8_t sub_menu = 0;
uint8_t fx_type = 0;
uint8_t fx_number = 0;
bool    fx_select = 0;
bool    last_fx_select = 0;
const uint8_t NUMBER_OF_FX = 4;

uint8_t fx_val_01 = 0;
uint8_t fx_val_02 = 0;
uint8_t fx_val_03 = 0;
uint8_t fx_val_11 = 0;
uint8_t fx_val_12 = 0;
uint8_t fx_val_13 = 0;

uint8_t amp[8] = {0};
uint8_t shape[8] = {0};
uint8_t phase[8] = {0}; 


// Function Declarations
void GetMidiTypeAsString(MidiEvent& msg, char* str);
void AudioCallback(AudioHandle::InputBuffer  in, AudioHandle::OutputBuffer out, size_t size);
void packetSend();
void resetValues();



int main(void){
    /** Initialization */ 
{   // Init here
    hw.Init();
    hw.SetAudioBlockSize(64);
    hw.StartLog(false); // set this to true to wait for terminal connection
    float sample_rate = hw.AudioSampleRate();
    
    // Initialize Wavetables
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

    // UART Config (Pins 14 TX, 15 RX)
    UartHandler::Config uart_conf;
    uart_conf.baudrate      = 115200;
    uart_conf.periph        = UartHandler::Config::Peripheral::USART_1;
    uart_conf.stopbits      = UartHandler::Config::StopBits::BITS_1;
    uart_conf.parity        = UartHandler::Config::Parity::NONE;
    uart_conf.wordlength    = UartHandler::Config::WordLength::BITS_8;
    uart_conf.mode          = UartHandler::Config::Mode::TX_RX;
    uart_conf.pin_config.tx = {DSY_GPIOB, 6}; // Pin 14
    uart_conf.pin_config.rx = {DSY_GPIOB, 7}; // Pin 15
    uart.Init(uart_conf);

    // Initalize Midi and Start receiving
    MidiUartHandler::Config midi_config;
    midi.Init(midi_config);
    midi.StartReceive();

    // ADC Config 
    // volume on pin 15/channel 0
    // wt index on pin 16/channel 1
    // ADSR is on pins 17 18 19 20/ channels 2 3 4 5 
    AdcChannelConfig adc_config[6];
    adc_config[0].InitSingle(A0); // volume     // 15
    adc_config[1].InitSingle(A1); // wt index   // 16
    adc_config[2].InitSingle(A2); // a          // 17
    adc_config[3].InitSingle(A3); // d          // 18
    adc_config[4].InitSingle(A4); // s          // 19
    adc_config[5].InitSingle(A5); // r          // 20
    hw.adc.Init(adc_config, 6);
    hw.adc.Start();
    
    // Encoder Config
    menu_encoder.Init(hw.GetPin(0), hw.GetPin(1), hw.GetPin(2));
    one_encoder.Init(hw.GetPin(3), hw.GetPin(4), hw.GetPin(5));
    two_encoder.Init(hw.GetPin(6), hw.GetPin(7), hw.GetPin(8));
    three_encoder.Init(hw.GetPin(9), hw.GetPin(10), hw.GetPin(11));

    // Switch Config
    forward_button.Init(hw.GetPin(29), 1000 / 1000);
    backward_button.Init(hw.GetPin(30), 1000 / 1000);

    // ADSR Initialization
    // currently using arbitrary values for testing
    for(int i = 0; i < 8; i++){
        adsr[i].Init(sample_rate);
        adsr[i].SetAttackTime(0.15F, 0.75F);
        adsr[i].SetDecayTime(0.5F);
        adsr[i].SetSustainLevel(0.25F);
        adsr[i].SetReleaseTime(0.15F);
    }
}
    packetSend(); // Send initial screen packet

    hw.StartAudio(AudioCallback);

    int idx = wt.AddNote(69);               // testing
    notes_on[idx] = 1;                      // tells the ADSR that this oscillator is on
    adsr[idx].Retrigger(0);                 // retriggers the ADSR in case of overflow

    // Main Loop
    while(1) {
        System::Delay(1);
        now = System::GetNow();

        // MIDI Input
        midi.Listen();  
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
            event_log.PushBack(msg); // add the message to output queue
        }
        
        // Debounce inputs
        menu_encoder.Debounce();
        one_encoder.Debounce();
        two_encoder.Debounce();
        three_encoder.Debounce();
        forward_button.Debounce();
        backward_button.Debounce();

        // Find Encoder Values
        short  menu_inc = menu_encoder.Increment();
        short  one_inc = one_encoder.Increment();
        short  two_inc = two_encoder.Increment();
        short  three_inc = three_encoder.Increment();

        //Check Encoder Changes
        if(menu_inc != 0) menu_counter += menu_inc;
        if(one_inc != 0) one_counter += one_inc;
        if(two_inc != 0) two_counter += two_inc;
        if(three_inc != 0) three_counter += three_inc;

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
        
            //State Machine
            switch(menu) {
                case 0: { //Splash Screen
                    hw.DelayMs(500);
                    loc++;
                    //At the end of the 
                    if(loc > 3) {
                        menu++;
                        resetValues();
                    }
                    break;
                }
                case 1: { //Main Menu
                    menu_counter = menu_counter % 4; // Round off from 0-3 for four menu options
                    loc = menu_counter;              // Set the current menu location to the menu counter
                    if (forward_button.RisingEdge()) {// Forward button is pressed
                        switch(loc) {
                            case 0: menu = 2; break;
                            case 1: menu = 3; break;
                            case 2: menu = 4; break;
                            case 3: menu = 5; break;
                        }
                        resetValues();
                    }
                    break;
                }
                case 2: { //Wave Menu
                    int modulo = 0;             // Variable modulo 
                    if (preset >= 2 && preset <= 3)      modulo = 3;
                    else if (preset >= 4 && preset <= 5) modulo = 5;
                    else if (preset >= 6 && preset <= 7) modulo = 9;
                    menu_counter = menu_counter % modulo;
                    loc = menu_counter;

                    if(backward_button.RisingEdge()) {
                        menu = 1;
                        resetValues();
                    }

                    switch(loc) {
                        case 0:
                            preset = one_counter % 8;
                            break;
                        default:
                            amp[loc-1] = one_counter % 100 + 1;
                            shape[loc-1] = two_counter % 3;
                            phase[loc-1] = three_counter % 360;
                            break;
                    }
                    break;
                }
                case 3: { //Effects
                    if(sub_menu) {
                        menu_counter = menu_counter % NUMBER_OF_FX;
                        //loc = menu_counter;
                        fx_number = menu_counter;
                    }
                    else {
                        menu_counter = menu_counter % 2;
                        //loc = menu_counter;
                        fx_select = menu_counter;
                    }

                    fx_type = fx_number + fx_select * 10;

                    if(backward_button.RisingEdge()) {
                        if(sub_menu) {
                            sub_menu = 0;
                        }
                        else {
                            menu = 1;
                            resetValues();
                        }
                    }

                    if(forward_button.RisingEdge()) {
                        sub_menu = 1;
                    }

                    if (fx_select != last_fx_select) {
                        if (fx_select) {
                            one_counter = fx_val_11;
                            two_counter = fx_val_12;
                            three_counter = fx_val_13;
                        }
                        else {
                            one_counter = fx_val_01;
                            two_counter = fx_val_02;
                            three_counter = fx_val_03;
                        }
                        
                        last_fx_select = fx_select;  // Sync the tracker
                    }

                    if (fx_select) {
                        fx_val_11 = one_counter   % 100;
                        fx_val_12 = two_counter   % 100;
                        fx_val_13 = three_counter % 100;
                    }
                    else {
                        fx_val_01 = one_counter   % 100;
                        fx_val_02 = two_counter   % 100;
                        fx_val_03 = three_counter % 100;
                    }
                    break;
                }
                case 4: { //ADSR
                    if(backward_button.RisingEdge()) {
                        menu = 1;
                        resetValues();
                    }

                    menu_atk = attack;
                    menu_dec = decay;
                    menu_sus = sustain;
                    menu_rel = release;
                    break;
                }
                case 5: { //Settings
                    //No settings for now
                    menu = 1;
                    break;
                }
            }
            //Send packet at the end of the cycle
            packetSend();

            /* Start debugging/logs, disable later */

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

void packetSend() {
    int idx = 0;
    // 1. START BYTE (The Sync Byte)
    tx_buffer[idx++] = 0xAA; 

    // 2. HEADER
    tx_buffer[idx++] = menu;
    tx_buffer[idx++] = loc; // Location 1 for testing CHANGE VALUE LATER

    switch(menu) {
    // 3. GENERATE PAYLOADS
        case 0: 
            break;
        case 1: // Payload (26 bytes total)
            tx_buffer[idx++] = sub_menu;                              // 1 Sub-Menu
            for(int i=0; i<8; i++) tx_buffer[idx++] = amp[i]; // 8 Amps
            for(int i=0; i<8; i++) tx_buffer[idx++] = shape[i];       // 8 Shapes
            for(int i=0; i<8; i++) tx_buffer[idx++] = phase[i];      // 8 Phases
            tx_buffer[idx++] = preset;                         // 1 Preset
            break;
        case 2: // FX Menu (4 bytes payload)
            tx_buffer[idx++] = fx_type;  // Select=1, Type=10
            if(fx_select) {
                tx_buffer[idx++] = fx_val_11;  
                tx_buffer[idx++] = fx_val_12; 
                tx_buffer[idx++] = fx_val_13;
            }
            else {
                tx_buffer[idx++] = fx_val_01;  
                tx_buffer[idx++] = fx_val_02; 
                tx_buffer[idx++] = fx_val_03;
            }
            break;
        case 3: // Envelope (4 bytes payload)
            tx_buffer[idx++] = menu_atk; 
            tx_buffer[idx++] = menu_dec; 
            tx_buffer[idx++] = menu_sus; 
            tx_buffer[idx++] = menu_rel;
            break;
        case 4:
            tx_buffer[idx++] = 0xFF;
            break;
    }
        
    // Send via DMA
    uart.DmaTransmit(tx_buffer, idx, NULL, NULL, NULL);
}

void resetValues() {
    loc = 0;
    sub_menu = 0;
    menu_counter = 0;
    if(menu == 3) {
        one_counter = fx_val_01;
        two_counter = fx_val_02;
        three_counter = fx_val_03;
    }
    else {
        one_counter = 0;
        two_counter = 0;
        three_counter = 0;
    }
    fx_select = 0;
    //for(int i = 0; i < 6; i++) pot_values[i] = 0.0f;
}