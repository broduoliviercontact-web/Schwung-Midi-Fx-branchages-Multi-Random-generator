# CLAUDE.md

## Project Purpose

This repository is used to design, port, implement, and package **Schwung MIDI FX modules**.

The main goal is to convert useful open-source MIDI FX ideas and projects into clean, maintainable, Move-friendly Schwung modules.

Priorities:
1. Musical usefulness
2. Reliability
3. Consistency with the real Schwung repo
4. Good UX on Ableton Move
5. Small, testable iterations

---

## Mandatory First Step

Before editing or generating code for any Schwung module, inspect if present:

- `CLAUDE.md`
- `BUILDING.md`
- `docs/MODULES.md`
- `docs/API.md`

If one of these files is missing locally, say so explicitly and continue with the available repo documentation.


Also inspect at least one existing reference implementation in:
- `src/modules/midi_fx/`
- `src/modules/chain/`
- `src/shared/`

Do not start implementation before checking how the real repo already does something similar.

---

## General Working Rules

- Prefer existing Schwung patterns over inventing new abstractions.
- Prefer a smaller stable V1 over a larger speculative implementation.
- Keep changes easy to review and easy to test.
- Be explicit about assumptions when the source material is incomplete.
- Do not claim full parity with a source project unless it is actually true.
- Do not add unsupported manifest fields or unsupported APIs.
- Do not create desktop-style UX patterns that do not fit Move.
- Keep code readable, boring, and maintainable.

When in doubt:
- simplify
- align with existing repo patterns
- preserve musical intent
- reduce feature scope

---

## Repo-Specific Goals

Most work in this repo should target one of these outcomes:

1. **Audit an open-source MIDI FX project**
2. **Design a Schwung-native MIDI FX module**
3. **Implement a native MIDI FX engine**
4. **Build Move-facing UI**
5. **Build compact chain editing support**
6. **Package an external drop-in module**

Always identify which of these outcomes is being worked on before coding.

---

## Schwung Module Assumptions

Assume Schwung modules are folder-based and centered on a `module.json` manifest.

A module may contain:
- `module.json`
- `ui.js`
- `ui_chain.js`
- `dsp.so`
- native source files under `dsp/`

For new work:
- prefer the current documented repo conventions
- use the real manifest structure already present in the repo
- keep file trees minimal
- do not add files that are not justified by the module design

---

## Default Target: MIDI FX in Signal Chain

Unless the request clearly says otherwise, assume the target is a **Signal Chain MIDI FX** module.

Default design assumptions:
- chainable MIDI FX
- compact parameter surface
- Move-friendly controls
- safe note lifecycle
- optional chain editing support
- native engine when timing or MIDI processing matters

Do not assume a module is audio FX or instrument unless the request clearly requires that.

---

## Design Rules for MIDI FX

When designing a Schwung MIDI FX module:

- Keep the first version focused.
- Prefer 2 to 8 primary parameters.
- Use simple parameter types:
  - `enum`
  - `int`
  - `float`
  - `toggle`
- Every parameter must have a real default.
- Parameter names must match real engine support.
- Expose only controls that matter on Move.
- Avoid over-deep UI hierarchy unless the musical problem really needs it.

Good first-version examples:
- arpeggiator
- scale quantizer
- chord memory
- note repeater
- humanizer
- transpose/filter
- velocity shaper
- simple CC generator

---

## Move UX Rules

Design for Ableton Move first.

### Control priorities
- Put the most important parameters on direct knobs.
- Use Shift only when it clearly improves usability.
- Use pads only when they add immediate musical value.
- Use step buttons only when they are genuinely useful as selectors, toggles, or shortcuts.
- Keep menu/back behavior consistent with repo patterns.

### LED rules
- LEDs should communicate state, not create noise.
- Prefer simple, readable feedback.
- Avoid decorative LED behavior unless it has clear musical meaning.

### Interaction rules
- Avoid hidden modes when possible.
- Avoid requiring memorization of many Shift layers.
- Keep the first interaction screen simple and obvious.

---

## Native Engine Rules

When generating native MIDI FX code:

- Prefer direct, readable C.
- Match the repo’s real MIDI FX structure and signatures.
- Create a clear per-instance state struct.
- Define defaults for every parameter.
- Make MIDI handling explicit.
- Clearly separate:
  - instance state
  - MIDI input handling
  - timed generation
  - parameter parsing
  - state serialization

