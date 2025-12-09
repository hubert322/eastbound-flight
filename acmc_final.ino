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
EventDelay clockMetro;

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

enum PotCtrl { MELODY_RHYTHM, CHORUS, REVERB, MELODY_2_SOUND, WIND_CONTROL, DRUM_CONTROL };
PotCtrl potCtrl = MELODY_RHYTHM;

int tonicMidi = 44;

int sixteenthLength = 500;

State *currState;

// Drums
// mSample<punk_rock_drums_NUM_CELLS, AUDIO_RATE, int16_t> punkRockDrums(punk_rock_drums_DATA);
mSample<neo_soul_drums_NUM_CELLS, AUDIO_RATE, int16_t> neoSoulDrums(neo_soul_drums_DATA);
bool playDrums = false; // Controlled by DIP 4
float drumSpeed = 1.0;
int drumVolume = 4095;

// Announcements
// #include "mandarin_land.h"
#include "landing_sample.h"
mSample<landing_sample_NUM_CELLS, AUDIO_RATE, int16_t> landingSample(landing_sample_DATA);
bool playAnnouncement = false;

// Performance
bool isPerformanceRunning = false;
unsigned long performanceStartTime = 0;

enum FlightPhase { BOARDING, TAKEOFF, CRUISING, PHASE_2, PHASE_3, LANDING };

FlightPhase currentFlightPhase = BOARDING;
unsigned long takeoffStartTime = 0;
const unsigned long TAKEOFF_DURATION = 15000; // 15 seconds

/*
WIND
*/
Wind wind;
int windVolTarget = 4095;
int windCutTarget = 255;
int windResTarget = 255;
float windVolCurrent = 4095.0;
float windCutCurrent = 255.0;
float windResCurrent = 255.0;

// Helper for Visualizer Data
String getVisualDescription(FlightPhase phase, String chordName) {
	// Simple mapping based on phase and chord tension/quality
	if (chordName.indexOf("Maj7") != -1)
		return "Crystal Clear";
	if (chordName.indexOf("Dom7") != -1)
		return "Stormy";
	if (chordName.indexOf("Min7") != -1)
		return "Cloudy";
	if (chordName.indexOf("HalfDim") != -1)
		return "Turbulent";
	return "Hazy";
}

String getVibeDescription(FlightPhase phase, String chordName) {
	switch (phase) {
	case BOARDING:
	case TAKEOFF:
	case CRUISING:
		return "Optimistic, Morning";
	case PHASE_2:
		return "Nostalgic, Sunset";
	case PHASE_3:
	case LANDING:
		return "Serious, Night";
	default:
		return "Unknown";
	}
}

