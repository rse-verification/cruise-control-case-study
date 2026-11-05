/* Verify a candidate implementation against the external requirements contract.
 * By default the candidate is cruise_control.c in this directory.
 * A candidate must provide both public entries and private cruise_integral.
 */
#ifndef CRUISE_CONTROL_IMPLEMENTATION
#define CRUISE_CONTROL_IMPLEMENTATION "cruise_control.c"
#endif
#include CRUISE_CONTROL_IMPLEMENTATION
#include "cruise_control.h"
#include "module_constants.h"

/* Verification-local names avoid collisions with candidate ACSL definitions. */
/*@
    logic real cc_verify_pi(real error, real integral) =
        KP * error + KI * (integral + error);

    logic real cc_verify_clamp(real value, real lower, real upper) =
        value < lower ? lower : (value > upper ? upper : value);

    predicate cc_verify_state(CruiseState state) =
        state == CRUISE_OFF || state == CRUISE_ON ||
        state == CRUISE_STDBY || state == CRUISE_INT;

    predicate cc_verify_speed_in_limits(real speed) =
        SPEED_MIN <= speed <= SPEED_MAX;
*/

/*@
    requires \valid(CC_OUTR_01);
    requires \valid(CC_OUTR_02);
    requires \valid(CC_OUTR_03);
    requires \separated(CC_OUTR_01, CC_OUTR_02, CC_OUTR_03, &cruise_integral);

    assigns cruise_integral, *CC_OUTR_01, *CC_OUTR_02, *CC_OUTR_03;

    ensures verify_init_speed: *CC_OUTR_01 == 0.0f;
    ensures verify_init_throttle: *CC_OUTR_02 == 0.0f;
    ensures verify_CCB_01: *CC_OUTR_03 == CRUISE_OFF;
    ensures verify_init_integral: cruise_integral == 0.0f;
*/
void cruise_control_init(
    float *CC_OUTR_01,
    float *CC_OUTR_02,
    CruiseState *CC_OUTR_03);

