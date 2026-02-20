#include "daisy_seed.h"

using namespace daisy;

DaisySeed hw;
Encoder   my_encoder;

// This variable persists outside the loop to keep track of the count
int my_counter = 0;

int main(void)
{
    hw.Init();
    
    // Pins: A = 0, B = 1, Click = 2
    my_encoder.Init(hw.GetPin(0), hw.GetPin(1), hw.GetPin(2));

    hw.StartLog(false);

    while(1)
    {
        my_encoder.Debounce();

        // Get the relative change
        int inc = my_encoder.Increment();

        if (inc != 0)
        {
            // Add the change to our total count
            my_counter += inc;

            // Optional: Clamp the value between 0 and 100
            // if (my_counter < 0)   my_counter = 0;
            // if (my_counter > 100) my_counter = 100;

            hw.PrintLine("Current Count: %d", my_counter);
        }

        // Reset counter on button press
        if (my_encoder.RisingEdge())
        {
            my_counter = 0;
            hw.PrintLine("Counter Reset!");
        }

        System::Delay(1);
    }
}