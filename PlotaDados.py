import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from datetime import datetime, timedelta
import os

# --- Configurações ---
# Nome do arquivo CSV que foi gerado pelo script de busca de dados.
# Certifique-se de que este nome é exatamente o mesmo do arquivo na sua pasta.
arquivo_csv = 'dados_pico.csv'

# --- Leitura e Preparação dos Dados ---
try:
    # Tenta ler o arquivo CSV. O separador é ponto e vírgula.
    df = pd.read_csv(arquivo_csv, sep=';')

    # --- LÓGICA PARA CALCULAR O TEMPO REAL ---
    # 1. Pega a data e hora da última modificação do arquivo (aproximadamente o fim da coleta)
    t_modificacao = os.path.getmtime(arquivo_csv)
    fim_da_coleta = datetime.fromtimestamp(t_modificacao)

    # 2. Calcula a duração total da coleta em segundos usando os timestamps do Pico
    timestamp_final_us = df['timestamp_us'].iloc[-1]
    timestamp_inicial_us = df['timestamp_us'].iloc[0]
    duracao_total_s = (timestamp_final_us - timestamp_inicial_us) / 1000000

    # 3. Calcula a hora de início aproximada da coleta (fim - duração)
    inicio_da_coleta = fim_da_coleta - timedelta(seconds=duracao_total_s)

    # 4. Cria a nova coluna 'tempo_real', convertendo cada timestamp do Pico para um horário real
    df['tempo_real'] = [inicio_da_coleta + timedelta(microseconds=int(ts - timestamp_inicial_us)) for ts in df['timestamp_us']]

    print("Dados carregados e tempo real calculado com sucesso!")
    # Mostra uma amostra dos novos dados de tempo para confirmação
    print(df[['tempo_real', 'ax', 'ay', 'az']].head())

except FileNotFoundError:
    print(f"ERRO: Arquivo '{arquivo_csv}' não encontrado na pasta.")
    print("Por favor, execute primeiro o script que busca os dados do Pico para criar este arquivo.")
    # Encerra o script se o arquivo não for encontrado
    exit()
except Exception as e:
    print(f"Ocorreu um erro ao processar o arquivo: {e}")
    exit()

# --- Plotando o Gráfico do Acelerômetro ---

# Cria uma figura com 3 subplots (3 linhas, 1 coluna)
# sharex=True faz com que todos compartilhem o mesmo eixo X (tempo)
fig1, eixos_acel = plt.subplots(3, 1, figsize=(14, 8), sharex=True)

# Título principal da figura
fig1.suptitle('Dados do Acelerômetro', fontsize=16)

# Plotando cada eixo usando a nova coluna 'tempo_real' no eixo X
eixos_acel[0].plot(df['tempo_real'], df['ax'], color='r')
eixos_acel[0].set_ylabel('Eixo X (ax)')
eixos_acel[0].grid(True)

eixos_acel[1].plot(df['tempo_real'], df['ay'], color='g')
eixos_acel[1].set_ylabel('Eixo Y (ay)')
eixos_acel[1].grid(True)

eixos_acel[2].plot(df['tempo_real'], df['az'], color='b')
eixos_acel[2].set_ylabel('Eixo Z (az)')
eixos_acel[2].grid(True)

# Formata o eixo X para mostrar a hora de forma legível (Horas:Minutos:Segundos)
eixos_acel[2].xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
plt.xlabel('Hora da Coleta (HH:MM:SS)')

# Ajusta o layout para não sobrepor os títulos
plt.tight_layout(rect=[0, 0.03, 1, 0.95])

# Salva a imagem do gráfico
plt.savefig('acelerometro_plot_tempo_real.png')
print("\nGráfico 'acelerometro_plot_tempo_real.png' salvo com sucesso.")


# --- Plotando o Gráfico do Giroscópio ---

# Cria uma segunda figura para o giroscópio
fig2, eixos_giro = plt.subplots(3, 1, figsize=(14, 8), sharex=True)
fig2.suptitle('Dados do Giroscópio', fontsize=16)

# Plotando cada eixo
eixos_giro[0].plot(df['tempo_real'], df['gx'], color='r')
eixos_giro[0].set_ylabel('Eixo X (gx)')
eixos_giro[0].grid(True)

eixos_giro[1].plot(df['tempo_real'], df['gy'], color='g')
eixos_giro[1].set_ylabel('Eixo Y (gy)')
eixos_giro[1].grid(True)

eixos_giro[2].plot(df['tempo_real'], df['gz'], color='b')
eixos_giro[2].set_ylabel('Eixo Z (gz)')
eixos_giro[2].grid(True)

eixos_giro[2].xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
plt.xlabel('Hora da Coleta (HH:MM:SS)')

plt.tight_layout(rect=[0, 0.03, 1, 0.95])
plt.savefig('giroscopio_plot_tempo_real.png')
print("Gráfico 'giroscopio_plot_tempo_real.png' salvo com sucesso.")

# Mostra os gráficos na tela (opcional)
plt.show()