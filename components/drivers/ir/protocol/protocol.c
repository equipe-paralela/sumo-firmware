#include "protocol.h"
#include "protocol_sony.h"

/**
 * @brief All possible protocol configurations.
 */
static const protocol_t *const protocols[NUMBER_OF_PROTOCOL_NAMES] = {
    [PROTOCOL_SONY] = &protocol_sony,
};

const protocol_command_t *protocol_get_command(
    protocol_command_name_t command_name, protocol_name_t protocol_name
) {
    const protocol_t *protocol = protocol_get_protocol(protocol_name);

    if (
        protocol == NULL ||
        protocol->commands == NULL ||
        (unsigned) command_name >= NUMBER_OF_PROTOCOL_COMMANDS
    ) {
        return NULL;
    }

    const protocol_command_t *command = &protocol->commands[command_name];
    if (command->bit_count != 0U) {
        return command;
    }

    return NULL;
}

const protocol_t *protocol_get_protocol(protocol_name_t protocol_name) {
    if ((unsigned) protocol_name >= NUMBER_OF_PROTOCOL_NAMES) {
        // todo add message
        return NULL;
    }

    return protocols[protocol_name];
}

const char *protocol_get_protocol_name(protocol_name_t protocol_name) {
    const protocol_t *protocol = protocol_get_protocol(protocol_name);

    if (protocol != NULL) {
        return protocol->name;
    }

    return NULL;
}
