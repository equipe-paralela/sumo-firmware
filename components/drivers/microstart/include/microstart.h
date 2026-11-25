/**
 * @file microstart.h
 * @brief JSumo's Microstart modulo.
 *
 * @author Guilherme Nunes Trofino
 * @date 2026-11-17
 */

#pragma once

#include "pinout.h"

/**
 * @brief Microstart states.
 *
 * Once in the ``MICROSTART_STATE_STOP``, power reset is required to return to
 * the ``MICROSTART_STATE_STANDBY``.
 */
typedef enum microstart_state {
    /**
     * @brief Microstart waiting on commands.
     */
    MICROSTART_STATE_STANDBY = 0,

    /**
     * @brief Microstart received the run command.
     */
    MICROSTART_STATE_RUN,

    /**
     * @brief Microstart received the stop command .
     */
    MICROSTART_STATE_STOP,

    /**
     * @brief Number of microstart states.
     */
    NUMBER_OF_MICROSTART_STATES
} microstart_state_t;

/**
 * @brief Microstart device.
 */
typedef struct microstart {
    /**
     * @brief Human-readable, null-terminated name of the microstart.
     */
    const char *name;

    /**
     * @brief Microstart pin connection.
     */
    pin_t pin;

    /**
     * @brief Microstart current state.
     */
    volatile microstart_state_t state;

    /**
     * @brief Array of human-readable, null-terminated names of the microstart
     * states.
     */
    const char *state_names[NUMBER_OF_MICROSTART_STATES];
} microstart_t;

/**
 * @brief Returns the microstart name.
 *
 * @return Human-readable null-terminated string representing the name.
 */
const char *microstart_get_name(void);

/**
 * @brief Returns the microstart current state.
 *
 * @return Current microstart state.
 */
microstart_state_t microstart_get_state(void);

/**
 * @brief Returns the microstart current state name.
 *
 * @return Human-readable null-terminated string representing the state name.
 */
const char *microstart_get_state_name(void);

/**
 * @brief Initializes the microstart interruption.
 */
void microstart_init(void);
