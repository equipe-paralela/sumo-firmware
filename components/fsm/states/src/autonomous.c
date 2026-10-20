#include "autonomous.h"
#include "controller.h"
#include "fsm.h"
#include "ir.h"
#include "sensor.h"

static const controller_command_t autonomous_commands[] = {
    {
        .duration_us = 1000,
        .motion      = CONTROLLER_MOTION_TRANSLATION,
        .power       = +100
    },
};

static const controller_sequence_t autonomous_sequence = {
    .name     = "autonomous sequence",
    .commands = autonomous_commands,
    .length   = sizeof(autonomous_commands) / sizeof(autonomous_commands[0]),
    .repeat   = false,
};

static autonomous_handler_t autonomous = {
    .name         = "AUTONOMOUS_HANDLER",
    .movement     = &autonomous_sequence,
    .interval_us  = 1000000,
};

void autonomous_entry(void) {
    controller_start(autonomous.movement);

    autonomous.last_time_us = esp_timer_get_time();
}

void autonomous_exit(void) {
    controller_stop();
}

void autonomous_run(void) {
    if (ir_get_state() != IR_STATE_START) {
        return fsm_transition(FSM_STATE_SAFE);
    }

    if (sensor_detected_line()) {
        return fsm_transition(FSM_STATE_SURVIVE);
    }

    if (sensor_detected_obstacle_front()) {
        return fsm_transition(FSM_STATE_ATTACK);
    }

    if (sensor_detected_obstacle_sides()) {
        return fsm_transition(FSM_STATE_SEARCH);
    }

    const int64_t now_us = esp_timer_get_time();
    if ((now_us - autonomous.last_time_us) < autonomous.interval_us) {
        return;
    }

    if (controller_get_state() == CONTROLLER_STATE_IDLE) {
        if (!controller_start(autonomous.movement)) {
            return;
        }

        autonomous.last_time_us = now_us;
    }

    if (controller_get_state() == CONTROLLER_STATE_ACTIVE) {
        controller_step();
    }
}