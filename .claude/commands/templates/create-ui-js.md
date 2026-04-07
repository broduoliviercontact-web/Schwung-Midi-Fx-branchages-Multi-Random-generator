---
description: Create a full Move-facing ui.js for a Schwung module
argument-hint: [module-name-and-ui-behavior]
---

Create a production-ready `ui.js` for a Schwung module based on this request:

$ARGUMENTS

Follow project memory in @.claude/CLAUDE.md.

Before writing code, inspect:
- `docs/MODULES.md`
- `docs/API.md`
- `src/shared/constants.mjs`
- `src/shared/input_filter.mjs`
- at least one existing module UI in the repo
- the target module’s `module.json` structure if it already exists

Goal:
Create a full Move-facing UI that feels native, compact, and consistent with Schwung.

Design principles:
- Keep the first screen simple.
- Put the most important controls on direct knobs.
- Use Shift only when it genuinely improves usability.
- Use pads or step buttons only if they add clear musical value.
- Prefer repo helper patterns over custom UI frameworks.
- Align all parameter keys with the manifest and engine.
- Keep LED behavior informative, not decorative.

Behavior requirements:
- Handle Move controls intentionally.
- Filter or ignore non-musical hardware messages where appropriate.
- Reuse shared constants and input helpers when possible.
- Make knob behavior predictable.
- Use menu/back behavior consistently.
- If pads are used, make their role obvious.
- If step buttons are used, make them shortcuts, selectors, or toggles with a clear reason.
- Use LEDs sparingly and consistently.
- Avoid hidden states unless clearly justified.

Implementation rules:
- Match the repo’s JS style and structure.
- Keep code readable and modular.
- Do not invent unsupported APIs.
- Do not hardcode parameter names that do not exist in the module.
- If the module does not truly need a custom full-screen UI, say so explicitly and return `OMIT_UI_JS` instead of forcing one.

When a custom UI is justified, include:
- initialization
- input handling
- parameter display/update logic
- LED update logic if needed
- cleanup/reset behavior if relevant

Return exactly one of these two outcomes:

Outcome A:
1. A short UI summary
2. One fenced `javascript` block containing the full `ui.js`
3. A short explanation of the interaction model

Outcome B:
1. A short explanation of why a full custom `ui.js` is not justified
2. The exact line: `OMIT_UI_JS`

Do not generate `module.json`, native C, or `ui_chain.js` in this step.