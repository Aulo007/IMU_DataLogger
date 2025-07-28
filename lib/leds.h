// leds.h
#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include "pico/stdlib.h"
#include "matrizRGB.h"

#define LED_GREEN_PIN 11
#define LED_BLUE_PIN 12
#define LED_RED_PIN 13

// Inicialização dos LEDs
void led_init(void);
void força_leds(float dutycicle);
void acender_led_rgb(uint8_t r, uint8_t g, uint8_t b);
void turn_off_leds(void);
void acender_led_rgb_cor(npColor_t cor);
void acender_led_rgb_cor_aleatoria(void);

#endif // LED_CONTROL_H