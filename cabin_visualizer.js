/**
 * Cabin Visualizer - ACMC Flight
 * Real-time airplane cabin visualization synced to Arduino performance
 * Uses p5.js for weather visualization (from best-visualizer.js)
 */

// ============================================================================
// State Management
// ============================================================================

const STATE_EFFECTS = {
	'Tonic Expansion Tonic': {
		weather: 'clear',
		shake: 'none',
		clouds: 'none',
		fog: 'none',
		rain: false,
		lightning: false,
		description: 'Crystal Clear',
		tilt: 'none'
	},
	'Tonic Expansion Pre-Dominant': {
		weather: 'wisps',
		shake: 'none',
		clouds: 'wisps',
		fog: 'none',
		rain: false,
		lightning: false,
		description: 'Light Wisps',
		tilt: 'none'
	},
	'Tonic Expansion Dominant': {
		weather: 'bump',
		shake: 'light',
		clouds: 'large',
		fog: 'none',
		rain: false,
		lightning: false,
		description: 'The Bump',
		tilt: 'none'
	},
	'Cadence Pre-Pre-Dominant': {
		weather: 'turn',
		shake: 'none',
		clouds: 'none',
		fog: 'none',
		rain: false,
		lightning: false,
		description: 'The Turn',
		tilt: 'left'
	},
	'Cadence Pre-Dominant': {
		weather: 'descent',
		shake: 'none',
		clouds: 'wisps',
		fog: 'medium',
		rain: false,
		lightning: false,
		description: 'The Descent',
		tilt: 'none'
	},
	'Cadence Dominant': {
		weather: 'storm',
		shake: 'intense',
		clouds: 'none',
		fog: 'heavy',
		rain: true,
		lightning: true,
		description: 'The Storm',
		tilt: 'none'
	},
	'Authentic Cadence': {
		weather: 'cloudbreak',
		shake: 'none',
		clouds: 'none',
		fog: 'none',
		rain: false,
		lightning: false,
		description: 'Cloud Break',
		tilt: 'none'
	},
	'Half Cadence': {
		weather: 'holding',
		shake: 'dull',
		clouds: 'none',
		fog: 'medium',
		rain: false,
		lightning: false,
		description: 'Holding Pattern',
		tilt: 'none'
	},
	'Deceptive Cadence': {
		weather: 'deceptive',
		shake: 'none',
		clouds: 'none',
		fog: 'none',
		rain: false,
		lightning: false,
		description: 'Wrong Sky',
		tilt: 'none'
	}
};

const PHASE_CLASSES = ['phase-1', 'phase-2', 'phase-3'];

// Current state
let currentState = {
	phase: 1,
	time: 0,
	chord: '',
	stateName: '',
	weather: 'Crystal Clear',
	vibe: '',
	drums: false,
	sample: false,
	isPerformanceRunning: false
};

// Serial connection
let port = null;
let reader = null;
let inputBuffer = '';

// ============================================================================
// p5.js Visualizer Variables (from best-visualizer.js)
// ============================================================================

let songProgress = 0.0;
let displayState = "BOARDING";

// --- VIBE VARIABLES (Interpolated) ---
let vibe = {
	fog: 0,           // 0 = clear, 1 = storm
	speed: 0,         // Horizontal speed
	altitude: 0,      // 0 = ground, 1 = cruising height
	shake: 0,
	bank: 0,
	rain: 0,
	light: 0          // Lightning flash
};

let target = { fog: 0, speed: 0, altitude: 0, shake: 0, bank: 0, rain: 0 };

// --- LAYERS ---
let distantClouds = [];
let nearClouds = [];
let rainDrops = [];
let buildingsBack = []; // Distant city silhouette
let buildingsFront = []; // Closer city silhouette
let cloudBrush;

// --- PALETTES ---
let p = {};

// Sky color palette presets for each phase
const SKY_PALETTES = {
	morning: {
		default: { top: [135, 206, 235], bottom: [255, 255, 255], name: 'Default (Light Blue)' },
		deepAzure: { top: [0, 127, 255], bottom: [173, 216, 230], name: 'Deep Azure (Richer Blue)' },
		dawnPink: { top: [255, 183, 197], bottom: [255, 229, 217], name: 'Dawn Pink (Warm Pink)' },
		coolMorning: { top: [70, 130, 180], bottom: [176, 224, 230], name: 'Cool Morning (Steel Blue)' },
		brightCyan: { top: [0, 191, 255], bottom: [224, 255, 255], name: 'Bright Cyan (Vibrant)' },
		paleLavender: { top: [200, 162, 200], bottom: [230, 230, 250], name: 'Pale Lavender (Dreamy)' }
	},
	sunset: {
		default: { top: [255, 140, 80], bottom: [255, 200, 100], name: 'Default (Orange Gold)' },
		deepSunset: { top: [255, 94, 77], bottom: [255, 165, 0], name: 'Deep Sunset (Red-Orange)' },
		purpleDusk: { top: [138, 43, 226], bottom: [255, 140, 0], name: 'Purple Dusk (Purple to Orange)' },
		warmGold: { top: [255, 120, 50], bottom: [255, 215, 0], name: 'Warm Gold (Rich Gold)' },
		pinkSunset: { top: [255, 69, 100], bottom: [255, 180, 120], name: 'Pink Sunset (Coral Pink)' },
		fireSky: { top: [255, 50, 50], bottom: [255, 140, 0], name: 'Fire Sky (Red to Orange)' }
	},
	night: {
		default: { top: [20, 20, 40], bottom: [50, 50, 80], name: 'Default (Dark Blue)' },
		deepNight: { top: [5, 5, 15], bottom: [20, 20, 40], name: 'Deep Night (Darker)' },
		purpleNight: { top: [30, 15, 50], bottom: [60, 40, 80], name: 'Purple Night (Purple Tint)' },
		starryNight: { top: [10, 10, 30], bottom: [25, 25, 60], name: 'Starry Night (Deep Indigo)' },
		midnightBlue: { top: [15, 25, 50], bottom: [25, 50, 100], name: 'Midnight Blue (Classic)' }
	}
};

