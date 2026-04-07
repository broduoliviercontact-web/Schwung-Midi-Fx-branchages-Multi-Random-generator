# Package and Release External Schwung Module

## Purpose
Use this skill to package a Schwung module as an external drop-in module that can be installed without rebuilding the full host.

## Goal
Prepare a self-contained module folder and release artifact that can be copied into the appropriate Schwung modules location and recognized by the host.

## Packaging Rules
The final package should contain only the files required by the module.

Typical contents:
- `module.json`
- `ui.js` if used
- `ui_chain.js` if used
- `dsp.so` if used
- any required static assets supported by the module design

Do not include:
- temporary build products
- object files
- editor folders
- repo-local debug clutter
- unrelated source material unless intentionally shipping source

## Packaging Process

### 1. Verify Folder Integrity
Check:
- manifest is present
- ids and names are correct
- required binaries exist
- optional UI files are present if referenced
- no missing referenced files remain

### 2. Verify Runtime Assumptions
Check:
- parameter defaults match the implementation
- chain editing parameters are valid
- state restore keys are valid
- the module can initialize from a clean install

### 3. Build Release Artifact
Prepare:
- module folder
- compressed release artifact if requested
- short install instructions
- version note / changelog summary

### 4. Install Instructions
Provide exact install instructions for the user, including:
- where the module folder or archive should go
- whether a rescan or restart is needed
- what to verify after install

### 5. Smoke Test Checklist
Provide a minimal test checklist:
- module appears in browser or chain
- module loads
- parameters edit correctly
- MIDI behavior works
- state recalls correctly
- no stuck notes after bypass/remove/stop

## Required Output Format

### Release Contents
List every file in the release package.

### Install Instructions
Provide the exact install steps.

### Validation Checklist
Provide a short but concrete checklist.

### Known Limitations
List any current limitations.

### Version Note
Provide a concise release note.

## Guardrails
- Do not package build junk.
- Do not reference files that are not included.
- Do not claim portability unless the release was structured for external installation.
- Always include a post-install test checklist.