/* SPDX-License-Identifier: GPL-3.0-only */

#ifndef CRUISE_CONTROL_TYPES_H
#define CRUISE_CONTROL_TYPES_H

#include "Std_Types.h"

typedef boolean tB;
typedef float32 tF32;

typedef enum
{
    CRUISE_OFF,
    CRUISE_ON,
    CRUISE_STDBY,
    CRUISE_INT
} CruiseState;

#endif