// Current selected palettes
let selectedPalettes = {
	morning: 'default',
	sunset: 'default',
	night: 'default'
};

// Canvas dimensions
let canvasW = 800;
let canvasH = 500;

// ============================================================================
// DOM Elements
// ============================================================================

const elements = {
	p5Container: null,
	textureOverlay: null,
	cabinImage: null,
	screenDisplay: null,
	flightStatus: null,
	infoPhase: null,
	infoTime: null,
	infoState: null,
	infoChord: null,
	infoWeather: null,
	infoVibe: null,
	drumsIndicator: null,
	sampleIndicator: null,
	connectionStatus: null,
	calibrationPanel: null,
	debugPanel: null
};

// ============================================================================
// p5.js Setup and Draw (from best-visualizer.js)
// ============================================================================

function setup() {
	const container = document.getElementById('p5-container');
	const canvas = createCanvas(canvasW, canvasH);
	canvas.parent(container);
	noStroke();

	// Colors
	p.skyDay = color(135, 206, 235);
	p.skySunset = color(255, 140, 80);
	p.skyNight = color(20, 20, 40);
	p.storm = color(60, 65, 70);

	// Generate Cloud Texture
	createCloudBrush();

	// Initialize Clouds
	for (let i = 0; i < 30; i++) distantClouds.push(new CloudLayer(0.5, 0.4, 0.7));
	for (let i = 0; i < 15; i++) nearClouds.push(new CloudLayer(1.5, 1.0, 2.0));

	// Initialize Rain
	for (let i = 0; i < 100; i++) rainDrops.push(new RainDrop());

	// Initialize City (Continuous generation handled in draw)
	// Fill initial buffer
	for (let x = 0; x < width * 2; x += 40) {
		buildingsBack.push(new Building(x, 200, 400, 0.5)); // Slower (taller)
		buildingsFront.push(new Building(x, 100, 250, 1.0)); // Faster (taller)
	}

	// Apply saved calibration after canvas is created
	applyCalibration();
}

function draw() {
	background(0);

	// 1. Logic
	updateVibeParameters();

	// 2. Draw Window View (Masked)
	push();
	// Clip to window area (simulated by drawing order)
	translate(width / 2, height / 2);

	// Apply Shake & Bank to the "World"
	let sx = random(-vibe.shake, vibe.shake);
	let sy = random(-vibe.shake / 2, vibe.shake / 2);
	translate(sx, sy);
	rotate(radians(vibe.bank * 0.3));
	translate(-width / 2, -height / 2);

	drawSky();

	// Draw City Layers (Only visible if altitude is low)
	// We offset Y based on altitude.
	// Alt 0 = City at bottom. Alt 1 = City pushed way down (off screen).
	let cityY = map(vibe.altitude * vibe.altitude, 0, 1, 0, 600);
	if (cityY < 500) {
		drawCityLayer(buildingsBack, vibe.speed * 0.5, cityY, color(60, 60, 80));
		drawCityLayer(buildingsFront, vibe.speed * 1.0, cityY, color(20, 20, 30));
	}

	// Draw Runway (Only at very start/end)
	if (cityY < 50) drawRunway(vibe.speed);

	drawClouds();
	if (vibe.light > 0) drawLightning();

	pop();

	// 3. Draw Rain on Glass
	drawRain();

	// 4. Draw Interior (Masking the edges)
	drawInterior();
	// drawPassenger();
	// drawUI();
}

// ============================================================================
// SCENERY DRAWING (from best-visualizer.js)
// ============================================================================

function drawSky() {
	let top, bot;

	// Get colors from selected palette for current phase
	if (currentState.phase === 1) {
		const palette = SKY_PALETTES.morning[selectedPalettes.morning] || SKY_PALETTES.morning.default;
		top = color(palette.top[0], palette.top[1], palette.top[2]);
		bot = color(palette.bottom[0], palette.bottom[1], palette.bottom[2]);
	} else if (currentState.phase === 2) {
		const palette = SKY_PALETTES.sunset[selectedPalettes.sunset] || SKY_PALETTES.sunset.default;
		top = color(palette.top[0], palette.top[1], palette.top[2]);
		bot = color(palette.bottom[0], palette.bottom[1], palette.bottom[2]);
	} else {
		const palette = SKY_PALETTES.night[selectedPalettes.night] || SKY_PALETTES.night.default;
		top = color(palette.top[0], palette.top[1], palette.top[2]);
		bot = color(palette.bottom[0], palette.bottom[1], palette.bottom[2]);
	}

	// Storm Override
	top = lerpColor(top, p.storm, vibe.fog * 0.9);
	bot = lerpColor(bot, p.storm, vibe.fog * 0.6);

	// Gradient
	beginShape(QUADS);
	fill(top); vertex(-100, -100); vertex(width + 100, -100);
	fill(bot); vertex(width + 100, height + 100); vertex(-100, height + 100);
	endShape();
}

