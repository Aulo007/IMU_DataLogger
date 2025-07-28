#include "buzzer.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

// Para lembrar, o pino do buzzer é o 21 e... falta lembra kkkksksksks

void inicializar_buzzer(uint pino)
{
    gpio_set_function(pino, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pino);

    pwm_config config = pwm_get_default_config();
    float div = (float)clock_get_hz(clk_sys) / (BUZZER_FREQUENCY * 4096);
    pwm_config_set_clkdiv(&config, div);

    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pino, 0);  // Começa desligado
}

void ativar_buzzer(uint pino)
{
    pwm_set_gpio_level(pino, 2048); // 50% duty cycle
}

void ativar_buzzer_com_intensidade(uint pino, float intensidade)
{
    pwm_set_gpio_level(pino, (int)(intensidade * 4096));
}

void desativar_buzzer(uint pino)
{
    pwm_set_gpio_level(pino, 0); // Desliga o buzzer
}
