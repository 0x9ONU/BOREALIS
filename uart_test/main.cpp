#include "daisy_seed.h"
#include <string.h>

using namespace daisy;

DaisySeed hw;
UartHandler uart;
uint8_t DMA_BUFFER_MEM_SECTION msg[16]; 

int main(void) {
    hw.Init();

    UartHandler::Config uart_conf;
    uart_conf.baudrate      = 9600; // Lowered to 9600 for maximum stability with Arduino
    uart_conf.periph        = UartHandler::Config::Peripheral::USART_1;
    uart_conf.stopbits      = UartHandler::Config::StopBits::BITS_1;
    uart_conf.parity        = UartHandler::Config::Parity::NONE;
    uart_conf.wordlength    = UartHandler::Config::WordLength::BITS_8;
    uart_conf.mode          = UartHandler::Config::Mode::TX_RX;
    uart_conf.pin_config.tx = {DSY_GPIOB, 6}; 
    uart_conf.pin_config.rx = {DSY_GPIOB, 7};

    uart.Init(uart_conf);

    int my_number = 0;

    while(1) {
        // Create a string like "42\n"
        int len = sprintf((char*)msg, "%d\n", my_number);

        uart.DmaTransmit(msg, len, NULL, NULL, NULL);

        // Increment and reset if it gets too high for a basic UI
        my_number++;
        if(my_number > 100) my_number = 0;

        hw.DelayMs(500); 
    }
}