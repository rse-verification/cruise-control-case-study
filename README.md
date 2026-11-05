# Cruise Control: C Implementation and ACSL Specification

A single-body cruise controller with natural-language requirements, embedded
ACSL contracts, and reproducible executable checks and Frama-C WP verification.

**Primary credit:** [Edvin Rungardt (@Spoonvin)](https://github.com/Spoonvin),
who carried out most of the work on this case study.

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

The validated toolchain is GCC 13.3.0, Frama-C 33.0 Arsenic, Alt-Ergo 2.6.4,
Z3 5.1.0, and CVC5 1.2.0.

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

The primary contributor to this case study is
[Edvin Rungardt (@Spoonvin)](https://github.com/Spoonvin). This package derives
from his single-body implementation, originally named
`cc_edvin_completed_single.c`, and its ACSL formalization. Subsequent changes
align PI control on entry and QuickAccel boundary behavior with the NL
requirements, and move shared types and constants into public headers.

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
[Edvin Rungardt (@Spoonvin)](https://github.com/Spoonvin).

Reuse must retain the applicable author and contributor attribution, cite the
original source, link to the license, and indicate changes. The adaptations
in this repository include ACSL formalization, requirement clarifications,
PI-on-entry and QuickAccel boundary corrections, verification checks, and
packaging. Material is provided without warranties under Section 5 of the
license. Attribution does not imply endorsement by the original authors.
