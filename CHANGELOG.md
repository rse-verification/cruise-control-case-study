# Changelog

This changelog records the development of the cruise-control case study from
the original example to the current requirements, formal specification, and
implementation.

## 1. Origin

The case study is based on the cruise-control example by Liu et al. The original
requirements and generated code are available in the
[upstream repository](https://github.com/lmxstar/GPT-prompts-for-safety-critical-software).

## 2. Initial formalization

Edvin translated the requirements into ACSL and adapted the implementation for
verification.

- **Requirements:** clarified signal types, OFF and braking priorities,
  activation conditions, and state retention. Added descriptions of persistent
  state ownership and the controller interface.
- **Formal specification:** added contracts for state transitions, PI regulation,
  target management, and pedal detection. Added pointer validity, separation,
  numeric preconditions, frame conditions, and logical definitions for PI
  calculation and clamping.
- **Implementation:** adapted the code to explicit input, persistent-state, and
  output objects. Retained separate helper functions for verification.
- **Interface:** declared the public cyclic entry and permitted external calls.

## 3. Restructuring and refinement

Merlijn reviewed the requirements and contracts and restructured the
implementation around the current execution model.

- **Requirements:** defined Enable as a level signal, with false selecting OFF,
  and required initialization before cyclic execution. While enabled, braking
  selects INT, which persists until Resume. Added rules for PI on every ON cycle,
  increment-then-clamp target updates, and successful Resume taking priority
  over simultaneous speed-management inputs.
- **State ownership:** moved the integral into private module state while keeping
  the target and cruise state caller-owned.
- **Formal specification:** added separate initialization and cyclic contracts.
  Added ACSL postconditions and behavior clauses for the NL transition and
  priority rules. Added OFF throttle pass-through, frame conditions, and a bounded
  incoming target precondition for non-OFF states.
- **Controller arithmetic:** rewrote PI calculations and bounds in ACSL real
  arithmetic for `-wp-model real`. The integral is held when the candidate output
  is saturated and accumulates error otherwise.
- **Implementation:** consolidated the interface around scalar inputs and separate
  initialization and cyclic entries. Kept candidate-based PI regulation and
  bounded target updates, including PI on entry to ON. QuickAccel can increment
  the target to the maximum speed exactly.
- **Verification:** added a separate verification file and requirement-focused
  unit tests. Checked the implementation with Frama-C and VerNFR.

## 4. Requirement clarifications and test updates — v1.1.0

Two contract rules were made explicit in the NL requirements.

- **OFF throttle (`CC_HLR_CDC_01`):** throttle equals the accelerator sensor value
  when it exceeds `PedalsMin`, and is zero otherwise.
- **Resume (`CC_HLR_CCB_08`):** successful Resume requires an enabled invocation
  from INT without braking. Only successful Resume preserves the target ahead of
  SET, QuickAccel, and QuickDecel. A blocked request leaves normal enabled speed
  management applicable.
- **Regression tests:** added three OFF-throttle tests and three Resume-with-braking
  tests, increasing the suite from 29 to 35. The disabled-state test now checks
  throttle output. Mutation tests confirm that the tests detect incorrect OFF
  throttle and target preservation during blocked Resume.

The formal specification and reference implementation already contained these rules.

## 5. Verification status

| Check | Result |
|---|---|
| Unit tests | 35 of 35 passed |
| Public API smoke test | Passed |
| Embedded contracts, real model | 81 of 81 goals proved |
| Combined external and embedded contracts, real model | 111 of 111 goals proved |
| VerNFR structural checks | Passed on the reference implementation |

These results apply to the reference implementation, selected contracts, real
arithmetic model, and stated proof assumptions. Tool versions and verification
commands are documented in the README.

## 6. Provenance and scope

- Original material: [upstream commit `2cee359`](https://github.com/lmxstar/GPT-prompts-for-safety-critical-software/tree/2cee3596c7c562bd709243c792ea7ddd2cc2bf73).
- Initial formalization: local files documenting the Edvin updates.
- Published development point: [`v1.0.0`, commit `75b13ab`](https://github.com/rse-verification/cruise-control-case-study/tree/v1.0.0).
- Requirement clarifications: [`v1.1.0`](https://github.com/rse-verification/cruise-control-case-study/tree/v1.1.0).

Attribution is based on the available development history and artifacts. The
unversioned local files do not identify the author of each algorithm change.
Verification results do not establish completeness of the original requirements
or satisfaction of all original non-functional requirements.
