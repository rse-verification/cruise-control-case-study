#ifndef CRUISE_CONTROL_CONTRACTS_H
#define CRUISE_CONTROL_CONTRACTS_H

#include "Rte_CruiseControl.h"
#include "module_constants.h"

/* Included after the private module state, never used to expose that state. */
/*@
    predicate cc_valid_state(CruiseState state) =
        state == CRUISE_OFF || state == CRUISE_ON ||
        state == CRUISE_STDBY || state == CRUISE_INT;

    predicate cc_read_prefix_ok(integer n) =
        \forall integer i; 0 <= i < n ==> model_read_status[i] == RTE_E_OK;

    logic real cc_clamp(real value, real lower, real upper) =
        value < lower ? lower : (value > upper ? upper : value);

    logic CruiseState cc_next_state(CruiseState previous) =
        !model_enable ? CRUISE_OFF :
        model_brake > PEDALS_MIN ? CRUISE_INT :
        previous == CRUISE_INT && !model_resume ? CRUISE_INT :
        model_accelerator <= PEDALS_MIN && SPEED_MIN <= model_speed <= SPEED_MAX
        ? CRUISE_ON : CRUISE_STDBY;

    predicate cc_successful_resume(CruiseState previous) =
        model_enable && previous == CRUISE_INT && model_resume && model_brake <= PEDALS_MIN;

    logic real cc_next_target(real target, CruiseState previous) =
        !model_enable || cc_successful_resume(previous) ? target :
        cc_clamp(model_set ? model_speed :
                 model_accel ? target + SPEED_INC :
                 model_decel ? target - SPEED_INC : target, SPEED_MIN, SPEED_MAX);

    logic real cc_candidate(real target, real integral, CruiseState previous) =
        KP * (cc_next_target(target, previous) - model_speed) +
        KI * (integral + cc_next_target(target, previous) - model_speed);

    logic real cc_next_throttle(real target, real integral, CruiseState previous) =
        cc_next_state(previous) == CRUISE_OFF
        ? (model_accelerator > PEDALS_MIN ? model_accelerator : 0.0) :
        cc_next_state(previous) != CRUISE_ON ? 0.0 :
        cc_clamp(cc_candidate(target, integral, previous), 0.0, THROTTLE_SAT_MAX);

    logic real cc_next_integral(real target, real integral, CruiseState previous) =
        cc_next_state(previous) != CRUISE_ON ? 0.0 :
        0.0 < cc_candidate(target, integral, previous) < THROTTLE_SAT_MAX
        ? integral + cc_next_target(target, previous) - model_speed : integral;
*/

/*@
    requires \is_finite(model_output_speed) && \is_finite(model_output_throttle);
    requires \forall integer i; 0 <= i < CC_RTE_OUTPUT_COUNT ==> model_write_count[i] < UINT32_MAX;
    terminates \true;
    exits \false;
    assigns cruise_speed, cruise_integral, cruise_state,
            model_output_speed, model_output_throttle, model_output_state,
            model_write_count[0 .. CC_RTE_OUTPUT_COUNT-1];
    ensures init_target: cruise_speed == 0.0;
    ensures init_integral: cruise_integral == 0.0;
    ensures init_state: cruise_state == CRUISE_OFF;
    ensures init_write_attempts:
        \forall integer i; 0 <= i < CC_RTE_OUTPUT_COUNT ==>
            model_write_count[i] == \old(model_write_count[i]) + 1;
    ensures init_output_speed: model_output_speed ==
        (model_write_status[0] == RTE_E_OK ? 0.0 : \old(model_output_speed));
    ensures init_output_throttle: model_output_throttle ==
        (model_write_status[1] == RTE_E_OK ? 0.0 : \old(model_output_throttle));
    ensures init_output_state: model_output_state ==
        (model_write_status[2] == RTE_E_OK ? CRUISE_OFF : \old(model_output_state));
*/
void cruise_control_init(void);

