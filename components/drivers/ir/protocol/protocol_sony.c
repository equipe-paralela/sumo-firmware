#include "protocol_sony.h"

/**
 * @brief Sony SIRC protocol commands.
 */
static const protocol_command_config_t protocol_sony_commands[
    NUMBER_OF_PROTOCOL_COMMANDS
] = {
    [PROTOCOL_COMMAND_READY] = {
        .key        = "NUMBER_1",
        .name       = "COMMAND_READY",
        .bit_count  = 12U,
        .address    = 0x01U,
        .command    = 0x00U,
        .extended   = 0x00U,
        .protocol   = PROTOCOL_SONY,
    },
    [PROTOCOL_COMMAND_RESET] = {
        .key        = "POWER_ON",
        .name       = "COMMAND_RESET",
        .bit_count  = 12U,
        .address    = 0x01U,
        .command    = 0x15U,
        .extended   = 0x00U,
        .protocol   = PROTOCOL_SONY,
    },
    [PROTOCOL_COMMAND_START] = {
        .key        = "NUMBER_2",
        .name       = "COMMAND_START",
        .bit_count  = 12U,
        .address    = 0x01U,
        .command    = 0x01U,
        .extended   = 0x00U,
        .protocol   = PROTOCOL_SONY,
    },
    [PROTOCOL_COMMAND_STOP]  = {
        .key        = "NUMBER_3",
        .name       = "COMMAND_STOP",
        .bit_count  = 12U,
        .address    = 0x01U,
        .command    = 0x02U,
        .extended   = 0x00U,
        .protocol   = PROTOCOL_SONY,
    },
};

const protocol_config_t protocol_sony_config = {
    .id                 = PROTOCOL_SONY,
    .name               = "SONY",
    .tolerance_us       =  200U,
    .header_mark_us     = 2400U,
    .header_space_us    =  600U,
    .bit0_mark_us       =  600U,
    .bit0_space_us      =  600U,
    .bit1_mark_us       = 1200U,
    .bit1_space_us      =  600U,
    .commands           = protocol_sony_commands
};

/**
 * @brief Evaluates if the measured value is target within given tolerances.
 *
 * @param[in] measured Measured value in us.
 * @param[in] expected Expected value in us.
 * @param[in] tolerance Measurement tolerance in us.
 *
 * @return true if value reached target within tolerances, false otherwise.
 */
static bool in_range(uint32_t measured, uint32_t expected, uint32_t tolerance) {
    return (
        (measured > (expected - tolerance)) &&
        (measured < (expected + tolerance))
    );
};

bool protocol_sony_decode(
    const rmt_symbol_word_t *symbols,
    size_t num_symbols,
    protocol_command_config_t *command
) {
    if (command == NULL || symbols == NULL) {
        return false;
    }

    if (num_symbols != 13U && num_symbols != 16U && num_symbols != 21U) {
        return false;
    }

    const uint32_t header_mark   = symbols[0].level0;
    const uint32_t header_space  = symbols[0].level1;

    // header validation
    if (
        (header_mark == header_space) ||
        !in_range(
            symbols[0].duration0,
            protocol_sony_config.header_mark_us,
            protocol_sony_config.tolerance_us
        ) ||
        !in_range(
            symbols[0].duration1,
            protocol_sony_config.header_space_us,
            protocol_sony_config.tolerance_us
        )
    ) {
        return false;
    }

    const size_t bit_count  = num_symbols - 1U;
    uint32_t data           = 0U;

    // data decoding
    for (size_t bit = 0U; bit < bit_count; bit++) {
        const rmt_symbol_word_t *symbol = &symbols[bit + 1U];

        if (symbol->level0 != header_mark || symbol->level1 != header_space) {
            return false;
        }

        if (
            ((bit + 1U) < bit_count) &&
            !in_range(
                symbol->duration1,
                protocol_sony_config.bit0_space_us,
                protocol_sony_config.tolerance_us
            )
        ) {
            return false;
        }

        if (
            in_range(
                symbol->duration0,
                protocol_sony_config.bit0_mark_us,
                protocol_sony_config.tolerance_us
            )
        ) {
            // set data[bit] to 0
            data &= (uint32_t) ~(1UL << bit);
        } else if (
            in_range(
                symbol->duration0,
                protocol_sony_config.bit1_mark_us,
                protocol_sony_config.tolerance_us
            )
        ) {
            // set data[bit] to 1
            data |= (uint32_t)  (1UL << bit);
        } else {
            return false;
        }
    }

    // command reconstruction
    command->bit_count  = (uint8_t) bit_count;
    command->command    = data & 0x7FU;
    command->extended   = 0U;
    command->protocol   = protocol_sony_config.id;

    switch (bit_count) {
        case 12U:
            command->address    = (data >> 7U)  & 0x1FU;
            break;

        case 15U:
            command->address    = (data >> 7U)  & 0xFFU;
            break;

        case 20U:
            command->address    = (data >> 7U)  & 0x1FU;
            command->extended   = (data >> 12U) & 0xFFU;
            break;

        default:
            return false;
    }

    return true;
}
