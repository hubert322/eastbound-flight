#include "enableable.h"

#include <Meap.h>
#include <string>

template <unsigned int NUM_CELLS, unsigned int UPDATE_RATE, class T = int8_t>
class Melody : public mOscil<NUM_CELLS, UPDATE_RATE, T>, public Enableable {
  private:
	const T *tables[4];
	std::string tableNames[4];
	std::string statusString;
	int tableCount = 0;
	int wave1Idx = 0;
	int wave2Idx = -1; // -1 indicates disabled or not set
	int mixVal = 0;	   // 0 = 100% wave1, 4095 = 100% wave2
	int morphVal = 0;  // Store the current morph input value
	mOscil<NUM_CELLS, UPDATE_RATE, T> osc2;

  public:
	Melody(const T *table_data, std::string name = "Wave")
		: mOscil<NUM_CELLS, UPDATE_RATE, T>(table_data), osc2(table_data) {
		tables[0] = table_data;
		tableNames[0] = name;
		tableCount = 1;
	}

	void addTable(const T *table, std::string name) {
		if (tableCount < 4) {
			tables[tableCount] = table;
			tableNames[tableCount] = name;
			tableCount++;
		}
	}

	void setWave1(int index) {
		if (index >= 0 && index < tableCount) {
			wave1Idx = index;
			this->setTable(tables[index]);
		}
	}

	void setWave2(int index) {
		if (index >= 0 && index < tableCount) {
			wave2Idx = index;
			osc2.setTable(tables[index]);
		}
	}

	void setMix(int mix) { mixVal = mix; }

	int getMix() { return mixVal; }

	// Volume control (0-4095)
	int volume = 4095;

	void setVolume(int vol) {
		volume = constrain(vol, 0, 4095);
	}

	int getVolume() { return volume; }

	// Linear morph through the first 4 tables based on 0-4095 input
	void setMorph(int val) {
		morphVal = val;
		if (tableCount < 2)
			return; // Need at least 2 tables to mix

		if (val < 1365) {
			// Segment 1: Table 0 -> Table 1
			setWave1(0);
			if (tableCount > 1)
				setWave2(1);
			setMix(map(val, 0, 1365, 0, 4095));
		} else if (val < 2730) {
			// Segment 2: Table 1 -> Table 2
			// If we don't have enough tables, stick to the last one
			if (tableCount > 1)
				setWave1(1);
			if (tableCount > 2)
				setWave2(2);
			setMix(map(val, 1366, 2730, 0, 4095));
		} else {
			// Segment 3: Table 2 -> Table 3
			if (tableCount > 2)
				setWave1(2);
			if (tableCount > 3)
				setWave2(3);
			setMix(map(val, 2731, 4095, 0, 4095));
		}
	}

	int getMorph() { return morphVal; }

	// Returns a pointer to a string literal describing the current state
	const char *getMorphStatus() {
		if (tableCount < 2)
			return tableNames[0].c_str(); // Fallback if no second table

		int idx1 = wave1Idx;
		int idx2 = (wave2Idx != -1) ? wave2Idx : wave1Idx;

		statusString = tableNames[idx1] + "->" + tableNames[idx2];
		return statusString.c_str();
	}

	void setFreq(float freq) {
		mOscil<NUM_CELLS, UPDATE_RATE, T>::setFreq(freq);
		osc2.setFreq(freq);
	}

	T next() {
		if (Enableable::isEnabled()) { // Explicitly qualify isEnabled
			T out1 = mOscil<NUM_CELLS, UPDATE_RATE, T>::next();
			int32_t mixedOutput;
			if (wave2Idx != -1) {
				T out2 = osc2.next();
				// Mix logic: (out1 * (4095 - mix)) + (out2 * mix) >> 12
				mixedOutput = ((int32_t)out1 * (4095 - mixVal) + (int32_t)out2 * mixVal) >> 12;
			} else {
				mixedOutput = out1;
			}
			// Apply volume scaling
			return (T)((mixedOutput * volume) >> 12);
		}
		return 0;
	}
};
