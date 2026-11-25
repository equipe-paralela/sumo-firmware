#include "attack.h"
#include "controller.h"
#include "fsm.h"
#include "ir.h"
#include "microstart.h"
#include "sensor.h"

static const controller_command_t attack_commands[] = {
    {
        .duration_us = 100,
        .motion      = CONTROLLER_MOTION_TRANSLATION,
        .power       = +25
    },
};

static const controller_sequence_t attack_sequence = {
    .name     = "attack sequence",
    .commands = attack_commands,
    .length   = sizeof(attack_commands) / sizeof(attack_commands[0]),
    .repeat   = true,
};

void attack_entry(void) {
    controller_start(&attack_sequence);
}

void attack_exit(void) {
    controller_stop();
}

void attack_run(void) {
    if (
#if   defined(CONFIG_MAINBOARD_V3)
        (microstart_get_state() != MICROSTART_STATE_START) ||
        (ir_get_state()         != IR_STATE_START)
#elif defined(CONFIG_MAINBOARD_V2)
        (ir_get_state()         != IR_STATE_START)
#elif defined(CONFIG_MAINBOARD_V1)
        (microstart_get_state() != MICROSTART_STATE_START)
#endif
    ) {
        return fsm_transition(FSM_STATE_SAFE);
    }

    if (sensor_detected_line()) {
        return fsm_transition(FSM_STATE_SURVIVE);
    }

    if (!sensor_detected_obstacle_front()) {
        if (sensor_detected_obstacle_sides()) {
            return fsm_transition(FSM_STATE_SEARCH);
        }

        return fsm_transition(FSM_STATE_AUTONOMOUS);
    }

    if (controller_get_state() == CONTROLLER_STATE_IDLE) {
        if (!controller_start(&attack_sequence)) {
            return;
        }
    }

    if (controller_get_state() == CONTROLLER_STATE_ACTIVE) {
        controller_step();
    }
}
