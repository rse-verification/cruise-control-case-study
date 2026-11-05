#include "cruise_control.h"
#include "module_constants.h"

static tF32 cruise_integral = 0.0f;

/*@
    requires \valid(CC_OUTR_01);
    requires \valid(CC_OUTR_02);
    requires \valid(CC_OUTR_03);
    requires \separated(
        CC_OUTR_01,
        CC_OUTR_02,
        CC_OUTR_03,
        &cruise_integral);

    assigns cruise_integral,
            *CC_OUTR_01,
            *CC_OUTR_02,
            *CC_OUTR_03;

    ensures *CC_OUTR_01 == 0.0f;
    ensures *CC_OUTR_02 == 0.0f;
    ensures CC_HLR_CCB_01:
        *CC_OUTR_03 == CRUISE_OFF;
    ensures cruise_integral == 0.0f;
*/
void cruise_control_init(
    float *CC_OUTR_01,
    float *CC_OUTR_02,
    CruiseState *CC_OUTR_03)
{
    cruise_integral = 0.0f;
    *CC_OUTR_01 = 0.0f;
    *CC_OUTR_02 = 0.0f;
    *CC_OUTR_03 = CRUISE_OFF;
}

/*@
    logic real pi_calc(real a, real b) =
        KP * a + KI * (b + a);

    logic real control_calc(
        real error,
        real integral) =
            pi_calc(error, integral);

    predicate pi_upper_saturated(real error, real integral) =
        pi_calc(error, integral) >= THROTTLE_SAT_MAX;

    predicate pi_lower_saturated(real error, real integral) =
        pi_calc(error, integral) <= 0.0f;

    predicate pi_unsaturated(real error, real integral) =
        pi_calc(error, integral) > 0.0f &&
        pi_calc(error, integral) < THROTTLE_SAT_MAX;
*/

/*@
    predicate state_invariant(CruiseState state) =
        state == CRUISE_ON ||
        state == CRUISE_STDBY ||
        state == CRUISE_INT ||
        state == CRUISE_OFF;
*/

/*@
    predicate CC_HLR_PPD_01(float accelerator_value) =
        accelerator_value > PEDALS_MIN;

    predicate CC_HLR_PPD_02(float brake_value) =
        brake_value > PEDALS_MIN;

    predicate val_in_limits(float val, float min, float max) =
        val >= min && val <= max;

    logic real cruise_speed_error(real cruise_speed, real current_speed) =
        cruise_speed - current_speed;
*/

