# Cruise Control: C Implementation and ACSL Specification

A cruise-control case study with natural-language requirements, ACSL contracts,
unit tests, and verification using Frama-C and VerNFR.

## Credits

| Contributor | Contribution |
|---|---|
| [Liu et al. (2024)](https://doi.org/10.3390/app14031046) | Original published case study. |
| [Edvin Rungardt (@Spoonvin)](https://github.com/Spoonvin) | Most of the implementation and ACSL formalization. |
| [Merlijn Sevenhuijsen (@SevenhuijsenM)](https://github.com/SevenhuijsenM) | Implementation restructuring, quality improvements, and validation. |

## Contents

| File | Purpose |
|---|---|
| `nlspec.txt` | Natural-language requirements. |
| `cruise_control.c` | Implementation and embedded ACSL contracts. |
| `cruise_control_ver.c` | External contracts for verifying an implementation. |
| `cruise_control.is` | Interface specification. |
| `headers/` | Public API, types, and constants. |
| `tests/unit_tests.c` | 29 requirement-focused unit tests. |
| `tests/smoke_test.c` | Public-API smoke test. |

## Prerequisites

Use Linux or WSL with GCC, Frama-C with WP and RTE, and the Alt-Ergo, Z3,
and CVC5 provers. Install [VerNFR](https://github.com/rse-verification/VerNFR)
for the interface checks. Check availability with `frama-c -wp-detect` and
`frama-c -nfr-h`.

### Tested versions

| Tool | Version |
|---|---|
| GCC | 13.3.0 |
| Frama-C | 33.0 (Arsenic) |
| Alt-Ergo | 2.6.4 |
| Z3 | 5.1.0 |
| CVC5 | 1.2.0 |
| VerNFR | `dev`, with Frama-C 33.0 |

These are the versions used for validation, not minimum requirements.
Run the commands below from the repository root.

## Formal verification

Verify the implementation's embedded contracts:

```sh
frama-c -cpp-extra-args=-Iheaders cruise_control.c \
  -wp -wp-rte -wp-model real \
  -wp-prover alt-ergo,z3,cvc5 -wp-timeout 3 -wp-cache none
```

Verify against the external specification:

```sh
frama-c -cpp-extra-args=-Iheaders cruise_control_ver.c \
  -wp -wp-rte -wp-model real \
  -wp-prover alt-ergo,z3,cvc5 -wp-timeout 3 -wp-cache none
```

The external verifier includes `cruise_control.c` by default. To test another
implementation, replace that file or change `CRUISE_CONTROL_IMPLEMENTATION`
in the verifier. It must provide the public entries and private
`cruise_integral` state. For an independent check, omit candidate ACSL
contracts; otherwise Frama-C merges them, including their preconditions.

Proofs use real arithmetic and the contracts' per-call assumptions: finite
inputs and state, vehicle speed between 0 and 1000, target between 0 and 150,
integral between -1000 and 1000, valid state, and valid, separated output pointers.
An incoming non-OFF state requires a target between 30 and 150.
Inspect the proved-goal summary and diagnostics, not only the exit status.

## Interface specification and VerNFR

`cruise_control.is` declares the two public entries, assumes initialization
before cyclic execution, and permits no external module calls. The caller
must satisfy the initialization order.

VerNFR checks structural and data-flow requirements, complementing the
functional ACSL proofs and concrete unit tests.

Check the implementation's entries, calls, private state, and initialization:

```sh
frama-c -vernfr -nfr-ispec cruise_control.is \
  -cpp-extra-args=-Iheaders -main cruise_control -keep-unused-functions all \
  -nfr-check-calls -nfr-fun-ptrs -nfr-all-entries-defined -nfr-only-entries \
  -nfr-static-vars -nfr-proper-init -nfr-check-ptr-literals cruise_control.c
```

Check public entry declarations and absence of function definitions in the header:

```sh
frama-c -vernfr -nfr-ispec cruise_control.is \
  -cpp-extra-args=-Iheaders -keep-unused-functions all -keep-unused-types \
  -nfr-all-entries-declared -nfr-no-fun-defs headers/cruise_control.h
```

These commands select the applicable VerNFR checks. Inspect their warnings;
a zero exit status alone does not establish success.

## Unit tests

Compile the unit-test program, list its cases, and run one case:

```sh
gcc -std=c11 -Wall -Wextra -Werror -pedantic -Iheaders tests/unit_tests.c -lm -o unit-tests
./unit-tests list
./unit-tests PI_on_entry_to_on
```

The first command builds `unit-tests`; `list` prints test names without running
them. Supply any listed name to run that test in a fresh process. The JSON
field `"passed": true` indicates a pass.

The 29 tests cover initialization, state transitions, pedal thresholds, speed
bounds, button priorities, PI control, and integral handling. Two tests exercise
sequences starting from initialization. Numeric comparisons use tolerance
`0.0001`; the suite is not exhaustive.

Build and run the public-API smoke test:

```sh
gcc -std=c11 -Wall -Wextra -Werror -pedantic -Iheaders cruise_control.c tests/smoke_test.c -o smoke
./smoke
```

## Validation results

| Check | Result |
|---|---|
| Compilation and smoke test | Pass |
| Unit tests | 29 of 29 pass |
| Embedded ACSL contracts | 81 of 81 goals proved |
| External contracts, combined with embedded contracts | 111 of 111 goals proved |
| VerNFR commands above | Pass, without VerNFR violation warnings |

Proof counts include termination and unreachable goals and may vary by tool
version. The implementation applies PI on every ON cycle and clamps QuickAccel
at the maximum speed; these are corrections made in this adaptation.

## Original paper

Mingxing Liu, Junfeng Wang, Tao Lin, Quan Ma, Zhiyang Fang, and Yanqun Wu.
**An Empirical Study of the Code Generation of Safety-Critical Software Using
LLMs.** *Applied Sciences*, 14(3), 1046, 2024.

[DOI and article](https://doi.org/10.3390/app14031046) ·
[Open-access PDF](https://www.mdpi.com/2076-3417/14/3/1046/pdf)

## License

[CC BY 4.0](LICENSE), matching the original paper's license, also applies to
this repository's adaptations and additions. Original paper material is
copyright 2024 the authors listed above.

Retain author and contributor attribution, link to the source and license,
and indicate changes. This adaptation adds formalization, requirement
clarifications, implementation corrections, tests, and verification tooling.
Material is provided without warranties; attribution does not imply endorsement.