function drawCityLayer(buildings, speed, yOffset, col) {
	fill(col);
	// Move buildings
	for (let i = buildings.length - 1; i >= 0; i--) {
		let b = buildings[i];
		b.x -= speed;

		// Draw Building
		rect(b.x, height - b.h + yOffset, b.w, b.h + 500); // +500 ensures it goes off bottom

		// Recycle if off screen
		if (b.x + b.w < -100) {
			buildings.splice(i, 1);
			// Add new one at right edge
			let lastX = buildings.length > 0 ? buildings[buildings.length - 1].x : width;
			let newX = max(width, lastX + 10); // Ensure gap or overlap
			// Parametric generation based on layer type
			if (speed > 0.8) buildings.push(new Building(newX + random(0, 20), 100, 250, 1.0)); // Front (taller)
			else buildings.push(new Building(newX + random(0, 20), 200, 400, 0.5)); // Back (taller)
		}
	}
}

function drawRunway(speed) {
	// Simple ground plane
	fill(30);
	rect(-100, height - 50, width + 200, 200);
	// Stripes
	fill(255);
	let stripeOffset = (millis() * speed * 0.5) % 200;
	for (let x = -200; x < width + 200; x += 200) {
		rect(x - stripeOffset, height - 30, 100, 10);
	}
}

function drawClouds() {
	let cloudCol = (currentState.phase === 3) ? color(60, 65, 80) : color(255);
	// Tint darker if storm
	cloudCol = lerpColor(cloudCol, color(30), vibe.fog * 0.5);

	// Fade clouds out if near ground (altitude < 0.2)
	let altAlpha = map(vibe.altitude, 0, 0.3, 0, 255, true);

	for (let c of distantClouds) {
		c.update(vibe.speed);
		c.display(cloudCol, 220 * (altAlpha / 255));
	}
	for (let c of nearClouds) {
		c.update(vibe.speed);
		c.display(cloudCol, 255 * (altAlpha / 255));
	}
}

function drawLightning() {
	fill(255, vibe.light);
	rect(-100, -100, width + 200, height + 200);
	vibe.light -= 20;
}

function drawRain() {
	if (vibe.rain < 0.1) return;
	stroke(200, 220, 255, 200);
	strokeWeight(3);
	for (let r of rainDrops) {
		r.update(vibe.speed, vibe.rain);
		if (r.x > 50 && r.x < width - 50 && r.y > 50 && r.y < height - 50) {
			line(r.x, r.y, r.x - r.len * 0.5, r.y + r.len);
		}
	}
	noStroke();
}

// ============================================================================
// INTERIOR & UI (from best-visualizer.js)
// ============================================================================

function drawInterior() {
	// We draw the "Plane Wall" to mask the outside world
	fill(30);
	// Draw giant borders
	rect(0, 0, width, 50); // Top
	rect(0, height - 50, width, 50); // Bottom
	rect(0, 0, 80, height); // Left
	rect(width - 80, 0, 80, height); // Right

	// Rounded corners for the window frame look
	stroke(50); strokeWeight(40); noFill();
	rect(80, 50, width - 160, height - 100, 50);
	noStroke();
}

function drawPassenger() {
	// Silhouette on the right side
	push();
	translate(width - 200, height);

	// Seat
	fill(15);
	beginShape();
	vertex(50, 0); vertex(50, -250); vertex(-50, -280); vertex(-150, -100); vertex(-150, 0);
	endShape(CLOSE);

	// Person
	fill(5);
	ellipse(-50, -130, 140, 180); // Shoulders
	ellipse(-60, -220, 90, 110); // Head

	// Headphones
	fill(80);
	ellipse(-60, -220, 100, 110); // Band
	fill(5); ellipse(-60, -220, 85, 95); // Inner hole
	fill(100); ellipse(-10, -220, 30, 50); // Earcup visible

	pop();
}

function drawUI() {
	textAlign(CENTER); fill(255, 255, 0); textSize(16);
	text(displayState, width / 2, 30);
}

// ============================================================================
// HELPERS & CLASSES (from best-visualizer.js)
// ============================================================================

class Building {
	constructor(x, minH, maxH, layerSpeed) {
		this.x = x;
		this.w = random(30, 80); // Building width
		this.h = random(minH, maxH); // Building height
		// Add random "spire" or variance
		if (random(1) < 0.3) this.h += 50;
	}
}

function createCloudBrush() {
	cloudBrush = createGraphics(150, 150);
	cloudBrush.noStroke();
	for (let i = 0; i < 120; i++) {
		cloudBrush.fill(255, random(25, 50));
		let r = random(20, 80);
		cloudBrush.ellipse(random(75 - r / 2, 75 + r / 2), random(75 - r / 2, 75 + r / 2), r, r);
	}
}

class CloudLayer {
	constructor(spd, sMin, sMax) { this.reset(true); this.spd = spd; this.sMin = sMin; this.sMax = sMax; }
	reset(rnd) { this.x = rnd ? random(width) : width + 200; this.y = random(0, height); this.sc = random(this.sMin, this.sMax); }
	update(s) { this.x -= s * this.spd; if (this.x < -200 * this.sc) this.reset(false); }
	display(c, a) { tint(red(c), green(c), blue(c), a); image(cloudBrush, this.x, this.y, 100 * this.sc, 100 * this.sc); noTint(); }
}

class RainDrop {
	constructor() { this.reset(); }
	reset() { this.x = random(width + 200); this.y = random(-200, 0); this.spd = random(15, 25); this.len = random(10, 20); }
	update(s, amt) {
		this.y += this.spd; this.x -= s;
		if (this.y > height) this.reset();
	}
}

// ============================================================================
// VIBE CONTROLLER (adapted from best-visualizer.js)
// ============================================================================

function updateVibeParameters() {
	// Interpolate
	vibe.fog = lerp(vibe.fog, target.fog, 0.05);
	vibe.speed = lerp(vibe.speed, target.speed, 0.02);
	vibe.altitude = lerp(vibe.altitude, target.altitude, 0.005); // Very slow climb/descent
	vibe.shake = lerp(vibe.shake, target.shake, 0.1);
	vibe.rain = lerp(vibe.rain, target.rain, 0.05);
	vibe.bank = lerp(vibe.bank, target.bank, 0.05);

	// Lightning from state
	if (target.rain > 0.8 && random(1) < 0.02) vibe.light = 255;
}

