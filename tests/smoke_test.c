/* SPDX-License-Identifier: GPL-3.0-only */

#include <assert.h>
#include "cruise_control.h"
#include "module_constants.h"
#include "rte_mock.h"

int main(void)
{
    RteMock_Reset();
    cruise_control_init();
    assert(rte_mock.output_speed == 0.0f);
    assert(rte_mock.output_throttle == 0.0f);
    assert(rte_mock.output_state == CRUISE_OFF);

    rte_mock.inputs.enable = true;
    rte_mock.inputs.speed = 20.0f;
    cruise_control();
    assert(rte_mock.output_state == CRUISE_STDBY);
    assert(rte_mock.output_speed == SPEED_MIN);

    rte_mock.inputs.speed = 50.0f;
    cruise_control();
    assert(rte_mock.output_state == CRUISE_ON);

    rte_mock.inputs.brake = PEDALS_MIN + 1.0f;
    cruise_control();
    assert(rte_mock.output_state == CRUISE_INT);
    assert(rte_mock.output_throttle == 0.0f);

    rte_mock.inputs.brake = 0.0f;
    rte_mock.inputs.resume = true;
    rte_mock.inputs.set = true;
    rte_mock.inputs.speed = 30.0f;
    cruise_control();
    assert(rte_mock.output_state == CRUISE_ON);
    assert(rte_mock.output_speed == SPEED_MIN);
    assert(rte_mock.output_throttle <= THROTTLE_SAT_MAX);
    return 0;
}
