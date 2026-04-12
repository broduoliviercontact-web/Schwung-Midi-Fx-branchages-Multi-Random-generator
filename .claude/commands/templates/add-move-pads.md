---
description: Add Move pad LED interaction to any Schwung module UI (glow, toggle, select, steps, level)
argument-hint: [target file: ui.js or ui_chain.js, use case: glow | toggle | select | steps | level, param keys if relevant]
---

Add Move pad support to an existing Schwung module UI based on this request:

$ARGUMENTS

The 32 Move pads (4×8 grid, notes 68–99) can be used for visual feedback,
input, or both. This skill covers all common pad patterns.

---

## Step 0 — Identify Target File and Use Case

**Which file are you editing?**

| File | Context |
|------|---------|
| `ui.js` | Full-screen module view (standalone) |
| `ui_chain.js` | Chain slot edit mode — PRIMARY interaction surface |

**What do the pads do?**

| Use case | Description |
|----------|-------------|
| `glow` | Radial brightness follows a 2D X/Y position (e.g. `map_x`/`map_y`) |
| `toggle` | Each pad is on/off independently (e.g. lane enable, mute) |
| `select` | Only one pad active at a time (e.g. mode, preset, scale) |
| `steps` | Sequencer steps — pads represent beats in a pattern |
| `level` | Pads as a vertical or horizontal meter/bar (e.g. density, volume) |

A pad can display state (output only), accept input (input only), or both.

---

## Critical: LED Function Differs Between ui.js and ui_chain.js

**This is the most common bug.** Getting this wrong causes pads to stay dark with no error.

| File | LED function |
|------|-------------|
| `ui.js` (full UI) | `move_midi_internal_send([0, 0x90, note, vel])` |
| `ui_chain.js` (chain UI) | `sharedSetLED(note, vel)` from shared module |

`move_midi_internal_send` does NOT work in the chain UI context — it silently drops all LED updates.

---

## Step 1 — Add Constants

```javascript
const PAD_BASE  = 68;   // First pad note — Move pads are notes 68–99 (32 pads)
const PAD_COUNT = 32;   // Total pads
// PAD layout: 4 rows × 8 cols, row 0 = bottom (notes 68–75), row 3 = top (notes 92–99)
// pad index 0 = note 68 (bottom-left), index 31 = note 99 (top-right)
```

Add use-case-specific constants as needed (see per-pattern section below).

---

## Step 2 — Add Import (ui_chain.js only)

Add `setLED as sharedSetLED` to the shared import:

```javascript
import {
  decodeDelta,
  isCapacitiveTouchMessage,
  setLED as sharedSetLED,              // ← add this
} from '/data/UserData/schwung/shared/input_filter.mjs';
```

---

## Step 3 — Add setLED Wrapper

**In `ui.js`:**
```javascript
function setLED(note, vel) {
  move_midi_internal_send([0, 0x90, note, vel]);
}
```

**In `ui_chain.js`:**
```javascript
function setLED(note, vel) { sharedSetLED(note, vel); }
```

All pad patterns below use `setLED(note, vel)` — never call the underlying function directly.

---

## Step 4 — Add Pad State to the State Object

Add these fields to the state object (`g` in `ui.js`, `s` in `ui_chain.js`):

```javascript
padLEDCache:   new Uint8Array(32),  // Last sent value per pad — avoids redundant sends
padDirty:      true,                // true = at least one pad needs update this frame
padDirtyPhase: 0,                   // Which 8-pad slice to update this tick (0–3)
```

> **Why phase-based?** Move's LED buffer holds ~60 packets/frame. Sending all 32 pads at once risks overflow. Splitting into 4 slices of 8 spreads the cost over 4 ticks — safe and invisible.

---

## Step 5 — Add the Phase Update Loop

```javascript
function updatePadSlice() {
  const base = g.padDirtyPhase * 8;

  for (let i = base; i < base + 8; i++) {
    const target = padColor(i);               // ← implement per pattern below
    if (g.padLEDCache[i] !== target) {
      g.padLEDCache[i] = target;
      setLED(PAD_BASE + i, target);
    }
  }

  g.padDirtyPhase = (g.padDirtyPhase + 1) & 3;
  if (g.padDirtyPhase === 0) g.padDirty = false;
}
```

Implement `padColor(idx)` according to the use case (see patterns below).
`padColor` must return a velocity 0–127 without side effects.

---

## Step 6 — Call in tick()

Always call `updatePadSlice` independently of the display dirty flag:

```javascript
function tick() {
  // ...
  if (g.dirty) { render(); g.dirty = false; }
  if (g.padDirty) updatePadSlice();   // ← always separate
}
```

Set `g.padDirty = true` in `init()` to trigger a full draw on first load.

---

## Step 7 — Handle Pad Input (if pads are interactive)

In `onMidiMessageInternal`:

```javascript
const type = data[1] & 0xF0;
const b1   = data[2];
const b2   = data[3];

if (type === 0x90 && b1 >= PAD_BASE && b1 < PAD_BASE + PAD_COUNT && b2 > 0) {
  const idx = b1 - PAD_BASE;
  onPadPress(idx);    // ← implement per pattern below
  return;
}
```

---

---

## Pattern A — X/Y Position Glow

Pads light up based on distance to a 2D position (`map_x` / `map_y`).
The closest pad glows brightest; surrounding pads fade outward.

**Constants:**
```javascript
const PAD_BRIGHT_NEAR = 0.07;
const PAD_BRIGHT_MED  = 0.22;
const PAD_BRIGHT_FAR  = 0.45;
```

