#include "controller.h"
#include "fsm.h"
#include "ir.h"
#include "search.h"
#include "sensor.h"

static const controller_command_t search_commands_l[] = {
    {
        .duration_us = 100,
        .motion      = CONTROLLER_MOTION_ROTATION,
        .power       = +25,
    }
};

static const controller_command_t search_commands_r[] = {
    {
        .duration_us = 100,
        .motion      = CONTROLLER_MOTION_ROTATION,
        .power       = -25,
    }
};

static const controller_sequence_t search_sequence_l = {
    .name     = "search sequence left",
    .commands = search_commands_l,
    .length   = sizeof(search_commands_l) / sizeof(search_commands_l[0]),
    .repeat   = true,
};

static const controller_sequence_t search_sequence_r = {
    .name     = "search sequence right",
    .commands = search_commands_r,
    .length   = sizeof(search_commands_r) / sizeof(search_commands_r[0]),
    .repeat   = true,
};

void search_entry(void) {
    controller_stop();
}

void search_exit(void) {
    controller_stop();
}

void search_run(void) {
    if (ir_get_state() != IR_STATE_START) {
        return fsm_transition(FSM_STATE_SAFE);
    }

    if (sensor_detected_line()) {
        return fsm_transition(FSM_STATE_SURVIVE);
    }

    if (!sensor_detected_obstacle_sides()) {
        return fsm_transition(FSM_STATE_AUTONOMOUS);
    }

    if (controller_get_state() == CONTROLLER_STATE_IDLE) {
        if (sensor_detected_obstacle_side_left()) {
            if (!controller_start(&search_sequence_r)) {
                return;
            }
        } else if (sensor_detected_obstacle_side_right()) {
            if (!controller_start(&search_sequence_l)) {
                return;
            }
        }
    }

    if (controller_get_state() == CONTROLLER_STATE_ACTIVE) {
        controller_step();
    }
}
