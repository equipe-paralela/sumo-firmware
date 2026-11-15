/**
 * @file radio.h
 * @brief Capture and normalize PWM pulse widths channels via interruption.
 *
 * @author Guilherme Nunes Trofino
 * @date 2026-05-03
 */

#pragma once

#include "pwm.h"
#include "pinout.h"

/**
 * @brief Radio channels.
 */
typedef enum radio_channel {
    RADIO_CHANNEL_FAILSAFE = 0, /**< Radio channel for fail-safe. */
    RADIO_CHANNEL_BUTTON,       /**< Radio channel for button. */
    RADIO_CHANNEL_STEERING,     /**< Radio channel for sterring. */
    RADIO_CHANNEL_THROTTLE,     /**< Radio channel for throttle. */
    RADIO_CHANNEL_5,            /**< Radio channel 5. */
    RADIO_CHANNEL_6,            /**< Radio channel 6. */
    NUMBER_OF_RADIO_CHANNELS    /**< Number of radio channels. */
} radio_channel_t;

/**
 * @brief Radio channel failsafe triggering condition.
 */
typedef enum radio_failsafe {
    /**
     * @brief Failsafe trigger disabled.
     *
     * Enabled channels with failsafe disable will still receive PWM signals
     * but their signals will not be considered for failsafe conditions.
     */
    RADIO_FAILSAFE_DISABLE = 0,
    /**
     * @brief Failsafe triggered by timeout in microseconds.
     * 
     * Enabled channels with failsafe by timeout will trigger failsafe
     * conditions if the interval since their last update is greater than the 
     * configured timeout interval.
     */
    RADIO_FAILSAFE_TIMEOUT,
    /**
     * @brief Failsafe triggered by PWM pulse mismatch.
     *
     * Enabled channels with failsafe by timeout will trigger failsafe
     * conditions if their measured PWM pulse is different than the the
     * configured PWM value.
     */
    RADIO_FAILSAFE_PWM_MISMATCH,
    NUMBER_OF_RADIO_FAILSAFES   /**< Number of radio failsafe triggers. */
} radio_failsafe_t;

/**
 * @brief Radio models.
 *
 * Each model is named following the convention: ``RADIO_MODEL_<BRAND>_<NAME>``.
 *
 * @note ``BRAND`` will be abbreviated with 2 letters and ``NAME`` will **not**
 * be abbreviated.
 */
typedef enum radio_model {
    RADIO_MODEL_FS_GT2B = 0,    /**< Radio Fly-Sky FS-GT2B. */
    RADIO_MODEL_RM_POCKET,      /**< Radio RadioMaster Pocket. */
    NUMBER_OF_RADIO_MODELS,     /**< Number of radio models. */
} radio_model_t;

/**
 * @brief Radio connection status.
 */
typedef enum radio_status {
    RADIO_STATUS_DISCONNECTED = 0,  /**< At least one Fail-Safe condition met. */
    RADIO_STATUS_CONNECTED,         /**< All channels not in Fail-Safe conditions. */
    NUMBER_OF_RADIO_STATUS          /**< Number of radio status. */
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
     * @brief PWM pulse width in microseconds.
     *
     * If the ``RADIO_FAILSAFE_PWM_VALUE`` triggering condition is selected, the
     * PWM pulse width is required and represents the failfree operation.
     *
     * If the channel PWM pulse width is different than the failsafe PWM pulse
     * width, the failsafe condition gets triggered and the radio status is set
     * to ``RADIO_STATUS_DISCONNECTED``.
     */
    pwm_norm_t pwm_expected;
    /**
     * @brief Timeout in microseconds.
     *
     * If the ``RADIO_FAILSAFE_TIMEOUT`` triggering condition is selected, the
     * maximum interval between any two PWM pulses is the timeout in
     * microseconds.
     *
     * If the interval between any two PWM pulses is greater than the timeout in
     * microseconds, the failsafe condition gets triggered and the radio status
     * is set to ``RADIO_STATUS_DISCONNECTED``.
     */
    int64_t timeout_us;
} radio_failsafe_config_t;

/**
 * @brief Radio channel configuration.
 */
typedef struct radio_channel_config {
    bool enabled;                       /**< If disabled, no measurements are taken. */
    const char *name;                   /**< Human-readable, null-terminated name of the channel. */
    pin_t pin;                          /**< Channel pin. */
    radio_failsafe_config_t failsafe;   /**< Fail-Safe configuration. */
    const pwm_norm_t pwm_default;       /**< Default normalized PWM pulse width in microseconds. */
    volatile pwm_norm_t pwm;            /**< Latest normalized PWM pulse width in microseconds capture via interruptions. */
    volatile int64_t rise_time_us;      /**< Last rising time in microseconds. */
    volatile int64_t last_time_us;      /**< Last update time in microseconds. */
} radio_channel_config_t;

/**
 * @brief Radio model configuration.
 */
typedef struct radio_model_config {
    const char *name;                                           /**< Human-readable, null-terminated name of the radio. */
    radio_channel_config_t channels[NUMBER_OF_RADIO_CHANNELS];  /**< Channels configurations. */
} radio_model_config_t;

/**
 * @brief Radio handler.
 */
typedef struct radio_handler {
    radio_model_t model;                                    /**< Current model. */
    radio_model_config_t models[NUMBER_OF_RADIO_MODELS];    /**< Models configurations. */
    radio_status_t status;                                  /**< Current connection status. */
    const char *status_names[NUMBER_OF_RADIO_STATUS];       /**< Array of human-readable, null-terminated names of the radio status. */
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
 * @brief Deinitialize radio channels GPIOs and interrupts.
 */
void radio_deinit(void);

/**
 * @brief Returns the current radio channel name.
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
 * @brief Returns the current radio name.
 *
 * @return Human-readable null-terminated string representing the radio name.
 */
const char *radio_get_name(void);

/**
 * @brief Returns the latest radio status.
 *
 * @return Current connection status
 */
radio_status_t radio_get_status(void);

/**
 * @brief Returns the current radio status name.
 *
 * @return Human-readable null-terminated string representing the status name.
 */
const char *radio_get_status_name(void);

/**
 * @brief Initialize radio channels GPIOs and interrupts.
 *
 * @note After initialization, channels start with their pre-configured PWM
 * initial values until pulses are received.
 */
void radio_init(void);

/**
 * @brief Return latest measured pulse width in microseconds from channel.
 *
 * Disabled channels are filled with ``PWM_NEUTRAL_US``.
 *
 * @note Interruptions briefly disabled while copying values.
 *
 * @param[in] channel Radio channel.
 *
 * @return Normalized pulse width in microseconds.
 */
pwm_norm_t radio_read_channel(radio_channel_t channel);

/**
 * @brief Read the latest pulse widths from all radio channels.
 *
 * Disabled channels are filled with ``PWM_NEUTRAL_US``.
 *
 * @note Interruptions briefly disabled while copying values.
 *
 * @param[out] pwms Output array containing at least ``NUMBER_OF_RADIO_CHANNELS``
 * elements.
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
