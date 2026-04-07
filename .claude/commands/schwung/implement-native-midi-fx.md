# Implement Native Schwung MIDI FX

## Purpose
Use this skill to implement the native engine of a Schwung MIDI FX module.

This skill assumes the module should process MIDI at the engine layer and optionally generate timed events.

## Goal
Build a stable native MIDI FX implementation that:
- transforms or generates MIDI correctly
- exposes parameters cleanly
- behaves correctly in the Signal Chain
- is safe with note lifecycle and state restore
- can be edited from the UI and chain context

## Recommended Strategy
Model the implementation after existing native MIDI FX modules in the repo.

Prefer a clear separation between:
- instance state
- MIDI input handling
- timed event generation
- parameter parsing
- state serialization

## Native Implementation Plan

### 1. Define Instance State
Create a per-instance struct that contains:
- current parameter values
- internal working state
- note tracking
- timing counters
- clock sync state if relevant
- any cached display/state strings if needed

### 2. Define MIDI Behavior
Document exactly how the module responds to:
- note on
- note off
- velocity 0 note on
- CC
- MIDI clock
- transport start/stop/continue
- sustain if relevant
- pass-through of unsupported messages

Be explicit about which messages are swallowed and which are passed through.

### 3. Handle Timed Generation
If the module generates timed notes or CC:
- decide whether timing comes from internal timing or MIDI clock
- use `tick()` if the engine must emit events over time
- define reset rules on stop / mode change / empty note state

### 4. Implement Parameter I/O
Implement:
- parameter setting
- parameter reading
- optional error/warning reporting
- `chain_params` exposure when needed
- serialized state when needed

### 5. Manage Note Lifecycle Safely
Always protect against:
- stuck notes
- missing note-offs
- mode changes while notes are held
- transport stop while note is sounding
- state reset while generator is active

### 6. Add Defensive Parsing
When parsing parameter strings or JSON state:
- validate values
- clamp numeric ranges
- handle missing keys gracefully
- avoid crashing on malformed state

## Required Engineering Checklist
- [ ] All parameters have defaults
- [ ] Note lifecycle is explicit
- [ ] Unsupported MIDI is either passed through or intentionally filtered
- [ ] Clock behavior is explicit
- [ ] Stop/reset behavior is explicit
- [ ] State restore is deterministic
- [ ] `chain_params` matches real parameter support
- [ ] Code is organized for future extension

## Required Output Format
When using this skill, produce:

### Engine Summary
One paragraph describing the runtime model.

### Instance State
Show the struct layout.

### MIDI Rules
List the exact MIDI behavior.

### Functions to Implement
List the required functions and purpose of each.

### Edge Cases
List the failure cases and how they are handled.

### Code
Generate the native implementation.

### Self-Review
At the end, review the implementation for:
- stuck note risk
- invalid state risk
- timing drift risk
- pass-through correctness
- parameter/UI mismatch

## Guardrails
- Do not emit notes without a reliable note-off strategy.
- Do not hide timing assumptions.
- Do not implement chain parameters that are not actually supported.
- Do not let malformed state crash the module.
- Prefer boring reliability over clever abstractions.