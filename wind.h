#ifndef WIND_H
#define WIND_H

#include "enableable.h"
#include <Meap.h>
#include <tables/whitenoise8192_int8.h>

class Wind : public Enableable {
  private:
	mOscil<WHITENOISE8192_NUM_CELLS, AUDIO_RATE> white_noise;
	MultiResonantFilter<uint8_t> filter;
	int volume = 4095;
	int cutoff = 255;
	int resonance = 255;

  public:
	Wind() : white_noise(WHITENOISE8192_DATA) {
		white_noise.setFreq(1.0f); // Standard reading rate for noise
	}

	void setVolume(int vol) { volume = constrain(vol, 0, 4095); }

	int getVolume() { return volume; }

	void setCutOffAndResonance(int cutoff, int resonance) {
		this->cutoff = cutoff;
		this->resonance = resonance;
		filter.setCutoffFreqAndResonance(cutoff, resonance);
	}

	int getCutoff() { return cutoff; }

	int getResonance() { return resonance; }

	int64_t next() {
		if (Enableable::isEnabled()) {
			int64_t sample = white_noise.next();
			filter.next(sample);
			int64_t filtered = filter.low(); // Using low pass as per original code behavior

			// Apply volume and scale.
			// Original code: out_sample += white_noise_out_sample << 4;
			// Requirement: "max of the white noise sample is << 8"
			// If input is 8-bit (-128 to 127) and volume is 0-4095 (12-bit).
			// (sample * volume) >> 4 gives roughly (128 * 4096) / 16 = 32768 range (16-bit).
			// Effectively same as sample << 8 when volume is max.
			return (filtered * volume) >> 4;
		}
		return 0;
	}
};

#endif
