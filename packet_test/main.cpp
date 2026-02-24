#include "daisy_seed.h"

using namespace daisy;

DaisySeed    hw;
UartHandler  uart;
uint8_t DMA_BUFFER_MEM_SECTION tx_buffer[64];

// Test variables
uint8_t test_menu = 2; 
uint32_t last_send = 0;
uint8_t loc_temp = 0;
uint8_t attack = 0;
uint8_t decay = 0;
uint8_t sustain = 0;
uint8_t release = 0;
uint8_t preset = 0;
uint8_t sub_menu = 1;

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

            // 3. GENERATE DUMMY PAYLOADS
            if (test_menu == 1) {
                //tx_buffer[idx++] = 2;    // Menu ID
                //tx_buffer[idx++] = 1;    // Location
            }
            if(test_menu == 2) {
                // Payload (26 bytes total)
                tx_buffer[idx++] = sub_menu;                              // 1 Sub-Menu
                for(int i=0; i<8; i++) tx_buffer[idx++] = 100 + i; // 8 Amps
                for(int i=0; i<8; i++) tx_buffer[idx++] = i % 3;       // 8 Shapes
                for(int i=0; i<8; i++) tx_buffer[idx++] = 10;      // 8 Phases
                tx_buffer[idx++] = preset;                         // 1 Preset
            }
            else if(test_menu == 3) { // FX Menu (4 bytes payload)
                tx_buffer[idx++] = 0b10001010; // Select=1, Type=10
                tx_buffer[idx++] = 50;  tx_buffer[idx++] = 100; tx_buffer[idx++] = 150;
            }
            else if(test_menu == 4) { // Envelope (4 bytes payload)
                tx_buffer[idx++] = attack; tx_buffer[idx++] = decay; tx_buffer[idx++] = sustain; tx_buffer[idx++] = release;
            }
            else if(test_menu == 5) { // Settings (1 byte payload)
                tx_buffer[idx++] = 0xFF;
            }

            // Send via DMA
            uart.DmaTransmit(tx_buffer, idx, NULL, NULL, NULL);

            //Test: Menu 2


            //TEST: Menu 4
            /*
            attack = ++attack % 100;
            decay = ++decay % 100;            
            sustain = ++sustain % 100;
            release = ++release % 100;
            */
            //Cycle Through locations for testings
            
            loc_temp++;
            sub_menu++;
            if(loc_temp > 8) {
                loc_temp = 0;
                sub_menu = 1;
                preset = ++preset % 8;         
            }
        }
    }
}