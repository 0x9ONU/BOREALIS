#include "daisy_seed.h"

using namespace daisy;

DaisySeed    hw;
UartHandler  uart;
uint8_t DMA_BUFFER_MEM_SECTION tx_buffer[64];

// Test variables
uint8_t test_menu = 0; 
uint32_t last_send = 0;
uint8_t loc_temp = 0;
uint8_t attack = 0;
uint8_t decay = 0;
uint8_t sustain = 0;
uint8_t release = 0;
uint8_t preset = 0;
uint8_t sub_menu = 0;
uint8_t fx_type = 0;

uint8_t fx_val_1 = 0;
uint8_t fx_val_2 = 0;
uint8_t fx_val_3 = 0;

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

    while(1) {
        // Send a test packet every second
        uint32_t now = System::GetNow();
        if(now - last_send > 500) {
            last_send = now;
            int idx = 0;

            // 1. START BYTE (The Sync Byte)
            tx_buffer[idx++] = 0xAA; 

            // 2. HEADER
            tx_buffer[idx++] = test_menu;
            tx_buffer[idx++] = loc_temp; // Location 1 for testing CHANGE VALUE LATER

            // 3. GENERATE PAYLOADS
            if (test_menu == 1) {
                //tx_buffer[idx++] = 2;    // Menu ID
                //tx_buffer[idx++] = 1;    // Location
            }
            if(test_menu == 2) {
                // Payload (26 bytes total)
                tx_buffer[idx++] = sub_menu;                              // 1 Sub-Menu
                for(int i=0; i<8; i++) tx_buffer[idx++] = 255; // 8 Amps
                for(int i=0; i<8; i++) tx_buffer[idx++] = 0 + i;       // 8 Shapes
                for(int i=0; i<8; i++) tx_buffer[idx++] = 0;      // 8 Phases
                tx_buffer[idx++] = preset;                         // 1 Preset
            }
            else if(test_menu == 3) { // FX Menu (4 bytes payload)
                tx_buffer[idx++] = fx_type; // Select=1, Type=10
                tx_buffer[idx++] = fx_val_1;  
                tx_buffer[idx++] = fx_val_2; 
                tx_buffer[idx++] = fx_val_3;
            }
            else if(test_menu == 4) { // Envelope (4 bytes payload)
                tx_buffer[idx++] = attack; 
                tx_buffer[idx++] = decay; 
                tx_buffer[idx++] = sustain; 
                tx_buffer[idx++] = release;
            }
            else if(test_menu == 5) { // Settings (1 byte payload)
                tx_buffer[idx++] = 0xFF;
            }

            // Send via DMA
            uart.DmaTransmit(tx_buffer, idx, NULL, NULL, NULL);

            //TEST: ALL MENUS
            
            switch(test_menu) {
                case 0:
                    loc_temp++;
                    if (loc_temp > 5) {
                        loc_temp = 0;
                        test_menu++;
                    }
                    break;
                case 1:
                    loc_temp++;
                    if (loc_temp > 4) {
                        loc_temp = 0;
                        test_menu++;
                    }
                    break;
                case 2:
                    loc_temp++;
                    if (loc_temp > 7) {
                        loc_temp = 0;
                        preset++;
                    }
                    if (preset > 7) {
                        preset = 0;
                        test_menu++;
                    }
                    break;
                case 3:
                    fx_val_1 += 20;
                    fx_val_2 += 20;
                    fx_val_3 += 20;
                    if (fx_val_1 > 99) {
                        fx_val_1 = 0;
                        fx_val_2 = 0;
                        fx_val_3 = 0;
                        fx_type++;

                        if (fx_type > 4) {
                            fx_type++;
                            test_menu++;
                        }
                    }
                    break;
                case 4:
                    attack += 10;
                    decay += 10;            
                    sustain += 10;
                    release += 10;
                    if (attack > 99) {
                        attack = 0;
                        decay = 0;
                        sustain = 0;
                        release = 0;
                        test_menu++;
                    }
                    break;
                case 5:
                    test_menu = 0;
                    break;
            }
            

            //TEST: Menu 3

            //TEST: Menu 4
            /*
            attack = ++attack % 100;
            decay = ++decay % 100;            
            sustain = ++sustain % 100;
            release = ++release % 100;
            */
            //Cycle Through locations for testings
            //preset = ++preset % 8;         
        }
    }
}