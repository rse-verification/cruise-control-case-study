#include "cruise_control.h"
#include "Rte_CruiseControl.h"
#include "module_constants.h"

static tF32 cruise_speed = 0.0f;
static tF32 cruise_integral = 0.0f;
static CruiseState cruise_state = CRUISE_OFF;

#ifndef CRUISE_CONTROL_EXTERNAL_CONTRACTS
#include "cruise_control_contracts.h"
#endif

void cruise_control_init(void)
{
    cruise_speed = 0.0f;
    cruise_integral = 0.0f;
    cruise_state = CRUISE_OFF;
    (void)Rte_Write_CruiseSpeedOut_CruiseSpeed(0.0f);
    (void)Rte_Write_ThrottleOut_ThrottleCommand(0.0f);
    (void)Rte_Write_CruiseStateOut_CruiseState(CRUISE_OFF);
}

void cruise_control(void)
{
    boolean enable = false, resume = false, set = false;
    boolean decel = false, accel = false;
    float32 accelerator = 0.0f, brake = 0.0f, speed = 0.0f;

    if (Rte_Read_Enable_CruiseEnable(&enable) != RTE_E_OK) return;
    if (Rte_Read_Resume_Resume(&resume) != RTE_E_OK) return;
    if (Rte_Read_Set_Set(&set) != RTE_E_OK) return;
    if (Rte_Read_QuickDecel_QuickDecel(&decel) != RTE_E_OK) return;
    if (Rte_Read_QuickAccel_QuickAccel(&accel) != RTE_E_OK) return;
    if (Rte_Read_Accelerator_Accelerator(&accelerator) != RTE_E_OK) return;
    if (Rte_Read_Brake_Brake(&brake) != RTE_E_OK) return;
    if (Rte_Read_SpeedIn_VehicleSpeed(&speed) != RTE_E_OK) return;

    CruiseState previous_state = cruise_state;
    boolean accelerator_pressed = accelerator > PEDALS_MIN;
    boolean brake_pressed = brake > PEDALS_MIN;
    CruiseState new_state;
    if (!enable) new_state = CRUISE_OFF;
    else if (brake_pressed) new_state = CRUISE_INT;
    else if (previous_state == CRUISE_INT && !resume) new_state = CRUISE_INT;
    else if (!accelerator_pressed && speed >= SPEED_MIN && speed <= SPEED_MAX)
        new_state = CRUISE_ON;
    else new_state = CRUISE_STDBY;

    boolean successful_resume = previous_state == CRUISE_INT && resume && new_state != CRUISE_INT;
    tF32 target = cruise_speed;
    if (new_state != CRUISE_OFF && !successful_resume)
    {
        if (set) target = speed;
        else if (accel)
        {
            if (target < SPEED_MAX) target += SPEED_INC;
        }
        else if (decel && target > SPEED_MIN) target -= SPEED_INC;
        if (target < SPEED_MIN) target = SPEED_MIN;
        else if (target > SPEED_MAX) target = SPEED_MAX;
    }

    tF32 throttle;
    if (new_state == CRUISE_OFF)
    {
        cruise_integral = 0.0f;
        throttle = accelerator_pressed ? accelerator : 0.0f;
    }
    else if (new_state != CRUISE_ON)
    {
        cruise_integral = 0.0f;
        throttle = 0.0f;
    }
    else
    {
        tF32 error = target - speed;
        tF32 next_integral = cruise_integral + error;
        tF32 candidate = KP * error + KI * next_integral;
        if (candidate >= THROTTLE_SAT_MAX) throttle = THROTTLE_SAT_MAX;
        else if (candidate <= 0.0f) throttle = 0.0f;
        else
        {
            cruise_integral = next_integral;
            throttle = candidate;
        }
    }
    cruise_speed = target;
    cruise_state = new_state;

    /* Writes are independent: a failed port does not suppress later writes. */
    (void)Rte_Write_CruiseSpeedOut_CruiseSpeed(cruise_speed);
    (void)Rte_Write_ThrottleOut_ThrottleCommand(throttle);
    (void)Rte_Write_CruiseStateOut_CruiseState(cruise_state);
}
