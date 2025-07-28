#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "hardware/adc.h"
#include "hardware/rtc.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "lib/leds.h"
#include "lib/ssd1306.h"

#include "ff.h"
#include "diskio.h"
#include "f_util.h"
#include "hw_config.h"
#include "my_debug.h"
#include "rtc.h"
#include "sd_card.h"

static bool logger_enabled;
static const uint32_t period = 1000;
static absolute_time_t next_log_time;

volatile bool cartao_montado = false;
volatile bool capturando_dados = false;

static FIL g_log_file;
static volatile bool g_log_ativo = false; // 'volatile' é importante para flags usadas em interrupções
static volatile bool g_dados_prontos_para_gravar = false;

// Estrutura para armazenar a média final que será gravada no arquivo
static int16_t g_dados_medios[7]; // ax, ay, az, gx, gy, gz, temp

// Estrutura do nosso temporizador
static repeating_timer_t g_repeating_timer;

static char filename[20] = "adc_data8.csv";

static sd_card_t *sd_get_by_name(const char *const name)
{
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name))
            return sd_get_by_num(i);
    DBG_PRINTF("%s: unknown name %s\n", __func__, name);
    return NULL;
}
static FATFS *sd_get_fs_by_name(const char *name)
{
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name))
            return &sd_get_by_num(i)->fatfs;
    DBG_PRINTF("%s: unknown name %s\n", __func__, name);
    return NULL;
}

static void run_setrtc()
{
    const char *dateStr = strtok(NULL, " ");
    if (!dateStr)
    {
        printf("Missing argument\n");
        return;
    }
    int date = atoi(dateStr);

    const char *monthStr = strtok(NULL, " ");
    if (!monthStr)
    {
        printf("Missing argument\n");
        return;
    }
    int month = atoi(monthStr);

    const char *yearStr = strtok(NULL, " ");
    if (!yearStr)
    {
        printf("Missing argument\n");
        return;
    }
    int year = atoi(yearStr) + 2000;

    const char *hourStr = strtok(NULL, " ");
    if (!hourStr)
    {
        printf("Missing argument\n");
        return;
    }
    int hour = atoi(hourStr);

    const char *minStr = strtok(NULL, " ");
    if (!minStr)
    {
        printf("Missing argument\n");
        return;
    }
    int min = atoi(minStr);

    const char *secStr = strtok(NULL, " ");
    if (!secStr)
    {
        printf("Missing argument\n");
        return;
    }
    int sec = atoi(secStr);

    datetime_t t = {
        .year = (int16_t)year,
        .month = (int8_t)month,
        .day = (int8_t)date,
        .dotw = 0, // 0 is Sunday
        .hour = (int8_t)hour,
        .min = (int8_t)min,
        .sec = (int8_t)sec};
    rtc_set_datetime(&t);
}

