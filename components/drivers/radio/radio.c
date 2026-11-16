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

static radio_handler_t radio_handler = {
    .model  = RADIO_MODEL_RM_POCKET,
    .models = {
        [RADIO_MODEL_FS_GT2B] = {
            .name       = "FlySky GT2B",
            .channels   = {
                [RADIO_CHANNEL_FAILSAFE] = {
                    .enabled    = false,
                },
                [RADIO_CHANNEL_BUTTON] = {
                    .enabled        = true,
                    .name           = "BUTTON",
                    .pin            = PIN_RC_CH3,
                    .pwm_default    = PWM_NEUTRAL_US,
                    .failsafe   = {
                        .trigger        = RADIO_FAILSAFE_DISABLE,
                    },
                },
                [RADIO_CHANNEL_STEERING] = {
                    .enabled        = true,
                    .name           = "STEERING",
                    .pin            = PIN_RC_CH1,
                    .pwm_default    = PWM_NEUTRAL_US,
                    .failsafe   = {
                        .trigger        = RADIO_FAILSAFE_TIMEOUT,
                        .timeout_us     = RADIO_DEFAULT_TIMEOUT_US,
                    },
                },
                [RADIO_CHANNEL_THROTTLE] = {
                    .enabled        = true,
                    .name           = "THROTTLE",
                    .pin            = PIN_RC_CH2,
                    .pwm_default    = PWM_NEUTRAL_US,
                    .failsafe   = {
                        .trigger        = RADIO_FAILSAFE_TIMEOUT,
                        .timeout_us     = RADIO_DEFAULT_TIMEOUT_US,
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
                    .enabled        = true,
                    .name           = "FAIL_SAFE",
                    .pin            = PIN_RC_CH1,
                    .pwm_default    = PWM_MINIMUM_US,
                    .failsafe   = {
                        .trigger        = RADIO_FAILSAFE_PWM_MISMATCH,
                        .pwm_expected   = PWM_MAXIMUM_US
                    },
                },
                [RADIO_CHANNEL_BUTTON] = {
                    .enabled        = true,
                    .name           = "BUTTON",
                    .pin            = PIN_RC_CH4,
                    .pwm_default    = PWM_NEUTRAL_US,
                    .failsafe   = {
                        .trigger        = RADIO_FAILSAFE_DISABLE,
                    },
                },
                [RADIO_CHANNEL_STEERING] = {
                    .enabled        = true,
                    .name           = "STEERING",
                    .pin            = PIN_RC_CH3,
                    .pwm_default    = PWM_NEUTRAL_US,
                    .failsafe   = {
                        .trigger        = RADIO_FAILSAFE_DISABLE,
                    },
                },
                [RADIO_CHANNEL_THROTTLE] = {
                    .enabled        = true,
                    .name           = "THROTTLE",
                    .pin            = PIN_RC_CH2,
                    .pwm_default    = PWM_NEUTRAL_US,
                    .failsafe   = {
                        .trigger        = RADIO_FAILSAFE_DISABLE
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
    .status         = RADIO_STATUS_DISCONNECTED,
    .status_names   = {
        [RADIO_STATUS_CONNECTED]    = "CONNECTED",
        [RADIO_STATUS_DISCONNECTED] = "DISCONNECTED",
    },
    .timer_handle   = NULL,
};

/**
 * @brief Returns the current radio model configuration reference.
 *
 * @return Current radio model configuration pointer.
 */
static radio_model_config_t *radio_get_model_config(void) {
    return &radio_handler.models[radio_get_model()];
}

/**
 * @brief Radio timer callback for status polling.
 */
static void radio_timer_callback(void* arg) {
    radio_model_config_t *radio = radio_get_model_config();

    bool failsafe_triggered = true;
    for (radio_channel_t i = 0; i < NUMBER_OF_RADIO_CHANNELS; i++) {
        radio_channel_config_t *channel = &radio->channels[i];

        if (!channel->enabled) {
            continue;
        }

        portDISABLE_INTERRUPTS();
        pwm_norm_t channel_pwm          = channel->pwm;
        int64_t channel_last_time_us    = channel->last_time_us;
        portENABLE_INTERRUPTS();

        switch (channel->failsafe.trigger) {
            case RADIO_FAILSAFE_DISABLE:
                failsafe_triggered = false;
                break;

            case RADIO_FAILSAFE_PWM_MISMATCH:
                failsafe_triggered = (
                    channel_pwm != channel->failsafe.pwm_expected
                );
                break;

            case RADIO_FAILSAFE_TIMEOUT:
                failsafe_triggered = (
                    esp_timer_get_time() - channel_last_time_us
                ) > channel->failsafe.timeout_us;
                break;

            default:
                failsafe_triggered = true;
                break;
        }

        if (failsafe_triggered) {
            break;
        }
    }

    radio_status_t new_status;
    if (failsafe_triggered) {
        new_status = RADIO_STATUS_DISCONNECTED;
    } else {
        new_status = RADIO_STATUS_CONNECTED;
    }
    radio_handler.status = new_status;
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
 * @note GPIO level is read with direct ESP32S3 register access providing faster
 * and ISR-safe input sampling:
 * - ``REG_READ(GPIO_IN_REG)`` for GPIOs 0 to 31
 * - ``REG_READ(GPIO_IN1_REG)`` for GPIOs 32 to 48
 */
static void IRAM_ATTR radio_isr(void *arg) {
    radio_channel_t channel = (radio_channel_t) (uintptr_t) arg;

    radio_model_config_t *radio = &radio_handler.models[radio_handler.model];

    uint32_t level;

    pin_t pin = radio->channels[channel].pin;
    if (pin < 32) {
        level = (REG_READ(GPIO_IN_REG)  >> (pin -  0U)) & 0x1U;
    } else {
        level = (REG_READ(GPIO_IN1_REG) >> (pin - 32U)) & 0x1U;
    }

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
    if (radio_handler.timer_handle != NULL) {
        ESP_ERROR_CHECK(esp_timer_stop(radio_handler.timer_handle));

        ESP_ERROR_CHECK(esp_timer_delete(radio_handler.timer_handle));

        radio_handler.timer_handle = NULL;
    }

    radio_model_config_t *radio = radio_get_model_config();

    for (radio_channel_t i = 0; i < NUMBER_OF_RADIO_CHANNELS; i++) {
        radio_channel_config_t *channel = &radio->channels[i];

        if (!channel->enabled) {
            continue;
        }

        assert(GPIO_IS_VALID_GPIO(channel->pin) == true);

        ESP_ERROR_CHECK(gpio_isr_handler_remove(channel->pin));

        ESP_ERROR_CHECK(gpio_reset_pin(channel->pin));

        LOG_I(
            "%s %s deinit on pin %02d.",
            radio_get_model_name(),
            radio_get_channel_name(i),
            channel->pin
        );
    }

    radio_handler.status = RADIO_STATUS_DISCONNECTED;
}

const char *radio_get_channel_name(radio_channel_t channel) {
    if ((unsigned) channel >= NUMBER_OF_RADIO_CHANNELS) {
        LOG_E("invalid channel for %d", channel);

        return NULL;
    };

    radio_model_config_t *radio = radio_get_model_config();

    if (!radio->channels[channel].enabled) {
        return NULL;
    }

    return radio->channels[channel].name;
}

radio_model_t radio_get_model(void) {
    return radio_handler.model;
}

const char *radio_get_model_name(void) {
    radio_model_config_t *radio = radio_get_model_config();

    return radio->name;
}

radio_status_t radio_get_status(void) {
    return radio_handler.status;
}

const char *radio_get_status_name(void) {
    return radio_handler.status_names[radio_handler.status];
}

void radio_init(void) {
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));

    radio_model_config_t *radio = radio_get_model_config();

    for (radio_channel_t i = 0; i < NUMBER_OF_RADIO_CHANNELS; i++) {
        radio_channel_config_t *channel = &radio->channels[i];

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

        channel->pwm            = channel->pwm_default;
        channel->rise_time_us   = 0;
        channel->last_time_us   = 0;

        LOG_I(
            "%s %s initialized on pin %02d.",
            radio_get_model_name(),
            radio_get_channel_name(i),
            channel->pin
        );
    }

    const esp_timer_create_args_t timer_args = {
        .name            = "RC status timer",
        .callback        = &radio_timer_callback,
        .dispatch_method = ESP_TIMER_TASK
    };
    ESP_ERROR_CHECK(
        esp_timer_create(&timer_args, &radio_handler.timer_handle)
    );
    ESP_ERROR_CHECK(
        esp_timer_start_periodic(
            radio_handler.timer_handle, RADIO_DEFAULT_TIMEOUT_US
        )
    );

    radio_handler.status = RADIO_STATUS_DISCONNECTED;
}

pwm_norm_t radio_read_channel(radio_channel_t channel) {
    if ((unsigned) channel >= NUMBER_OF_RADIO_CHANNELS) {
        LOG_E("invalid channel for %d", channel);

        return PWM_NEUTRAL_US;
    };

    radio_model_config_t *radio = radio_get_model_config();

    if (!radio->channels[channel].enabled) {
        return PWM_NEUTRAL_US;
    }

    pwm_norm_t pwm;
    switch (radio_handler.status) {
        case RADIO_STATUS_CONNECTED:
            portDISABLE_INTERRUPTS();
            pwm = radio->channels[channel].pwm;
            portENABLE_INTERRUPTS();
            break;

        case RADIO_STATUS_DISCONNECTED:
            pwm = radio->channels[channel].pwm_default;
            break;

        default:
            pwm = PWM_NEUTRAL_US;
            break;
    }

    return pwm;
}

void radio_read_channels(pwm_norm_t *pwms) {
    if (pwms == NULL) {
        LOG_E("Invalid output buffer");

        return;
    }

    radio_model_config_t *radio = radio_get_model_config();

    for (radio_channel_t i = 0; i < NUMBER_OF_RADIO_CHANNELS; i++) {
        radio_channel_config_t *channel = &radio->channels[i];

        if (!channel->enabled) {
            pwms[i] = PWM_NEUTRAL_US;

            continue;
        }

        switch (radio_handler.status) {
            case RADIO_STATUS_CONNECTED:
                portDISABLE_INTERRUPTS();
                pwms[i] = radio->channels[i].pwm;
                portENABLE_INTERRUPTS();
                break;

            case RADIO_STATUS_DISCONNECTED:
                pwms[i] = radio->channels[i].pwm_default;
                break;

            default:
                pwms[i] = PWM_NEUTRAL_US;
                break;
        }
    }
}

void radio_set_model(radio_model_t model) {
    if ((unsigned) model >= NUMBER_OF_RADIO_MODELS) {
        LOG_E("invalid model for %d", model);

        return;
    }

    if (radio_handler.model == model) {
        LOG_W("%s radio already selected", radio_get_model_name());

        return;
    }

    radio_deinit();

    radio_handler.model = model;

    radio_init();
}
