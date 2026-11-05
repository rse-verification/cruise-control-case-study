# Cruise Control: C Implementation and ACSL Specification

A single-body cruise controller with natural-language requirements, embedded
ACSL contracts, and reproducible executable checks and Frama-C WP verification.

**Original case study:** [Liu et al. (2024)](https://doi.org/10.3390/app14031046).
**Primary implementation and formalization work:**
[Edvin Rungardt (@Spoonvin)](https://github.com/Spoonvin).
**Quality improvements and validation:**
[Merlijn Sevenhuijsen (@SevenhuijsenM)](https://github.com/SevenhuijsenM).

The controller applies PI regulation on **every ON cycle**, including entry,
and uses increment-then-clamp QuickAccel behavior. For example, both 147.5 km/h
and 149 km/h advance to the 150 km/h maximum when QuickAccel is selected.

## Contents

```text
cruise_control.c
                         Implementation and embedded ACSL contracts
cruise_control_ver.c      External ACSL contracts for checking an implementation
headers/cruise_control.h  Public API
headers/cruise_control_types.h
                         State types and scalar aliases
headers/module_constants.h
                         Numeric constants
headers/cruise_control.is Entry signatures and initialization order
headers/cruise_control_check.c
                         Public-API initialization and transition checks
nlspec.txt               Detailed natural-language specification
tests/requirements_checks.c
                         29 selected executable requirement checks
```

## Prerequisites

- Linux or WSL.
- GCC with C11 support for compilation and executable checks.
- Frama-C with the WP and RTE plugins for formal verification.
- Alt-Ergo, Z3, and CVC5 installed and configured as WP provers. Check their
  availability with `frama-c -wp-detect`.

### Tested tool versions

| Tool | Tested version |
|---|---|
| GCC | 13.3.0 |
| Frama-C | 33.0 (Arsenic) |
| Alt-Ergo | 2.6.4 |
| Z3 | 5.1.0 |
| CVC5 | 1.2.0 |

These are the versions used for the recorded validation results, rather than
minimum supported versions. Validation was performed on Linux through WSL.

## Formal verification

From this directory:

```sh
frama-c -cpp-extra-args=-Iheaders cruise_control.c \
  -wp -wp-rte -wp-model real \
  -wp-prover alt-ergo,z3,cvc5 -wp-timeout 3 -wp-cache none
```

Check the final proved-goal summary: a successful process exit alone does not
mean all obligations were proved. The real-model proof establishes the
embedded contracts under their preconditions. It includes initialization,
cyclic behavior, assigns clauses, behavior partitions, and generated runtime
obligations. WP reports unsupported alignment and function-pointer-validity
RTE guards separately; the arithmetic model is real-number arithmetic.

### Check an implementation against the external specification

`cruise_control_ver.c` supplies its own contracts for initialization, state
transitions, PI regulation, integral handling, speed management, and input
priorities. It includes `cruise_control.c` by default, so the private integral
is visible to the specification without exposing it through the public API.

```sh
frama-c -cpp-extra-args=-Iheaders cruise_control_ver.c \
  -wp -wp-rte -wp-model real \
  -wp-prover alt-ergo,z3,cvc5 -wp-timeout 3 -wp-cache none
```

To check another implementation, put it in `cruise_control.c`, or change the
default `CRUISE_CONTROL_IMPLEMENTATION` filename in the verification file.
The candidate must provide the two public entry functions and a private
file-scope `cruise_integral`, using the supplied types and constants.

For an independent candidate check, use an implementation without its own
ACSL contracts. If the candidate has embedded contracts, Frama-C merges them
with the external specification, including their preconditions. Verification
logic and behavior names in the external file are prefixed to avoid name
collisions. Implementations with helpers also need sufficient helper contracts
or explicit inlining for WP to reason about their calls.

## Native checks

Build and run the public-API smoke test:

```sh
gcc -std=c11 -Wall -Wextra -Werror -pedantic -Iheaders cruise_control.c headers/cruise_control_check.c -o smoke
./smoke
```

Build the selected requirement checks:

```sh
gcc -std=c11 -Wall -Wextra -Werror -pedantic -Iheaders tests/requirements_checks.c -lm -o requirements-checks
./requirements-checks list
./requirements-checks PI_on_entry_to_on
./requirements-checks quick_accel_exact_maximum
./requirements-checks quick_accel_near_maximum
```

Run each listed case in a fresh process and inspect its JSON `passed` field.
The suite covers initialization, 26 individual cases, and two sequences
starting from initialization. Individual cases may seed private state;
passing them does not establish reachability or exhaustive NL coverage.

### Test-case coverage

`headers/cruise_control_check.c` is the public-API smoke test. The 29 cases in
`tests/requirements_checks.c` are grouped below; `./requirements-checks list`
prints every executable case name.

| Area | Cases | Test names |
|---|---:|---|
| Initialization | 1 | `init_resets_outputs_and_integral` |
| Disable, brake priority, interruption, and resume | 5 | `disabled_retains_speed`, `brake_priority`, `interrupted_waits_for_resume`, `resume_preserves_speed_over_set`, `resume_to_standby` |
| Inclusive vehicle-speed eligibility | 4 | `below_minimum_standby`, `minimum_speed_inclusive`, `maximum_speed_inclusive`, `above_maximum_standby` |
| Accelerator and brake thresholds | 4 | `accelerator_threshold_equal`, `accelerator_above_threshold`, `brake_threshold_equal`, `brake_above_threshold` |
| Target-speed management and button priorities | 7 | `set_has_highest_speed_priority`, `quick_accel_priority_over_decel`, `quick_accel_near_maximum`, `quick_accel_exact_maximum`, `quick_decel_near_minimum`, `set_outside_range_is_bounded`, `no_speed_button_retains_target` |
| PI calculation, saturation, and integral reset | 6 | `PI_on_entry_to_on`, `PI_continued_on`, `upper_saturation_freezes_integral`, `lower_saturation_freezes_integral`, `float_rounding_example`, `inactive_resets_integral` |
| Sequences starting from initialization | 2 | `reachable_quick_accel_boundary`, `reachable_PI_reentry` |

The external verification file is also checked separately with a valid
contract-free candidate and a deliberately faulty QuickAccel candidate, as
reported below. Those experiments are not additional cases in the 29-case
executable suite.

## Validation results

- C11 compilation with strict warnings: pass.
- Public-API smoke test: pass.
- Selected executable requirement checks: 29/29 pass.
- Frama-C WP with `-wp-model real`: 81/81 goals proved.
- External verification file against the supplied implementation, including
  its embedded contracts: 111/111 goals proved.

The external verifier was also checked against a contract-free candidate:
75/75 goals proved. Reintroducing the strict QuickAccel boundary bug in that
temporary candidate left `verify_CSM_03` unproved (75/76 goals proved).

The total includes two termination and two unreachable goals reported by WP.
Goal totals may vary between Frama-C versions.

## Interface and state

Call `cruise_control_init` before cyclic calls to `cruise_control`. It resets
the target speed, throttle, private integral, and state. Keep the caller-owned
target speed and state between cycles. The integral is private module state,
so this module represents one controller instance.

The cyclic input order is:

```text
Enable, Resume, Set, QuickDecel, QuickAccel,
Accelerator, Brake, VehicleSpeed,
TargetSpeed*, ThrottleCommand*, CruiseState*
```

Enable is a level signal: false selects OFF. Otherwise, braking selects INT;
INT persists until Resume; eligible speed and accelerator conditions then
select ON or STDBY. Successful resume preserves the stored target ahead of
speed-management buttons. Otherwise, SET takes priority over QuickAccel,
which takes priority over QuickDecel.

## Formalization choices

The embedded contracts make the following interpretation explicit:

- Enabled target speed is bounded to [30,150], including out-of-range SET.
- Successful resume preserves the target; pressing Resume while braking does
  not count as a successful resume.
- While OFF, throttle passes through accelerator values above the pedal
  threshold and is zero otherwise. In STDBY/INT it is zero.
- ON throttle is clamped to [0,45]. Saturation freezes the integral;
  unsaturated PI updates it. Inactive cycles reset it.

The NL does not supply numeric input ranges. The cyclic proof additionally
assumes finite analog inputs and state, vehicle speed in [0,1000], target in
[0,150], integral in [-1000,1000], and a target in [30,150] for an incoming
non-OFF state. Output pointers must be valid and separated. See the ACSL
`requires` clauses for the precise per-call domain.

The executable checks cover selected finite-input cases and use a `0.0001`
absolute tolerance for numeric comparisons. OFF/STDBY/INT throttle magnitude
is not asserted by the numerical oracle.

## Credits and provenance

The original cruise-control case study comes from the paper cited below.
This repository develops and validates an adapted implementation and formal
specification based on that published case study.

| Contributor / source | Contribution |
|---|---|
| Liu et al., original paper authors | The original published cruise-control case study on which this work is based. |
| [Edvin Rungardt (@Spoonvin)](https://github.com/Spoonvin) | Most of the implementation and ACSL formalization work forming the basis of this repository, including the single-body version originally named `cc_edvin_completed_single.c`. |
| [Merlijn Sevenhuijsen (@SevenhuijsenM)](https://github.com/SevenhuijsenM) | Subsequent quality improvements and checking: reviewing NL/ACSL alignment, correcting PI-on-entry and QuickAccel boundary behavior, running executable checks and real-model proofs, and preparing the standalone verification file and public package. |

The paper is credited as the source of the original case study; Edvin's
implementation/formalization work and Merlijn's subsequent improvements and
validation are contributions to this adaptation.

### Original paper

Mingxing Liu, Junfeng Wang, Tao Lin, Quan Ma, Zhiyang Fang, and Yanqun Wu.
**An Empirical Study of the Code Generation of Safety-Critical Software Using
LLMs.** *Applied Sciences*, 14(3), 1046, 2024.

- DOI: [10.3390/app14031046](https://doi.org/10.3390/app14031046)
- [Publisher article](https://www.mdpi.com/2076-3417/14/3/1046)
- [Open-access PDF](https://www.mdpi.com/2076-3417/14/3/1046/pdf)

The cruise-control case study is adapted from the paper; this package includes
the subsequent ACSL formalization and requirement clarifications described
above.

## License

This repository is licensed under the **Creative Commons Attribution 4.0
International license (CC BY 4.0)**; see [LICENSE](LICENSE). This is the same
license under which the original paper is published, as recorded in the
[publisher-deposited metadata](https://api.crossref.org/works/10.3390/app14031046).
The repository's adaptations and additions are also offered under CC BY 4.0.

Original paper material: copyright 2024 Mingxing Liu, Junfeng Wang, Tao Lin,
Quan Ma, Zhiyang Fang, and Yanqun Wu. The principal contributor to the
subsequent implementation and ACSL formalization is
[Edvin Rungardt (@Spoonvin)](https://github.com/Spoonvin), with subsequent
quality improvements and validation by
[Merlijn Sevenhuijsen (@SevenhuijsenM)](https://github.com/SevenhuijsenM).

Reuse must retain the applicable author and contributor attribution, cite the
original source, link to the license, and indicate changes. The adaptations
in this repository include ACSL formalization, requirement clarifications,
PI-on-entry and QuickAccel boundary corrections, verification checks, and
packaging. Material is provided without warranties under Section 5 of the
license. Attribution does not imply endorsement by the original authors.
