#include "freertos/FreeRTOS.h"
#include "fsm.h"
#include "ir.h"
#include "led.h"
#include "logging.h"

void app_main(void) {
    logging_init_async();

    ir_init();

    fsm_init();
    led_init();

    while (1) {
        ir_poll();

        fsm_step();
        led_step();

        vTaskDelay(1);
    }
}
