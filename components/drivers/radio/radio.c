#include "assert.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "logging.h"
#include "portmacro.h"
#include "pinout.h"
#include "pwm.h"
#include "radio.h"
#include "soc/gpio_reg.h"

static volatile radio_handler_t radio_handler = {
    .model  = RADIO_MODEL_RM_POCKET,
    .models = {
        [RADIO_MODEL_FS_GT2B] = {
            .name       = "FlySky GT2B",
            .channels   = {
                [RADIO_CHANNEL_FAILSAFE] = {
                    .enabled    = false,
                },
                [RADIO_CHANNEL_BUTTON] = {
                    .enabled    = true,
                    .name       = "BUTTON",
                    .pin        = PIN_RC_CH3,
                    .failsafe   = {
                        .trigger    = RADIO_FAILSAFE_DISABLE,
                    },
                },
                [RADIO_CHANNEL_STEERING] = {
                    .enabled    = true,
                    .name       = "STEERING",
                    .pin        = PIN_RC_CH1,
                    .failsafe   = {
                        .trigger    = RADIO_FAILSAFE_TIMEOUT,
                        .timeout_us = RADIO_TIMEOUT_US,
                    },
                },
                [RADIO_CHANNEL_THROTTLE] = {
                    .enabled    = true,
                    .name       = "THROTTLE",
                    .pin        = PIN_RC_CH2,
                    .failsafe   = {
                        .trigger    = RADIO_FAILSAFE_TIMEOUT,
                        .timeout_us = RADIO_TIMEOUT_US,
                    },
                },
                [RADIO_CHANNEL_5] = {
                    .enabled    = false,
                },
                [RADIO_CHANNEL_6] = {
                    .enabled    = false,
                },
            },
        },
        [RADIO_MODEL_RM_POCKET] = {
            .name       = "RadioMaster Pocket",
            .channels   = {
                [RADIO_CHANNEL_FAILSAFE] = {
                    .enabled    = true,
                    .name       = "FAIL_SAFE",
                    .pin        = PIN_RC_CH1,
                    .failsafe   = {
                        .trigger    = RADIO_FAILSAFE_PWM_VALUE,
                        .pwm        = PWM_MINIMUM_US
                    },
                },
                [RADIO_CHANNEL_BUTTON] = {
                    .enabled    = true,
                    .name       = "BUTTON",
                    .pin        = PIN_RC_CH4,
                    .failsafe   = {
                        .trigger    = RADIO_FAILSAFE_DISABLE,
                    },
                },
                [RADIO_CHANNEL_STEERING] = {
                    .enabled    = true,
                    .name       = "STEERING",
                    .pin        = PIN_RC_CH3,
                    .failsafe   = {
                        .trigger    = RADIO_FAILSAFE_DISABLE,
                    },
                },
                [RADIO_CHANNEL_THROTTLE] = {
                    .enabled    = true,
                    .name       = "THROTTLE",
                    .pin        = PIN_RC_CH2,
                    .failsafe   = {
                        .trigger    = RADIO_FAILSAFE_DISABLE
                    },
                },
                [RADIO_CHANNEL_5] = {
                    .enabled    = false,
                },
                [RADIO_CHANNEL_6] = {
                    .enabled    = false,
                },
            },
        },
    },
    .status_names = {
        [RADIO_STATUS_CONNECTED]    = "CONNECTED",
        [RADIO_STATUS_DISCONNECTED] = "DISCONNECTED",
    }
};

/**
 * @brief Returns the current radio model configuration reference.
 *
 * @return Current radio model configuration pointer.
 */
static volatile radio_model_config_t *radio_get_model_config(void) {
    return &radio_handler.models[radio_get_model()];
}

/**
 * @brief GPIO Interrupt Service Routine for the radio PWM pulse capture.
 *
 * ISR attached to each radio channel pins and triggered on any edge.
 *
 * It measures the high-level pulse width in microseconds of a PWM signal
 * using edge timing.
 *
 * @param arg Radio channel encoded as void*.
 *
 * @note GPIO level is read with direct ESP32 register access
 * ``REG_READ(GPIO_IN_REG)`` providing faster and ISR-safe input sampling.
 */
static void IRAM_ATTR radio_isr(void *arg) {
    radio_channel_t channel = (radio_channel_t) (uintptr_t) arg;

    volatile radio_model_config_t *radio = radio_get_model_config();

    uint32_t level = (
        REG_READ(GPIO_IN1_REG) >> (radio->channels[channel].pin - 32)
    ) & 0x1;

    int64_t now = esp_timer_get_time();
    if (level) { // rising  edge
        radio->channels[channel].rise_time_us = now;
    } else {     // falling edge
        if (radio->channels[channel].rise_time_us != 0) {
            pwm_t pwm = (pwm_t) (now - radio->channels[channel].rise_time_us);

            radio->channels[channel].pwm = pwm_normalize(pwm);
            radio->channels[channel].last_time_us = now;
        }
    }
}

void radio_deinit(void) {
    volatile radio_model_config_t *radio = radio_get_model_config();

    for (radio_channel_t i = 0; i < NUMBER_OF_RADIO_CHANNELS; i++) {
        volatile radio_channel_config_t *channel = &radio->channels[i];

        if (!channel->enabled) {
            continue;
        }

        assert(GPIO_IS_VALID_GPIO(channel->pin) == true);

        ESP_ERROR_CHECK(gpio_isr_handler_remove(channel->pin));

        ESP_ERROR_CHECK(gpio_reset_pin(channel->pin));

        LOG_I(
            "%s %s deinit on pin %02d.",
            radio_get_name(),
            radio_get_channel_name(i),
            channel->pin
        );
    }

    radio_handler.status = RADIO_STATUS_DISCONNECTED;
}

