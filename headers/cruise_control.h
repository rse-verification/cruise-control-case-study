#ifndef CRUISE_CONTROL_H
#define CRUISE_CONTROL_H

#include "cruise_control_types.h"

void cruise_control_init(
    float *CC_OUTR_01,
    float *CC_OUTR_02,
    CruiseState *CC_OUTR_03);

void cruise_control(
    const bool CC_INR_01,
    const bool CC_INR_02,
    const bool CC_INR_03,
    const bool CC_INR_04,
    const bool CC_INR_05,
    const float CC_INR_06,
    const float CC_INR_07,
    const float CC_INR_08,
    float *CC_OUTR_01,
    float *CC_OUTR_02,
    CruiseState *CC_OUTR_03);

#endif
