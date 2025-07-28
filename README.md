# Datalogger de Movimento com IMU (MPU6050) e Raspberry Pi Pico

Este projeto implementa um datalogger de movimento portátil e autônomo usando a plataforma **BitDogLab** com um **Raspberry Pi Pico W**. O dispositivo captura dados de aceleração e giroscópio do sensor MPU6050, armazena-os em um cartão MicroSD e fornece feedback de status em tempo real ao usuário através de um display OLED e um LED RGB.

O controle do dispositivo é feito inteiramente por botões físicos, com uma interface intuitiva para iniciar e parar a captura de dados, além de gerenciar o cartão SD com segurança.

## Funcionalidades Principais

- **Captura de Dados:** Leitura contínua dos dados do acelerômetro (eixos X, Y, Z) e do giroscópio (eixos X, Y, Z) do sensor MPU6050.
- **Armazenamento:** Os dados são salvos de forma estruturada em um arquivo `.csv` em um cartão MicroSD, utilizando o sistema de arquivos FatFS.
- **Feedback Visual:**
    - **Display OLED:** Exibe o status atual do sistema ("Inicializando", "Sistema Pronto", "Capturando...", "Erro de SD").
    - **LED RGB:** Indica o estado geral do dispositivo com cores distintas para cada modo de operação.
- **Controle por Botões:** A operação do datalogger é controlada por dois botões, com lógica de *debounce* implementada via software para garantir precisão.
- **Análise de Dados:** Um script em Python é fornecido para ler o arquivo `.csv` gerado, processar os dados e plotar gráficos detalhados de aceleração e giroscópio para análise posterior.

## Hardware Necessário

- Raspberry Pi Pico W
- Placa de desenvolvimento BitDogLab (ou componentes equivalentes):
    - Sensor IMU MPU6050
    - Módulo para Cartão MicroSD
    - Display OLED SSD1306
    - LED RGB
    - Botões (Push Buttons)
- Cartão MicroSD

## Operação do Dispositivo

A interface com o usuário é projetada para ser simples e direta, utilizando o LED RGB e o display OLED como guias visuais.

#### Estados do LED RGB

| Cor               | Significado                                      |
| ----------------- | ------------------------------------------------ |
| 🟡 **Amarelo** | Sistema inicializando ou aguardando a montagem do cartão SD. |
| 🟢 **Verde** | Sistema pronto para iniciar a captura de dados. |
| 🔴 **Vermelho** | Captura de dados em andamento.                  |
| 🔵 **Azul (piscando)** | Acessando o cartão SD (via comandos seriais).    |
| 🟣 **Roxo (piscando)** | Erro fatal (ex: falha ao montar o cartão SD).     |

#### Funções dos Botões

| Botão         | Ação                                                             |
| ------------- | ---------------------------------------------------------------- |
| **Botão 1 (A)** | Inicia a captura de dados (se estiver pronto) ou para a captura (se estiver gravando). |
| **Botão 2 (SW)**| Monta o cartão SD para prepará-lo para a gravação ou o desmonta com segurança. |

## Análise e Gráficos

Para visualizar os dados coletados, um conjunto de scripts em Python é fornecido.

1.  **`Python_serial.py`:**
    Este script utilitário se conecta ao Pico via porta serial para extrair o arquivo `.csv` do cartão SD e salvá-lo no seu computador, eliminando a necessidade de um leitor de cartão externo.

2.  **`plot_data.py`:**
    Este é o script principal de análise. Ele lê o arquivo `.csv` local, converte os dados brutos do sensor para unidades físicas padrão (g para aceleração e °/s para velocidade angular) e gera dois gráficos:
    - Um gráfico para os três eixos do acelerômetro.
    - Um gráfico para os três eixos do giroscópio.

Ambos os gráficos usam o tempo real no eixo X para uma análise precisa.

**Para executar:**
```bash
# Instale as dependências (se necessário)
pip install pandas matplotlib pyserial

# Execute o script de plotagem
python PlotaDados.py
```
