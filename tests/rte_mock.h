#ifndef RTE_MOCK_H
#define RTE_MOCK_H

#include "Rte_CruiseControl.h"

typedef struct {
    boolean enable, resume, set, decel, accel;
    float32 accelerator, brake, speed;
} RteMockInputs;

typedef struct {
    RteMockInputs inputs;
    Std_ReturnType read_status[CC_RTE_INPUT_COUNT];
    Std_ReturnType write_status[CC_RTE_OUTPUT_COUNT];
    unsigned read_count[CC_RTE_INPUT_COUNT];
    unsigned write_count[CC_RTE_OUTPUT_COUNT];
    unsigned trace[128];
    unsigned trace_count;
    float32 output_speed, output_throttle;
    CruiseState output_state;
} RteMock;

extern RteMock rte_mock;
void RteMock_Reset(void);
void RteMock_ClearCalls(void);

#endif