// Apply state effects to vibe targets
function applyStateToVibe(stateName) {
	const effect = STATE_EFFECTS[stateName];
	if (!effect) {
		// Default cruising
		target.fog = 0.2;
		target.speed = 8;
		target.rain = 0;
		target.shake = 0.5;
		target.bank = 0;
		return;
	}

	// Map weather effect to vibe targets
	switch (effect.weather) {
		case 'clear':
		case 'cloudbreak':
			target.fog = 0;
			target.speed = 8;
			target.rain = 0;
			target.shake = 0;
			break;
		case 'wisps':
			target.fog = 0.1;
			target.speed = 10;
			target.rain = 0;
			target.shake = 0.5;
			break;
		case 'bump':
			target.fog = 0.3;
			target.speed = 12;
			target.rain = 0;
			target.shake = 3;
			break;
		case 'turn':
			target.fog = 0;
			target.speed = 10;
			target.rain = 0;
			target.shake = 0.5;
			break;
		case 'descent':
			target.fog = 0.4;
			target.speed = 15;
			target.rain = 0;
			target.shake = 1;
			break;
		case 'storm':
			target.fog = 1.0;
			target.speed = 25;
			target.rain = 1.0;
			target.shake = 5;
			break;
		case 'holding':
			target.fog = 0.5;
			target.speed = 6;
			target.rain = 0;
			target.shake = 2;
			break;
		case 'deceptive':
			target.fog = 0.3;
			target.speed = 8;
			target.rain = 0.5;
			target.shake = 1;
			break;
		default:
			target.fog = 0.2;
			target.speed = 8;
			target.rain = 0;
			target.shake = 0.5;
	}

	// Apply tilt as bank
	if (effect.tilt === 'left') target.bank = -5;
	else if (effect.tilt === 'right') target.bank = 5;
	else target.bank = 0;
}

// ============================================================================
// Initialization
// ============================================================================

document.addEventListener('DOMContentLoaded', () => {
	initElements();
	initCalibration();
	initControls();
	initDebugPanel();
	loadCalibration();

	// Start with idle state
	setPhase(1);
	updateFlightStatus('BOARDING');
});

function initElements() {
	elements.p5Container = document.getElementById('p5-container');
	elements.textureOverlay = document.getElementById('texture-overlay');
	elements.cabinImage = document.getElementById('cabin-image');
	elements.screenDisplay = document.getElementById('screen-display');
	elements.flightStatus = document.getElementById('flight-status');
	elements.infoPhase = document.getElementById('info-phase');
	elements.infoTime = document.getElementById('info-time');
	elements.infoState = document.getElementById('info-state');
	elements.infoChord = document.getElementById('info-chord');
	elements.infoWeather = document.getElementById('info-weather');
	elements.infoVibe = document.getElementById('info-vibe');
	elements.drumsIndicator = document.getElementById('drums-indicator');
	elements.sampleIndicator = document.getElementById('sample-indicator');
	elements.connectionStatus = document.getElementById('connection-status');
	elements.calibrationPanel = document.getElementById('calibration-panel');
	elements.debugPanel = document.getElementById('debug-panel');
}

// ============================================================================
// Web Serial API
// ============================================================================

async function connectSerial() {
	try {
		port = await navigator.serial.requestPort();
		await port.open({ baudRate: 115200 });

		elements.connectionStatus.textContent = 'Connected';
		elements.connectionStatus.classList.add('connected');

		readSerialData();
	} catch (error) {
		console.error('Serial connection error:', error);
		elements.connectionStatus.textContent = 'Error: ' + error.message;
	}
}

async function readSerialData() {
	const decoder = new TextDecoderStream();
	const readableStreamClosed = port.readable.pipeTo(decoder.writable);
	reader = decoder.readable.getReader();

	try {
		while (true) {
			const { value, done } = await reader.read();
			if (done) break;

			inputBuffer += value;
			processSerialBuffer();
		}
	} catch (error) {
		console.error('Serial read error:', error);
	}
}

function processSerialBuffer() {
	// Look for VISUAL:{...} JSON blocks
	const visualMatch = inputBuffer.match(/VISUAL:\{([^}]+)\}/);

	if (visualMatch) {
		try {
			const jsonStr = '{' + visualMatch[1] + '}';
			const data = JSON.parse(jsonStr);
			handleVisualData(data);
		} catch (e) {
			console.warn('Failed to parse VISUAL data:', e);
		}

		// Clear the processed part of the buffer
		inputBuffer = inputBuffer.slice(inputBuffer.indexOf(visualMatch[0]) + visualMatch[0].length);
	}

	// Prevent buffer from growing too large
	if (inputBuffer.length > 5000) {
		inputBuffer = inputBuffer.slice(-2000);
	}
}

function handleVisualData(data) {
	// Update current state
	currentState.time = data.time || 0;
	currentState.phase = data.phase || 1;
	currentState.chord = data.chord || '';
	currentState.stateName = data.state || '';
	currentState.weather = data.weather || '';
	currentState.vibe = data.vibe || '';
	currentState.drums = data.drums || false;
	currentState.sample = data.sample || false;

	// Determine if performance is running
	const wasRunning = currentState.isPerformanceRunning;
	currentState.isPerformanceRunning = currentState.time > 0;

	// Handle performance start (takeoff)
	if (!wasRunning && currentState.isPerformanceRunning) {
		triggerTakeoff();
	}

	// Handle performance end (landing) - around 3 minutes
	if (currentState.time >= 180 && currentState.isPerformanceRunning) {
		triggerLanding();
	}

	// Update visuals
	updateScreenDisplay();
	setPhase(currentState.phase);
	applyStateToVibe(currentState.stateName);
}

