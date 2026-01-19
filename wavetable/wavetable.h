#include <stdint.h>
#include <array>


/** Class for containing the data on 1 wavetable
*/
class Wavetable
{
  public:
    Wavetable(float samp_freq, const std::array<std::array<float, 2048>, 8>& wavesin, int num_waves);
    Wavetable::Wavetable();
    ~Wavetable() {}
    void Init(float samp_freq, const std::array<std::array<float, 2048>, 8>& wavesin, int num_waves);
    float Process(uint8_t oscillator);
    void SetFreq(int osc, float freq);
    void SetIndex(int osc, float index);

  private:
    std::array<std::array<float, 2048>, 8> waves; // table to hold up to 8 waves of 2048 samples each
    int num_waves;                                // actual number of waves in the table, up to 8
    std::array<float, 8> frequencies;               // the current frequency  of each oscillator 
    std::array<float, 8> phases;                  // the current phase value of each oscillator
    std::array<float, 8> indicies;                // the current wave index of each oscillator
    float fs;                                     // sampling frequency (44.1 kHz probably)

    //float waves[8][2048];                       // table to hold up to 8 waves of 2048 samples each
    //float phases[8];                            // the current phase value of each oscillator (up to 8)

};
