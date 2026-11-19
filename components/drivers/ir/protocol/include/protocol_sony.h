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
 * @brief
 */
extern const protocol_config_t protocol_sony_config;

/**
 * @brief
 */
bool protocol_sony_decode(
    const protocol_config_t *protocol,
    const rmt_symbol_word_t *symbols,
    size_t num_symbols,
    protocol_command_config_t *command
);
