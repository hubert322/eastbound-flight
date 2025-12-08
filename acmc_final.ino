/*
	Basic template for working with a stock MEAP board.
*/

#define CONTROL_RATE 128 // Hz, powers of 2 are most reliable
#include <Meap.h>		 // MEAP library, includes all dependent libraries, including all Mozzi modules

Meap meap; // creates MEAP object to handle inputs and other MEAP library functions
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI); // defines MIDI in/out ports

// ---------- YOUR GLOBAL VARIABLES BELOW ----------
#include <tables/saw8192_int16.h> // loads saw wave
#include <tables/sin8192_int16.h>
#include <tables/sq8192_int16.h>  // loads square wave
#include <tables/tri8192_int16.h> // loads triangle wave
// #include "punk_rock_drums.h"
#include "neo_soul_drums.h"

#include "effects.h"
#include "melody.h"
#include "phrase_model.h"
#include "wind.h"

EventDelay chordMetro;
EventDelay melodyMetro;

Chord currentChord;

ChordVoice chordVoice;

Melody<sin8192_int16_NUM_CELLS, AUDIO_RATE, int16_t> melody(sin8192_int16_DATA);
Melody<sin8192_int16_NUM_CELLS, AUDIO_RATE, int16_t> melody2(sin8192_int16_DATA, "Sin");
int melodyNumber = 0;
float swing = 0;

// Effects
Chorus chorus(0.0, 0.0, 0.5);
Reverb reverb(0.0, 0.8, 0.3, 0.0);

bool modify = true;

// Effect State Tracking
float storedChorusFreq = 0.0;
float storedChorusDepth = 0.0;
float storedReverbDecay = 0.0;
float storedReverbMix = 0.0;
int systemVolume = 4095;

enum PotCtrl { MELODY_RHYTHM, CHORUS, REVERB, MELODY_2_SOUND, WIND_CONTROL };
PotCtrl potCtrl = MELODY_RHYTHM;

int tonicMidi = 44;

int sixteenthLength = 500;

State *currState;

// Drums
// mSample<punk_rock_drums_NUM_CELLS, AUDIO_RATE, int16_t> punkRockDrums(punk_rock_drums_DATA);
mSample<neo_soul_drums_NUM_CELLS, AUDIO_RATE, int16_t> neoSoulDrums(neo_soul_drums_DATA);
bool playDrums = false;

/*
WIND
*/
Wind wind;

void setup() {

	Serial.begin(115200); // begins Serial communication with computer
	meap.begin();		  // sets up MEAP object
	// sets up MIDI: baud rate, serialmode, rx pin, tx pin
	Serial1.begin(31250, SERIAL_8N1, meap.MEAP_MIDI_IN_PIN, meap.MEAP_MIDI_OUT_PIN);
	startMozzi(CONTROL_RATE); // starts Mozzi engine with control rate defined above

	// ---------- YOUR SETUP CODE BELOW ----------
	// Using a dummy head because we would call .nextState when the metronome ticks
	// This to ensure that we display the correct state in the printStatus function
	currState = new State("Dummy");
	currState->addState(PhraseModel::createPhraseGraph(tonicMidi));

	// Drums
	neoSoulDrums.setLoopingOn();

	// Melody 2 Configuration
	melody2.addTable(tri8192_int16_DATA, "Tri"); // Index 1
	melody2.addTable(sq8192_int16_DATA, "Sq");	 // Index 2
	melody2.addTable(saw8192_int16_DATA, "Saw"); // Index 3
	melody2.setWave1(0);						 // Sine
	melody2.setWave2(1);						 // Triangle
}

void loop() {
	audioHook(); // handles Mozzi audio generation behind the scenes
}

