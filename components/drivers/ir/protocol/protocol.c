#include "logging.h"
#include "protocol.h"
#include "protocol_sony.h"

/**
 * @brief All available protocols configurations.
 */
static const protocol_handler_t protocols[NUMBER_OF_PROTOCOLS] = {
    [PROTOCOL_SONY] = {
        .config = &protocol_sony_config,
        .decode =  protocol_sony_decode
    },
};

bool protocol_decode(
    const protocol_t protocol,
    const rmt_symbol_word_t *symbols,
    size_t num_symbols,
    protocol_command_config_t *command
) {
    return protocols[protocol].decode(
        symbols,
        num_symbols,
        command
    );
}

const protocol_config_t *protocol_get_protocol_config(protocol_t protocol) {
    if ((unsigned) protocol >= NUMBER_OF_PROTOCOLS) {
        LOG_E("invalid protocol for %d", protocol);

        return NULL;
    }

    return protocols[protocol].config;
}

protocol_command_t protocol_get_command(protocol_command_config_t *command) {
    if (command == NULL) {
        LOG_E("invalid command");

        return PROTOCOL_COMMAND_UNDEFINED;
    }

    const protocol_config_t *protocol = protocol_get_protocol_config(
        command->protocol
    );
    if (protocol == NULL || protocol->commands == NULL) {
        return PROTOCOL_COMMAND_UNDEFINED;
    }

    for (protocol_command_t i = 0; i < NUMBER_OF_PROTOCOL_COMMANDS; i++) {
        const protocol_command_config_t *candidate = &protocol->commands[i];

        if (
            candidate->bit_count  == command->bit_count &&
            candidate->address    == command->address   &&
            candidate->command    == command->command   &&
            candidate->extended   == command->extended
        ) {
            LOG_V("matching protocol command name is %s", candidate->name);

            return ((protocol_command_t) i);
        }
    }

    LOG_E(
        "no matching command with %02d bits, address 0x%02X, command 0x%02X"
        ", and extended 0x%02X",
        command->bit_count,
        command->address,
        command->command,
        command->extended
    );

    return PROTOCOL_COMMAND_UNDEFINED;
}

const char *protocol_get_command_name(
    protocol_t protocol, protocol_command_t command
) {
    if ((unsigned) protocol >= NUMBER_OF_PROTOCOLS) {
        LOG_E("invalid protocol for %d", protocol);

        return NULL;
    }

    if ((unsigned) command >= NUMBER_OF_PROTOCOL_COMMANDS) {
        LOG_E("invalid protocol command for %d", command);

        return NULL;
    }

    return protocols[protocol].config->commands[command].name;
}