/*@
    requires \valid(CC_OUTR_01);
    requires \valid(CC_OUTR_02);
    requires \valid(CC_OUTR_03);
    requires \separated(CC_OUTR_01, CC_OUTR_02, CC_OUTR_03, &cruise_integral);
    requires \is_finite(CC_INR_06);
    requires \is_finite(CC_INR_07);
    requires \is_finite(CC_INR_08);
    requires \is_finite(*CC_OUTR_01);
    requires \is_finite(cruise_integral);
    requires 0.0f <= CC_INR_08 <= 1000.0f;
    requires 0.0f <= *CC_OUTR_01 <= SPEED_MAX;
    requires -1000.0f <= cruise_integral <= 1000.0f;
    requires cc_verify_state(*CC_OUTR_03);
    requires *CC_OUTR_03 != CRUISE_OFF ==>
        cc_verify_speed_in_limits(*CC_OUTR_01);

    assigns cruise_integral, *CC_OUTR_01, *CC_OUTR_02, *CC_OUTR_03;

    ensures verify_state: cc_verify_state(*CC_OUTR_03);
    ensures verify_nonnegative_throttle: *CC_OUTR_02 >= 0.0f;
    ensures verify_CCB_02:
        CC_INR_01 ==> *CC_OUTR_03 != CRUISE_OFF;
    ensures verify_CDC_01:
        *CC_OUTR_03 == CRUISE_OFF ==>
            *CC_OUTR_02 == (CC_INR_06 > PEDALS_MIN ? CC_INR_06 : 0.0f);
    ensures verify_CDC_02:
        *CC_OUTR_03 == CRUISE_ON &&
        *CC_OUTR_01 - CC_INR_08 > 0.0f &&
        cc_verify_pi(*CC_OUTR_01 - CC_INR_08, \old(cruise_integral)) > 0.0f
        ==> *CC_OUTR_02 > 0.0f;
    ensures verify_CDC_03:
        *CC_OUTR_03 == CRUISE_ON ==>
            *CC_OUTR_02 == cc_verify_clamp(
                cc_verify_pi(*CC_OUTR_01 - CC_INR_08, \old(cruise_integral)),
                0.0f, THROTTLE_SAT_MAX);
    ensures verify_CDC_04_inactive:
        *CC_OUTR_03 != CRUISE_ON ==>
            cruise_integral == 0.0f &&
            (*CC_OUTR_03 != CRUISE_OFF ==> *CC_OUTR_02 == 0.0f);
    ensures verify_CDC_04_freeze:
        *CC_OUTR_03 == CRUISE_ON &&
        (cc_verify_pi(*CC_OUTR_01 - CC_INR_08, \old(cruise_integral)) <= 0.0f ||
         cc_verify_pi(*CC_OUTR_01 - CC_INR_08, \old(cruise_integral)) >= THROTTLE_SAT_MAX)
        ==> cruise_integral == \old(cruise_integral);
    ensures verify_CDC_04_update:
        *CC_OUTR_03 == CRUISE_ON &&
        0.0f < cc_verify_pi(*CC_OUTR_01 - CC_INR_08, \old(cruise_integral)) < THROTTLE_SAT_MAX
        ==> cruise_integral == \old(cruise_integral) + (*CC_OUTR_01 - CC_INR_08);
    ensures verify_CDC_05:
        *CC_OUTR_03 == CRUISE_ON ==> *CC_OUTR_02 <= THROTTLE_SAT_MAX;

    ensures verify_CSM_01:
        *CC_OUTR_03 == CRUISE_OFF ==> *CC_OUTR_01 == \old(*CC_OUTR_01);
    ensures verify_resume_target:
        CC_INR_01 && CC_INR_07 <= PEDALS_MIN &&
        \old(*CC_OUTR_03) == CRUISE_INT && CC_INR_02
        ==> *CC_OUTR_01 == \old(*CC_OUTR_01);
    ensures verify_CSM_02:
        *CC_OUTR_03 != CRUISE_OFF &&
        !(\old(*CC_OUTR_03) == CRUISE_INT && CC_INR_02 && *CC_OUTR_03 != CRUISE_INT) &&
        CC_INR_03 ==>
            *CC_OUTR_01 == cc_verify_clamp(CC_INR_08, SPEED_MIN, SPEED_MAX);
    ensures verify_CSM_03:
        *CC_OUTR_03 != CRUISE_OFF &&
        !(\old(*CC_OUTR_03) == CRUISE_INT && CC_INR_02 && *CC_OUTR_03 != CRUISE_INT) &&
        !CC_INR_03 && CC_INR_05 ==>
            *CC_OUTR_01 == cc_verify_clamp(\old(*CC_OUTR_01) + SPEED_INC, SPEED_MIN, SPEED_MAX);
    ensures verify_CSM_04:
        *CC_OUTR_03 != CRUISE_OFF &&
        !(\old(*CC_OUTR_03) == CRUISE_INT && CC_INR_02 && *CC_OUTR_03 != CRUISE_INT) &&
        !CC_INR_03 && !CC_INR_05 && CC_INR_04 ==>
            *CC_OUTR_01 == cc_verify_clamp(\old(*CC_OUTR_01) - SPEED_INC, SPEED_MIN, SPEED_MAX);
    ensures verify_no_speed_button:
        *CC_OUTR_03 != CRUISE_OFF && !CC_INR_03 && !CC_INR_05 && !CC_INR_04 ==>
            *CC_OUTR_01 == cc_verify_clamp(\old(*CC_OUTR_01), SPEED_MIN, SPEED_MAX);
    ensures verify_CSM_05:
        *CC_OUTR_03 != CRUISE_OFF ==> cc_verify_speed_in_limits(*CC_OUTR_01);

    behavior verify_disabled:
        assumes !CC_INR_01;
        ensures verify_CCB_03: *CC_OUTR_03 == CRUISE_OFF;

    behavior verify_brake:
        assumes CC_INR_01;
        assumes CC_INR_07 > PEDALS_MIN;
        ensures verify_CCB_07: *CC_OUTR_03 == CRUISE_INT;

    behavior verify_interrupted:
        assumes CC_INR_01;
        assumes CC_INR_07 <= PEDALS_MIN;
        assumes *CC_OUTR_03 == CRUISE_INT;
        assumes !CC_INR_02;
        ensures verify_wait_for_resume: *CC_OUTR_03 == CRUISE_INT;

    behavior verify_regulate:
        assumes CC_INR_01;
        assumes CC_INR_07 <= PEDALS_MIN;
        assumes *CC_OUTR_03 != CRUISE_INT || CC_INR_02;
        ensures verify_CCB_04:
            CC_INR_06 <= PEDALS_MIN && cc_verify_speed_in_limits(CC_INR_08)
            ==> *CC_OUTR_03 == CRUISE_ON;
        ensures verify_CCB_05:
            CC_INR_06 > PEDALS_MIN || !cc_verify_speed_in_limits(CC_INR_08)
            ==> *CC_OUTR_03 == CRUISE_STDBY;
        ensures verify_CCB_06:
            \old(*CC_OUTR_03) == CRUISE_STDBY &&
            CC_INR_06 <= PEDALS_MIN && cc_verify_speed_in_limits(CC_INR_08)
            ==> *CC_OUTR_03 == CRUISE_ON;
        ensures verify_CCB_08:
            \old(*CC_OUTR_03) == CRUISE_INT && CC_INR_02 ==>
                (*CC_OUTR_03 ==
                    (CC_INR_06 <= PEDALS_MIN && cc_verify_speed_in_limits(CC_INR_08)
                     ? CRUISE_ON : CRUISE_STDBY) &&
                 *CC_OUTR_01 == \old(*CC_OUTR_01));

    complete behaviors verify_disabled, verify_brake, verify_interrupted, verify_regulate;
    disjoint behaviors verify_disabled, verify_brake, verify_interrupted, verify_regulate;
*/
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
