/**
 * @file radio.h
 * @brief Capture and normalize PWM pulse widths from radio channels using GPIO
 * interrupts.
 *
 * The driver supports multiple radio configurations and provides channel
 * readings and receiver connection status.
 *
 * @author Guilherme Nunes Trofino
 * @date 2026-05-03
 */

#pragma once

#include "esp_timer.h"
#include "pinout.h"
#include "pwm.h"

/**
 * @brief Radio channels.
 *
 * Logical channel identifiers, not physical receiver channel numbers.
 */
typedef enum radio_channel {
    RADIO_CHANNEL_FAILSAFE = 0, /**< Radio channel for failsafe. */
    RADIO_CHANNEL_BUTTON,       /**< Radio channel for button. */
    RADIO_CHANNEL_STEERING,     /**< Radio channel for steering. */
    RADIO_CHANNEL_THROTTLE,     /**< Radio channel for throttle. */
    RADIO_CHANNEL_5,            /**< Radio channel 5. */
    RADIO_CHANNEL_6,            /**< Radio channel 6. */
    NUMBER_OF_RADIO_CHANNELS    /**< Number of radio channels. */
} radio_channel_t;

/**
 * @brief Radio channel failsafe triggering condition.
 *
 * @note Only configurable for enabled channels.
 */
typedef enum radio_failsafe {
    /**
     * @brief Failsafe trigger disabled.
     */
    RADIO_FAILSAFE_DISABLE = 0,

    /**
     * @brief Failsafe triggered by timeout in microseconds.
     *
     * @note Timeout measured in microseconds since the last falling edge.
     */
    RADIO_FAILSAFE_TIMEOUT,

    /**
     * @brief Failsafe triggered by PWM pulse mismatch.
     *
     * @note All PWM pulses different than the expected trigger the failsafe.
     */
    RADIO_FAILSAFE_PWM_MISMATCH,

    /**
     * @brief Number of radio failsafe triggers.
     */
    NUMBER_OF_RADIO_FAILSAFES
} radio_failsafe_t;

/**
 * @brief Radio transmitter models.
 *
 * Each model is named following the convention: ``RADIO_MODEL_<BRAND>_<NAME>``.
 *
 * @note ``BRAND`` will be abbreviated with 2 letters and ``NAME`` will **not**
 * be abbreviated.
 */
typedef enum radio_model {
    /**
     * @brief Fly-Sky FS-GT2B
     *
     * - **Brand**: Fly-Sky
     *
     * - **Frequency**: 2.4 GHz
     *
     * - **Model**: FS-GT2B
     *
     * - **N. of Channels**: 3
     *
     * - **Protocol**: AFHDS
     */
    RADIO_MODEL_FS_GT2B = 0,

    /**
     * @brief Radio RadioMaster Pocket
     *
     * - **Brand**: RadioMaster
     *
     * - **Frequency**: 2.4 GHz
     *
     * - **Model**: Pocket
     *
     * - **N. of Channels**: 10
     *
     * - **Protocol**: FrSky X D16
     */
    RADIO_MODEL_RM_POCKET,

    /**
     * @brief Number of radio models.
     */
    NUMBER_OF_RADIO_MODELS,
} radio_model_t;

/**
 * @brief Radio connection status.
 *
 * Connection status is determined using the failsafe conditions configured for
 * the current radio model.
 */
typedef enum radio_status {
    /**
     * @brief Radio disconnected due to failsafe condition.
     *
     * @note Disconnection happens when the first failsafe condition is
     * triggered by an enabled channel.
     */
    RADIO_STATUS_DISCONNECTED = 0,

    /**
     * @brief Radio connected with no failsafe conditions.
     *
     * @note All enabled channels must not meet their respective failsafe
     * conditions.
     */
    RADIO_STATUS_CONNECTED,

    /**
     * @brief Number of radio status.
     */
    NUMBER_OF_RADIO_STATUS
} radio_status_t;

/**
 * @brief Radio channel failsafe configuration.
 */
typedef struct radio_failsafe_config {
    /**
     * @brief Triggering condition.
     */
    radio_failsafe_t trigger;

    /**
     * @brief Channel expected PWM pulse in microseconds under normal operation.
     *
     * A failsafe condition is triggered when the last measured PWM pulse
     * differs from the configured expected value.
     *
     * @note **Required** when trigger is ``RADIO_FAILSAFE_PWM_MISMATCH``.
     */
    pwm_norm_t pwm_expected;

    /**
     * @brief Channel timeout in microseconds.
     *
     * A failsafe condition is triggered when the elapsed time since the last
     * valid PWM pulse exceeds the configured timeout.
     *
     * @note **Required** when trigger is ``RADIO_FAILSAFE_TIMEOUT``.
     */
    int64_t timeout_us;
} radio_failsafe_config_t;

/**
 * @brief Radio channel configuration and runtime measurements.
 *
 * Contains the hardware configuration, failsafe settings and latest
 * measurements of a radio channel.
 */
