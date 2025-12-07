#include "enableable.h"

#include <Meap.h>

template <unsigned int NUM_CELLS, unsigned int UPDATE_RATE, class T = int8_t>
class Melody : public mOscil<NUM_CELLS, UPDATE_RATE, T>, public Enableable {
public:
  Melody(const T *table_data) : mOscil<NUM_CELLS, UPDATE_RATE, T>(table_data) {}
};
