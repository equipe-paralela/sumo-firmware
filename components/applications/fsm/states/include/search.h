/**
 * @file search.h
 * @brief ``FSM_STATE_SEARCH`` definition of the FSM callback functions
 * ``on_entry()``, ``on_exit()``, and ``on_run()``
 *
 * @author Guilherme Nunes Trofino
 * @date 2026-05-13
 */

#pragma once

/**
 * @brief Entry handler for ``FSM_STATE_SEARCH``.
 *
 * Calls ``controller_stop()``.
 */
void search_entry(void);

/**
 * @brief Exit handler for ``FSM_STATE_SEARCH``.
 *
 * Calls ``controller_stop()``.
 */
void search_exit(void);

/**
 * @brief Run handler for ``FSM_STATE_SEARCH``.
 *
 * While active, autonomous align with the adversary.
 *
 * @note Transition to ``FSM_STATE_SAFE`` when:
 * - in ``CONFIG_MAINBOARD_V3``: **not** ``IR_STATE_START`` or **not**
 * ``MICROSTART_STATE_START``
 *
 * - in ``CONFIG_MAINBOARD_V2``: **not** ``IR_STATE_START``
 *
 * - in ``CONFIG_MAINBOARD_V1``: **not** ``MICROSTART_STATE_START``
 *
 * @note Transition to ``FSM_STATE_SURVIVE`` when:
 * - ``sensor_detected_line()``
 *
 * @note Transition to ``FSM_STATE_AUTONOMOUS`` when:
 * - **not** ``sensor_detected_obstacle_sides()``
 */
void search_run(void);
