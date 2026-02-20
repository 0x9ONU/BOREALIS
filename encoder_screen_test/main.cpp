#include "daisy_seed.h"
#include <string.h>
#include <stdio.h>

using namespace daisy;

DaisySeed hw;
UartHandler uart;
Encoder my_encoder;

// DMA buffer for UART transmission
uint8_t DMA_BUFFER_MEM_SECTION msg[16]; 

int my_counter = 0;

int main(void) {
    hw.Init();

    // 1. Configure UART
    UartHandler::Config uart_conf;
    uart_conf.baudrate      = 9600; 
    uart_conf.periph        = UartHandler::Config::Peripheral::USART_1;
    uart_conf.stopbits      = UartHandler::Config::StopBits::BITS_1;
    uart_conf.parity        = UartHandler::Config::Parity::NONE;
    uart_conf.wordlength    = UartHandler::Config::WordLength::BITS_8;
    uart_conf.mode          = UartHandler::Config::Mode::TX_RX;
    // Pin 14 (TX) and Pin 15 (RX) on Daisy Seed correspond to PB6 and PB7
    uart_conf.pin_config.tx = {DSY_GPIOB, 6}; 
    uart_conf.pin_config.rx = {DSY_GPIOB, 7};
    uart.Init(uart_conf);

    // 2. Configure Encoder (Pins 0, 1, and 2)
    my_encoder.Init(hw.GetPin(0), hw.GetPin(1), hw.GetPin(2));

    while(1) {
        // Essential: Keep debouncing the encoder
        my_encoder.Debounce();

        int inc = my_encoder.Increment();
        bool pressed = my_encoder.RisingEdge();
        bool needs_update = false;

        // Handle rotation
        if (inc != 0) {
            my_counter += inc;
            needs_update = true;
        }

        // Handle reset on click
        if (pressed) {
            my_counter = 0;
            needs_update = true;
        }

        // 3. Only send data if something actually changed
        if (needs_update) {
            // Format the string with a newline for the receiver
            int len = sprintf((char*)msg, "%d\n", my_counter);
            
            // Send via DMA
            uart.DmaTransmit(msg, len, NULL, NULL, NULL);
        }

        // Small delay to prevent the CPU from redlining, 
        // while remaining responsive to turns.
        hw.DelayMs(1); 
    }
}