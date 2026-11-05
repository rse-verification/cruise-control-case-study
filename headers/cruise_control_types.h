#ifndef CRUISE_CONTROL_TYPES_H
#define CRUISE_CONTROL_TYPES_H

#include <stdbool.h>

typedef bool tB;
typedef float tF32;

typedef enum
{
    CRUISE_OFF,
    CRUISE_ON,
    CRUISE_STDBY,
    CRUISE_INT
} CruiseState;

#endif
