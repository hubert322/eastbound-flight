#include "tables/sin8192_int16.h"
#include <Meap.h> // MEAP library, includes all dependent libraries, including all Mozzi modules

enum ChordQuality {    
	MAJOR_SEVENTH,
	DOMINANT_SEVENTH,
	MINOR_SEVENTH,
	HALF_DIMINISHED_SEVENTH,
	FULL_DIMINISHED_SEVENTH
};

class Chord {
  private:
	int rootMidiNote;
	int thirdInterval;
	int fifthInterval;
	int seventhInterval;
	ChordQuality quality;
	String romanNumeral;

  public:
	Chord() : rootMidiNote(0), thirdInterval(0), fifthInterval(0), seventhInterval(0), romanNumeral("") {}

	Chord(int rootMidiNote, ChordQuality chordQuality, String romanNumeral)
		: rootMidiNote(rootMidiNote), thirdInterval(0), fifthInterval(0), seventhInterval(0),
		  romanNumeral(romanNumeral) {
		setChord(rootMidiNote, chordQuality);
	}

	void setChord(int rootMidiNote, ChordQuality chordQuality) {
		this->rootMidiNote = rootMidiNote;
		this->quality = chordQuality;
		switch (chordQuality) {
		case MAJOR_SEVENTH:
			thirdInterval = 4;
			fifthInterval = 7;
			seventhInterval = 11;
			break;
		case DOMINANT_SEVENTH:
			thirdInterval = 4;
			fifthInterval = 7;
			seventhInterval = 10;
			break;
		case MINOR_SEVENTH:
			thirdInterval = 3;
			fifthInterval = 7;
			seventhInterval = 10;
			break;
		case HALF_DIMINISHED_SEVENTH:
			thirdInterval = 3;
			fifthInterval = 6;
			seventhInterval = 10;
			break;
		case FULL_DIMINISHED_SEVENTH:
			thirdInterval = 3;
			fifthInterval = 6;
			seventhInterval = 9;
			break;
		}
	}

	int getMidiNote(int index) const {
		if (index == 0)
			return rootMidiNote;
		if (index == 1)
			return rootMidiNote + thirdInterval;
		if (index == 2)
			return rootMidiNote + fifthInterval;
		if (index == 3)
			return rootMidiNote + seventhInterval;
		return rootMidiNote; // Fallback
	}

	ChordQuality getQuality() const { return quality; }

	String getQualityName() const {
		switch (quality) {
		case MAJOR_SEVENTH:
			return "Maj7";
		case DOMINANT_SEVENTH:
			return "Dom7";
		case MINOR_SEVENTH:
			return "Min7";
		case HALF_DIMINISHED_SEVENTH:
			return "HalfDim7";
		case FULL_DIMINISHED_SEVENTH:
			return "Dim7";
		default:
			return "Unknown";
		}
	}

	String getNoteName(int note) const {
		// String names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
		// Use flats for names instead because for this project we're doing Ab Major
		String names[] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
		return names[note % 12];
	}

	String getName() const { return getNoteName(rootMidiNote) + " " + getQualityName() + " (" + romanNumeral + ")"; }
};

class ChordVoice {
  public:
	mOscil<sin8192_int16_NUM_CELLS, AUDIO_RATE, int16_t> root;
	mOscil<sin8192_int16_NUM_CELLS, AUDIO_RATE, int16_t> third;
	mOscil<sin8192_int16_NUM_CELLS, AUDIO_RATE, int16_t> fifth;
	mOscil<sin8192_int16_NUM_CELLS, AUDIO_RATE, int16_t> seventh;

	ChordVoice()
		: root(sin8192_int16_DATA), third(sin8192_int16_DATA), fifth(sin8192_int16_DATA), seventh(sin8192_int16_DATA) {}

	void setChord(const Chord &c) {
		root.setFreq(mtof(c.getMidiNote(0)));
		third.setFreq(mtof(c.getMidiNote(1)));
		fifth.setFreq(mtof(c.getMidiNote(2)));
		seventh.setFreq(mtof(c.getMidiNote(3)));
	}

	int64_t next() { return root.next() + third.next() + fifth.next() + seventh.next(); }
};

class State {
  public:
	Chord chords[10]; // Don't want to deal with dynamic allocation
	State *states[5]; // Don't want to deal with dynamic allocation

	String name;
	int currChordSize = 0;
	int currStateSize = 0;

	State(String name, bool addSelf = true) : name(name) {
		if (addSelf) {
			states[0] = this;
			++currStateSize;
		}
	}

	void addChord(Chord chord) {
		chords[currChordSize] = chord;
		++currChordSize;
	}

	void addState(State *state) {
		states[currStateSize] = state;
		++currStateSize;
	}

	Chord getChord() {
		return chords[meap.irand(0, currChordSize - 1)];
	}

	String getName() {
		return name;
	}

	State *nextState() {
		return states[meap.irand(0, currStateSize - 1)];
	}
};

