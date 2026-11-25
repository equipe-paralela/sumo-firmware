/**
 * @file ir.h
 * @brief Infrared receiver device.
 *
 * @author Guilherme Nunes Trofino
 * @date 2026-11-17
 */

#pragma once

#include "driver/rmt_rx.h"
#include "freertos/FreeRTOS.h"
#include "ir_constants.h"
#include "pinout.h"
#include "protocol.h"

/**
 * @brief Infrared states.
 *
 * @note The @ref protocol_command_name command names must match the
 * defined @ref ir_state infrared states sequence.
 */
typedef enum ir_state {
    IR_STATE_READY= 0,  /**< Infrared receiver received the ready command. */
    IR_STATE_START,     /**< Infrared receiver received the start command. */
    IR_STATE_STOP,      /**< Infrared receiver received the stop command . */
    IR_STATE_WAIT,      /**< Infrared receiver waiting commands. */
    NUMBER_OF_IR_STATES /**< Number of infrared states. */
} ir_state_t;

/**
 * @brief Infrared receiver device.
 */
typedef struct ir {
    /**
     * @brief Human-readable null-terminated infrared name.
     */
    const char *name;

    /**
     * @brief Infrared state.
     */
    ir_state_t state;

    /**
     * @brief Human-readable null-terminated infrared states names.
     */
    const char *state_names[NUMBER_OF_IR_STATES];

    /**
     * @brief
     */
    QueueHandle_t queue;

    /**
     * @brief Infrared decoding protocol.
     */
    protocol_t protocol;

    /**
     * @brief Infrared pin connection.
     */
    pin_t pin;

    /**
     * @brief Infrared pin pullup.
     */
    bool pin_pullup;

    /**
     * @brief Infrared input inversion flag.
     */
    bool rmt_inverted_in;

    /**
     * @brief Infrared RMT RX channel.
     */
    rmt_channel_handle_t rmt_rx_channel;

    /**
     * @brief
     */
    rmt_receive_config_t rmt_receive_config;

    /**
     * @brief
     */
    rmt_rx_done_event_data_t rmt_rx_done_event_data;

    /**
     * @brief Infrared RMT RX buffer.
     */
    rmt_symbol_word_t rmt_rx_buffer[IR_RMT_RX_BUFFER_SYMBOLS];
} ir_t;

/**
 * @brief Returns the infrared name.
 *
 * @return Human-readable null-terminated string representing the name.
 */
const char *ir_get_name(void);

/**
 * @brief Returns the infrared current state.
 *
 * @return Current infrared state.
 */
ir_state_t ir_get_state(void);

/**
 * @brief Returns the infrared current state name.
 *
 * @return Human-readable null-terminated string representing the state name.
 */
const char *ir_get_state_name(void);

/**
 * @brief Initializes the infrared RMT RX channel.
 */
void ir_init(void);

/**
 * @brief Polls the infrared receiver state.
 *
 * Must be called continuously.
 */
void ir_poll(void);

/**
 * @brief Updates the infrared current state.
 *
 * @param[in] state Desired infrared state.
 */
void ir_set_state(ir_state_t state);
