/* SPDX-License-Identifier: GPL-3.0-only */

/* Apply the v2 runnable contracts to a candidate with the required private state. */
#define CRUISE_CONTROL_EXTERNAL_CONTRACTS
#ifndef CRUISE_CONTROL_IMPLEMENTATION
#include "cruise_control.c"
#else
#include CRUISE_CONTROL_IMPLEMENTATION
#endif
#include "cruise_control_contracts.h"
