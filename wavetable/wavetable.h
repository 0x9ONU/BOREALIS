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
    float Process(float target_f, float index, uint8_t oscillator);

  private:
    std::array<std::array<float, 2048>, 8> waves;
    std::array<float, 8> phases;
    //float waves[8][2048];       // table to hold up to 8 waves of 2048 samples each
    int num_waves;              // number of waves in the table
    //float phases[8];            // the current phase value of each oscillator (up to 8)
    float fs;                   // sampling frequency (44.1 kHz probably)

};