void printStatus() {
	Serial.println("\n--- Status ---");

	// Public Status
	Serial.print("Current State: ");
	if (currState != NULL) {
		Serial.println(currState->getName());
	} else {
		Serial.println("None");
	}

	// Current Chord
	Serial.print("Current Chord: ");
	Serial.println(currentChord.getName());

	// DIP 0: Melody
	Serial.print("DIP 0 (Melody): ");
	Serial.print(melody.isEnabled() ? "ON" : "OFF");
	Serial.print(" | Swing: ");
	Serial.print(swing);
	Serial.print(", Length: ");
	Serial.println(sixteenthLength);

	// DIP 1: Chorus
	Serial.print("DIP 1 (Chorus): ");
	Serial.print(chorus.isEnabled() ? "ON" : "OFF");
	Serial.print(" | Freq: ");
	Serial.print(storedChorusFreq);
	Serial.print(", Depth: ");
	Serial.println(storedChorusDepth);

	// DIP 2: Reverb
	Serial.print("DIP 2 (Reverb): ");
	Serial.print(reverb.isEnabled() ? "ON" : "OFF");
	Serial.print(" | Decay: ");
	Serial.print(storedReverbDecay);
	Serial.print(", Mix: ");
	Serial.println(storedReverbMix);

	// DIP 3: Melody2
	Serial.print("DIP 3 (Melody2): ");
	Serial.print(melody2.isEnabled() ? "ON" : "OFF");
	Serial.print(" | Morph: ");
	Serial.print(melody2.getMorphStatus());
	Serial.print(" (");
	Serial.print(melody2.getMorph());
	Serial.print(", Vol: ");
	Serial.print(melody2.getVolume());
	Serial.println(")");

	// DIP 6: Wind
	Serial.print("DIP 6 (Wind): ");
	Serial.print(wind.isEnabled() ? "ON" : "OFF");
	Serial.print(" | Cutoff: ");
	Serial.print(wind.getCutoff());
	Serial.print(", Res: ");
	Serial.print(wind.getResonance());
	Serial.print(", Vol: ");
	Serial.print(wind.getVolume());
	Serial.println(")");

	// Other Controls
	Serial.print("Drums: ");
	Serial.println(playDrums ? "ON" : "OFF");

	Serial.print("Modify Mode: ");
	Serial.println(modify ? "ON" : "OFF");

	Serial.print("Current PotCtrl: ");
	switch (potCtrl) {
	case MELODY_RHYTHM:
		Serial.println("MELODY_RHYTHM");
		break;
	case CHORUS:
		Serial.println("CHORUS");
		break;
	case REVERB:
		Serial.println("REVERB");
		break;
	case MELODY_2_SOUND:
		Serial.println("MELODY_2_SOUND");
		break;
	case WIND_CONTROL:
		Serial.println("WIND_CONTROL");
		break;
	}
	Serial.print("Current Pot Values: ");
	Serial.print(meap.pot_vals[0]);
	Serial.print(", ");
	Serial.println(meap.pot_vals[1]);
	Serial.println("--------------");
}

/** Called automatically at rate specified by CONTROL_RATE macro, most of your
 * code should live in here
 */
void updateControl() {
	meap.readInputs();
	// ---------- YOUR updateControl CODE BELOW ----------
	static int lastPot0 = -1;
	static int lastPot1 = -1;
	static int lastVolume = meap.volume_val;
	static int potEpsilon = 20;

	if (potCtrl == MELODY_RHYTHM && modify) {
		sixteenthLength = map(meap.pot_vals[1], 0, 4095, 100, 2000);
		// neoSoulDrums.setSpeed(250.0 / sixteenthLength);
	}

	if (chordMetro.ready()) {
		currState = currState->nextState();
		currentChord = currState->getChord();
		chordVoice.setChord(currentChord);

		if (currState == &cadenceEnd) {
			chordMetro.start(sixteenthLength * 2);
		} else {
			chordMetro.start(sixteenthLength);
		}

		melodyNumber = 0;
		melodyMetro.start(0);
		printStatus();
	}

	if (melodyMetro.ready()) {
		if (potCtrl == MELODY_RHYTHM && modify) {
			swing = map(meap.pot_vals[0], 0, 4095, 0, 100) / 100.0;
		}
		if (melodyNumber == 0 || melodyNumber == 2) {
			melodyMetro.start(sixteenthLength / 4 * (1 + swing));
		} else if (melodyNumber == 1 || melodyNumber == 3) {
			melodyMetro.start(sixteenthLength / 4 * (1 - swing));
		}
		int note = currentChord.getMidiNote(melodyNumber);
		melody.setFreq(mtof(note + 12));
		melody2.setFreq(mtof(note + 12));
		melodyNumber = (melodyNumber + 1) % 4;
	}

	// Wind
	if (potCtrl == WIND_CONTROL) {
		if (modify) {
			int cutoff = map(meap.pot_vals[0], 0, 4095, 0, 255);
			int resonance = map(meap.pot_vals[1], 0, 4095, 0, 255);
			wind.setCutOffAndResonance(cutoff, resonance);
		}

		wind.setVolume(meap.volume_val);
		if (abs(wind.getVolume() - lastVolume) > potEpsilon) {
			lastVolume = meap.volume_val;
			printStatus();
		}
	} else {
		systemVolume = meap.volume_val;
	}

	// Effects
	if (potCtrl == CHORUS && modify) {
		storedChorusFreq = map(meap.pot_vals[0], 0, 4095, 0, 500) / 100.0;
		storedChorusDepth = meap.pot_vals[1] / 4095.0;
		chorus.setModFreq(storedChorusFreq);
		chorus.setModDepth(storedChorusDepth);
	}
	if (potCtrl == REVERB && modify) {
		storedReverbDecay = meap.pot_vals[0] / 4095.0;
		storedReverbMix = meap.pot_vals[1] / 4095.0;
		reverb.setDecay(storedReverbDecay);
		reverb.setMix(storedReverbMix);
	}

	if (potCtrl == MELODY_2_SOUND && modify) {
		melody2.setMorph(meap.pot_vals[0]);
		melody2.setVolume(meap.pot_vals[1]);
	}

	if (abs(meap.pot_vals[0] - lastPot0) > potEpsilon || abs(meap.pot_vals[1] - lastPot1) > potEpsilon) {
		lastPot0 = meap.pot_vals[0];
		lastPot1 = meap.pot_vals[1];
		printStatus();
	}
}

