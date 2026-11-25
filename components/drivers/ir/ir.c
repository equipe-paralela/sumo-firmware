#include "driver/gpio.h"
#include "driver/rmt_rx.h"
#include "driver/rmt_types.h"
#include "esp_err.h"
#include "ir.h"
#include "logging.h"
#include "pinout.h"
#include "portmacro.h"
#include "protocol.h"

/**
 * @brief GPIO Interrupt Service Routine for Infrared RMT pulse capture.
 *
 * ISR attached to the RMT on received done callback.
 *
 * @param[in] rmt_channel RMT receive configured channel.
 * @param[in] event_data Pointer to the contents of the triggered event.
 * @param[in] user_data Pointer to the ``ir.rmt_rx_event_queue``.
 */
static bool IRAM_ATTR ir_rmt_rx_on_recv_done(
    rmt_channel_handle_t rmt_channel,
    const rmt_rx_done_event_data_t *event_data,
    void *user_data
) {
    if (event_data == NULL) {
        return false;
    }

    BaseType_t woken = pdFALSE;
    xQueueSendFromISR((QueueHandle_t) user_data, event_data, &woken);

    return woken == pdTRUE;
}

static ir_t ir = {
    .name               = "VS1838B",
    .protocol           = PROTOCOL_SONY,
#if   defined(CONFIG_MAINBOARD_V3)
    .pin                = PIN_IR,
#elif defined(CONFIG_MAINBOARD_V2)
    .pin                = PIN_IR,
#elif defined(CONFIG_MAINBOARD_V1)
    .pin                = PIN_SPARE,
#endif
    .pin_pullup         = GPIO_PULLUP_ENABLE,
    .rmt_rx_inverted_in = true,
    .state              = IR_STATE_STANDBY,
    .state_names = {
        [IR_STATE_READY]    = "READY",
        [IR_STATE_STANDBY]  = "STANDBY",
        [IR_STATE_RUN]    = "START",
        [IR_STATE_STOP]     = "STOP",
    }
};

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
    if ((unsigned) state >= NUMBER_OF_IR_STATES) {
        return NULL;
    }

    return ir.state_names[state];
};

void ir_init(void) {
    esp_err_t error = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);

    if (error != ESP_OK && error != ESP_ERR_INVALID_STATE) {
        LOG_E("initialization failed with %s", esp_err_to_name(error));

        return;
    }

    assert(GPIO_IS_VALID_GPIO(ir.pin) == true);

    gpio_config_t pin_config = {
        .pin_bit_mask   = (1ULL << ir.pin),
        .mode           = GPIO_MODE_INPUT,
        .pull_down_en   = GPIO_PULLDOWN_DISABLE,
        .pull_up_en     = ir.pin_pullup,
        .intr_type      = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&pin_config));

    rmt_rx_channel_config_t rmt_rx_channel_config = {
        .gpio_num           = ir.pin,
        .clk_src            = RMT_CLK_SRC_DEFAULT,
        .resolution_hz      = IR_RMT_RX_CLOCK_RESOLUTION_HZ,
        .mem_block_symbols  = IR_RMT_RX_MEMORY_BLOCK_SYMBOL,
        .flags.invert_in    = ir.rmt_rx_inverted_in,
        .flags.with_dma     = false,
    };
    ESP_ERROR_CHECK(
        rmt_new_rx_channel(&rmt_rx_channel_config, &ir.rmt_rx_channel)
    );

    ir.rmt_rx_event_queue = xQueueCreate(4, sizeof(rmt_rx_done_event_data_t));

    rmt_rx_event_callbacks_t rmt_rx_event_callback = {
        .on_recv_done = ir_rmt_rx_on_recv_done
    };
    ESP_ERROR_CHECK(
        rmt_rx_register_event_callbacks(
            ir.rmt_rx_channel, &rmt_rx_event_callback, ir.rmt_rx_event_queue
        )
    );

    ESP_ERROR_CHECK(rmt_enable(ir.rmt_rx_channel));

    ir.rmt_receive_config = (rmt_receive_config_t) {
        .signal_range_max_ns = IR_RMT_SIGNAL_MAX_NS,
        .signal_range_min_ns = IR_RMT_SIGNAL_MIN_NS,
    };

    const rmt_receive_config_t rmt_receive_config = ir.rmt_receive_config;

    ESP_ERROR_CHECK(
        rmt_receive(
            ir.rmt_rx_channel,
            ir.rmt_rx_buffer,
            sizeof(ir.rmt_rx_buffer),
            &rmt_receive_config
        )
    );

    ir.state = IR_STATE_STANDBY;

    LOG_I("%s initialized to state %s", ir_get_name(), ir_get_state_name());
};

void ir_poll(void) {
    if (xQueueReceive(ir.rmt_rx_event_queue, &ir.rmt_rx_event, 0) == pdTRUE) {
        protocol_command_config_t command_config = {0};

        protocol_decode(
            ir.protocol,
            ir.rmt_rx_event.received_symbols,
            ir.rmt_rx_event.num_symbols,
            &command_config
        );

        protocol_command_t command = protocol_get_command(&command_config);

        ir_state_t current_state = ir_get_state();

        switch (command) {
            case PROTOCOL_COMMAND_READY:
                if (current_state == IR_STATE_STANDBY) {
                    ir_set_state(IR_STATE_READY);
                }
                break;

            case PROTOCOL_COMMAND_RESET:
                if (current_state == IR_STATE_STOP) {
                    ir_set_state(IR_STATE_STANDBY);
                }
                break;

            case PROTOCOL_COMMAND_START:
                if (current_state == IR_STATE_READY) {
                    ir_set_state(IR_STATE_RUN);
                }
                break;

            case PROTOCOL_COMMAND_STOP:
                if (current_state == IR_STATE_RUN) {
                    ir_set_state(IR_STATE_STOP);
                }
                break;

            default:
                break;
        }

        const rmt_receive_config_t rmt_receive_config = ir.rmt_receive_config;

        ESP_ERROR_CHECK(
            rmt_receive(
                ir.rmt_rx_channel,
                ir.rmt_rx_buffer,
                sizeof(ir.rmt_rx_buffer),
                &rmt_receive_config
            )
        );
    }
}

void ir_set_state(ir_state_t state) {
    if ((unsigned) state >= NUMBER_OF_IR_STATES) {
        LOG_E("invalid state for %02d", state);
    }

    portDISABLE_INTERRUPTS();
    ir.state = state;
    portENABLE_INTERRUPTS();

    LOG_I("state set to %s", ir_get_state_name());
};
