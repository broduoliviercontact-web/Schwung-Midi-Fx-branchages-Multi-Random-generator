# Build Move UI and Controls for Schwung Module

## Purpose
Use this skill to design and implement the Move-facing control surface for a Schwung module.

This includes:
- parameter-to-knob mapping
- shift behaviors
- pad behaviors
- step button behaviors
- LED feedback
- optional `ui.js`
- optional `ui_chain.js`

## Goal
Make the module feel native on Move.
The user should be able to understand and control the module quickly with minimal cognitive load.

## UI Design Principles
- Keep the first screen simple.
- Put the most important parameters on direct knobs.
- Use Shift only when it meaningfully improves control density.
- Use pads or steps only if they genuinely improve musical interaction.
- Prefer stable visual conventions over novelty.
- Reuse shared helpers and patterns from the repo.

## Hardware Awareness
Assume Move controls may include:
- 8 knobs
- pads
- step buttons
- menu/back buttons
- shift
- jog / main encoder
- LEDs

Treat hardware noise and non-musical internal messages carefully.
Filter or ignore hardware messages that should not reach musical logic.

## UI Design Process

### 1. Identify Core Controls
Choose the primary controls that deserve direct knob access.

Typical direct controls:
- mode
- rate/division
- range
- gate
- amount
- probability
- min/max
- channel
- scale
- transpose

### 2. Define Shift Layer
Use shift for:
- secondary value adjustment
- alternate interpretation
- quick reset
- fine/coarse resolution
- hidden but useful performance options

Do not hide critical functionality behind Shift unless necessary.

### 3. Define Pads / Step Buttons
Use them only if they add immediate value, such as:
- mode selection
- scale selection
- lane enable/disable
- step toggles
- octave selection
- chord slot recall

Otherwise leave them unused or decorative.

### 4. Define LED Behavior
Specify:
- status indication
- active mode indication
- selection feedback
- error/warning state
- timing or pulse feedback only if it helps

Avoid LED spam.
LEDs should communicate state, not create noise.

### 5. Chain UI Strategy
If a chain UI is needed:
- expose only the most important controls
- keep chain editing compact
- avoid interactions that depend on a full-screen module context unless clearly supported

## Required Output Format

### Interaction Summary
Describe how the user operates the module on Move.

### Knob Map
List knob 1 through knob 8.

### Shift Layer
List all shift-modified actions.

### Pad Map
If pads are used, define each zone or behavior.

### Step Button Map
If steps are used, define their meanings.

### LED Plan
Describe all visual feedback.

### Files
Specify whether to create:
- `ui.js`
- `ui_chain.js`
- neither

### Implementation Notes
Provide concrete notes for coding.

## Guardrails
- Do not create a UI that requires memorizing too many hidden states.
- Do not use pads just because they are available.
- Do not overload LEDs with decorative behavior.
- Keep chain UI smaller and more focused than full UI.
- Prefer consistency with existing modules over experimental interaction design.