/*@
    requires cc_valid_state(cruise_state);
    requires \is_finite(cruise_speed) && 0.0 <= cruise_speed <= SPEED_MAX;
    requires cruise_state != CRUISE_OFF ==> SPEED_MIN <= cruise_speed;
    requires \is_finite(cruise_integral) && -1000.0 <= cruise_integral <= 1000.0;
    requires \is_finite(model_accelerator) && \is_finite(model_brake);
    requires \is_finite(model_speed) && 0.0 <= model_speed <= 1000.0;
    requires \is_finite(model_output_speed) && \is_finite(model_output_throttle);
    requires \forall integer i; 0 <= i < CC_RTE_INPUT_COUNT ==> model_read_count[i] < UINT32_MAX;
    requires \forall integer i; 0 <= i < CC_RTE_OUTPUT_COUNT ==> model_write_count[i] < UINT32_MAX;
    terminates \true;
    exits \false;
    assigns cruise_speed, cruise_integral, cruise_state,
            model_output_speed, model_output_throttle, model_output_state,
            model_read_count[0 .. CC_RTE_INPUT_COUNT-1],
            model_write_count[0 .. CC_RTE_OUTPUT_COUNT-1];
    ensures read_attempts:
        \forall integer i; 0 <= i < CC_RTE_INPUT_COUNT ==>
            model_read_count[i] == \old(model_read_count[i]) + (cc_read_prefix_ok(i) ? 1 : 0);
    ensures write_attempts:
        \forall integer i; 0 <= i < CC_RTE_OUTPUT_COUNT ==>
            model_write_count[i] == \old(model_write_count[i]) +
                (cc_read_prefix_ok(CC_RTE_INPUT_COUNT) ? 1 : 0);

    behavior read_failure:
        assumes !cc_read_prefix_ok(CC_RTE_INPUT_COUNT);
        ensures failed_read_target: cruise_speed == \old(cruise_speed);
        ensures failed_read_integral: cruise_integral == \old(cruise_integral);
        ensures failed_read_state: cruise_state == \old(cruise_state);
        ensures failed_read_outputs:
            model_output_speed == \old(model_output_speed) &&
            model_output_throttle == \old(model_output_throttle) &&
            model_output_state == \old(model_output_state);

    behavior accepted_inputs:
        assumes cc_read_prefix_ok(CC_RTE_INPUT_COUNT);
        ensures next_state: cruise_state == cc_next_state(\old(cruise_state));
        ensures next_target: cruise_speed == cc_next_target(\old(cruise_speed), \old(cruise_state));
        ensures next_integral: cruise_integral ==
            cc_next_integral(\old(cruise_speed), \old(cruise_integral), \old(cruise_state));
        ensures published_speed: model_output_speed ==
            (model_write_status[0] == RTE_E_OK ? cruise_speed : \old(model_output_speed));
        ensures published_throttle: model_output_throttle ==
            (model_write_status[1] == RTE_E_OK
             ? cc_next_throttle(\old(cruise_speed), \old(cruise_integral), \old(cruise_state))
             : \old(model_output_throttle));
        ensures published_state: model_output_state ==
            (model_write_status[2] == RTE_E_OK ? cruise_state : \old(model_output_state));
        ensures state_invariant: cc_valid_state(cruise_state);
        ensures bounded_target: cruise_state != CRUISE_OFF ==> SPEED_MIN <= cruise_speed <= SPEED_MAX;
        ensures nonnegative_throttle:
            model_write_status[1] == RTE_E_OK ==> model_output_throttle >= 0.0;
        ensures bounded_automatic_throttle:
            cruise_state == CRUISE_ON && model_write_status[1] == RTE_E_OK ==>
                model_output_throttle <= THROTTLE_SAT_MAX;

    complete behaviors;
    disjoint behaviors;
*/
void cruise_control(void);

#endif