const char *radio_get_channel_name(radio_channel_t channel) {
    if (channel >= NUMBER_OF_RADIO_CHANNELS) {
        LOG_E("invalid channel for %d", channel);

        return NULL;
    };

    volatile radio_model_config_t *radio = radio_get_model_config();

    if (!radio->channels[channel].enabled) {
        return NULL;
    }

    return radio->channels[channel].name;
}

radio_model_t radio_get_model(void) {
    return radio_handler.model;
}

const char *radio_get_name(void) {
    volatile radio_model_config_t *radio = radio_get_model_config();

    return radio->name;
}

radio_status_t radio_get_status(void) {
    volatile radio_model_config_t *radio = radio_get_model_config();

    int64_t now = esp_timer_get_time();

    bool channel_failsafe;
    for (radio_channel_t i = 0; i < NUMBER_OF_RADIO_CHANNELS; i++) {
        portDISABLE_INTERRUPTS();
        volatile radio_channel_config_t *channel = &radio->channels[i];
        portENABLE_INTERRUPTS();

        if (!channel->enabled) {
            continue;
        }

        switch (channel->failsafe.trigger) {
            case RADIO_FAILSAFE_DISABLE:
                channel_failsafe = false;
                break;

            case RADIO_FAILSAFE_PWM_VALUE:
                channel_failsafe = (
                    channel->pwm == channel->failsafe.pwm
                );
                break;

            case RADIO_FAILSAFE_TIMEOUT:
                channel_failsafe = (
                    now - channel->last_time_us
                ) > channel->failsafe.timeout_us;
                break;

            default:
                channel_failsafe = false;
                LOG_E(
                    "Fail-Safe triggering condition %d not recognized",
                    channel->failsafe.trigger
                );
                break;
        }

        if (channel_failsafe) {
            break;
        }
    }

    radio_status_t new_status;
    if (channel_failsafe) {
        new_status = RADIO_STATUS_DISCONNECTED;
    } else {
        new_status = RADIO_STATUS_CONNECTED;
    }

    if (new_status != radio_handler.status) {
        radio_handler.status = new_status;

        LOG_W("%s radio %s", radio_get_name(), radio_get_status_name());
    }

    return radio_handler.status;
}

const char *radio_get_status_name(void) {
    return radio_handler.status_names[radio_handler.status];
}

void radio_init(void) {
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));

    volatile radio_model_config_t *radio = radio_get_model_config();

    int64_t now = esp_timer_get_time();
    for (radio_channel_t i = 0; i < NUMBER_OF_RADIO_CHANNELS; i++) {
        volatile radio_channel_config_t *channel = &radio->channels[i];

        if (!channel->enabled) {
            continue;
        }

        assert(GPIO_IS_VALID_GPIO(channel->pin) == true);

        gpio_config_t pin_config = {
            .pin_bit_mask   = (1ULL << channel->pin),
            .mode           = GPIO_MODE_INPUT,
            .pull_up_en     = GPIO_PULLUP_DISABLE,
            .pull_down_en   = GPIO_PULLDOWN_DISABLE,
            .intr_type      = GPIO_INTR_ANYEDGE
        };
        ESP_ERROR_CHECK(gpio_config(&pin_config));

        ESP_ERROR_CHECK(
            gpio_isr_handler_add(
                channel->pin,
                radio_isr,
                (void *) (uintptr_t) i
            )
        );

        channel->pwm = (pwm_t) PWM_NEUTRAL_US;
        channel->rise_time_us = now;
        channel->last_time_us = now;

        LOG_I(
            "%s %s initialized on pin %02d.",
            radio_get_name(),
            radio_get_channel_name(i),
            channel->pin
        );
    }

    radio_handler.status = RADIO_STATUS_DISCONNECTED;
}

pwm_norm_t radio_read_channel(radio_channel_t channel) {
    if (channel >= NUMBER_OF_RADIO_CHANNELS) {
        LOG_E("invalid channel for %d", channel);

        return PWM_NEUTRAL_US;
    };

    volatile radio_model_config_t *radio = radio_get_model_config();

    pwm_norm_t pwm;
    if (!radio->channels[channel].enabled) {
        pwm = PWM_NEUTRAL_US;
    } else {
        portDISABLE_INTERRUPTS();
        pwm = radio->channels[channel].pwm;
        portENABLE_INTERRUPTS();
    }

    return pwm;
}

void radio_read_channels(pwm_norm_t *pwms) {
    volatile radio_model_config_t *radio = radio_get_model_config();

    for (radio_channel_t i = 0; i < NUMBER_OF_RADIO_CHANNELS; i++) {
        portDISABLE_INTERRUPTS();
        volatile radio_channel_config_t *channel = &radio->channels[i];
        portENABLE_INTERRUPTS();

        if (!channel->enabled) {
            pwms[i] = PWM_NEUTRAL_US;
        } else {
            portDISABLE_INTERRUPTS();
            pwms[i] = channel->pwm;
            portENABLE_INTERRUPTS();
        }
    }
}

void radio_set_model(radio_model_t model) {
    if (model >= NUMBER_OF_RADIO_MODELS) {
        LOG_E("invalid model for %d", model);

        return;
    };

    radio_deinit();

    radio_handler.model = model;

    radio_init();
}
