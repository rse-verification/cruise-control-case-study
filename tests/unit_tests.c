/* Unit tests for nlspec.txt, not an ACSL substitute.
 * The implementation is included to inspect private state without changing
 * the public API. Every test invocation runs in a fresh process.
 * Resume while braking uses the normal enabled speed-management rules.
 * Bounded SET and step results use increment-then-clamp.
 * OFF throttle is checked; STDBY and INT throttle magnitude is not asserted.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../cruise_control.c"

typedef struct {
    const char *name;
    CruiseState state;
    float target, integral;
    bool enabled, resume, set, decel, accel;
    float gas, brake, vehicle;
} AuditCase;
typedef struct { CruiseState state; float target, throttle, integral; } AuditResult;

static const AuditCase cases[] = {
    {"disabled_retains_speed", CRUISE_ON,100,5, false,false,false,false,false,10,0,80},
    {"off_accelerator_below_threshold", CRUISE_OFF,100,0, false,false,false,false,false,2,0,80},
    {"off_accelerator_at_threshold", CRUISE_OFF,100,0, false,false,false,false,false,3,0,80},
    {"off_manual_throttle_above_cruise_limit", CRUISE_OFF,100,0, false,false,false,false,false,50,0,80},
    {"brake_priority", CRUISE_ON,100,0, true,false,false,false,false,0,4,80},
    {"interrupted_waits_for_resume", CRUISE_INT,100,0, true,false,false,false,false,0,0,80},
    {"resume_preserves_speed_over_set", CRUISE_INT,100,0, true,true,true,false,false,0,0,80},
    {"resume_to_standby", CRUISE_INT,100,0, true,true,false,false,false,10,0,80},
    {"resume_while_braking_allows_set", CRUISE_INT,100,0, true,true,true,false,false,0,4,80},
    {"resume_while_braking_allows_quick_accel", CRUISE_INT,100,0, true,true,false,false,true,0,4,80},
    {"resume_while_braking_allows_quick_decel", CRUISE_INT,100,0, true,true,false,true,false,0,4,80},
    {"below_minimum_standby", CRUISE_ON,100,0, true,false,false,false,false,0,0,20},
    {"minimum_speed_inclusive", CRUISE_ON,100,0, true,false,false,false,false,0,0,30},
    {"maximum_speed_inclusive", CRUISE_ON,100,0, true,false,false,false,false,0,0,150},
    {"above_maximum_standby", CRUISE_ON,100,0, true,false,false,false,false,0,0,151},
    {"accelerator_threshold_equal", CRUISE_ON,100,0, true,false,false,false,false,3,0,100},
    {"accelerator_above_threshold", CRUISE_ON,100,0, true,false,false,false,false,3.1F,0,100},
    {"brake_threshold_equal", CRUISE_ON,100,0, true,false,false,false,false,0,3,100},
    {"brake_above_threshold", CRUISE_ON,100,0, true,false,false,false,false,0,3.1F,100},
    {"set_has_highest_speed_priority", CRUISE_ON,100,0, true,false,true,true,true,0,0,80},
    {"quick_accel_priority_over_decel", CRUISE_ON,100,0, true,false,false,true,true,0,0,100},
    {"quick_accel_near_maximum", CRUISE_ON,149,0, true,false,false,false,true,0,0,140},
    {"quick_accel_exact_maximum", CRUISE_ON,147.5F,0, true,false,false,false,true,0,0,140},
    {"quick_decel_near_minimum", CRUISE_ON,31,0, true,false,false,true,false,0,0,30},
    {"set_outside_range_is_bounded", CRUISE_ON,100,0, true,false,true,false,false,0,0,20},
    {"PI_on_entry_to_on", CRUISE_STDBY,100,0, true,false,false,false,false,0,0,95},
    {"PI_continued_on", CRUISE_ON,100,2, true,false,false,false,false,0,0,99},
    {"upper_saturation_freezes_integral", CRUISE_ON,100,10, true,false,false,false,false,0,0,95},
    {"lower_saturation_freezes_integral", CRUISE_ON,100,-10, true,false,false,false,false,0,0,105},
    {"float_rounding_example", CRUISE_ON,100,2.7F, true,false,false,false,false,0,0,98.7F},
    {"inactive_resets_integral", CRUISE_ON,100,20, true,false,false,false,false,5,0,95},
    {"no_speed_button_retains_target", CRUISE_ON,100,0, true,false,false,false,false,0,0,95}
};

static float bounded(float value, float low, float high)
{ return value < low ? low : value > high ? high : value; }

static AuditResult oracle(AuditCase c)
{
    AuditResult r = { c.state, c.target, 0, c.integral };
    if (!c.enabled) r.state = CRUISE_OFF;
    else if (c.brake > PEDALS_MIN) r.state = CRUISE_INT;
    else if (c.state == CRUISE_INT && !c.resume) r.state = CRUISE_INT;
    else r.state = c.gas <= PEDALS_MIN && c.vehicle >= SPEED_MIN && c.vehicle <= SPEED_MAX ? CRUISE_ON : CRUISE_STDBY;
    bool successful_resume = c.enabled && c.state == CRUISE_INT && c.resume && c.brake <= PEDALS_MIN;
    if (r.state != CRUISE_OFF && !successful_resume) {
        if (c.set) r.target = c.vehicle;
        else if (c.accel && c.target < SPEED_MAX) r.target += SPEED_INC;
        else if (!c.accel && c.decel && c.target > SPEED_MIN) r.target -= SPEED_INC;
        r.target = bounded(r.target, SPEED_MIN, SPEED_MAX);
    }
    if (r.state == CRUISE_ON) {
        float error = r.target - c.vehicle;
        float next_integral = c.integral + error;
        float candidate = KP * error + KI * next_integral;
        r.throttle = bounded(candidate, 0, THROTTLE_SAT_MAX);
        if (candidate > 0 && candidate < THROTTLE_SAT_MAX) r.integral = next_integral;
    } else {
        r.integral = 0;
        if (r.state == CRUISE_OFF) r.throttle = c.gas > PEDALS_MIN ? c.gas : 0;
    }
    return r;
}

static int close_float(float a, float b) { return fabsf(a-b) <= 0.0001F; }
static void emit(const char *name, AuditResult actual, AuditResult expected, bool throttle_defined)
{
    int pass = actual.state == expected.state && close_float(actual.target, expected.target) &&
        close_float(actual.integral, expected.integral) && (!throttle_defined || close_float(actual.throttle, expected.throttle));
    printf("{\"name\":\"%s\",\"passed\":%s,\"actual\":{\"state\":%d,\"speed\":%.9g,\"throttle\":%.9g,\"integral\":%.9g},\"expected\":{\"state\":%d,\"speed\":%.9g,\"throttle\":%.9g,\"integral\":%.9g},\"throttle_asserted\":%s}\n",
        name, pass ? "true" : "false", actual.state, (double)actual.target, (double)actual.throttle, (double)actual.integral,
        expected.state, (double)expected.target, (double)expected.throttle, (double)expected.integral, throttle_defined ? "true" : "false");
}

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    if (strcmp(argv[1], "list") == 0) {
        puts("init_resets_outputs_and_integral");
        for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; ++i) puts(cases[i].name);
        puts("reachable_quick_accel_boundary");
        puts("reachable_PI_reentry");
        return 0;
    }
    if (strcmp(argv[1], "init_resets_outputs_and_integral") == 0) {
        float speed = 80, throttle = 20; CruiseState state = CRUISE_ON;
        cruise_integral = 9;
        cruise_control_init(&speed, &throttle, &state);
        emit(argv[1], (AuditResult){state,speed,throttle,cruise_integral}, (AuditResult){CRUISE_OFF,0,0,0}, true);
        return 0;
    }
    if (strcmp(argv[1], "reachable_quick_accel_boundary") == 0 || strcmp(argv[1], "reachable_PI_reentry") == 0) {
        float speed, throttle; CruiseState state;
        cruise_control_init(&speed, &throttle, &state);
        if (strcmp(argv[1], "reachable_quick_accel_boundary") == 0) {
            cruise_control(true,false,true,false,false,0,0,149,&speed,&throttle,&state);
            cruise_control(true,false,false,false,true,0,0,140,&speed,&throttle,&state);
            emit(argv[1], (AuditResult){state,speed,throttle,cruise_integral}, (AuditResult){CRUISE_ON,150,45,0}, true);
        } else {
            cruise_control(true,false,true,false,false,0,0,100,&speed,&throttle,&state);
            cruise_control(true,false,false,false,false,0,4,100,&speed,&throttle,&state);
            cruise_control(true,true,false,false,false,5,0,95,&speed,&throttle,&state);
            cruise_control(true,false,false,false,false,0,0,95,&speed,&throttle,&state);
            emit(argv[1], (AuditResult){state,speed,throttle,cruise_integral}, (AuditResult){CRUISE_ON,100,KP*5+KI*5,5}, true);
        }
        return 0;
    }
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        AuditCase c = cases[i];
        if (strcmp(argv[1], c.name) != 0) continue;
        float speed = c.target, throttle = 0; CruiseState state = c.state;
        cruise_integral = c.integral;
        cruise_control(c.enabled,c.resume,c.set,c.decel,c.accel,c.gas,c.brake,c.vehicle,&speed,&throttle,&state);
        AuditResult expected = oracle(c);
        emit(c.name, (AuditResult){state,speed,throttle,cruise_integral}, expected,
             expected.state == CRUISE_ON || expected.state == CRUISE_OFF);
        return 0;
    }
    return 2;
}