**Helper functions:**
```javascript
function padIndexToXY(idx) {
  return { x: (idx % 8) / 7, y: Math.floor(idx / 8) / 3 };
}

function padGlow(idx, mx, my) {
  const { x, y } = padIndexToXY(idx);
  const d = Math.sqrt((x - mx) ** 2 + (y - my) ** 2);
  if (d < PAD_BRIGHT_NEAR) return 127;
  if (d < PAD_BRIGHT_MED)  return 50;
  if (d < PAD_BRIGHT_FAR)  return 12;
  return 0;
}

function padColor(idx) {
  return padGlow(idx, g.params.map_x, g.params.map_y);  // replace key names
}
```

**Input — press sets position:**
```javascript
function onPadPress(idx) {
  const { x, y } = padIndexToXY(idx);
  setParam('map_x', x);
  setParam('map_y', y);
  g.padDirty = true;
}
```

**Mark dirty when position changes:**
```javascript
// In setParam, after updating value:
if (key === 'map_x' || key === 'map_y') g.padDirty = true;
```

**Reference:** Branchage and Grilles `ui_chain.js` — both use this pattern with `map_x`/`map_y`.

---

## Pattern B — Toggle (independent on/off per pad)

Each pad is independently on or off (e.g. lane mutes, active steps).

**State:**
```javascript
padState: new Uint8Array(32),   // 0 = off, 1 = on
```

**Colors:**
```javascript
const PAD_ON  = 127;
const PAD_OFF = 0;

function padColor(idx) {
  return g.padState[idx] ? PAD_ON : PAD_OFF;
}
```

**Input:**
```javascript
function onPadPress(idx) {
  g.padState[idx] ^= 1;          // toggle
  g.padDirty = true;
}
```

**Sync with param (if backed by a bitmask or array param):**
```javascript
// When loading state from params:
for (let i = 0; i < 32; i++) {
  g.padState[i] = (g.params.pattern >> i) & 1;
}
g.padDirty = true;
```

---

## Pattern C — Group Select (one active at a time)

Only one pad is active (e.g. mode selection, scale, preset slot).

**State:**
```javascript
activePad: 0,    // index of currently selected pad
```

**Colors:**
```javascript
const PAD_ACTIVE   = 127;
const PAD_INACTIVE = 8;    // dim, shows available options
const PAD_EMPTY    = 0;    // use for pads that have no option assigned

function padColor(idx) {
  if (idx >= NUM_OPTIONS) return PAD_EMPTY;  // hide unused pads
  return idx === g.activePad ? PAD_ACTIVE : PAD_INACTIVE;
}
```

**Input:**
```javascript
function onPadPress(idx) {
  if (idx >= NUM_OPTIONS) return;
  g.activePad = idx;
  setParam('mode', idx);   // sync to param
  g.padDirty = true;
}
```

---

## Pattern D — Step Sequencer

Pads represent steps in a pattern. Active steps lit, inactive dark.
Often mapped to a subset of pads (e.g. first 16 = first bar).

**State:**
```javascript
steps:      new Uint8Array(16),  // 1 = active, 0 = inactive
currentStep: 0,                  // for playhead display
```

**Colors:**
```javascript
const STEP_ON       = 127;
const STEP_OFF      = 0;
const STEP_PLAYHEAD = 50;   // highlight current step even if off

function padColor(idx) {
  if (idx >= g.steps.length) return 0;   // unused pads dark
  if (idx === g.currentStep) return STEP_PLAYHEAD;
  return g.steps[idx] ? STEP_ON : STEP_OFF;
}
```

**Input:**
```javascript
function onPadPress(idx) {
  if (idx >= g.steps.length) return;
  g.steps[idx] ^= 1;
  g.padDirty = true;
}
```

**Advance playhead (from DSP param or tick counter):**
```javascript
const step = parseInt(g.params.current_step);
if (step !== g.currentStep) {
  g.currentStep = step;
  g.padDirty = true;
}
```

---

## Pattern E — Level / Meter Bar

Pads display a value as a filled bar (e.g. density, volume, count).

**Horizontal bar (8 pads wide, 1 row):**
```javascript
const METER_ROW  = 0;     // which row (0 = bottom)
const METER_FULL = 127;
const METER_EMPTY = 0;

function padColor(idx) {
  const row = Math.floor(idx / 8);
  const col = idx % 8;
  if (row !== METER_ROW) return 0;
  const filled = Math.round(g.params.level * 8);  // 0–8
  return col < filled ? METER_FULL : METER_EMPTY;
}
```

**Mark dirty when value changes:**
```javascript
if (key === 'level') g.padDirty = true;
```

---

## Complete Checklist

- [ ] `PAD_BASE = 68` and any use-case constants added
- [ ] Correct `setLED` wrapper for target file (`sharedSetLED` in chain, `move_midi_internal_send` in full)
- [ ] `sharedSetLED` imported if `ui_chain.js`
- [ ] `padLEDCache`, `padDirty`, `padDirtyPhase` added to state
- [ ] `padColor(idx)` function implemented for chosen pattern
- [ ] `updatePadSlice()` added with `padColor` call
- [ ] `updatePadSlice()` called in `tick()` when `padDirty`, separate from render dirty
- [ ] `padDirty = true` set in `init()`
- [ ] `padDirty = true` set whenever displayed state changes
- [ ] Pad note-on handler added to `onMidiMessageInternal` if pads are interactive
- [ ] Input handler calls `setParam` + sets `padDirty` as needed

---

## Output Contract

Return:
1. A short summary of which pattern was applied and why
2. The updated file with all additions inline
3. A confirmation checklist of every step applied
