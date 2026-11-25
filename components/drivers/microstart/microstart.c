#include "assert.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "microstart.h"
#include "logging.h"
#include "portmacro.h"
#include "pinout.h"
#include "soc/gpio_reg.h"

static microstart_t microstart = {
    .name           = "Microstart",
#if   defined(CONFIG_MAINBOARD_V3)
    .pin            = PIN_MICROSTART,
#elif defined(CONFIG_MAINBOARD_V2)
    .pin            = PIN_IO10,
#elif defined(CONFIG_MAINBOARD_V1)
    .pin            = PIN_MICROSTART,
#endif
    .state          = MICROSTART_STATE_STANDBY,
    .state_names    = {
        [MICROSTART_STATE_STANDBY] = "STANDBY",
        [MICROSTART_STATE_START]   = "START",
        [MICROSTART_STATE_STOP]    = "STOP",
    },
};

/**
 * @brief GPIO Interrupt Service Routine for the microstart signal capture.
 *
 * When in ``MICROSTART_STATE_STOP``, only system power reset will reinitialize
 * the microstart.
 *
 * @param arg Microstart pin as void *.
 *
 * @note GPIO level is read with direct ESP32S3 register access providing faster
 * and ISR-safe input sampling:
 * - ``REG_READ(GPIO_IN_REG)`` for GPIOs 0 to 31
 * - ``REG_READ(GPIO_IN1_REG)`` for GPIOs 32 to 48
 */
static void IRAM_ATTR microstart_isr(void *arg) {
    pin_t pin = (pin_t) (uintptr_t) arg;

    uint32_t signal_start;
    if (pin < 32) {
        signal_start = (REG_READ(GPIO_IN_REG)  >> (pin -  0U)) & 0x1U;
    } else {
        signal_start = (REG_READ(GPIO_IN1_REG) >> (pin - 32U)) & 0x1U;
    }

    switch (microstart.state) {
        case MICROSTART_STATE_STANDBY:
            if ( signal_start)   { microstart.state = MICROSTART_STATE_START; };
            break;

        case MICROSTART_STATE_START:
            if (!signal_start)   { microstart.state = MICROSTART_STATE_STOP; };
            break;

        case MICROSTART_STATE_STOP:
            break;

        default:
            break;
    }
};

const char *microstart_get_name(void) {
    return microstart.name;
};

microstart_state_t microstart_get_state(void) {
    return microstart.state;
};

const char *microstart_get_state_name(void) {
    return microstart.state_names[microstart.state];
};

void microstart_init(void) {
    esp_err_t error = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);

    if (error != ESP_OK && error != ESP_ERR_INVALID_STATE) {
        LOG_E("initialization failed with %s", esp_err_to_name(error));

        return;
    }

    assert(GPIO_IS_VALID_GPIO(microstart.pin) == true);

    gpio_config_t pin_config = {
        .pin_bit_mask   = (1ULL << microstart.pin),
        .mode           = GPIO_MODE_INPUT,
        .pull_down_en   = GPIO_PULLDOWN_DISABLE,
        .pull_up_en     = GPIO_PULLUP_DISABLE,
        .intr_type      = GPIO_INTR_ANYEDGE
    };
    ESP_ERROR_CHECK(gpio_config(&pin_config));

    ESP_ERROR_CHECK(
        gpio_isr_handler_add(
            microstart.pin,
            microstart_isr,
            (void *) (uintptr_t) microstart.pin
        )
    );

    microstart.state = MICROSTART_STATE_STANDBY;
    LOG_I(
        "%s initialized to state %s",
        microstart_get_name(),
        microstart_get_state_name()
    );
};
