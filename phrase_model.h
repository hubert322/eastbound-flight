#include <Meap.h>        // MEAP library, includes all dependent libraries, including all Mozzi modules
#include "tables/sin8192_int16.h"

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

public:
  mOscil<sin8192_int16_NUM_CELLS, AUDIO_RATE, int16_t> root;
  mOscil<sin8192_int16_NUM_CELLS, AUDIO_RATE, int16_t> third;
  mOscil<sin8192_int16_NUM_CELLS, AUDIO_RATE, int16_t> fifth;
  mOscil<sin8192_int16_NUM_CELLS, AUDIO_RATE, int16_t> seventh;

  Chord()
    : rootMidiNote(0), thirdInterval(0), fifthInterval(0), seventhInterval(0),
      root(sin8192_int16_DATA), third(sin8192_int16_DATA), fifth(sin8192_int16_DATA), seventh(sin8192_int16_DATA) {

    }

  Chord(int rootMidiNote, ChordQuality chordQuality)
    : rootMidiNote(rootMidiNote), thirdInterval(0), fifthInterval(0), seventhInterval(0),
      root(sin8192_int16_DATA), third(sin8192_int16_DATA), fifth(sin8192_int16_DATA), seventh(sin8192_int16_DATA) {
      setChord(rootMidiNote, chordQuality);
    }

  void setChord(int rootMidiNote, ChordQuality chordQuality) {
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

    root.setFreq(mtof(rootMidiNote));
    third.setFreq(mtof(rootMidiNote + thirdInterval));
    fifth.setFreq(mtof(rootMidiNote + fifthInterval));
    seventh.setFreq(mtof(rootMidiNote + seventhInterval));
  }
  
  int64_t nextChord() {
    return root.next() + third.next() + fifth.next() + seventh.next();
  }

  int* getMidiNotes() {
    int *notes = (int*) malloc(4 * sizeof(int));

    notes[0] = rootMidiNote;
    notes[1] = rootMidiNote + thirdInterval;
    notes[2] = rootMidiNote + fifthInterval;
    notes[3] = rootMidiNote + seventhInterval;

    return notes;
  }
};

class State {
public:
  Chord chords[10];  // Don't want to deal with dynamic allocation
  State *states[5];  // Don't want to deal with dynamic allocation

  int currChordSize = 0;
  int currStateSize = 0;

  State() {
    states[0] = this;
    ++currStateSize;
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

  State *nextState() {
    return states[meap.irand(0, currStateSize - 1)];
  }
};

ChordQuality scaleChordQualities[7] = { MAJOR_SEVENTH, MINOR_SEVENTH, MINOR_SEVENTH, MAJOR_SEVENTH, DOMINANT_SEVENTH, MINOR_SEVENTH, HALF_DIMINISHED_SEVENTH };
int scale[7] = { 0, 2, 4, 5, 7, 9, 11};
Chord scaleChords[7];

State tonicExpansionTonic;
State tonicExpansionPreDominant;
State tonicExpansionDominant;
State cadencePrePreDominant;
State cadencePreDominant;
State cadenceDominant;
State cadenceEnd;

namespace PhraseModel {
  State *createPhraseGraph(int tonicMidi) {
    for (int i = 0; i < 7; ++i) {
      scaleChords[i] = Chord(tonicMidi + scale[i], scaleChordQualities[i]);
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

    cadenceDominant.addChord(scaleChords[4]);
    cadenceDominant.addState(&cadenceEnd);

    // Skipping Half Cadence for now
    cadenceEnd.addChord(scaleChords[0]);
    cadenceEnd.addChord(scaleChords[5]);
    cadenceEnd.addState(&tonicExpansionTonic);

    return &tonicExpansionTonic;
  }
}