// ============================================================================
// Phase Effects
// ============================================================================

function setPhase(phase) {
	currentState.phase = phase;
	// Phase is now handled in drawSky() for the p5.js visualizer
}

// ============================================================================
// Takeoff & Landing Sequences
// ============================================================================

function triggerTakeoff() {
	updateFlightStatus('TAKEOFF');
	displayState = 'TAKEOFF';

	// Set takeoff vibe
	target.altitude = 0;
	target.speed = 15;
	target.shake = 3;
	target.fog = 0;
	target.rain = 0;

	// Gradually rise
	setTimeout(() => {
		target.altitude = 0.5;
		target.speed = 12;
		target.shake = 2;
	}, 3000);

	setTimeout(() => {
		target.altitude = 1;
		target.shake = 0.5;
		updateFlightStatus('CRUISING');
		displayState = 'CRUISING';
	}, 6000);
}

function triggerLanding() {
	updateFlightStatus('LANDING');
	displayState = 'LANDING';

	// Start descent
	target.altitude = 0.5;
	target.shake = 1;

	setTimeout(() => {
		target.altitude = 0.2;
		target.shake = 2;
	}, 3000);

	setTimeout(() => {
		target.altitude = 0;
		target.speed = 5;
		target.shake = 0;
		updateFlightStatus('ARRIVED');
		displayState = 'ARRIVED';
	}, 6000);
}

// ============================================================================
// Screen Display
// ============================================================================

function updateScreenDisplay() {
	const minutes = Math.floor(currentState.time / 60);
	const seconds = currentState.time % 60;
	const timeStr = `${minutes}:${seconds.toString().padStart(2, '0')}`;

	elements.infoPhase.textContent = getPhaseLabel(currentState.phase);
	elements.infoTime.textContent = timeStr;
	elements.infoState.textContent = currentState.stateName || '—';
	elements.infoChord.textContent = currentState.chord || '—';
	elements.infoWeather.textContent = currentState.weather || '—';
	elements.infoVibe.textContent = currentState.vibe || '—';

	// Update indicators
	if (currentState.drums) {
		elements.drumsIndicator.classList.add('active');
	} else {
		elements.drumsIndicator.classList.remove('active');
	}

	if (currentState.sample) {
		elements.sampleIndicator.classList.add('active');
	} else {
		elements.sampleIndicator.classList.remove('active');
	}
}

function getPhaseLabel(phase) {
	switch (phase) {
		case 1: return 'Morning';
		case 2: return 'Sunset';
		case 3: return 'Night';
		default: return '—';
	}
}

function updateFlightStatus(status) {
	elements.flightStatus.textContent = status;
	elements.flightStatus.className = '';

	switch (status) {
		case 'TAKEOFF':
			elements.flightStatus.classList.add('takeoff');
			break;
		case 'CRUISING':
			elements.flightStatus.classList.add('cruising');
			break;
		case 'LANDING':
		case 'ARRIVED':
			elements.flightStatus.classList.add('landing');
			break;
	}
}

// ============================================================================
// Calibration System
// ============================================================================

const calibrationDefaults = {
	window: { x: 0, y: 0, w: 800, h: 500, skewX: 0, skewY: 0, perspective: 800 },
	screen: { x: 0, y: 0, w: 180, h: 140, skewX: 0, skewY: 0, rotation: 0, rotateX: 0, rotateY: 0 },
	cabin: { x: 0, y: 0, w: 100, h: 100, objX: 50, objY: 50 },
	text: { logo: 10, status: 8, labels: 9, values: 9, icons: 12 }
};

let calibration = JSON.parse(JSON.stringify(calibrationDefaults));

function initCalibration() {
	// Window sliders (now control the p5 canvas container)
	['x', 'y', 'w', 'h', 'skewx', 'skewy', 'perspective'].forEach(prop => {
		const slider = document.getElementById(`window-${prop}`);
		if (slider) {
			slider.addEventListener('input', () => {
				const val = parseInt(slider.value);
				document.getElementById(`window-${prop}-val`).textContent = val;

				const mappedProp = prop === 'skewx' ? 'skewX' : (prop === 'skewy' ? 'skewY' : prop);
				calibration.window[mappedProp] = val;
				applyCalibration();
			});
		}
	});

	// Screen sliders
	['x', 'y', 'w', 'h', 'skewx', 'skewy', 'rotation', 'rotatex', 'rotatey'].forEach(prop => {
		const slider = document.getElementById(`screen-${prop}`);
		if (slider) {
			slider.addEventListener('input', () => {
				const val = (prop === 'rotation') ? parseFloat(slider.value) : parseInt(slider.value);
				document.getElementById(`screen-${prop}-val`).textContent = val;

				const propMap = { skewx: 'skewX', skewy: 'skewY', rotatex: 'rotateX', rotatey: 'rotateY' };
				const mappedProp = propMap[prop] || prop;
				calibration.screen[mappedProp] = val;
				applyCalibration();
			});
		}
	});

	// Cabin sliders
	['x', 'y', 'w', 'h', 'objX', 'objY'].forEach(prop => {
		let sliderId, valId;
		if (prop === 'w') { sliderId = 'cabin-width'; valId = 'cabin-width-val'; }
		else if (prop === 'h') { sliderId = 'cabin-height'; valId = 'cabin-height-val'; }
		else if (prop === 'objX') { sliderId = 'cabin-objx'; valId = 'cabin-objx-val'; }
		else if (prop === 'objY') { sliderId = 'cabin-objy'; valId = 'cabin-objy-val'; }
		else { sliderId = `cabin-${prop}`; valId = `cabin-${prop}-val`; }

		const slider = document.getElementById(sliderId);
		if (slider) {
			slider.addEventListener('input', () => {
				const val = parseInt(slider.value);
				document.getElementById(valId).textContent = val;
				calibration.cabin[prop] = val;
				applyCalibration();
			});
		}
	});

	// Text size sliders
	['logo', 'status', 'labels', 'values', 'icons'].forEach(prop => {
		const slider = document.getElementById(`text-${prop}`);
		if (slider) {
			slider.addEventListener('input', () => {
				const val = parseInt(slider.value);
				document.getElementById(`text-${prop}-val`).textContent = val;
				calibration.text[prop] = val;
				applyCalibration();
			});
		}
	});

	// Save/Reset buttons
	document.getElementById('save-calibration').addEventListener('click', saveCalibration);
	document.getElementById('reset-calibration').addEventListener('click', resetCalibration);

	// Sky color dropdowns
	initSkyColorPicker();
}

