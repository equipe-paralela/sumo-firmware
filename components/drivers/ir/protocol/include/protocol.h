/**
 * @file protocol.h
 * @brief Infrared receiver protocols definition.
 *
 * @author Guilherme Nunes Trofino
 * @date 2026-11-17
 */

#pragma once

#include "driver/rmt_rx.h"
#include "driver/rmt_types.h"
#include <stdint.h>

/**
 * @brief Supported infrared protocols commands names.
 */
typedef enum protocol_command {
    /**
     * @brief Protocol command not defined.
     *
     * @note Only used in error reporting.
     */
    PROTOCOL_COMMAND_UNDEFINED = -1,

    /**
     * @brief Protocol command for setting infrared state to ready.
     */
    PROTOCOL_COMMAND_READY,

    /**
     * @brief Protocol command for setting infrared state to wait.
     */
    PROTOCOL_COMMAND_RESET,

    /**
     * @brief Protocol command for setting infrared state to start.
     */
    PROTOCOL_COMMAND_START,

    /**
     * @brief Protocol command for setting infrared state to stop.
     */
    PROTOCOL_COMMAND_STOP,

    /**
     * @brief Number of protocol commands names.
     */
    NUMBER_OF_PROTOCOL_COMMANDS
} protocol_command_t;

/**
 * @brief Supported infrared protocols names.
 */
typedef enum protocol {
    /**
     * @brief Protocol name not defined.
     *
     * @note Only used in error reporting.
     */
    PROTOCOL_UNDEFINED = -1,

    /**
     * @brief Sony SIRC protocol.
     *
     * @note Serial Infra-Red Control
     */
    PROTOCOL_SONY = 0,

    /**
     * @brief Number of protocol names.
     */
    NUMBER_OF_PROTOCOLS
} protocol_t;

/**
 * @brief Infrared protocol command.
 */
typedef struct protocol_command_config {
    /**
     * @brief Human-readable null-terminated command key.
     */
    const char *key;

    /**
     * @brief Human-readable null-terminated command name.
     */
    const char *name;

    /**
     * @brief Protocol name.
     */
    protocol_t protocol;

    /**
     * @brief Command payload in bits.
     */
    uint8_t bit_count;

    /**
     * @brief Extracted address bits.
     */
    uint32_t address;

    /**
     * @brief Extracted command bits.
     */
    uint32_t command;

    /**
     * @brief Extended payload bits.
     */
    uint32_t extended;
} protocol_command_config_t;

/**
 * @brief Infrared communication protocol configuration.
 *
 * @note Not all fields are required for every protocol. Defined only the
 * suitable fields.
 */
typedef struct protocol_config {
    /**
     * @brief Protocol identification.
     */
    protocol_t id;

    /**
     * @brief Human-readable null-terminated protocol name.
     */
    const char *name;

    /**
     * @brief Protocol pulse tolerance in microseconds.
     */
    uint16_t tolerance_us;

    /**
     * @brief Protocol header pulse high duration in microseconds.
     */
    uint16_t header_mark_us;

    /**
     * @brief Protocol header pulse low duration in microseconds.
     */
    uint16_t header_space_us;

    /**
     * @brief Protocol logic 0 pulse high duration in microseconds.
     */
    uint16_t bit0_mark_us;

    /**
     * @brief Protocol logic 0 pulse low duration in microseconds.
     */
    uint16_t bit0_space_us;

    /**
     * @brief Protocol logic 1 pulse high duration in microseconds.
     */
    uint16_t bit1_mark_us;

    /**
     * @brief Protocol logic 1 pulse low duration in microseconds.
     */
    uint16_t bit1_space_us;

    /**
     * @brief Protocol footer pulse high duration in microseconds.
     */
    uint16_t footer_mark_us;

    /**
     * @brief Protocol footer pulse low duration in microseconds.
     */
    uint16_t footer_space_us;

    /**
     * @brief Pointter to protocol commands.
     */
    const protocol_command_config_t *commands;
} protocol_config_t;

/**
 * @brief Function pointer interface for protocol decode.
 *
 * @param[in] symbols Array of RMT symbols.
 * @param[in] num_symbols Number of RMT symbols in the array.
 * @param[out] command Pointer to command where data will be stored.
 *
 * @return true if decoding succeeded, false otherwise.
 */
typedef bool (*protocol_decode_t) (
    const rmt_symbol_word_t *symbols,
    size_t num_symbols,
    protocol_command_config_t *command
);

/**
 * @brief Infrared communication protocol handler.
 */
typedef struct protocol_handler {
    /**
     * @brief Protocol configuration pointer.
     */
    const protocol_config_t *config;

    /**
     * @brief Protocol decode function pointer.
     */
    protocol_decode_t decode;
} protocol_handler_t;

/**
 * @brief Attempts to decode RMT symbols in the provided protocol.
 *
 * @note Initialize command pointer to NULL to avoid undefined garbage values.
 *
 * @param[in] protocol Protocol to be decoded.
 * @param[in] symbols RMT symbols received from infrared receiver.
 * @param[in] num_symbols Number of RMT symbols.
 * @param[out] command Decoded command if successful.
 *
 * @return true if decoding was successful and false otherwise
 */
bool protocol_decode(
    const protocol_t protocol,
    const rmt_symbol_word_t *symbols,
    size_t num_symbols,
    protocol_command_config_t *command
);

/**
 * @brief Return protocol configuration pointer.
 *
 * @param[in] protocol Protocol.
 *
 * @return Pointer to protocol configuration pointer.
 */
const protocol_config_t *protocol_get_protocol_config(protocol_t protocol);

/**
 * @brief Returns the protocol name.
 *
 * @param[in] protocol Infrared protocol name.
 *
 * @return Human-readable null-terminated string representing the name.
 */
const char *protocol_get_protocol_name(protocol_t protocol);

/**
 * @brief Return protocol command.
 *
 * @param[in] command Protocol command configuration pointer.
 *
 * @note Returns ``PROTOCOL_COMMAND_UNDEFINED`` if the command is undefined.
 *
 * @return Pointer protocol command.
 */
protocol_command_t protocol_get_command(protocol_command_config_t *command);

/**
 * @brief Return protocol command configuration.
 *
 * @param[in] protocol Infrared protocol name.
 * @param[in] command Infrared protocol command.
 *
 * @note Returns NULL if either the command or the protocol are undefined.
 *
 * @return Human-readable null-terminated string representing the name.
 */
const char *protocol_get_command_name(
    protocol_t protocol, protocol_command_t command
);
