---
description: Create the native C engine for a Schwung MIDI FX module
argument-hint: [module-name-and-behavior]
---

Create the native C implementation for a Schwung MIDI FX module based on this request:

$ARGUMENTS

Follow project memory in @.claude/CLAUDE.md.

Before writing code, inspect:
- `docs/MODULES.md`
- `docs/API.md`
- at least one existing native MIDI FX implementation in `src/modules/midi_fx/`
- any closely related module already present in the repo

Your job:
- Generate the contents of a `dsp/<module>.c` source file for the module.
- Match the repo’s real native MIDI FX patterns instead of inventing a new architecture.
- Infer the correct headers, entry points, and function signatures from the repo examples.
- Keep the implementation readable, direct, and maintainable.

Implementation requirements:
- Define a clear per-instance state struct.
- Include explicit defaults for every parameter.
- Clearly separate:
  - instance state
  - MIDI input handling
  - timed generation
  - parameter parsing
  - state serialization
- Implement safe note lifecycle handling.
- Prevent stuck notes during:
  - mode changes
  - transport stop
  - empty note state
  - state restore
  - parameter changes that affect active playback
- Clearly define which MIDI messages are:
  - consumed
  - transformed
  - passed through
- If the effect is time-based, implement timed generation using the repo’s established pattern.
- If internal timing is needed, make the timing model explicit.
- If MIDI clock sync is relevant, implement it explicitly and safely.
- If timing is not needed, keep the tick path minimal or no-op.

Parameter requirements:
- Implement parameter setters and getters.
- Clamp numeric values.
- Validate enums.
- Support state serialization if the module needs custom restore behavior.
- Expose `chain_params` only if it matches real editable controls.

Coding rules:
- Prefer boring reliability over cleverness.
- Avoid hidden behavior.
- Avoid fragile parsing.
- Handle malformed state gracefully.
- Keep comments focused and useful.
- Do not generate placeholder pseudocode.
- Generate real compilable C in the style of the repo.

Return exactly:
1. A short engine summary
2. One fenced `c` block containing the full source file
3. A short edge-case review covering:
   - stuck note risk
   - invalid state risk
   - timing risk
   - pass-through behavior

Do not generate `module.json`, `ui.js`, or `ui_chain.js` in this step.