function applyCalibration() {
	const w = calibration.window;
	const s = calibration.screen;

	// Apply to p5 container (the canvas)
	if (elements.p5Container) {
		elements.p5Container.style.width = `${w.w}px`;
		elements.p5Container.style.height = `${w.h}px`;
		elements.p5Container.style.left = `calc(35% + ${w.x}px)`;
		elements.p5Container.style.top = `calc(50% + ${w.y}px)`;
		elements.p5Container.style.transform = `translate(-50%, -55%) skewX(${w.skewX}deg) skewY(${w.skewY}deg)`;

		// Resize the p5 canvas if dimensions changed
		if (canvasW !== w.w || canvasH !== w.h) {
			canvasW = w.w;
			canvasH = w.h;
			resizeCanvas(w.w, w.h);
		}
	}

	// Apply to texture overlay
	if (elements.textureOverlay) {
		elements.textureOverlay.style.width = `${w.w}px`;
		elements.textureOverlay.style.left = `calc(35% + ${w.x}px)`;
		elements.textureOverlay.style.top = `calc(50% + ${w.y}px)`;
	}

	// Apply to screen
	if (elements.screenDisplay) {
		elements.screenDisplay.style.width = `${s.w}px`;
		elements.screenDisplay.style.height = `${s.h}px`;
		elements.screenDisplay.style.left = `calc(62% + ${s.x}px)`;
		elements.screenDisplay.style.top = `calc(50% + ${s.y}px)`;
		elements.screenDisplay.style.transform = `translate(-50%, -20%) perspective(500px) rotate(${s.rotation}deg) rotateX(${s.rotateX}deg) rotateY(${s.rotateY}deg) skewX(${s.skewX}deg) skewY(${s.skewY}deg)`;
	}

	// Apply cabin size and position
	const c = calibration.cabin;
	if (elements.cabinImage) {
		elements.cabinImage.style.left = `calc(50% + ${c.x}px)`;
		elements.cabinImage.style.top = `calc(50% + ${c.y}px)`;
		elements.cabinImage.style.width = `${c.w}%`;
		elements.cabinImage.style.height = `${c.h}%`;
		elements.cabinImage.style.backgroundPosition = `${c.objX}% ${c.objY}%`;
	}

	// Apply text sizes
	const t = calibration.text;
	if (t) {
		const root = document.documentElement;
		root.style.setProperty('--text-logo-size', `${t.logo}px`);
		root.style.setProperty('--text-status-size', `${t.status}px`);
		root.style.setProperty('--text-labels-size', `${t.labels}px`);
		root.style.setProperty('--text-values-size', `${t.values}px`);
		root.style.setProperty('--text-icons-size', `${t.icons}px`);
	}
}

function loadCalibration() {
	const saved = localStorage.getItem('cabinVisualizerCalibration');
	if (saved) {
		try {
			const loaded = JSON.parse(saved);

			// Detect new vs old format
			const calData = loaded.calibration || loaded;

			// Merge loaded data with defaults to ensure new properties exist
			calibration = {
				window: { ...calibrationDefaults.window, ...calData.window },
				screen: { ...calibrationDefaults.screen, ...calData.screen },
				cabin: {
					...calibrationDefaults.cabin,
					...(calData.cabin || {
						x: 0,
						y: 0,
						w: calData.cabinWidth || 100,
						h: calData.cabinHeight || 100,
						objX: 50,
						objY: 50
					})
				},
				text: { ...calibrationDefaults.text, ...(calData.text || {}) }
			};

			// Update sliders to match
			Object.keys(calibration.window).forEach(prop => {
				const sliderId = `window-${prop.toLowerCase()}`;
				const slider = document.getElementById(sliderId);
				if (slider) {
					slider.value = calibration.window[prop];
					document.getElementById(`${sliderId}-val`).textContent = calibration.window[prop];
				}
			});

			Object.keys(calibration.screen).forEach(prop => {
				const sliderId = `screen-${prop.toLowerCase()}`;
				const slider = document.getElementById(sliderId);
				if (slider) {
					slider.value = calibration.screen[prop];
					document.getElementById(`${sliderId}-val`).textContent = calibration.screen[prop];
				}
			});

			// Load cabin settings
			if (calibration.cabin) {
				const cabinSliderMap = {
					x: 'cabin-x',
					y: 'cabin-y',
					w: 'cabin-width',
					h: 'cabin-height',
					objX: 'cabin-objx',
					objY: 'cabin-objy'
				};
				Object.keys(cabinSliderMap).forEach(prop => {
					const sliderId = cabinSliderMap[prop];
					const slider = document.getElementById(sliderId);
					if (slider && calibration.cabin[prop] !== undefined) {
						slider.value = calibration.cabin[prop];
						document.getElementById(`${sliderId}-val`).textContent = calibration.cabin[prop];
					}
				});
			}

			// Load text size settings
			if (calibration.text) {
				['logo', 'status', 'labels', 'values', 'icons'].forEach(prop => {
					const slider = document.getElementById(`text-${prop}`);
					if (slider && calibration.text[prop] !== undefined) {
						slider.value = calibration.text[prop];
						document.getElementById(`text-${prop}-val`).textContent = calibration.text[prop];
					}
				});
			}

			applyCalibration();

			// Load sky palette selections - always update dropdowns even if no saved palettes
			console.log('Loading sky palettes from saved data:', loaded.skyPalettes);
			if (loaded.skyPalettes) {
				selectedPalettes = {
					morning: loaded.skyPalettes.morning || 'default',
					sunset: loaded.skyPalettes.sunset || 'default',
					night: loaded.skyPalettes.night || 'default'
				};
				console.log('Sky palettes loaded:', selectedPalettes);
			} else {
				console.log('No sky palettes found in saved data, using defaults');
			}
			updateSkyDropdowns();
			updateSkySwatches();
		} catch (e) {
			console.warn('Failed to load calibration:', e);
		}
	}
}