ChordQuality scaleChordQualities[7] = { MAJOR_SEVENTH, MINOR_SEVENTH, MINOR_SEVENTH, MAJOR_SEVENTH, DOMINANT_SEVENTH, MINOR_SEVENTH, HALF_DIMINISHED_SEVENTH };
int scale[7] = { 0, 2, 4, 5, 7, 9, 11};
String numerals[7] = {"I", "ii", "iii", "IV", "V", "vi", "vii"};
Chord scaleChords[7];

State tonicExpansionTonic("Tonic Expansion Tonic");
State tonicExpansionPreDominant("Tonic Expansion Pre-Dominant");
State tonicExpansionDominant("Tonic Expansion Dominant");
State cadencePrePreDominant("Cadence Pre-Pre-Dominant");
State cadencePreDominant("Cadence Pre-Dominant");
State cadenceDominant("Cadence Dominant");
State authenticCadence("Authentic Cadence", false);
State halfCadence("Half Cadence", false);
State deceptiveCadence("Deceptive Cadence", false);

namespace PhraseModel {
	State *createPhraseGraph(int tonicMidi) {
		for (int i = 0; i < 7; ++i) {
			scaleChords[i] = Chord(tonicMidi + scale[i], scaleChordQualities[i], numerals[i]);
		}

		tonicExpansionTonic.addChord(scaleChords[0]);
		tonicExpansionTonic.addState(&tonicExpansionPreDominant);
		tonicExpansionTonic.addState(&tonicExpansionDominant);
		tonicExpansionTonic.addState(&cadencePrePreDominant);
		tonicExpansionTonic.addState(&cadencePreDominant);

		tonicExpansionPreDominant.addChord(scaleChords[1]);
		tonicExpansionPreDominant.addChord(scaleChords[3]);
		tonicExpansionPreDominant.addState(&tonicExpansionDominant);

		tonicExpansionDominant.addChord(scaleChords[4]);
		tonicExpansionDominant.addChord(scaleChords[6]);
		tonicExpansionDominant.addState(&tonicExpansionTonic);

		cadencePrePreDominant.addChord(scaleChords[2]);
		cadencePrePreDominant.addChord(scaleChords[5]);
		cadencePrePreDominant.addState(&cadencePreDominant);

		cadencePreDominant.addChord(scaleChords[1]);
		cadencePreDominant.addChord(scaleChords[3]);
		cadencePreDominant.addState(&cadenceDominant);
		cadencePreDominant.addState(&halfCadence);

		cadenceDominant.addChord(scaleChords[4]);
		cadenceDominant.addState(&authenticCadence);
		cadenceDominant.addState(&deceptiveCadence);

		// Cadences
		authenticCadence.addChord(scaleChords[0]);
		authenticCadence.addState(&tonicExpansionTonic);

		halfCadence.addChord(scaleChords[4]);
		halfCadence.addState(&tonicExpansionTonic);

		deceptiveCadence.addChord(scaleChords[5]);
		deceptiveCadence.addState(&tonicExpansionTonic);

		// Print out all the chords in each State
		Serial.println("Tonic Expansion Tonic");
		for (int i = 0; i < tonicExpansionTonic.currChordSize; ++i) {
			Serial.println(tonicExpansionTonic.chords[i].getName());
		}
		Serial.println("Tonic Expansion Pre-Dominant");
		for (int i = 0; i < tonicExpansionPreDominant.currChordSize; ++i) {
			Serial.println(tonicExpansionPreDominant.chords[i].getName());
		}
		Serial.println("Tonic Expansion Dominant");
		for (int i = 0; i < tonicExpansionDominant.currChordSize; ++i) {
			Serial.println(tonicExpansionDominant.chords[i].getName());
		}
		Serial.println("Cadence Pre-Pre-Dominant");
		for (int i = 0; i < cadencePrePreDominant.currChordSize; ++i) {
			Serial.println(cadencePrePreDominant.chords[i].getName());
		}
		Serial.println("Cadence Pre-Dominant");
		for (int i = 0; i < cadencePreDominant.currChordSize; ++i) {
			Serial.println(cadencePreDominant.chords[i].getName());
		}
		Serial.println("Cadence Dominant");
		for (int i = 0; i < cadenceDominant.currChordSize; ++i) {
			Serial.println(cadenceDominant.chords[i].getName());
		}
		Serial.println("Authentic Cadence");
		for (int i = 0; i < authenticCadence.currChordSize; ++i) {
			Serial.println(authenticCadence.chords[i].getName());
		}
		Serial.println("Half Cadence");
		for (int i = 0; i < halfCadence.currChordSize; ++i) {
			Serial.println(halfCadence.chords[i].getName());
		}
		Serial.println("Deceptive Cadence");
		for (int i = 0; i < deceptiveCadence.currChordSize; ++i) {
			Serial.println(deceptiveCadence.chords[i].getName());
		}

		return &tonicExpansionTonic;
	}
}