/** Called automatically at rate specified by AUDIO_RATE macro, for calculating
 * samples sent to DAC, too much code in here can disrupt your output
 */
AudioOutput_t updateAudio() {
	int64_t melody_out = 0;
	melody_out = melody.next();
	melody_out = chorus.next(melody_out);
	melody_out = reverb.next(melody_out);

	melody_out += melody2.next() << 2;

	int64_t out_sample = chordVoice.next() + melody_out;
	if (playDrums) {
		out_sample += neoSoulDrums.next() << 4;
	}

	// Wind
	out_sample += wind.next();

	return StereoOutput::fromNBit(22, (out_sample * systemVolume) >> 12, (out_sample * systemVolume) >> 12);
}

/**
 * Runs whenever a touch pad is pressed or released
 *
 * int number: the number (0-7) of the pad that was pressed
 * bool pressed: true indicates pad was pressed, false indicates it was released
 */
void updateTouch(int number, bool pressed) {
	if (pressed) { // Any pad pressed
	} else {	   // Any pad released
	}
	switch (number) {
	case 0:
		if (pressed) { // Pad 0 pressed
			Serial.println("t0 pressed ");
			potCtrl = MELODY_RHYTHM;
			melody.setTable(sin8192_int16_DATA);
			// melody.setTable(sq8192_int16_DATA);
			// melody.setTable(saw8192_int16_DATA);
		} else { // Pad 0 released
			Serial.println("t0 released");
		}
		break;
	case 1:
		if (pressed) { // Pad 1 pressed
			Serial.println("t1 pressed");
			potCtrl = CHORUS;
		} else { // Pad 1 released
			Serial.println("t1 released");
		}
		break;
	case 2:
		if (pressed) { // Pad 2 pressed
			Serial.println("t2 pressed");
			potCtrl = REVERB;
		} else { // Pad 2 released
			Serial.println("t2 released");
		}
		break;
	case 3:
		if (pressed) { // Pad 3 pressed
			Serial.println("t3 pressed");
			potCtrl = MELODY_2_SOUND;
		} else { // Pad 3 released
			Serial.println("t3 released");
		}
		break;
	case 4:
		if (pressed) { // Pad 4 pressed
			Serial.println("t4 pressed");
			playDrums = !playDrums;
		} else { // Pad 4 released
			Serial.println("t4 released");
		}
		break;
	case 5:
		if (pressed) { // Pad 5 pressed
			Serial.println("t5 pressed");
		} else { // Pad 5 released
			Serial.println("t5 released");
		}
		break;
	case 6:
		if (pressed) { // Pad 6 pressed
			Serial.println("t6 pressed");
			potCtrl = WIND_CONTROL;
		} else { // Pad 6 released
			Serial.println("t6 released");
		}
		break;
	case 7:
		if (pressed) { // Pad 7 pressed
			Serial.println("t7 pressed");
			modify = !modify;
		} else { // Pad 7 released
			Serial.println("t7 released");
		}
		break;
	}

	printStatus();
}

/**
 * Runs whenever a DIP switch is toggled
 *
 * int number: the number (0-7) of the switch that was toggled
 * bool up: true indicated switch was toggled up, false indicates switch was
 * toggled
 */
void updateDip(int number, bool up) {
	if (up) { // Any DIP toggled up
	} else {  // Any DIP toggled down
	}
	switch (number) {
	case 0:
		if (up) { // DIP 0 up
			Serial.println("d0 up");
			melody.setEnabled(true);
		} else { // DIP 0 down
			Serial.println("d0 down");
			melody.setEnabled(false);
		}
		break;
	case 1:
		if (up) { // DIP 1 up
			Serial.println("d1 up");
			chorus.setEnabled(true);
		} else { // DIP 1 down
			Serial.println("d1 down");
			chorus.setEnabled(false);
		}
		break;
	case 2:
		if (up) { // DIP 2 up
			Serial.println("d2 up");
			reverb.setEnabled(true);
		} else { // DIP 2 down
			Serial.println("d2 down");
			reverb.setEnabled(false);
		}
		break;
	case 3:
		if (up) { // DIP 3 up
			Serial.println("d3 up");
			melody2.setEnabled(true);
		} else { // DIP 3 down
			Serial.println("d3 down");
			melody2.setEnabled(false);
		}
		break;
	case 4:
		if (up) { // DIP 4 up
			Serial.println("d4 up");
		} else { // DIP 4 down
			Serial.println("d4 down");
		}
		break;
	case 5:
		if (up) { // DIP 5 up
			Serial.println("d5 up");
		} else { // DIP 5 down
			Serial.println("d5 down");
		}
		break;
	case 6:
		if (up) { // DIP 6 up
			Serial.println("d6 up");
			wind.setEnabled(true);
		} else { // DIP 6 down
			Serial.println("d6 down");
			wind.setEnabled(false);
		}
		break;
	case 7:
		if (up) { // DIP 7 up
			Serial.println("d7 up");
		} else { // DIP 7 down
			Serial.println("d7 down");
		}
		break;
	}
	printStatus();
}
