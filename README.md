# Cruise Control — version 2

A cruise-control case study with parameterless runnables, AUTOSAR Classic-style
RTE ports, ACSL contracts, and unit tests.

[![Test and verify](https://github.com/rse-verification/cruise-control-case-study/actions/workflows/ci.yml/badge.svg)](https://github.com/rse-verification/cruise-control-case-study/actions/workflows/ci.yml)

## Credits

| Contributor | Contribution |
|---|---|
| [Liu et al. (2024)](https://doi.org/10.3390/app14031046) | Original published case study. |
| [Edvin Rungardt (@Spoonvin)](https://github.com/Spoonvin) | Initial ACSL formalization and code adaptation. |
| [Merlijn Sevenhuijsen (@SevenhuijsenM)](https://github.com/SevenhuijsenM) | Implementation restructuring, specification refinement, and validation. |

## Brief changelog

- **Original:** case study and generated code from Liu et al.
- **Initial formalization:** ACSL contracts and code adaptation by Edvin.
- **Refinement:** requirements, implementation, and tests reviewed by Merlijn.
- **Version 2:** RTE reads and writes replace signal parameters and output pointers.

[Detailed changelog](CHANGELOG.md).

## Interface

```c
void cruise_control_init(void);
void cruise_control(void);
```

Call initialization before cyclic execution and serialize runnable invocations.
The controller owns its target,
cruise state, and integral. Eight `Rte_Read_*` functions provide inputs and three
`Rte_Write_*` functions publish outputs. Signals retain the boolean and
floating-point types used by version 1; no integer scaling is introduced.

On a failed read, the cycle returns without changing controller state or
writing outputs. After successful reads, all three writes are attempted once.
Failed writes retain the previous value of that port; they do not suppress
later writes or roll back controller state. Outputs are not updated atomically.

The RTE header and standard types are case-study definitions, not generated
AUTOSAR integration files. Native tests provide the RTE implementation through
a mock; formal verification uses the RTE function contracts.

## Contents

| File | Purpose |
|---|---|
| `nlspec.txt` | Version 2 requirements and I/O failure behavior. |
| `cruise_control.c` | RTE-based runnables and private controller state. |
| `cruise_control_ver.c` | Apply the runnable contracts to a candidate implementation. |
| `cruise_control.is` | Public entries, permitted RTE calls, and call ordering. |
| `headers/cruise_control.h` | Parameterless public API. |
| `headers/Rte_CruiseControl.h` | RTE ports and their environment contracts. |
| `headers/cruise_control_contracts.h` | Controller behavior and transport contracts. |
| `headers/Std_Types.h` | Minimal standard return and signal types. |
| `tests/unit_tests.c` | Control and RTE failure tests. |
| `tests/rte_mock.c` | Test RTE with configurable statuses and call tracing. |
| `tests/smoke_test.c` | Public runnable lifecycle test. |

## Prerequisites

Use Linux or WSL with GCC, Frama-C with WP and RTE, and the Alt-Ergo, Z3,
and CVC5 provers. Install [VerNFR](https://github.com/rse-verification/VerNFR)
for interface checks. Check availability with `frama-c -wp-list-provers` and
`frama-c -nfr-h`.

| Tool | Tested version |
|---|---|
| GCC | 13.3.0 |
| Frama-C | 33.0 (Arsenic) |
| Alt-Ergo | 2.6.4 |
| Z3 | 5.1.0 |
| CVC5 | 1.2.0 |
| VerNFR | `dev`, with Frama-C 33.0 |

Run the following commands from the repository root.

## Formal verification

```sh
frama-c -cpp-extra-args=-Iheaders cruise_control.c \
  -wp -wp-rte -wp-model real \
  -wp-prover alt-ergo,z3,cvc5 -wp-timeout 10 -wp-cache none

frama-c -cpp-extra-args=-Iheaders cruise_control_ver.c \
  -wp -wp-rte -wp-model real \
  -wp-prover alt-ergo,z3,cvc5 -wp-timeout 10 -wp-cache none
```

The implementation and external verifier use the same runnable contracts.
The external verifier applies them after including a candidate, so that
candidate need not include the contract header. A candidate must provide the
two public runnables and private `cruise_speed`, `cruise_state`, and
`cruise_integral`. Existing candidate ACSL contracts would also be merged.

Proofs use real arithmetic and assume the RTE contracts, stable inputs and
statuses during each invocation, finite numeric signals, vehicle speed between
0 and 1000, and valid bounded private state. Verification-only call counters
must have room for one increment. Their values are not runtime module state.
Successful RTE writes update model output ports; failed writes preserve them.
This proves controller behavior under the RTE contracts, not the RTE implementation.

## Interface specification and VerNFR

The interface requires initialization before cyclic execution and whitelists
the eleven RTE functions. Read order and output-write order are specified
separately because initialization publishes outputs without reading inputs.

```sh
frama-c -vernfr -nfr-ispec cruise_control.is \
  -cpp-extra-args=-Iheaders -main cruise_control -keep-unused-functions all \
  -nfr-check-calls -nfr-fun-ptrs -nfr-all-entries-defined -nfr-only-entries \
  -nfr-static-vars -nfr-proper-init -nfr-check-ptr-literals \
  -nfr-external-call-order cruise_control.c

frama-c -vernfr -nfr-ispec cruise_control.is \
  -cpp-extra-args=-Iheaders -keep-unused-functions all -keep-unused-types \
  -nfr-all-entries-declared -nfr-no-fun-defs headers/cruise_control.h
```

Inspect the diagnostics: a zero exit status alone does not establish success.

## Unit tests

```sh
gcc -std=c11 -Wall -Wextra -Werror -pedantic -Iheaders \
  tests/unit_tests.c tests/rte_mock.c -lm -o unit-tests
./unit-tests list
./unit-tests PI_on_entry_to_on
./unit-tests rte_read_failure_7
./unit-tests rte_write_failures_7

gcc -std=c11 -Wall -Wextra -Werror -pedantic -Iheaders \
  cruise_control.c tests/smoke_test.c tests/rte_mock.c -o smoke
./smoke
```

Each unit case runs in a fresh process and returns nonzero on failure. The
58 cases include the 35 control cases from version 1 and 23 RTE cases: every
read-failure position, all nonempty output-failure combinations during cyclic
execution and initialization, and recovery after a failed read. Tests check
call order, call counts, private state, and published outputs.

GitHub Actions runs the native tests and both WP targets on every push and pull
request. CI pins Frama-C 33.0 and Alt-Ergo 2.6.4, uses the real model, and rejects
unproved goals. Full logs and JSON goal reports are workflow artifacts.
VerNFR is checked separately with the commands above.

## Version 2 validation

| Check | Result |
|---|---|
| Unit tests and RTE status cases | 58 of 58 pass |
| Public runnable smoke test | Pass |
| Runnable contracts, real model | 148 of 148 goals proved |
| External candidate contracts, real model | 148 of 148 goals proved |
| Selected VerNFR checks | Pass |

Both proof targets use the same contract header, not two independent sets of
obligations. Ignoring a failed read or suppressing later writes after a write
failure was also tested as an incorrect variant; the tests reject both.

## Original paper

Mingxing Liu, Junfeng Wang, Tao Lin, Quan Ma, Zhiyang Fang, and Yanqun Wu.
**An Empirical Study of the Code Generation of Safety-Critical Software Using
LLMs.** *Applied Sciences*, 14(3), 1046, 2024.

[DOI and article](https://doi.org/10.3390/app14031046) ·
[Open-access PDF](https://www.mdpi.com/2076-3417/14/3/1046/pdf)

Original code and prompts:
[lmxstar/GPT-prompts-for-safety-critical-software](https://github.com/lmxstar/GPT-prompts-for-safety-critical-software).

## License

[GNU GPL version 3](LICENSE), following the upstream code repository. The paper
itself is separately published under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
Original paper material is copyright 2024 the authors listed above.

This adaptation adds formalization, requirement clarifications, controller
corrections, tests, and RTE-based execution. Version 2 modifications: 2026-11-09.
Retain source and contributor
attribution. Material is provided without warranties.