function saveCalibration(showAlert = true) {
	const saveData = {
		calibration: calibration,
		skyPalettes: selectedPalettes
	};
	console.log('Saving calibration:', saveData);
	console.log('Sky palettes being saved:', selectedPalettes);
	localStorage.setItem('cabinVisualizerCalibration', JSON.stringify(saveData));
	if (showAlert) {
		alert('Calibration saved!');
	}
}

function resetCalibration() {
	calibration = JSON.parse(JSON.stringify(calibrationDefaults));
	selectedPalettes = { morning: 'default', sunset: 'default', night: 'default' };
	localStorage.removeItem('cabinVisualizerCalibration');

	// Reset text size sliders
	['logo', 'status', 'labels', 'values', 'icons'].forEach(prop => {
		const slider = document.getElementById(`text-${prop}`);
		if (slider) {
			slider.value = calibrationDefaults.text[prop];
			document.getElementById(`text-${prop}-val`).textContent = calibrationDefaults.text[prop];
		}
	});

	updateSkyDropdowns();
	updateSkySwatches();
	applyCalibration();
}

// ============================================================================
// Sky Color Picker
// ============================================================================

function initSkyColorPicker() {
	// Morning dropdown
	const morningSelect = document.getElementById('sky-morning');
	if (morningSelect) {
		morningSelect.addEventListener('change', () => {
			selectedPalettes.morning = morningSelect.value;
			updateSkySwatches();
		});
	}

	// Sunset dropdown
	const sunsetSelect = document.getElementById('sky-sunset');
	if (sunsetSelect) {
		sunsetSelect.addEventListener('change', () => {
			selectedPalettes.sunset = sunsetSelect.value;
			updateSkySwatches();
		});
	}

	// Night dropdown
	const nightSelect = document.getElementById('sky-night');
	if (nightSelect) {
		nightSelect.addEventListener('change', () => {
			selectedPalettes.night = nightSelect.value;
			updateSkySwatches();
		});
	}

	// Initialize swatches
	updateSkySwatches();
}

function updateSkyDropdowns() {
	const morningSelect = document.getElementById('sky-morning');
	const sunsetSelect = document.getElementById('sky-sunset');
	const nightSelect = document.getElementById('sky-night');

	if (morningSelect) morningSelect.value = selectedPalettes.morning;
	if (sunsetSelect) sunsetSelect.value = selectedPalettes.sunset;
	if (nightSelect) nightSelect.value = selectedPalettes.night;
}

function updateSkySwatches() {
	// Morning swatch
	const morningSwatch = document.getElementById('sky-preview-morning');
	if (morningSwatch) {
		const palette = SKY_PALETTES.morning[selectedPalettes.morning] || SKY_PALETTES.morning.default;
		morningSwatch.style.background = `linear-gradient(to bottom, rgb(${palette.top.join(',')}), rgb(${palette.bottom.join(',')}))`;
	}

	// Sunset swatch
	const sunsetSwatch = document.getElementById('sky-preview-sunset');
	if (sunsetSwatch) {
		const palette = SKY_PALETTES.sunset[selectedPalettes.sunset] || SKY_PALETTES.sunset.default;
		sunsetSwatch.style.background = `linear-gradient(to bottom, rgb(${palette.top.join(',')}), rgb(${palette.bottom.join(',')}))`;
	}

	// Night swatch
	const nightSwatch = document.getElementById('sky-preview-night');
	if (nightSwatch) {
		const palette = SKY_PALETTES.night[selectedPalettes.night] || SKY_PALETTES.night.default;
		nightSwatch.style.background = `linear-gradient(to bottom, rgb(${palette.top.join(',')}), rgb(${palette.bottom.join(',')}))`;
	}
}

// ============================================================================
// Controls
// ============================================================================

function initControls() {
	// Toggle calibration panel
	document.getElementById('toggle-controls').addEventListener('click', () => {
		elements.calibrationPanel.classList.toggle('hidden');
		elements.debugPanel.classList.toggle('hidden');
	});

	// Connect serial
	document.getElementById('connect-serial').addEventListener('click', connectSerial);

	// Download config
	document.getElementById('download-config').addEventListener('click', downloadConfig);

	// Upload config - trigger file input
	document.getElementById('upload-config').addEventListener('click', () => {
		document.getElementById('config-file-input').click();
	});

	// File input handler for upload
	document.getElementById('config-file-input').addEventListener('change', uploadConfig);
}