/*@
    requires \valid(CC_OUTR_01);
    requires \valid(CC_OUTR_02);
    requires \valid(CC_OUTR_03);
    requires \separated(
        CC_OUTR_01,
        CC_OUTR_02,
        CC_OUTR_03,
        &cruise_integral);

    requires \is_finite(CC_INR_06);
    requires \is_finite(CC_INR_07);
    requires \is_finite(CC_INR_08);
    requires \is_finite(*CC_OUTR_01);
    requires \is_finite(cruise_integral);

    requires CC_INR_08 >= (float)0;
    requires CC_INR_08 <= (float)1000;
    requires *CC_OUTR_01 >= (float)0;
    requires *CC_OUTR_01 <= SPEED_MAX;
    requires cruise_integral >= (float)-1000;
    requires cruise_integral <= (float)1000;
    requires state_invariant(*CC_OUTR_03);
    requires *CC_OUTR_03 != CRUISE_OFF ==>
        val_in_limits(*CC_OUTR_01, SPEED_MIN, SPEED_MAX);

    assigns cruise_integral,
            *CC_OUTR_01,
            *CC_OUTR_02,
            *CC_OUTR_03;

    ensures state_invariant(*CC_OUTR_03);
    ensures *CC_OUTR_02 >= 0.0f;
    ensures CC_HLR_CDC_05:
        *CC_OUTR_03 == CRUISE_ON ==>
            *CC_OUTR_02 <= THROTTLE_SAT_MAX;

    ensures CC_HLR_CDC_01:
        *CC_OUTR_03 == CRUISE_OFF ==>
            *CC_OUTR_02 ==
                (CC_INR_06 > PEDALS_MIN ? CC_INR_06 : 0.0f);

    ensures CC_HLR_CDC_02:
        (*CC_OUTR_03 == CRUISE_ON &&
         cruise_speed_error(*CC_OUTR_01, CC_INR_08) > 0.0f &&
         control_calc(
             cruise_speed_error(*CC_OUTR_01, CC_INR_08),
             \old(cruise_integral)) > 0.0f)
        ==> *CC_OUTR_02 > 0.0f;

    ensures CC_HLR_CDC_03:
        *CC_OUTR_03 == CRUISE_ON ==>
            *CC_OUTR_02 ==
                (control_calc(
                    cruise_speed_error(*CC_OUTR_01, CC_INR_08),
                    \old(cruise_integral)) >= THROTTLE_SAT_MAX
                 ? THROTTLE_SAT_MAX
                 : (control_calc(
                        cruise_speed_error(*CC_OUTR_01, CC_INR_08),
                        \old(cruise_integral)) <= 0.0f
                    ? 0.0f
                    : control_calc(
                        cruise_speed_error(*CC_OUTR_01, CC_INR_08),
                        \old(cruise_integral))));

    ensures CC_HLR_CCB_02:
        CC_INR_01 ==>
        (*CC_OUTR_03 == CRUISE_ON ||
         *CC_OUTR_03 == CRUISE_STDBY ||
         *CC_OUTR_03 == CRUISE_INT);

    ensures CC_HLR_CSM_01:
        *CC_OUTR_03 == CRUISE_OFF ==>
            *CC_OUTR_01 == \old(*CC_OUTR_01);

    ensures CC_HLR_CSM_02:
        *CC_OUTR_03 != CRUISE_OFF &&
        !(\old(*CC_OUTR_03) == CRUISE_INT &&
          CC_INR_02 &&
          *CC_OUTR_03 != CRUISE_INT) &&
        CC_INR_03 ==>
            *CC_OUTR_01 ==
                (CC_INR_08 < SPEED_MIN ? SPEED_MIN :
                (CC_INR_08 > SPEED_MAX ? SPEED_MAX : CC_INR_08));

    ensures CC_HLR_CSM_03:
        *CC_OUTR_03 != CRUISE_OFF &&
        !(\old(*CC_OUTR_03) == CRUISE_INT &&
          CC_INR_02 &&
          *CC_OUTR_03 != CRUISE_INT) &&
        !CC_INR_03 && CC_INR_05 ==>
            *CC_OUTR_01 ==
                (\old(*CC_OUTR_01) + SPEED_INC < SPEED_MIN ? SPEED_MIN :
                (\old(*CC_OUTR_01) + SPEED_INC > SPEED_MAX ? SPEED_MAX :
                \old(*CC_OUTR_01) + SPEED_INC));

    ensures CC_HLR_CSM_04:
        *CC_OUTR_03 != CRUISE_OFF &&
        !(\old(*CC_OUTR_03) == CRUISE_INT &&
          CC_INR_02 &&
          *CC_OUTR_03 != CRUISE_INT) &&
        !CC_INR_03 &&
        !CC_INR_05 &&
        CC_INR_04 ==>
            *CC_OUTR_01 ==
                (\old(*CC_OUTR_01) - SPEED_INC < SPEED_MIN ? SPEED_MIN :
                (\old(*CC_OUTR_01) - SPEED_INC > SPEED_MAX ? SPEED_MAX :
                \old(*CC_OUTR_01) - SPEED_INC));

    ensures *CC_OUTR_03 != CRUISE_OFF &&
            !CC_INR_03 &&
            !CC_INR_05 &&
            !CC_INR_04 ==>
                *CC_OUTR_01 ==
                    (\old(*CC_OUTR_01) < SPEED_MIN ? SPEED_MIN :
                    (\old(*CC_OUTR_01) > SPEED_MAX ? SPEED_MAX :
                    \old(*CC_OUTR_01)));

    ensures CC_HLR_CDC_04_reset_inactive:
        *CC_OUTR_03 != CRUISE_ON ==>
            cruise_integral == 0.0f &&
            (*CC_OUTR_03 != CRUISE_OFF ==>
                *CC_OUTR_02 == 0.0f);

    ensures CC_HLR_CDC_04_freeze:
        *CC_OUTR_03 == CRUISE_ON &&
        (pi_upper_saturated(
             cruise_speed_error(*CC_OUTR_01, CC_INR_08),
             \old(cruise_integral)) ||
         pi_lower_saturated(
             cruise_speed_error(*CC_OUTR_01, CC_INR_08),
             \old(cruise_integral))) ==>
            cruise_integral == \old(cruise_integral);

    ensures CC_HLR_CDC_03_integral:
        *CC_OUTR_03 == CRUISE_ON &&
        pi_unsaturated(
            cruise_speed_error(*CC_OUTR_01, CC_INR_08),
            \old(cruise_integral)) ==>
                cruise_integral ==
                    \old(cruise_integral) +
                    cruise_speed_error(*CC_OUTR_01, CC_INR_08);

    ensures CC_HLR_CSM_05:
        (*CC_OUTR_03 == CRUISE_ON ||
         *CC_OUTR_03 == CRUISE_STDBY ||
         *CC_OUTR_03 == CRUISE_INT) ==>
            (SPEED_MIN <= *CC_OUTR_01 &&
             *CC_OUTR_01 <= SPEED_MAX);

    behavior disabled:
        assumes !CC_INR_01;
        ensures CC_HLR_CCB_03:
            *CC_OUTR_03 == CRUISE_OFF;

    behavior brake_press:
        assumes CC_INR_01;
        assumes CC_INR_07 > PEDALS_MIN;
        ensures CC_HLR_CCB_07:
            *CC_OUTR_03 == CRUISE_INT;

    behavior interrupted:
        assumes CC_INR_01;
        assumes CC_INR_07 <= PEDALS_MIN;
        assumes *CC_OUTR_03 == CRUISE_INT;
        assumes !CC_INR_02;
        ensures *CC_OUTR_03 == CRUISE_INT;

    behavior regulate:
        assumes CC_INR_01;
        assumes CC_INR_07 <= PEDALS_MIN;
        assumes *CC_OUTR_03 != CRUISE_INT || CC_INR_02;

        ensures CC_HLR_CCB_04:
            (CC_INR_06 <= PEDALS_MIN &&
             val_in_limits(CC_INR_08, SPEED_MIN, SPEED_MAX))
            ==> *CC_OUTR_03 == CRUISE_ON;

        ensures CC_HLR_CCB_05:
            (CC_INR_06 > PEDALS_MIN ||
             !val_in_limits(CC_INR_08, SPEED_MIN, SPEED_MAX))
            ==> *CC_OUTR_03 == CRUISE_STDBY;

        ensures CC_HLR_CCB_06:
            (\old(*CC_OUTR_03) == CRUISE_STDBY &&
             CC_INR_06 <= PEDALS_MIN &&
             val_in_limits(CC_INR_08, SPEED_MIN, SPEED_MAX))
            ==> *CC_OUTR_03 == CRUISE_ON;

        ensures CC_HLR_CCB_08:
            (\old(*CC_OUTR_03) == CRUISE_INT && CC_INR_02)
            ==>
            (*CC_OUTR_03 ==
                ((CC_INR_06 <= PEDALS_MIN &&
                  val_in_limits(CC_INR_08, SPEED_MIN, SPEED_MAX))
                 ? CRUISE_ON
                 : CRUISE_STDBY)
             && *CC_OUTR_01 == \old(*CC_OUTR_01));

    complete behaviors;
    disjoint behaviors;
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
    CruiseState *CC_OUTR_03)
{
    CruiseState previous_state = *CC_OUTR_03;
    tB accelerator_pressed = CC_INR_06 > PEDALS_MIN;
    tB brake_pressed = CC_INR_07 > PEDALS_MIN;
    tB speed_in_limits =
        (CC_INR_08 >= SPEED_MIN) && (CC_INR_08 <= SPEED_MAX);

    CruiseState new_state;
    if (!CC_INR_01)
    {
        new_state = CRUISE_OFF;
    }
    else if (brake_pressed)
    {
        new_state = CRUISE_INT;
    }
    else if ((previous_state == CRUISE_INT) && !CC_INR_02)
    {
        new_state = CRUISE_INT;
    }
    else if (!accelerator_pressed && speed_in_limits)
    {
        new_state = CRUISE_ON;
    }
    else
    {
        new_state = CRUISE_STDBY;
    }

    tB resume_from_interrupt =
        (previous_state == CRUISE_INT) &&
        CC_INR_02 &&
        (new_state != CRUISE_INT);

    tF32 cruise_speed = *CC_OUTR_01;
    if ((new_state != CRUISE_OFF) && !resume_from_interrupt)
    {
        if (CC_INR_03)
        {
            if (CC_INR_08 < SPEED_MIN)
            {
                cruise_speed = SPEED_MIN;
            }
            else if (CC_INR_08 > SPEED_MAX)
            {
                cruise_speed = SPEED_MAX;
            }
            else
            {
                cruise_speed = CC_INR_08;
            }
        }
        else if (CC_INR_05)
        {
            if (cruise_speed < SPEED_MAX)
            {
                cruise_speed = cruise_speed + SPEED_INC;
            }
        }
        else if (CC_INR_04 && (cruise_speed > SPEED_MIN))
        {
            cruise_speed = cruise_speed - SPEED_INC;
        }

        if (cruise_speed < SPEED_MIN)
        {
            cruise_speed = SPEED_MIN;
        }
        else if (cruise_speed > SPEED_MAX)
        {
            cruise_speed = SPEED_MAX;
        }
    }

    tF32 speed_error = cruise_speed - CC_INR_08;
    tF32 throttle_cmd;

    if (new_state == CRUISE_OFF)
    {
        cruise_integral = 0.0f;
        throttle_cmd = accelerator_pressed ? CC_INR_06 : 0.0f;
    }
    else if (new_state != CRUISE_ON)
    {
        cruise_integral = 0.0f;
        throttle_cmd = 0.0f;
    }
    else
    {
        tF32 next_integral = cruise_integral + speed_error;
        tF32 controller_output =
            (KP * speed_error) + (KI * next_integral);

        if (controller_output >= THROTTLE_SAT_MAX)
        {
            throttle_cmd = THROTTLE_SAT_MAX;
        }
        else if (controller_output <= 0.0f)
        {
            throttle_cmd = 0.0f;
        }
        else
        {
            cruise_integral = next_integral;
            throttle_cmd = controller_output;
        }
    }

    *CC_OUTR_01 = cruise_speed;
    *CC_OUTR_02 = throttle_cmd;
    *CC_OUTR_03 = new_state;
}
