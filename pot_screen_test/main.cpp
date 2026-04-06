#include "daisy_seed.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

using namespace daisy;

DaisySeed    hw;
UartHandler  uart;
Encoder      my_encoder;

// DMA buffer for UART
uint8_t DMA_BUFFER_MEM_SECTION msg[16];

// volatile is required for variables shared between the callback and main loop
volatile int   my_counter = 0;
volatile float pot_values[4] = {0.0f};
volatile bool  needs_uart_update = false;

// This callback is triggered by the hardware audio clock.
// It is the most stable way to handle timing on the Daisy.
void MyCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    // We process sensors once per block
    my_encoder.Debounce();

    int  inc     = my_encoder.Increment();
    bool pressed = my_encoder.RisingEdge();

    // Check Pots
    for(int i = 0; i < 4; i++) {
        float val = hw.adc.GetFloat(i);
        if(fabsf(val - pot_values[i]) > 0.01f) {
            pot_values[i] = val;
            needs_uart_update = true;
        }
    }

    // Check Encoder
    if(inc != 0) {
        my_counter += inc;
        needs_uart_update = true;
    }
    if(pressed) {
        my_counter = 0;
        needs_uart_update = true;
    }

    // Since we aren't using audio, we just pass silence/input through
    for (size_t i = 0; i < size; i++) {
        out[0][i] = in[0][i];
        out[1][i] = in[1][i];
    }
}

int main(void) {
    hw.Init();

    // 1. UART Config (Pins 14 & 15)
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

    // 2. Encoder Config (Pins 0, 1, 2)
    my_encoder.Init(hw.GetPin(0), hw.GetPin(1), hw.GetPin(2));

    // 3. ADC Config (Pins 15, 16, 17, 18)
    AdcChannelConfig adc_config[4];
    adc_config[0].InitSingle(hw.GetPin(17));
    adc_config[1].InitSingle(hw.GetPin(18));
    adc_config[2].InitSingle(hw.GetPin(19));
    adc_config[3].InitSingle(hw.GetPin(20));
    hw.adc.Init(adc_config, 4);
    hw.adc.Start();

    // 4. Start the background callback
    hw.StartAudio(MyCallback);

    while(1) {
        // The main loop waits for the callback to flag a change
        if(needs_uart_update) {
            needs_uart_update = false; 

            int p1 = (int)(pot_values[0] * 100);
            int p2 = (int)(pot_values[1] * 100);
            int p3 = (int)(pot_values[2] * 100);
            int p4 = (int)(pot_values[3] * 100);

            int len = sprintf((char*)msg, "%d %d %d %d %d\n", 
                              my_counter, p1, p2, p3, p4);
            
            uart.DmaTransmit(msg, len, NULL, NULL, NULL);
        }
    }
}