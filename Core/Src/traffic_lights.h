#ifndef TRAFFIC_LIGHTS_H
#define TRAFFIC_LIGHTS_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    GREEN = 0,
    GREEN_BLINK,
    YELLOW,
    RED,
    RED_YELLOW,
    YELLOW_BLINK
} led_state_t;

void traffic_lights_init(TIM_HandleTypeDef *htim);

void traffic_lights_timer_callback(TIM_HandleTypeDef *htim);

void traffic_lights_button_pressed(void);

#endif
