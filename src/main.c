#include "freertos/FreeRTOS.h"
#include "fsm.h"
#if   defined(CONFIG_MAINBOARD_V3)
#include "ir.h"
#elif defined(CONFIG_MAINBOARD_V2)
#include "ir.h"
#elif defined(CONFIG_MAINBOARD_V1)
#endif
#include "led.h"
#include "logging.h"

void app_main(void) {
    logging_init_async();

#if   defined(CONFIG_MAINBOARD_V3)
    ir_init();
#elif defined(CONFIG_MAINBOARD_V2)
    ir_init();
#elif defined(CONFIG_MAINBOARD_V1)
#endif

    fsm_init();
    led_init();

    while (1) {
#if   defined(CONFIG_MAINBOARD_V3)
        ir_poll();
#elif defined(CONFIG_MAINBOARD_V2)
        ir_poll();
#elif defined(CONFIG_MAINBOARD_V1)
#endif

        fsm_step();
        led_step();

        vTaskDelay(1);
    }
}