### MIDI lifecycle safety
Always handle:
- note on
- note off
- velocity-0 note on
- mode changes while notes are active
- transport stop
- empty held-note state
- module reset/state restore
- pass-through of non-consumed messages

Never risk stuck notes through unclear logic.

### Timing rules
If the effect is time-based:
- make the timing model explicit
- define whether timing is internal or MIDI-clock based
- define reset behavior on stop/start/mode change
- keep timing deterministic

If timing is not needed:
- keep the tick path minimal

### Parsing rules
- Validate enum values
- Clamp numeric ranges
- Handle malformed state gracefully
- Never crash because of missing or bad state

---

## UI Rules

### Full UI (`ui.js`)
Only create a custom `ui.js` when it is justified.

Use a full UI only if the module truly needs:
- custom interaction logic
- pad or step interaction
- special LED behavior
- custom browsing or selection flow

If a full UI is not justified, do not force one.

### Chain UI (`ui_chain.js`)
Chain UI should be:
- smaller
- simpler
- focused on the most important live-edit controls

Do not duplicate full-screen UI complexity in chain mode.

If chain editing does not need custom JS, do not invent extra UI.

---

## Parameter / Manifest Integrity Rules

Always keep these aligned:
- `module.json`
- engine parameter support
- UI parameter names
- chain-edit parameter exposure
- state serialization keys

Never expose:
- parameters the engine does not implement
- controls that are unsafe in chain mode
- UI labels for nonexistent keys

---

## Open-Source Porting Rules

When converting an external project:

1. Identify the real musical core.
2. Separate the core from wrapper/UI/DAW glue.
3. Decide what to keep, rewrite, discard, or simplify.
4. Port the musical intent, not the entire original complexity.
5. Prefer a reduced but strong V1 over a fragile feature-complete clone.

Always state clearly:
- what is being ported
- what is intentionally excluded
- which behaviors are inferred
- which parts are rewritten rather than directly ported

---

## Packaging Rules

For external drop-in modules:
- package only the required files
- do not include build junk
- do not include unrelated temporary files
- ensure referenced files actually exist
- provide install steps
- provide a smoke-test checklist

Release package should be minimal and self-contained.

---

## Testing Checklist

Before considering a MIDI FX module “done”, check:

### Core behavior
- module loads
- parameters edit correctly
- MIDI input transforms correctly
- pass-through behavior is correct
- no unexpected swallowed messages

### Note safety
- no stuck notes on note release
- no stuck notes on mode change
- no stuck notes on transport stop
- no stuck notes on module removal/bypass if relevant

### Timing
- internal timing works
- clock sync works if implemented
- stop/start/reset behavior is clear
- no unstable timing assumptions

### State
- defaults are correct
- state restores correctly
- malformed state does not break the module

### UX
- knobs map to the right controls
- Shift behavior is understandable
- pad/step usage is clear if present
- LEDs communicate useful state

---

## Preferred Output Style

When working on a task, prefer this structure:

1. **Summary**
2. **Assumptions**
3. **Design or implementation plan**
4. **File tree**
5. **Generated code**
6. **Self-review / risks**

For audits, prefer:
1. source summary
2. feasibility
3. conversion strategy
4. target module design
5. risks
6. recommended next milestone

---

## Default Decision Heuristics

If a choice is unclear, prefer:

- existing repo pattern over invention
- compact control surface over large control surface
- native engine over JS for timing-sensitive MIDI behavior
- no custom UI over unnecessary custom UI
- simple chain editing over deep chain UX
- explicit state over magic behavior
- stable V1 over ambitious V2

---

## Things To Avoid

- inventing undocumented Schwung APIs
- inventing unsupported manifest fields
- feature creep in first implementation
- overcomplicated UI hierarchies
- hidden interaction modes without strong justification
- ambiguous MIDI pass-through behavior
- unsafe note handling
- fragile parsing
- code that looks generic but does not match the real repo

---

## Quick Task Reminder

When asked to build or port something, first decide:

- Is this an audit task?
- Is this a design task?
- Is this a native engine task?
- Is this a UI task?
- Is this a chain UI task?
- Is this a packaging task?

Then do only the necessary scope for that step.

---

## Mission Reminder

Build Schwung MIDI FX modules that:
- respect the real repo
- feel good on Move
- are musically useful
- are safe and testable
- are small enough to maintain