static void run_format()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = sd_get_by_num(0)->pcName;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        return;
    }
    /* Format the drive with default parameters */
    FRESULT fr = f_mkfs(arg1, 0, 0, FF_MAX_SS * 2);
    if (FR_OK != fr)
        printf("f_mkfs error: %s (%d)\n", FRESULT_str(fr), fr);
}
static void run_mount()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = sd_get_by_num(0)->pcName;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        return;
    }
    FRESULT fr = f_mount(p_fs, arg1, 1);
    if (FR_OK != fr)
    {
        printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    sd_card_t *pSD = sd_get_by_name(arg1);
    myASSERT(pSD);
    pSD->mounted = true;
    cartao_montado = true;
    printf("Processo de montagem do SD ( %s ) concluído\n", pSD->pcName);
}
static void run_unmount()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = sd_get_by_num(0)->pcName;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        return;
    }
    FRESULT fr = f_unmount(arg1);
    if (FR_OK != fr)
    {
        printf("f_unmount error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    sd_card_t *pSD = sd_get_by_name(arg1);
    myASSERT(pSD);
    pSD->mounted = false;
    pSD->m_Status |= STA_NOINIT; // in case medium is removed
    cartao_montado = false;
    printf("SD ( %s ) desmontado\n", pSD->pcName);
}
static void run_getfree()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = sd_get_by_num(0)->pcName;
    DWORD fre_clust, fre_sect, tot_sect;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Unknown logical drive number: \"%s\"\n", arg1);
        return;
    }
    FRESULT fr = f_getfree(arg1, &fre_clust, &p_fs);
    if (FR_OK != fr)
    {
        printf("f_getfree error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    tot_sect = (p_fs->n_fatent - 2) * p_fs->csize;
    fre_sect = fre_clust * p_fs->csize;
    printf("%10lu KiB total drive space.\n%10lu KiB available.\n", tot_sect / 2, fre_sect / 2);
}
static void run_ls()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = "";
    char cwdbuf[FF_LFN_BUF] = {0};
    FRESULT fr;
    char const *p_dir;
    if (arg1[0])
    {
        p_dir = arg1;
    }
    else
    {
        fr = f_getcwd(cwdbuf, sizeof cwdbuf);
        if (FR_OK != fr)
        {
            printf("f_getcwd error: %s (%d)\n", FRESULT_str(fr), fr);
            return;
        }
        p_dir = cwdbuf;
    }
    printf("Directory Listing: %s\n", p_dir);
    DIR dj;
    FILINFO fno;
    memset(&dj, 0, sizeof dj);
    memset(&fno, 0, sizeof fno);
    fr = f_findfirst(&dj, &fno, p_dir, "*");
    if (FR_OK != fr)
    {
        printf("f_findfirst error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    while (fr == FR_OK && fno.fname[0])
    {
        const char *pcWritableFile = "writable file",
                   *pcReadOnlyFile = "read only file",
                   *pcDirectory = "directory";
        const char *pcAttrib;
        if (fno.fattrib & AM_DIR)
            pcAttrib = pcDirectory;
        else if (fno.fattrib & AM_RDO)
            pcAttrib = pcReadOnlyFile;
        else
            pcAttrib = pcWritableFile;
        printf("%s [%s] [size=%llu]\n", fno.fname, pcAttrib, fno.fsize);

        fr = f_findnext(&dj, &fno);
    }
    f_closedir(&dj);
}
static void run_cat()
{
    char *arg1 = strtok(NULL, " ");
    if (!arg1)
    {
        printf("Missing argument\n");
        return;
    }
    FIL fil;
    FRESULT fr = f_open(&fil, arg1, FA_READ);
    if (FR_OK != fr)
    {
        printf("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    char buf[256];
    while (f_gets(buf, sizeof buf, &fil))
    {
        printf("%s", buf);
    }
    fr = f_close(&fil);
    if (FR_OK != fr)
        printf("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
}

// MPU6050 I2C address
#define I2C_PORT i2c0 // i2c0 pinos 0 e 1, i2c1 pinos 2 e 3
#define I2C_SDA 0     // 0 ou 2
#define I2C_SCL 1     // 1 ou 3
// Oi, eu sou o display
#define I2C_PORT_DISP i2c1
#define I2C_SDA_DISP 14
#define I2C_SCL_DISP 15
#define endereco 0x3C

static int addr = 0x68;

static void mpu6050_reset()
{
    // Two byte reset. First byte register, second byte data
    // There are a load more options to set up the device in different ways that could be added here
    uint8_t buf[] = {0x6B, 0x80};
    i2c_write_blocking(I2C_PORT, addr, buf, 2, false);
    sleep_ms(100); // Allow device to reset and stabilize

    // Clear sleep mode (0x6B register, 0x00 value)
    buf[1] = 0x00; // Clear sleep mode by writing 0x00 to the 0x6B register
    i2c_write_blocking(I2C_PORT, addr, buf, 2, false);
    sleep_ms(10); // Allow stabilization after waking up
}

static void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp)
{
    // For this particular device, we send the device the register we want to read
    // first, then subsequently read from the device. The register is auto incrementing
    // so we don't need to keep sending the register we want, just the first.

    uint8_t buffer[6];

    // Start reading acceleration registers from register 0x3B for 6 bytes
    uint8_t val = 0x3B;
    i2c_write_blocking(I2C_PORT, addr, &val, 1, true); // true to keep master control of bus
    i2c_read_blocking(I2C_PORT, addr, buffer, 6, false);

    for (int i = 0; i < 3; i++)
    {
        accel[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);
    }

    // Now gyro data from reg 0x43 for 6 bytes
    // The register is auto incrementing on each read
    val = 0x43;
    i2c_write_blocking(I2C_PORT, addr, &val, 1, true);
    i2c_read_blocking(I2C_PORT, addr, buffer, 6, false); // False - finished with bus

    for (int i = 0; i < 3; i++)
    {
        gyro[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);
    }

    // Now temperature from reg 0x41 for 2 bytes
    // The register is auto incrementing on each read
    val = 0x41;
    i2c_write_blocking(I2C_PORT, addr, &val, 1, true);
    i2c_read_blocking(I2C_PORT, addr, buffer, 2, false); // False - finished with bus

    *temp = buffer[0] << 8 | buffer[1];
}

// Funções para capturar log de forma contínua
// Esta função é chamada pela interrupção do timer a cada 10ms.
bool timer_callback(struct repeating_timer *t)
{
    // Se o log não estiver ativo, simplesmente para o timer e sai.
    if (!g_log_ativo)
    {
        g_dados_prontos_para_gravar = false;
        return false; // Retornar false cancela o timer
    }

    // Variáveis para acumular as 100 amostras. Usamos 32-bit para não estourar.
    static int32_t acc_accum[3], gyro_accum[3], temp_accum;
    static int sample_count = 0;

    // Variáveis temporárias para a leitura do sensor
    int16_t accel[3], gyro[3], temp_raw;
    mpu6050_read_raw(accel, gyro, &temp_raw);

    // Acumula os valores
    for (int i = 0; i < 3; i++)
    {
        acc_accum[i] += accel[i];
        gyro_accum[i] += gyro[i];
    }
    temp_accum += temp_raw;
    sample_count++;

    if (sample_count >= 100)
    {
        // Calcula a média de cada sensor
        g_dados_medios[0] = acc_accum[0] / 100;
        g_dados_medios[1] = acc_accum[1] / 100;
        g_dados_medios[2] = acc_accum[2] / 100;
        g_dados_medios[3] = gyro_accum[0] / 100;
        g_dados_medios[4] = gyro_accum[1] / 100;
        g_dados_medios[5] = gyro_accum[2] / 100;
        g_dados_medios[6] = temp_accum / 100;

        // Reseta os acumuladores e o contador para o próximo segundo
        for (int i = 0; i < 3; i++)
        {
            acc_accum[i] = 0;
            gyro_accum[i] = 0;
        }
        temp_accum = 0;
        sample_count = 0;

        // Sinaliza para o loop principal que há dados prontos para serem gravados no arquivo
        g_dados_prontos_para_gravar = true;
    }

    return true; // Retornar true mantém o timer ativo
}

// Função para INICIAR o processo de log
void iniciar_log_robusto()
{
    if (g_log_ativo)
    {
        printf("Log já está ativo!\n");
        return;
    }

    // Abre o arquivo para adicionar dados no final
    FRESULT fr = f_open(&g_log_file, filename, FA_OPEN_APPEND | FA_WRITE);
    if (fr != FR_OK)
    {
        printf("ERRO: Nao foi possivel abrir o arquivo '%s' (%s)\n", filename, FRESULT_str(fr));
        return;
    }

    // Se o arquivo estiver vazio, escreve o cabeçalho
    if (f_tell(&g_log_file) == 0)
    {
        f_printf(&g_log_file, "timestamp_us;ax_avg;ay_avg;az_avg;gx_avg;gy_avg;gz_avg;temp_avg\n");
    }

    g_log_ativo = true;
    capturando_dados = true;

    // Inicia um timer que chamará a 'timer_callback' a cada 10 milissegundos
    // 10ms * 100 amostras = 1000ms = 1 segundo por linha de dados gravada.
    add_repeating_timer_ms(-10, timer_callback, NULL, &g_repeating_timer);

    printf(">>> LOG INICIADO. Coletando médias de 100 amostras por segundo...\n");
}

// Função para PARAR o processo de log
void parar_log_robusto()
{
    if (!g_log_ativo)
    {
        printf("Nenhum log ativo para parar.\n");
        return;
    }

    // Sinaliza para a interrupção do timer parar
    g_log_ativo = false;
    capturando_dados = false;

    // Cancela o timer explicitamente
    cancel_repeating_timer(&g_repeating_timer);

    // Fecha o arquivo, salvando todos os dados restantes.
    f_close(&g_log_file);

    printf(">>> LOG PARADO. Arquivo salvo com segurança.\n");
}

// Função para ler o conteúdo de um arquivo e exibir no terminal
void read_file(const char *filename)
{
    FIL file;
    FRESULT res = f_open(&file, filename, FA_READ);
    if (res != FR_OK)
    {
        printf("[ERRO] Não foi possível abrir o arquivo para leitura. Verifique se o Cartão está montado ou se o arquivo existe.\n");

        return;
    }
    char buffer[128];
    UINT br;
    printf("Conteúdo do arquivo %s:\n", filename);
    while (f_read(&file, buffer, sizeof(buffer) - 1, &br) == FR_OK && br > 0)
    {
        buffer[br] = '\0';
        printf("%s", buffer);
    }
    f_close(&file);
    printf("\nLeitura do arquivo %s concluída.\n\n", filename);
}

#include "pico/bootrom.h"
#define button_A 5
#define button_B 6
#define SW_BUTTON 22
volatile uint32_t last_button_time = 0;

void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint32_t now_button_time = to_ms_since_boot(get_absolute_time());

    if (now_button_time - last_button_time < 200)
    {
        return;
    }
    last_button_time = now_button_time;

    if (gpio == button_A)
    {
        printf("Botão A pressionado!\n");
    }
    else if (gpio == button_B)
    {
        printf("Botão B pressionado!\n");
        reset_usb_boot(0, 0);
    }

    else if (gpio == SW_BUTTON)
    {
        printf("Botão SW pressionado!\n");
    }
}

static void run_help()
{
    printf("\nComandos disponíveis:\n\n");
    printf("Digite 'a' para montar o cartão SD\n");
    printf("Digite 'b' para desmontar o cartão SD\n");
    printf("Digite 'c' para listar arquivos\n");
    printf("Digite 'd' para mostrar conteúdo do arquivo\n");
    printf("Digite 'e' para obter espaço livre no cartão SD\n");
    printf("Digite 'f' para capturar dados do ADC e salvar no arquivo\n");
    printf("Digite 'g' para formatar o cartão SD\n");
    printf("Digite 'h' para exibir os comandos disponíveis\n");
    printf("Digite 's' para iniciar a gravar dados no cartão sd\n");
    printf("Digite 'p' para parar de gravar dados no cartão\n");
    printf("\nEscolha o comando:  ");
}

typedef void (*p_fn_t)();
typedef struct
{
    char const *const command;
    p_fn_t const function;
    char const *const help;
} cmd_def_t;

static cmd_def_t cmds[] = {
    {"setrtc", run_setrtc, "setrtc <DD> <MM> <YY> <hh> <mm> <ss>: Set Real Time Clock"},
    {"format", run_format, "format [<drive#:>]: Formata o cartão SD"},
    {"mount", run_mount, "mount [<drive#:>]: Monta o cartão SD"},
    {"unmount", run_unmount, "unmount <drive#:>: Desmonta o cartão SD"},
    {"getfree", run_getfree, "getfree [<drive#:>]: Espaço livre"},
    {"ls", run_ls, "ls: Lista arquivos"},
    {"cat", run_cat, "cat <filename>: Mostra conteúdo do arquivo"},
    {"help", run_help, "help: Mostra comandos disponíveis"}};

static void process_stdio(int cRxedChar)
{
    static char cmd[256];
    static size_t ix;

    if (!isprint(cRxedChar) && !isspace(cRxedChar) && '\r' != cRxedChar &&
        '\b' != cRxedChar && cRxedChar != (char)127)
        return;
    printf("%c", cRxedChar); // echo
    stdio_flush();
    if (cRxedChar == '\r')
    {
        printf("%c", '\n');
        stdio_flush();

        if (!strnlen(cmd, sizeof cmd))
        {
            printf("> ");
            stdio_flush();
            return;
        }
        char *cmdn = strtok(cmd, " ");
        if (cmdn)
        {
            size_t i;
            for (i = 0; i < count_of(cmds); ++i)
            {
                if (0 == strcmp(cmds[i].command, cmdn))
                {
                    (*cmds[i].function)();
                    break;
                }
            }
            if (count_of(cmds) == i)
                printf("Command \"%s\" not found\n", cmdn);
        }
        ix = 0;
        memset(cmd, 0, sizeof cmd);
        printf("\n> ");
        stdio_flush();
    }
    else
    {
        if (cRxedChar == '\b' || cRxedChar == (char)127)
        {
            if (ix > 0)
            {
                ix--;
                cmd[ix] = '\0';
            }
        }
        else
        {
            if (ix < sizeof cmd - 1)
            {
                cmd[ix] = cRxedChar;
                ix++;
            }
        }
    }
}

int main()
{
    // display
    i2c_init(I2C_PORT_DISP, 400 * 1000);
    gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);
    ssd1306_t ssd;
    ssd1306_init(&ssd, 128, 64, false, endereco, I2C_PORT_DISP);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    ssd1306_draw_string(&ssd, "Iniciando", 28, 24);
    ssd1306_draw_string(&ssd, "Sistema...", 24, 36);
    ssd1306_send_data(&ssd);
    // led
    led_init();
    acender_led_rgb(255, 255, 51);

    // iniciando botões
    gpio_init(button_A);
    gpio_init(button_B);
    gpio_init(SW_BUTTON);
    gpio_set_dir(button_A, GPIO_IN);
    gpio_pull_up(button_A);
    gpio_set_dir(button_B, GPIO_IN);
    gpio_pull_up(button_B);
    gpio_set_dir(SW_BUTTON, GPIO_IN);
    gpio_pull_up(SW_BUTTON);
    gpio_set_irq_enabled_with_callback(button_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(button_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(SW_BUTTON, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    stdio_init_all();
    sleep_ms(5000);
    time_init();
    adc_init();

    printf("Hello, MPU6050! Reading raw data from registers...\n");

    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    bi_decl(bi_2pins_with_func(I2C_SDA, I2C_SCL, GPIO_FUNC_I2C));

    printf("Antes do reset MPU...\n");
    mpu6050_reset();

    printf("FatFS SPI example\n");
    printf("\033[2J\033[H"); // Limpa tela
    printf("\n> ");
    stdio_flush();
    run_help();
    sleep_ms(1000);

    turn_off_leds();
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Loop principal
    while (true)
    {
        // Tarefa 1: Verificar se a interrupção do timer sinalizou que há dados para gravar
        if (g_dados_prontos_para_gravar)
        {
            // Reseta a flag primeiro para não entrar aqui de novo sem querer
            g_dados_prontos_para_gravar = false;

            // Agora, com calma e fora da interrupção, gravamos os dados médios no arquivo
            f_printf(&g_log_file, "%llu;%d;%d;%d;%d;%d;%d;%d\n",
                     time_us_64(),
                     g_dados_medios[0], g_dados_medios[1], g_dados_medios[2],
                     g_dados_medios[3], g_dados_medios[4], g_dados_medios[5],
                     g_dados_medios[6]);
        }

        if (cartao_montado == true && capturando_dados == false && g_log_ativo == false)
        {
            acender_led_rgb(0, 255, 0);
            ssd1306_fill(&ssd, false);
            ssd1306_draw_string(&ssd, "Sistema pronto", 8, 16);
            ssd1306_draw_string(&ssd, "para capturar", 12, 28);
            ssd1306_draw_string(&ssd, "dados", 44, 40);
            ssd1306_send_data(&ssd);
        }
        else if (cartao_montado == false && capturando_dados == false && g_log_ativo == false)
        {
            acender_led_rgb(255, 255, 51);
            ssd1306_fill(&ssd, false);
            ssd1306_draw_string(&ssd, "Cartao SD", 28, 16);
            ssd1306_draw_string(&ssd, "Desmontado", 24, 28);
            ssd1306_draw_string(&ssd, "Aguardando...", 16, 40);
            ssd1306_send_data(&ssd);
        }

        // Tarefa 2: Processar comandos do usuário
        int cRxedChar = getchar_timeout_us(0);
        if (cRxedChar != PICO_ERROR_TIMEOUT)
        {
            bool atalho_usado = true;
            switch (cRxedChar)
            {
            case 'a':
                run_mount();
                break;
            case 'b':
                run_unmount();
                break;
            case 'c':
                run_ls();
                break;
            case 'd':
                read_file(filename);
                break;
            case 'e':
                run_getfree();
                break;
            case 'g':
                run_format();
                break;
            case 'h':
                run_help();
                break;
            case 's':
                iniciar_log_robusto();
                break; // START
            case 'p':
                parar_log_robusto();
                break; // PARAR
            default:
                atalho_usado = false;
                break;
            }

            if (atalho_usado)
            {
                printf("\n> ");
            }
            else
            {
                // Deixamos o process_stdio aqui caso queira reativar comandos completos no futuro
                // Mas a lógica de atalhos é a principal agora.
            }
        }
    }
    return 0;
}