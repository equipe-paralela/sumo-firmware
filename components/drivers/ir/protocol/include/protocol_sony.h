/**
 * @file protocol_sony.h
 * @brief Sony SIRC protocol configuration and decode function.
 *
 * @author Guilherme Nunes Trofino
 * @date 2026-11-16
 */

#pragma once

#include "protocol.h"

/**
 * @brief Sony SIRC protocol configuration.
 *
 * ``protocol_config_t.id``:
 *
 * - **Default Value**: ``PROTOCOL_SONY``,
 *
 * ``protocol_config_t.name``:
 *
 * - **Default Value**: "SONY",
 *
 * ``protocol_config_t.tolerance_us``:
 *
 * - **Default Value**:  200 us
 *
 * ``protocol_config_t.header_mark_us``:
 *
 * - **Default Value**: 2400 us
 *
 * ``protocol_config_t.header_space_us``:
 *
 * - **Default Value**:  600 us
 *
 * ``protocol_config_t.bit0_mark_us``:
 *
 * - **Default Value**:  600 us
 *
 * ``protocol_config_t.bit0_space_us``:
 *
 * - **Default Value**:  600 us
 *
 * ``protocol_config_t.bit1_mark_us``:
 *
 * - **Default Value**: 1200 us
 *
 * ``protocol_config_t.bit1_space_us``:
 *
 * - **Default Value**:  600 us
 */
extern const protocol_config_t protocol_sony_config;

/**
 * @brief Sony SIRC protocol decode function.
 *
 * @param[in] symbols Array of RMT symbols.
 * @param[in] num_symbols Number of RMT symbols in the array.
 * @param[out] command Pointer to command where data will be stored.
 *
 * @return true if decoding succeeded, false otherwise.
 */
bool protocol_sony_decode(
    const rmt_symbol_word_t *symbols,
    size_t num_symbols,
    protocol_command_config_t *command
);