void updateWindState() {
	if (potCtrl == WIND_CONTROL)
		return;

	String chordName = currentChord.getName();
	int targetVol = 1500;
	int targetCutoff = 100;
	int targetRes = 80;

	if (chordName.indexOf("Maj7") != -1) {
		targetVol = 1000;
		targetCutoff = 60;
		targetRes = 50;
	} else if (chordName.indexOf("Dom7") != -1) {
		targetVol = 3500;
		targetCutoff = 220;
		targetRes = 180;
	} else if (chordName.indexOf("Min7") != -1) {
		targetVol = 2000;
		targetCutoff = 120;
		targetRes = 100;
	} else if (chordName.indexOf("HalfDim") != -1) {
		targetVol = 3000;
		targetCutoff = 200;
		targetRes = 200;
	}

	windVolTarget = targetVol;
	windCutTarget = targetCutoff;
	windResTarget = targetRes;
}

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

	// Announcements
	landingSample.start();

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

	// Dip 4: Drums
	Serial.print("DIP 4 (Drums): ");
	Serial.print(playDrums ? "ON" : "OFF");
	Serial.print(" | Speed: ");
	Serial.print(drumSpeed);
	Serial.print(", Vol: ");
	Serial.println(drumVolume);

	// DIP 5: Sample
	Serial.print("DIP 5 (Sample): ");
	Serial.print(playAnnouncement ? "ON" : "OFF");
	Serial.print(" | Speed: ");
	Serial.print(drumSpeed);
	Serial.print(", Vol: ");
	Serial.println(drumVolume);

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
	case DRUM_CONTROL:
		Serial.println("DRUM_CONTROL");
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

	Serial.println("}");

	// Visualizer Data Block
	unsigned long elapsed = 0;
	if (isPerformanceRunning) {
		elapsed = (millis() - performanceStartTime) / 1000;
	}

	Serial.print("VISUAL:{");
	// Basic Info
	Serial.print("\"time\":");
	Serial.print(elapsed);
	Serial.print(",\"phase\":\"");
	switch (currentFlightPhase) {
	case BOARDING:
		Serial.print("BOARDING");
		break;
	case TAKEOFF:
		Serial.print("TAKEOFF");
		break;
	case CRUISING:
		Serial.print("CRUISING");
		break;
	case PHASE_2:
		Serial.print("PHASE_2");
		break;
	case PHASE_3:
		Serial.print("PHASE_3");
		break;
	case LANDING:
		Serial.print("LANDING");
		break;
	}
	Serial.print("\"");
	Serial.print(",\"chord\":\"");
	Serial.print(currentChord.getName());
	Serial.print("\"");
	Serial.print(",\"state\":\"");
	Serial.print(currState ? currState->getName() : "None");
	Serial.print("\"");
	Serial.print(",\"vibe\":\"");
	Serial.print(getVibeDescription(currentFlightPhase, currentChord.getName()));
	Serial.print("\"");
	Serial.print(",\"weather\":\"");
	Serial.print(getVisualDescription(currentFlightPhase, currentChord.getName()));
	Serial.print("\"");

	// Details Object
	Serial.print(",\"details\":{");

	// Melody
	Serial.print("\"mel\":{");
	Serial.print("\"on\":");
	Serial.print(melody.isEnabled() ? 1 : 0);
	Serial.print(",\"sw\":");
	Serial.print(swing);
	Serial.print(",\"len\":");
	Serial.print(sixteenthLength);
	Serial.print("},");

	// Chorus
	Serial.print("\"cho\":{");
	Serial.print("\"on\":");
	Serial.print(chorus.isEnabled() ? 1 : 0);
	Serial.print(",\"fr\":");
	Serial.print(storedChorusFreq);
	Serial.print(",\"dp\":");
	Serial.print(storedChorusDepth);
	Serial.print("},");

	// Reverb
	Serial.print("\"rev\":{");
	Serial.print("\"on\":");
	Serial.print(reverb.isEnabled() ? 1 : 0);
	Serial.print(",\"dec\":");
	Serial.print(storedReverbDecay);
	Serial.print(",\"mix\":");
	Serial.print(storedReverbMix);
	Serial.print("},");

	// Melody 2
	Serial.print("\"mel2\":{");
	Serial.print("\"on\":");
	Serial.print(melody2.isEnabled() ? 1 : 0);
	Serial.print(",\"mph\":\"");
	Serial.print(melody2.getMorphStatus());
	Serial.print("\"");
	Serial.print(",\"vol\":");
	Serial.print(melody2.getVolume());
	Serial.print("},");

	// Drums
	Serial.print("\"drm\":{");
	Serial.print("\"on\":");
	Serial.print(playDrums ? 1 : 0);
	Serial.print(",\"spd\":");
	Serial.print(drumSpeed);
	Serial.print(",\"vol\":");
	Serial.print(drumVolume);
	Serial.print("},");

	// Sample
	Serial.print("\"smp\":{");
	Serial.print("\"on\":");
	Serial.print(playAnnouncement ? 1 : 0);
	Serial.print(",\"spd\":");
	Serial.print(drumSpeed); // Note: using drumSpeed as per original printStatus
	Serial.print(",\"vol\":");
	Serial.print(drumVolume);
	Serial.print("},");

	// Wind
	Serial.print("\"wnd\":{");
	Serial.print("\"on\":");
	Serial.print(wind.isEnabled() ? 1 : 0);
	Serial.print(",\"cut\":");
	Serial.print(wind.getCutoff());
	Serial.print(",\"res\":");
	Serial.print(wind.getResonance());
	Serial.print(",\"vol\":");
	Serial.print(wind.getVolume());
	Serial.print("},");

	// Controls
	Serial.print("\"mod\":");
	Serial.print(modify ? 1 : 0);
	Serial.print(",\"pot\":\"");
	switch (potCtrl) {
	case MELODY_RHYTHM:
		Serial.print("MELODY_RHYTHM");
		break;
	case CHORUS:
		Serial.print("CHORUS");
		break;
	case REVERB:
		Serial.print("REVERB");
		break;
	case MELODY_2_SOUND:
		Serial.print("MELODY_2_SOUND");
		break;
	case DRUM_CONTROL:
		Serial.print("DRUM_CONTROL");
		break;
	case WIND_CONTROL:
		Serial.print("WIND_CONTROL");
		break;
	}
	Serial.print("\"");
	Serial.print(",\"vals\":[");
	Serial.print(meap.pot_vals[0]);
	Serial.print(",");
	Serial.print(meap.pot_vals[1]);
	Serial.print("]");

	Serial.print("}"); // End details
	Serial.println("}");
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
		updateWindState();

		if (currState == &authenticCadence || currState == &halfCadence || currState == &deceptiveCadence) {
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
			windCutTarget = map(meap.pot_vals[0], 0, 4095, 0, 255);
			windResTarget = map(meap.pot_vals[1], 0, 4095, 0, 255);
		}

		windVolTarget = meap.volume_val;
		if (abs(windVolTarget - lastVolume) > potEpsilon) {
			lastVolume = windVolTarget;
			printStatus();
		}
	} else if (potCtrl == DRUM_CONTROL) {
		// Drum Control
		// Pot 0: Speed (0.5x to 2.0x)
		// Pot 1: Volume
		float speedVal = map(meap.pot_vals[0], 0, 4095, 50, 200) / 100.0;
		if (abs(speedVal - drumSpeed) > 0.01) {
			drumSpeed = speedVal;
			neoSoulDrums.setSpeed(drumSpeed);
		}

		drumVolume = meap.pot_vals[1];

		systemVolume = meap.volume_val; // Volume knob still controls system volume?
		// "volume hijack" was for wind... sticking to system vol for now unless specified otherwise.
		// Wait, user said "volume with pot1". So Drum Volume is Pot 1. Main knob is likely still system.
	} else {
		systemVolume = meap.volume_val;
	}

	// Performance Logic
	if (isPerformanceRunning) {
		if (currentFlightPhase == TAKEOFF) {
			if (millis() - takeoffStartTime > TAKEOFF_DURATION) {
				currentFlightPhase = CRUISING;
				Serial.println("Auto Transition: TAKEOFF -> CRUISING");
				printStatus();
			}
		}
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

	// Apply Wind Smoothing
	float windAlpha = 0.02; // Adjust for "knob turn" speed
	windVolCurrent += (windVolTarget - windVolCurrent) * windAlpha;
	windCutCurrent += (windCutTarget - windCutCurrent) * windAlpha;
	windResCurrent += (windResTarget - windResCurrent) * windAlpha;

	wind.setVolume((int)windVolCurrent);
	wind.setCutOffAndResonance((int)windCutCurrent, (int)windResCurrent);

	// For visualizer
	if (clockMetro.ready()) {
		clockMetro.start(1000);
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
		out_sample += (neoSoulDrums.next() * drumVolume) >> 8; // Apply drum volume
	}
	if (playAnnouncement) {
		out_sample += landingSample.next() << 2;
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
			potCtrl = DRUM_CONTROL;
		} else { // Pad 4 released
			Serial.println("t4 released");
		}
		break;
	case 5:
		if (pressed) { // Pad 5 pressed
			Serial.println("t5 pressed");
			isPerformanceRunning = !isPerformanceRunning;
			if (isPerformanceRunning) {
				performanceStartTime = millis();
				currentFlightPhase = BOARDING;
				Serial.println("Performance Started!");
			} else {
				performanceStartTime = 0;
				currentFlightPhase = BOARDING;
				Serial.println("Performance Stopped.");
			}
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
			// Phase Transitions
			if (isPerformanceRunning) {
				if (currentFlightPhase == BOARDING) {
					currentFlightPhase = TAKEOFF;
					takeoffStartTime = millis();
					Serial.println("Transition: BOARDING -> TAKEOFF");
				} else if (currentFlightPhase == CRUISING) {
					currentFlightPhase = PHASE_2;
					Serial.println("Transition: CRUISING -> PHASE_2");
				} else if (currentFlightPhase == PHASE_2) {
					currentFlightPhase = PHASE_3;
					Serial.println("Transition: PHASE_2 -> PHASE_3");
				} else if (currentFlightPhase == PHASE_3) {
					currentFlightPhase = LANDING;
					Serial.println("Transition: PHASE_3 -> LANDING");
				} else if (currentFlightPhase == LANDING) {
					currentFlightPhase = BOARDING;
					Serial.println("Transition: LANDING -> BOARDING");
				}
			}
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
			playDrums = true;
		} else { // DIP 4 down
			Serial.println("d4 down");
			playDrums = false;
		}
		break;
	case 5:
		if (up) { // DIP 5 up
			Serial.println("d5 up");
			playAnnouncement = true;
			landingSample.start();
		} else { // DIP 5 down
			Serial.println("d5 down");
			playAnnouncement = false;
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
			modify = true;
		} else { // DIP 7 down
			Serial.println("d7 down");
			modify = false;
		}
		break;
	}
	printStatus();
}
