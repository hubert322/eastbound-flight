#ifndef EFFECTS_H_
#define EFFECTS_H_

#include <Meap.h>        // MEAP library, includes all dependent libraries, including all Mozzi modules
#include "enableable.h"

template<typename T = int32_t>
class Chorus : public mChorus<T>, public Enableable {
public:
  Chorus(float modFreq = 0.2, float modDepth = 0.05, float mix = 0.5)
    :mChorus<T>(modFreq, modDepth, mix) {
      // There's currently a bug in Meap where the constructor doesn't set the mix
      mChorus<T>::setMix(mix);
    }

  T next(T inSample) {
    if (isEnabled()) {
      return mChorus<T>::next(inSample);
    }
    return inSample;
  }
};

template<typename T = int32_t>
class Reverb : public mPlateReverb<T>, public Enableable {
public:
  Reverb (float decay = 0.6, float damping = 0.8, float bandwidth = 0.3, float mix = 0.5)
    : mPlateReverb<T>(decay, damping, bandwidth, mix) {}

  T next(T inSample) {
    if (isEnabled()) {
      return mPlateReverb<T>::next(inSample);
    }
    return inSample;
  }
};

#endif // EFFECTS_H_
