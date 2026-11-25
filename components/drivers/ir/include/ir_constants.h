/**
 * @file ir_constants.h
 * @brief IR constants definitions
 *
 * @author Guilherme Nunes Trofino
 * @date 2026-11-24
 */

#pragma once

/**
 * @def IR_RMT_RX_BUFFER_SYMBOLS
 * @brief RMT RX channel symbols buffer size
 *
 * **Default Value:** 64 symbols
 */
#define IR_RMT_RX_BUFFER_SYMBOLS        64U

/**
 * @def IR_RMT_RX_CLOCK_RESOLUTION_HZ
 * @brief RMT RX channel clock resolution in hertz.
 *
 * **Default Value:** 1 MHz, i.e., 1 tick = 1 µs
 */
#define IR_RMT_RX_CLOCK_RESOLUTION_HZ   (1U * 1000U * 1000U)

/**
 * @def IR_RMT_RX_MEMORY_BLOCK_SYMBOL
 * @brief RMT RX channel memory block symbol size.
 *
 * **Default Value:** 48 blocks
 */
#define IR_RMT_RX_MEMORY_BLOCK_SYMBOL   48U

/**
 * @def IR_RMT_SIGNAL_MAX_NS
 * @brief RMT signal maximum duration in nano-seconds.
 *
 * **Default Value:** 4 ms
 */
#define IR_RMT_SIGNAL_MAX_NS            (4U * 1000U * 1000U)

/**
 * @def IR_RMT_SIGNAL_MIN_NS
 * @brief RMT signal minimum duration in nano-seconds.
 *
 * **Default Value:** 2 us.
 */
#define IR_RMT_SIGNAL_MIN_NS            (2U * 1000U)
