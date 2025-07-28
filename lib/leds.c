#include "leds.h"
#include "hardware/pwm.h"
#include "matrizRGB.h"
#include <stdio.h>
#include <stdlib.h>


// Configuração do PWM
static uint slice_num_red;
static uint slice_num_green;
static uint slice_num_blue;

void led_init(void)
{
    // Configurar os pinos como PWM
    gpio_set_function(LED_RED_PIN, GPIO_FUNC_PWM);
    gpio_set_function(LED_GREEN_PIN, GPIO_FUNC_PWM);
    gpio_set_function(LED_BLUE_PIN, GPIO_FUNC_PWM);
    
    // Obter os números dos slices PWM para cada pino
    slice_num_red = pwm_gpio_to_slice_num(LED_RED_PIN);
    slice_num_green = pwm_gpio_to_slice_num(LED_GREEN_PIN);
    slice_num_blue = pwm_gpio_to_slice_num(LED_BLUE_PIN);
    
    // Configurar o PWM para cada slice
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.0f); // Define o divisor de clock para evitar flickering
    pwm_config_set_wrap(&config, 4095);   // Mesma resolução do ADC (12 bits)
    
    // Inicializar PWM para cada slice e ativar
    pwm_init(slice_num_red, &config, true);
    pwm_init(slice_num_green, &config, true);
    pwm_init(slice_num_blue, &config, true);
    
    // Garantir que os LEDs comecem desligados
    turn_off_leds();
}

void força_leds(float dutycicle)
{
    // Limitar o duty cycle entre 0 e 100%
    if (dutycicle < 0.0f) dutycicle = 0.0f;
    if (dutycicle > 100.0f) dutycicle = 100.0f;
    
    // Converter de porcentagem (0-100) para o valor do contador PWM (0-4095)
    uint16_t valor_pwm = (uint16_t)((dutycicle / 100.0f) * 4095);
    
    // Aplicar o mesmo duty cycle para todos os LEDs
    pwm_set_gpio_level(LED_RED_PIN, valor_pwm);
    pwm_set_gpio_level(LED_GREEN_PIN, valor_pwm);
    pwm_set_gpio_level(LED_BLUE_PIN, valor_pwm);
}

void acender_led_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    // Converter os valores de 8 bits (0-255) para valores do contador PWM (0-4095)
    uint16_t valor_r = (uint16_t)(r * 4095 / 255);
    uint16_t valor_g = (uint16_t)(g * 4095 / 255);
    uint16_t valor_b = (uint16_t)(b * 4095 / 255);
    
    // Configurar o nível PWM para cada cor
    pwm_set_gpio_level(LED_RED_PIN, valor_r);
    pwm_set_gpio_level(LED_GREEN_PIN, valor_g);
    pwm_set_gpio_level(LED_BLUE_PIN, valor_b);
}

void acender_led_rgb_cor(npColor_t cor)
{
    // Chamar a função com os valores RGB da estrutura npColor_t
    acender_led_rgb(cor.r, cor.g, cor.b);
}

void acender_led_rgb_cor_aleatoria(void)
{
    acender_led_rgb_cor((npColor_t){rand() % 256, rand() % 256, rand() % 256});
}


void turn_off_leds(void)
{
    // Desligar todos os LEDs configurando o nível PWM para 0
    pwm_set_gpio_level(LED_RED_PIN, 0);
    pwm_set_gpio_level(LED_GREEN_PIN, 0);
    pwm_set_gpio_level(LED_BLUE_PIN, 0);
}