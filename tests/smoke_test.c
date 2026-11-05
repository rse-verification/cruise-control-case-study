#include <assert.h>
#include "cruise_control.h"
#include "module_constants.h"

int main(void)
{
    float cruise_speed = -1.0f;
    float throttle_cmd = -1.0f;
    CruiseState cruise_state = CRUISE_ON;

    cruise_control_init(&cruise_speed, &throttle_cmd, &cruise_state);
    assert(cruise_speed == 0.0f);
    assert(throttle_cmd == 0.0f);
    assert(cruise_state == CRUISE_OFF);

    cruise_control(true, false, false, false, false,
                   0.0f, 0.0f, 20.0f,
                   &cruise_speed, &throttle_cmd, &cruise_state);
    assert(cruise_state == CRUISE_STDBY);
    assert(cruise_speed == SPEED_MIN);

    cruise_control(true, false, false, false, false,
                   0.0f, 0.0f, 50.0f,
                   &cruise_speed, &throttle_cmd, &cruise_state);
    assert(cruise_state == CRUISE_ON);

    cruise_control(true, false, false, false, false,
                   0.0f, PEDALS_MIN + 1.0f, 50.0f,
                   &cruise_speed, &throttle_cmd, &cruise_state);
    assert(cruise_state == CRUISE_INT);
    assert(throttle_cmd == 0.0f);

    cruise_control(true, true, true, false, false,
                   0.0f, 0.0f, 30.0f,
                   &cruise_speed, &throttle_cmd, &cruise_state);
    assert(cruise_state == CRUISE_ON);
    assert(cruise_speed == SPEED_MIN);
    assert(throttle_cmd <= THROTTLE_SAT_MAX);

    return 0;
}