// ============================================================================
// Configuration Export/Import
// ============================================================================

function downloadConfig() {
	const saveData = {
		calibration: calibration,
		skyPalettes: selectedPalettes,
		exportedAt: new Date().toISOString()
	};

	const blob = new Blob([JSON.stringify(saveData, null, 2)], { type: 'application/json' });
	const url = URL.createObjectURL(blob);

	const link = document.createElement('a');
	link.href = url;
	const now = new Date();
	const dateStr = now.toISOString().slice(0, 10);
	const timeStr = now.toTimeString().slice(0, 8).replace(/:/g, '-');
	link.download = `cabin-visualizer-config-${dateStr}_${timeStr}.json`;
	document.body.appendChild(link);
	link.click();
	document.body.removeChild(link);
	URL.revokeObjectURL(url);
}

function uploadConfig(event) {
	const file = event.target.files[0];
	if (!file) return;

	const reader = new FileReader();
	reader.onload = (e) => {
		try {
			const loaded = JSON.parse(e.target.result);

			// Load calibration
			const calData = loaded.calibration || loaded;
			calibration = {
				window: { ...calibrationDefaults.window, ...calData.window },
				screen: { ...calibrationDefaults.screen, ...calData.screen },
				cabin: { ...calibrationDefaults.cabin, ...(calData.cabin || {}) }
			};

			// Update calibration sliders
			Object.keys(calibration.window).forEach(prop => {
				const sliderId = `window-${prop.toLowerCase()}`;
				const slider = document.getElementById(sliderId);
				if (slider) {
					slider.value = calibration.window[prop];
					document.getElementById(`${sliderId}-val`).textContent = calibration.window[prop];
				}
			});

			Object.keys(calibration.screen).forEach(prop => {
				const sliderId = `screen-${prop.toLowerCase()}`;
				const slider = document.getElementById(sliderId);
				if (slider) {
					slider.value = calibration.screen[prop];
					document.getElementById(`${sliderId}-val`).textContent = calibration.screen[prop];
				}
			});

			const cabinSliderMap = {
				x: 'cabin-x', y: 'cabin-y', w: 'cabin-width', h: 'cabin-height',
				objX: 'cabin-objx', objY: 'cabin-objy'
			};
			Object.keys(cabinSliderMap).forEach(prop => {
				const sliderId = cabinSliderMap[prop];
				const slider = document.getElementById(sliderId);
				if (slider && calibration.cabin[prop] !== undefined) {
					slider.value = calibration.cabin[prop];
					document.getElementById(`${sliderId}-val`).textContent = calibration.cabin[prop];
				}
			});

			applyCalibration();

			// Load sky palettes
			if (loaded.skyPalettes) {
				selectedPalettes = {
					morning: loaded.skyPalettes.morning || 'default',
					sunset: loaded.skyPalettes.sunset || 'default',
					night: loaded.skyPalettes.night || 'default'
				};
				updateSkyDropdowns();
				updateSkySwatches();
			}

			// Also save to localStorage (silently)
			saveCalibration(false);

			alert('Configuration loaded successfully!');
		} catch (err) {
			console.error('Failed to load config:', err);
			alert('Failed to load configuration: ' + err.message);
		}
	};
	reader.readAsText(file);

	// Reset the file input so the same file can be re-selected
	event.target.value = '';
}

// ============================================================================
// Debug Panel
// ============================================================================

function initDebugPanel() {
	// State buttons
	document.querySelectorAll('#state-buttons button').forEach(btn => {
		btn.addEventListener('click', () => {
			const state = btn.dataset.state;
			currentState.stateName = state;
			applyStateToVibe(state);
			updateScreenDisplay();

			// Update active state
			document.querySelectorAll('#state-buttons button').forEach(b => b.classList.remove('active'));
			btn.classList.add('active');
		});
	});

	// Phase buttons
	document.querySelectorAll('#phase-buttons button').forEach(btn => {
		btn.addEventListener('click', () => {
			const phase = parseInt(btn.dataset.phase);
			setPhase(phase);
			currentState.phase = phase;
			updateScreenDisplay();
		});
	});

	// Performance toggle
	document.getElementById('toggle-performance').addEventListener('click', () => {
		if (currentState.isPerformanceRunning) {
			// Stop
			currentState.isPerformanceRunning = false;
			currentState.time = 0;
			target.fog = 0;
			target.speed = 0;
			target.shake = 0;
			target.rain = 0;
			target.altitude = 0;
			updateFlightStatus('BOARDING');
			displayState = 'BOARDING';
			document.getElementById('toggle-performance').textContent = '▶️ Start Performance';
		} else {
			// Start
			currentState.isPerformanceRunning = true;
			currentState.time = 1;
			triggerTakeoff();
			document.getElementById('toggle-performance').textContent = '⏹️ Stop Performance';

			// Simulate time passing if not connected to serial
			if (!port) {
				simulatePerformance();
			}
		}
		updateScreenDisplay();
	});
}

function simulatePerformance() {
	const interval = setInterval(() => {
		if (!currentState.isPerformanceRunning) {
			clearInterval(interval);
			return;
		}

		currentState.time++;

		// Update phase based on time
		if (currentState.time < 90) {
			currentState.phase = 1;
		} else if (currentState.time < 150) {
			currentState.phase = 2;
		} else {
			currentState.phase = 3;
		}

		setPhase(currentState.phase);
		updateScreenDisplay();

		// Trigger landing near end
		if (currentState.time >= 180 && currentState.time < 182) {
			triggerLanding();
		}

		if (currentState.time >= 200) {
			currentState.isPerformanceRunning = false;
			clearInterval(interval);
			document.getElementById('toggle-performance').textContent = '▶️ Start Performance';
		}
	}, 1000);
}
