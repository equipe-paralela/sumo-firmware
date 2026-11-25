#include "driver/rmt_types.h"
#include "driver/rmt_rx.h"
#include "esp_err.h"
#include "ir.h"
#include "logging.h"
#include "pinout.h"
#include "portmacro.h"
#include "protocol.h"

/**
 * @brief Updates infrared state based on to the received command.
 *
 * @param[in] command_received Pointer to a protocol command.
 */
static void ir_received(const protocol_command_t *command_received) {
    if (command_received == NULL) {
        return;
    }

    for (uint8_t i = 0; i < NUMBER_OF_PROTOCOL_COMMANDS; i++) {
        const protocol_command_t *command_expected = protocol_get_command(
            (protocol_command_name_t) i, command_received->protocol
        );

        if (
            (command_expected ==  NULL) ||
            (command_expected->bit_count != command_received->bit_count) ||
            (command_expected->address   != command_received->address) ||
            (command_expected->command   != command_received->command) ||
            (command_expected->extended  != command_received->extended)
        ) {
            continue;
        };
        ir_state_t current_state = ir_get_state();
        ir_state_t desired_state = (ir_state_t) i;

        switch (current_state) {
            case IR_STATE_READY:
                if (desired_state == IR_STATE_START) {
                    ir_set_state(desired_state);
                }
                break;

            case IR_STATE_START:
                if (desired_state == IR_STATE_STOP) {
                    ir_set_state(desired_state);
                }
                break;

            case IR_STATE_STOP:
                break;

            case IR_STATE_WAIT:
                if (desired_state == IR_STATE_READY) {
                    ir_set_state(desired_state);
                }
                break;

            default:
                break;
        }

        break;
    }
}

/**
 * @brief GPIO Interrupt Service Routine for Infrared RMT pulse capture.
 *
 * ISR attached to the RMT on received done callback. If the defined protocol
 * decodes the received event data the reconstructed command is then passed to
 * the @ref ir_received function for treatment.
 *
 * @param[in] rx_channel RMT receive configured channel.
 * @param[in] event_data Pointer to the contents of the triggered event.
 * @param[in] user_data Pointer to the @ref ir_t device defined on the file.
 */
static bool IRAM_ATTR ir_rmt_rx_callback(
    rmt_channel_handle_t rx_channel,
    const rmt_rx_done_event_data_t *event_data,
    void *user_data
) {
    ir_t *device = (ir_t *) user_data;
    if (device == NULL || event_data == NULL) {
        return false;
    }

    portDISABLE_INTERRUPTS();
    device->rx_num_symbols  = event_data->num_symbols;
    device->rx_done         = true;
    device->rx_armed        = false;
    portENABLE_INTERRUPTS();

    return false;
}

static ir_t ir = {
    .name       = "TL1838 IR",
    .callback   = ir_received,
    .active_low = false,
    .pin        = PIN_MICROSTART,
    .state      = IR_STATE_WAIT,
    .state_names = {
        [IR_STATE_READY] = "READY",
        [IR_STATE_START] = "START",
        [IR_STATE_STOP]  = "STOP",
        [IR_STATE_WAIT]  = "WAIT",
    }
};

static void ir_start_receive(void) {
    portDISABLE_INTERRUPTS();
    ir.rx_armed = true;
    portENABLE_INTERRUPTS();

    const rmt_receive_config_t ir_receive_config = {
        .signal_range_max_ns = IR_RMT_SIGNAL_MAX_NS,
        .signal_range_min_ns = IR_RMT_SIGNAL_MIN_NS,
    };

    esp_err_t error = rmt_receive(
        ir.rx_channel,
        ir.rx_buffer,
        sizeof(ir.rx_buffer),
        &ir_receive_config
    );

    if (error != ESP_OK) {
        portDISABLE_INTERRUPTS();
        ir.rx_armed = false;
        portENABLE_INTERRUPTS();
    }
}

const char *ir_get_name(void) {
    return ir.name;
};

ir_state_t ir_get_state(void) {
    ir_state_t state;

    portDISABLE_INTERRUPTS();
    state = ir.state;
    portENABLE_INTERRUPTS();

    return state;
};

const char *ir_get_state_name(void) {
    const ir_state_t state = ir_get_state();
    if ((unsigned)state >= NUMBER_OF_IR_STATES) {
        return "INVALID";
    }

    return ir.state_names[state];
};

void ir_init(void) {
    if (ir.rx_channel != NULL) {
        // invalid state
        return;
    }

    ir.protocol = protocol_get_protocol(PROTOCOL_SONY);
    if (ir.protocol == NULL || ir.protocol->decode == NULL) {
        // invalid protocol
        return;
    }

    rmt_rx_channel_config_t rmt_rx_config = {
        .gpio_num           = ir.pin,
        .clk_src            = RMT_CLK_SRC_DEFAULT,
        .resolution_hz      = IR_RMT_RX_CLOCK_RESOLUTION_HZ,
        .mem_block_symbols  = IR_RMT_RX_MEMORY_BLOCK_SYMBOL,
        .flags.invert_in    = ir.rmt_inverted_in,
        .flags.with_dma     = false,
    };
    ESP_ERROR_CHECK(rmt_new_rx_channel(&rmt_rx_config, &ir.rx_channel));

    rmt_rx_event_callbacks_t rmt_rx_callback = {
        .on_recv_done = ir_rmt_rx_callback
    };
    ESP_ERROR_CHECK(
        rmt_rx_register_event_callbacks(ir.rx_channel, &rmt_rx_callback, &ir)
    );

    ESP_ERROR_CHECK(rmt_enable(ir.rx_channel));

    ir_start_receive();

    ir.state = IR_STATE_WAIT;

    LOG_I("%s initialized to state %s", ir_get_name(), ir_get_state_name());
};

void ir_poll(void) {
    if (ir.rx_channel == NULL) {
        // invalid state
        return;
    }

    bool frame_ready;
    bool receiver_armed;
    size_t num_symbols = 0U;

    portDISABLE_INTERRUPTS();
    frame_ready = ir.rx_done;
    if (frame_ready) {
        num_symbols = ir.rx_num_symbols;
        ir.rx_done = false;
    }
    receiver_armed = ir.rx_armed;
    portENABLE_INTERRUPTS();

    if (frame_ready && num_symbols <= IR_RMT_RX_BUFFER_SYMBOLS) {
        protocol_command_t decoded_command = {0};

        if (
            ir.protocol->decode(
                ir.protocol,
                ir.rx_buffer,
                num_symbols,
                &decoded_command
            )
        ) {
            if (ir.callback != NULL) {
                ir.callback(&decoded_command);
            }
        }
    }

    if (!receiver_armed) {
        ir_start_receive();
    }
}

void ir_set_state(ir_state_t state) {
    // todo check valid state
    portDISABLE_INTERRUPTS();
    ir.state = state;
    portENABLE_INTERRUPTS();
};
