#include "daisy_seed.h"
 
using namespace daisy;
using namespace daisy::seed;
 
DaisySeed hw;
 
int main(void) {
  // Initialize the Daisy Seed
  hw.Init();
  hw.StartLog();
 
  // Create a GPIO object
  GPIO my_button;
 
  // Initialize the GPIO object
  my_button.Init(D14, GPIO::Mode::INPUT, daisy::GPIO::Config::pulldown, daisy::GPIO::Speed::VERY_HIGH);
 
  while(1) {
    // And let's store the state of the button in a variable called "button_state"
    bool button_state = my_button.Read();
    hw.Print("%o", button_state);
  }
}