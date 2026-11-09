#include <assert.h>
#include <string.h>
#include "rte_mock.h"

RteMock rte_mock;

void RteMock_Reset(void)
{
    memset(&rte_mock, 0, sizeof rte_mock);
}

void RteMock_ClearCalls(void)
{
    memset(rte_mock.read_count, 0, sizeof rte_mock.read_count);
    memset(rte_mock.write_count, 0, sizeof rte_mock.write_count);
    rte_mock.trace_count = 0;
}

static void record(unsigned port)
{
    assert(rte_mock.trace_count < sizeof rte_mock.trace / sizeof rte_mock.trace[0]);
    rte_mock.trace[rte_mock.trace_count++] = port;
}

#define READ_PORT(function, type, field, index) \
    Std_ReturnType function(type *value) { \
        record(index); \
        ++rte_mock.read_count[index]; \
        if (rte_mock.read_status[index] == RTE_E_OK) *value = rte_mock.inputs.field; \
        return rte_mock.read_status[index]; \
    }

READ_PORT(Rte_Read_Enable_CruiseEnable, boolean, enable, 0u)
READ_PORT(Rte_Read_Resume_Resume, boolean, resume, 1u)
READ_PORT(Rte_Read_Set_Set, boolean, set, 2u)
READ_PORT(Rte_Read_QuickDecel_QuickDecel, boolean, decel, 3u)
READ_PORT(Rte_Read_QuickAccel_QuickAccel, boolean, accel, 4u)
READ_PORT(Rte_Read_Accelerator_Accelerator, float32, accelerator, 5u)
READ_PORT(Rte_Read_Brake_Brake, float32, brake, 6u)
READ_PORT(Rte_Read_SpeedIn_VehicleSpeed, float32, speed, 7u)

#define WRITE_PORT(function, type, field, index) \
    Std_ReturnType function(type value) { \
        record(CC_RTE_INPUT_COUNT + index); \
        ++rte_mock.write_count[index]; \
        if (rte_mock.write_status[index] == RTE_E_OK) rte_mock.field = value; \
        return rte_mock.write_status[index]; \
    }

WRITE_PORT(Rte_Write_CruiseSpeedOut_CruiseSpeed, float32, output_speed, 0u)
WRITE_PORT(Rte_Write_ThrottleOut_ThrottleCommand, float32, output_throttle, 1u)
WRITE_PORT(Rte_Write_CruiseStateOut_CruiseState, CruiseState, output_state, 2u)