typedef struct radio_channel_config {
    /**
     * @brief Channel enable flag.
     *
     * Disabled channels do not initialized GPIO interrupts.
     *
     * @note Disabled channels return ``PWM_NEUTRAL_US`` when read.
     */
    bool enabled;

    /**
     * @brief Channel name.
     *
     * Human-readable, null-terminated name of the channel.
     */
    const char *name;

    /**
     * @brief GPIO pin associated with the channel.
     */
    pin_t pin;

    /**
     * @brief Channel failsafe configuration.
     */
    radio_failsafe_config_t failsafe;

    /**
     * @brief Default normalized PWM pulse width in microseconds.
     *
     * @note Value returned while the radio status is
     * ``RADIO_STATUS_DISCONNECTED``.
     */
    const pwm_norm_t pwm_default;

    /**
     * @brief Latest normalized PWM pulse width in microseconds.
     */
    volatile pwm_norm_t pwm;

    /**
     * @brief Timestamp of the most recent observed rising edge.
     *
     * Expressed in microseconds since ESP Timer initialization.
     */
    volatile int64_t rise_time_us;

    /**
     * @brief Timestamp of the last PWM pulse.
     *
     * Expressed in microseconds since ESP Timer initialization.
     */
    volatile int64_t last_time_us;
} radio_channel_config_t;

/**
 * @brief Radio model configuration.
 */
typedef struct radio_model_config {
    /**
     * @brief Radio model readable name.
     *
     * Human-readable, null-terminated name of the radio.
     */
    const char *name;

    /**
     * @brief Radio model channels configurations.
     */
    radio_channel_config_t channels[NUMBER_OF_RADIO_CHANNELS];
} radio_model_config_t;

/**
 * @brief Radio handler.
 */
typedef struct radio_handler {
    /**
     * @brief Timer handler for status polling.
     */
    esp_timer_handle_t timer_handle;

    /**
     * @brief Current radio model.
     */
    radio_model_t model;

    /**
     * @brief All radio models configurations.
     */
    radio_model_config_t models[NUMBER_OF_RADIO_MODELS];

    /**
     * @brief Current radio model connection status.
     */
    radio_status_t status;

    /**
     * @brief All radio status readable names.
     *
     * Array of human-readable, null-terminated names of the radio status.
     */
    const char *status_names[NUMBER_OF_RADIO_STATUS];
} radio_handler_t;

/**
 * @def RADIO_DEFAULT_TIMEOUT_US
 * @brief Radio disconnection timeout interval in microseconds.
 *
 * The timeout must account for the receiver's transmission period and the
 * acceptable number of missed frames.
 *
 * **Default Value:** 20000 us (20 ms)
 */
#define RADIO_DEFAULT_TIMEOUT_US 20000

/**
 * @brief Deinitialize radio channels GPIOs, interrupts and timer.
 */
void radio_deinit(void);

/**
 * @brief Returns the radio channel name.
 *
 * @return Human-readable null-terminated string representing the channel name.
 * @retval NULL If channel is disabled or invalid.
 */
const char *radio_get_channel_name(radio_channel_t channel);

/**
 * @brief Returns the current radio model.
 *
 * @return Current radio model.
 */
radio_model_t radio_get_model(void);

/**
 * @brief Returns the current radio model name.
 *
 * @return Human-readable null-terminated string representing the radio name.
 */
const char *radio_get_model_name(void);

/**
 * @brief Return the radio status.
 *
 * @return Current radio status.
 */
radio_status_t radio_get_status(void);

/**
 * @brief Returns the radio status name.
 *
 * @return Human-readable null-terminated string representing the status name.
 */
const char *radio_get_status_name(void);

/**
 * @brief Initialize radio channels GPIOs, interrupts and timers.
 *
 * @note After initialization, channels start with their
 * ``radio_channel_config.pwm_default`` until PWM pulses are received.
 */
void radio_init(void);

/**
 * @brief Read the latest normalized PWM pulse width from a channel.
 *
 * Returns the ``radio_channel_config.pwm_default`` when the radio status is
 * ``RADIO_STATUS_DISCONNECTED``.
 *
 * @note Disabled or invalid channels return ``PWM_NEUTRAL_US``.
 *
 * @param[in] channel Logical radio channel identifier.
 *
 * @return Normalized PWM pulse width in microseconds.
 */
pwm_norm_t radio_read_channel(radio_channel_t channel);

/**
 * @brief Read the latest normalized PWM pulse widths from all channels.
 *
 * Returns the ``radio_channel_config.pwm_default`` when the radio status is
 * ``RADIO_STATUS_DISCONNECTED``.
 *
 * @note Disabled or invalid channels return ``PWM_NEUTRAL_US``.
 *
 * @param[out] pwms Output buffer containing at least
 * ``NUMBER_OF_RADIO_CHANNELS`` elements.
 */
void radio_read_channels(pwm_norm_t *pwms);

/**
 * @brief Set current radio model.
 *
 * @param[in] model Desired radio model.
 *
 * @note If model is invalid, the current model is not changed.
 */
void radio_set_model(radio_model_t model);
