#include "protocol_sony.h"

#define PROTOCOL_SONY_HEADER_MARK_US    2400U
#define PROTOCOL_SONY_HEADER_SPACE_US    600U
#define PROTOCOL_SONY_BIT0_MARK_US       600U
#define PROTOCOL_SONY_BIT1_MARK_US      1200U
#define PROTOCOL_SONY_BIT_SPACE_US       600U
#define PROTOCOL_SONY_TOLERANCE_US       200U

/**
 * @brief Evaluates if the measured value is target within given tolerances.
 *
 * @param[in] measured Measured value in us.
 * @param[in] expected Target measure in us.
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

static bool protocol_sony_decode(
    const protocol_t *protocol,
    const rmt_symbol_word_t *symbols,
    size_t num_symbols,
    protocol_command_t *command
) {
    if (command == NULL || protocol == NULL || symbols == NULL) {
        return false;
    }

    if (num_symbols != 13U && num_symbols != 16U && num_symbols != 21U) {
        return false;
    }

    const uint32_t level_mark   = symbols[0].level0;
    const uint32_t level_space  = symbols[0].level1;

    // header validation
    if (
        (level_mark == level_space) ||
        !in_range(
            symbols[0].duration0,
            protocol->header_mark_us,
            protocol->tolerance_us
        ) ||
        !in_range(
            symbols[0].duration1,
            protocol->header_space_us,
            protocol->tolerance_us
        )
    ) {
        return false;
    }

    const size_t bit_count  = num_symbols - 1U;
    uint32_t data           = 0U;


    // data decoding
    for (size_t bit = 0U; bit < bit_count; ++bit) {
        const rmt_symbol_word_t *symbol = &symbols[bit + 1U];

        if (symbol->level0 != level_mark || symbol->level1 != level_space) {
            return false;
        }

        if (
            ((bit + 1U) < bit_count) &&
            !in_range(
                symbol->duration1,
                protocol->bit_space_us,
                protocol->tolerance_us
            )
        ) {
            return false;
        }

        if (
            in_range(
                symbol->duration0,
                protocol->bit0_mark_us,
                protocol->tolerance_us
            )
        ) {

        } else if (
            in_range(
                symbol->duration0,
                protocol->bit1_mark_us,
                protocol->tolerance_us
            )
        ) {
            data |= (uint32_t) (1UL << bit);
        } else {
            return false;
        }
    }

    // command reconstruction
    command->bit_count  = (uint8_t) bit_count;
    command->command    = data & 0x7FU;
    command->extended   = 0U;
    command->protocol   = protocol->id;

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

/**
 * @brief All possible commands for each protocol.
 */
static const protocol_command_t sony_commands[NUMBER_OF_PROTOCOL_COMMANDS] = {
    [PROTOCOL_COMMAND_READY] = {
        .bit_count  = 12U,
        .address    = 0x01U,
        .command    = 0x00U,
        .extended   = 0U,
        .protocol   = PROTOCOL_SONY,
    },
    [PROTOCOL_COMMAND_START] = {
        .bit_count  = 12U,
        .address    = 0x01U,
        .command    = 0x01U,
        .extended   = 0U,
        .protocol   = PROTOCOL_SONY,
    },
    [PROTOCOL_COMMAND_STOP]  = {
        .bit_count  = 12U,
        .address    = 0x01U,
        .command    = 0x02U,
        .extended   = 0U,
        .protocol   = PROTOCOL_SONY,
    },
};

/**
 * @brief All possible protocol configurations.
 */
const protocol_t protocol_sony = {
    .id                 = PROTOCOL_SONY,
    .name               = "SONY",
    .header_mark_us     = PROTOCOL_SONY_HEADER_MARK_US,
    .header_space_us    = PROTOCOL_SONY_HEADER_SPACE_US,
    .bit0_mark_us       = PROTOCOL_SONY_BIT0_MARK_US,
    .bit1_mark_us       = PROTOCOL_SONY_BIT1_MARK_US,
    .bit_space_us       = PROTOCOL_SONY_BIT_SPACE_US,
    .tolerance_us       = PROTOCOL_SONY_TOLERANCE_US,
    .decode             = protocol_sony_decode,
    .commands           = sony_commands
};