#ifndef RTE_CRUISE_CONTROL_H
#define RTE_CRUISE_CONTROL_H

#include "cruise_control_types.h"

#define RTE_E_OK E_OK
#define RTE_E_NOT_OK E_NOT_OK
#define CC_RTE_INPUT_COUNT 8u
#define CC_RTE_OUTPUT_COUNT 3u

/* Verification-only environment state. No runtime signal storage is defined here. */
/*@ ghost extern boolean model_enable, model_resume, model_set, model_decel, model_accel; */
/*@ ghost extern float32 model_accelerator, model_brake, model_speed; */
/*@ ghost extern Std_ReturnType model_read_status[CC_RTE_INPUT_COUNT]; */
/*@ ghost extern Std_ReturnType model_write_status[CC_RTE_OUTPUT_COUNT]; */
/*@ ghost extern uint32 model_read_count[CC_RTE_INPUT_COUNT]; */
/*@ ghost extern uint32 model_write_count[CC_RTE_OUTPUT_COUNT]; */
/*@ ghost extern float32 model_output_speed, model_output_throttle; */
/*@ ghost extern CruiseState model_output_state; */

/*@
    requires \valid(value);
    requires model_read_count[0] < UINT32_MAX;
    terminates \true;
    exits \false;
    assigns \result, *value, model_read_count[0]
        \from *value, model_read_count[0], model_read_status[0], model_enable;
    ensures \result == model_read_status[0];
    ensures model_read_count[0] == \old(model_read_count[0]) + 1;
    ensures \result == RTE_E_OK ==> *value == model_enable;
*/
Std_ReturnType Rte_Read_Enable_CruiseEnable(boolean *value);

/*@
    requires \valid(value);
    requires model_read_count[1] < UINT32_MAX;
    terminates \true;
    exits \false;
    assigns \result, *value, model_read_count[1]
        \from *value, model_read_count[1], model_read_status[1], model_resume;
    ensures \result == model_read_status[1];
    ensures model_read_count[1] == \old(model_read_count[1]) + 1;
    ensures \result == RTE_E_OK ==> *value == model_resume;
*/
Std_ReturnType Rte_Read_Resume_Resume(boolean *value);

/*@
    requires \valid(value);
    requires model_read_count[2] < UINT32_MAX;
    terminates \true;
    exits \false;
    assigns \result, *value, model_read_count[2]
        \from *value, model_read_count[2], model_read_status[2], model_set;
    ensures \result == model_read_status[2];
    ensures model_read_count[2] == \old(model_read_count[2]) + 1;
    ensures \result == RTE_E_OK ==> *value == model_set;
*/
Std_ReturnType Rte_Read_Set_Set(boolean *value);

/*@
    requires \valid(value);
    requires model_read_count[3] < UINT32_MAX;
    terminates \true;
    exits \false;
    assigns \result, *value, model_read_count[3]
        \from *value, model_read_count[3], model_read_status[3], model_decel;
    ensures \result == model_read_status[3];
    ensures model_read_count[3] == \old(model_read_count[3]) + 1;
    ensures \result == RTE_E_OK ==> *value == model_decel;
*/
Std_ReturnType Rte_Read_QuickDecel_QuickDecel(boolean *value);

/*@
    requires \valid(value);
    requires model_read_count[4] < UINT32_MAX;
    terminates \true;
    exits \false;
    assigns \result, *value, model_read_count[4]
        \from *value, model_read_count[4], model_read_status[4], model_accel;
    ensures \result == model_read_status[4];
    ensures model_read_count[4] == \old(model_read_count[4]) + 1;
    ensures \result == RTE_E_OK ==> *value == model_accel;
*/
Std_ReturnType Rte_Read_QuickAccel_QuickAccel(boolean *value);

/*@
    requires \valid(value);
    requires model_read_count[5] < UINT32_MAX;
    terminates \true;
    exits \false;
    assigns \result, *value, model_read_count[5]
        \from *value, model_read_count[5], model_read_status[5], model_accelerator;
    ensures \result == model_read_status[5];
    ensures model_read_count[5] == \old(model_read_count[5]) + 1;
    ensures \result == RTE_E_OK ==> *value == model_accelerator;
*/
Std_ReturnType Rte_Read_Accelerator_Accelerator(float32 *value);

/*@
    requires \valid(value);
    requires model_read_count[6] < UINT32_MAX;
    terminates \true;
    exits \false;
    assigns \result, *value, model_read_count[6]
        \from *value, model_read_count[6], model_read_status[6], model_brake;
    ensures \result == model_read_status[6];
    ensures model_read_count[6] == \old(model_read_count[6]) + 1;
    ensures \result == RTE_E_OK ==> *value == model_brake;
*/
Std_ReturnType Rte_Read_Brake_Brake(float32 *value);

/*@
    requires \valid(value);
    requires model_read_count[7] < UINT32_MAX;
    terminates \true;
    exits \false;
    assigns \result, *value, model_read_count[7]
        \from *value, model_read_count[7], model_read_status[7], model_speed;
    ensures \result == model_read_status[7];
    ensures model_read_count[7] == \old(model_read_count[7]) + 1;
    ensures \result == RTE_E_OK ==> *value == model_speed;
*/
Std_ReturnType Rte_Read_SpeedIn_VehicleSpeed(float32 *value);

/*@
    requires \is_finite(value);
    requires model_write_count[0] < UINT32_MAX;
    terminates \true;
    exits \false;
    assigns \result, model_output_speed, model_write_count[0]
        \from value, model_output_speed, model_write_count[0], model_write_status[0];
    ensures \result == model_write_status[0];
    ensures model_write_count[0] == \old(model_write_count[0]) + 1;
    ensures model_output_speed ==
        (\result == RTE_E_OK ? value : \old(model_output_speed));
*/
Std_ReturnType Rte_Write_CruiseSpeedOut_CruiseSpeed(float32 value);

/*@
    requires \is_finite(value);
    requires model_write_count[1] < UINT32_MAX;
    terminates \true;
    exits \false;
    assigns \result, model_output_throttle, model_write_count[1]
        \from value, model_output_throttle, model_write_count[1], model_write_status[1];
    ensures \result == model_write_status[1];
    ensures model_write_count[1] == \old(model_write_count[1]) + 1;
    ensures model_output_throttle ==
        (\result == RTE_E_OK ? value : \old(model_output_throttle));
*/
Std_ReturnType Rte_Write_ThrottleOut_ThrottleCommand(float32 value);

/*@
    requires model_write_count[2] < UINT32_MAX;
    terminates \true;
    exits \false;
    assigns \result, model_output_state, model_write_count[2]
        \from value, model_output_state, model_write_count[2], model_write_status[2];
    ensures \result == model_write_status[2];
    ensures model_write_count[2] == \old(model_write_count[2]) + 1;
    ensures model_output_state ==
        (\result == RTE_E_OK ? value : \old(model_output_state));
*/
Std_ReturnType Rte_Write_CruiseStateOut_CruiseState(CruiseState value);

#endif
