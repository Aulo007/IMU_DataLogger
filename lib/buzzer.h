#ifndef BUZZER_H
#define BUZZER_H

#include "pico/stdlib.h"

#define BUZZER_FREQUENCY 100  // Pode ser ajustado se quiser configurar externamente

void inicializar_buzzer(uint pino);
void ativar_buzzer_com_intensidade(uint pino, float intensidade);
void ativar_buzzer(uint pino);
void desativar_buzzer(uint pino);

#endif // BUZZER_H
