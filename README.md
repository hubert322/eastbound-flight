# Eastbound Flight 長途班機

This project implements an interactive musical flight simulator using the MEAP board. It combines a generative music performance with a synchronized "Cabin Visualizer" web application.

## Controls

The system's controls are organized by index (0-7), grouping the Touch Pad, DIP Switch, and Potentiometer functions for each channel.

### Control Map

| # | Name | Touch Pad (Mode Select) | DIP Switch (Enable/Disable) | Potentiometer 0 (Needs DIP 7 ON) | Potentiometer 1 (Needs DIP 7 ON) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **0** | **Melody Rhythm** | Select Mode | Enable Melody | Swing Amount (0-100%) | Tempo (Sixteenth Length) |
| **1** | **Chorus** | Select Mode | Enable Chorus | Mod Frequency | Mod Depth |
| **2** | **Reverb** | Select Mode | Enable Reverb | Decay Time | Mix Level |
| **3** | **Melody 2** | Select Mode | Enable Melody 2 | Wave Morph (Sin->Saw) | Volume |
| **4** | **Drums** | Select Mode | Enable Drums | Speed (0.5x - 2.0x) | Drum Volume |
| **5** | **Perf/Sample** | **Start/Stop** Performance | Enable Announcement | *None* | *None* |
| **6** | **Wind** | Select Mode | Enable Wind | Cutoff Frequency | Resonance |
| **7** | **System** | **Phase Advance** | **Modify Mode** (On/Off) | *None* | *None* |

> **Note on Volume Controls:**
> *   **Global Volume:** The main hardware Volume Knob controls system volume in all modes **EXCEPT** Wind (6).
> *   **Wind Volume:** When **Pad 6 (Wind)** is selected, the Volume Knob controls the specific volume of the wind synthesizer ("Volume Hijack").
> *   **Modify Mode (DIP 7):** Must be **ON** for Potentiometers 0 & 1 to affect parameters in modes 0, 1, 2, 3, and 6.

## Visuals

The project includes a web-based **Cabin Visualizer** (`cabin_visualizer.html`) that reads status data from the Arduino via Serial. It defaults to an immersive passenger window view.

### Flight Phases & Environment
The visualizer changes the environment based on the current flight phase:

*   **BOARDING:** Ground view (runway/city), Morning sky. Plane is stationary.
*   **TAKEOFF:** Accelerating down runway, "shake" effects, climbing into sky.
*   **CRUISING:** High altitude clouds, Morning sky (Blue/Cyan).
*   **PHASE 2:** Sunset colors (Orange/Gold/Purple), nostalgic vibe.
*   **PHASE 3:** Night colors (Dark Blue/Indigo), starry sky, serious vibe.
*   **LANDING:** Descent through clouds, city lights appear, landing on runway.

### Dynamic Weather
The weather outside the window reacts to the musical harmonies (Chords/Cadences):
*   **Crystal Clear:** Tonic chords. Clear skies.
*   **Wisps / Clouds:** Pre-dominant chords. Light turbulence.
*   **The Bump:** Dominant chords. Larger clouds, shaking.
*   **The Storm:** Intense musical tension. Heavy rain, lightning, dark clouds, heavy shaking.
*   **The Turn:** Banking turn simulation.

### In-Flight Screen
A simulated seatback screen displays real-time flight data:
*   **Status/Phase:** Current flight phase (e.g., CRUISING).
*   **Time:** Elapsed performance time.
*   **State/Chord:** Current musical state and chord name.
*   **Weather/Vibe:** Text description of the current visual generation parameters.
*   **Indicators:** Icons for Drums and Sample playback.

### Calibration & Customization
A hidden control panel allows for extensive customization:
*   **Window/Screen:** Adjust position, size, skew, and perspective of the window and screen elements.
*   **Tech Overlay (Shift + D):** Detailed readout of all raw Arduino parameters (frequencies, values).
*   **Sky Colors:** Selectable color palettes for Morning, Sunset, and Night phases.
*   **Config:** Save and load calibration settings to JSON.
