#include "daisy_seed.h"

using namespace daisy;

//Function Definitions

void packetSend();
void resetValues();

// Hardware Classes
DaisySeed    hw;
UartHandler  uart;
Encoder      menu_encoder;
Encoder      one_encoder;
Encoder      two_encoder;
Encoder      three_encoder;
Switch       forward_button;
Switch       backward_button;

uint8_t DMA_BUFFER_MEM_SECTION tx_buffer[64];

// volatile is required for variables shared between the callback and main loop
volatile int   menu_counter = 0;
volatile int   one_counter = 0;
volatile int   two_counter = 0;
volatile int   three_counter = 0;
volatile float pot_values[6] = {0.0f};
//volatile bool  needs_uart_update = false;

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

int main(void) {
    hw.Init();

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

    //Encoder Config
    menu_encoder.Init(hw.GetPin(0), hw.GetPin(1), hw.GetPin(2));
    one_encoder.Init(hw.GetPin(3), hw.GetPin(4), hw.GetPin(5));
    two_encoder.Init(hw.GetPin(6), hw.GetPin(7), hw.GetPin(8));
    three_encoder.Init(hw.GetPin(9), hw.GetPin(10), hw.GetPin(11));

    //ADC Config
    AdcChannelConfig adc_config[6];
    adc_config[0].InitSingle(hw.GetPin(15));
    adc_config[1].InitSingle(hw.GetPin(16));
    adc_config[2].InitSingle(hw.GetPin(17));
    adc_config[3].InitSingle(hw.GetPin(18));
    adc_config[4].InitSingle(hw.GetPin(19));
    adc_config[5].InitSingle(hw.GetPin(20));
    hw.adc.Init(adc_config, 6);
    hw.adc.Start();

    //Switch Config
    forward_button.Init(hw.GetPin(29), 1000 / 1000);
    backward_button.Init(hw.GetPin(30), 1000 / 1000);

    //Send initial screen packet
    packetSend();

    while(1) {
        //Debounce Inputs
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

        // Update Potentiometers
        for(int i = 0; i < 6; i++) {
        float val = hw.adc.GetFloat(i);
        if(fabsf(val - pot_values[i]) > 0.01f) {
            pot_values[i] = val;          //Make them readable by the other side
        }

        //Check Encoder Changes
        if(menu_inc != 0) menu_counter += menu_inc;
        if(one_inc != 0) one_counter += one_inc;
        if(two_inc != 0) two_counter += two_inc;
        if(three_inc != 0) three_counter += three_inc;

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

                menu_atk = pot_values[2];
                menu_dec = pot_values[3];
                menu_sus = pot_values[4];
                menu_rel = pot_values[5];
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
    }
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
    if(menu = 3) {
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