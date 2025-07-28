import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from matplotlib.ticker import AutoMinorLocator
from datetime import datetime, timedelta
import os

# --- Configurações ---
plt.style.use("seaborn-v0_8-whitegrid")

# Nome do arquivo CSV
arquivo_csv = "dados_pico.csv"

# --- Leitura e Preparação dos Dados ---
try:
    df = pd.read_csv(arquivo_csv, sep=";")
    # Lógica de tempo
    t_modificacao = os.path.getmtime(arquivo_csv)
    fim_da_coleta = datetime.fromtimestamp(t_modificacao)
    timestamp_final_us = df["timestamp_us"].iloc[-1]
    timestamp_inicial_us = df["timestamp_us"].iloc[0]
    duracao_total_s = (timestamp_final_us - timestamp_inicial_us) / 1000000
    inicio_da_coleta = fim_da_coleta - timedelta(seconds=duracao_total_s)
    df["tempo_real"] = [
        inicio_da_coleta + timedelta(microseconds=int(ts - timestamp_inicial_us))
        for ts in df["timestamp_us"]
    ]

    # Conversão para unidades físicas
    ACCEL_FACTOR = 16384.0
    GYRO_FACTOR = 131.0
    df["ax_g"] = df["ax_avg"] / ACCEL_FACTOR
    df["ay_g"] = df["ay_avg"] / ACCEL_FACTOR
    df["az_g"] = df["az_avg"] / ACCEL_FACTOR
    df["gx_dps"] = df["gx_avg"] / GYRO_FACTOR
    df["gy_dps"] = df["gy_avg"] / GYRO_FACTOR
    df["gz_dps"] = df["gz_avg"] / GYRO_FACTOR

    print("Dados carregados e convertidos com sucesso!")

except FileNotFoundError:
    print(f"ERRO: Arquivo '{arquivo_csv}' não encontrado na pasta.")
    exit()
except Exception as e:
    print(f"Ocorreu um erro ao processar o arquivo: {e}")
    exit()

# Plotando o Gráfico do Acelerômetro 
fig1, eixos_acel = plt.subplots(3, 1, figsize=(15, 9), sharex=True)
data_coleta = df["tempo_real"].iloc[0].strftime("%d/%m/%Y")
fig1.suptitle(f"Análise do Acelerômetro - Coleta de {data_coleta}", fontsize=16)

# Dados do Eixo X
max_ax = df["ax_g"].max()
min_ax = df["ax_g"].min()
label_ax = f"Eixo X (Máx: {max_ax:.2f}, Mín: {min_ax:.2f}) g"
eixos_acel[0].plot(
    df["tempo_real"], df["ax_g"], color="r", label=label_ax, linewidth=1.5
)

# Dados do Eixo Y
max_ay = df["ay_g"].max()
min_ay = df["ay_g"].min()
label_ay = f"Eixo Y (Máx: {max_ay:.2f}, Mín: {min_ay:.2f}) g"
eixos_acel[1].plot(
    df["tempo_real"], df["ay_g"], color="g", label=label_ay, linewidth=1.5
)

# Dados do Eixo Z
max_az = df["az_g"].max()
min_az = df["az_g"].min()
label_az = f"Eixo Z (Máx: {max_az:.2f}, Mín: {min_az:.2f}) g"
eixos_acel[2].plot(
    df["tempo_real"], df["az_g"], color="b", label=label_az, linewidth=1.5
)


for ax in eixos_acel:
    ax.set_ylabel("Aceleração (g)")
    ax.legend(loc="upper left")
    ax.set_ylim(-2, 2)
    # MELHORIA: Grade principal (major) e secundária (minor)
    ax.grid(which="major", linestyle="-", linewidth="0.8")
    ax.grid(which="minor", linestyle=":", linewidth="0.5", color="gray")
    ax.yaxis.set_minor_locator(AutoMinorLocator())
    ax.xaxis.set_minor_locator(AutoMinorLocator())

eixos_acel[2].xaxis.set_major_formatter(mdates.DateFormatter("%H:%M:%S"))
plt.xlabel("Hora da Coleta (HH:MM:SS)")
plt.tight_layout(rect=[0, 0.03, 1, 0.95])
# MUDANÇA: Voltando ao nome de arquivo anterior
plt.savefig("acelerometro_plot_tempo_real.png")
print("\nGráfico 'acelerometro_plot_tempo_real.png' salvo com sucesso.")


# --- Plotando o Gráfico do Giroscópio (Versão Final) ---
fig2, eixos_giro = plt.subplots(3, 1, figsize=(15, 9), sharex=True)
fig2.suptitle(f"Análise do Giroscópio - Coleta de {data_coleta}", fontsize=16)

# Dados do Eixo X
max_gx = df["gx_dps"].max()
min_gx = df["gx_dps"].min()
label_gx = f"Eixo X (Máx: {max_gx:.1f}, Mín: {min_gx:.1f}) °/s"
eixos_giro[0].plot(
    df["tempo_real"], df["gx_dps"], color="r", label=label_gx, linewidth=1.5
)

# Dados do Eixo Y
max_gy = df["gy_dps"].max()
min_gy = df["gy_dps"].min()
label_gy = f"Eixo Y (Máx: {max_gy:.1f}, Mín: {min_gy:.1f}) °/s"
eixos_giro[1].plot(
    df["tempo_real"], df["gy_dps"], color="g", label=label_gy, linewidth=1.5
)

# Dados do Eixo Z
max_gz = df["gz_dps"].max()
min_gz = df["gz_dps"].min()
label_gz = f"Eixo Z (Máx: {max_gz:.1f}, Mín: {min_gz:.1f}) °/s"
eixos_giro[2].plot(
    df["tempo_real"], df["gz_dps"], color="b", label=label_gz, linewidth=1.5
)


for ax in eixos_giro:
    ax.set_ylabel("Velocidade Angular (°/s)")
    ax.legend(loc="upper left")
    ax.set_ylim(-250, 250)
    # MELHORIA: Grade principal (major) e secundária (minor)
    ax.grid(which="major", linestyle="-", linewidth="0.8")
    ax.grid(which="minor", linestyle=":", linewidth="0.5", color="gray")
    ax.yaxis.set_minor_locator(AutoMinorLocator())
    ax.xaxis.set_minor_locator(AutoMinorLocator())

eixos_giro[2].xaxis.set_major_formatter(mdates.DateFormatter("%H:%M:%S"))
plt.xlabel("Hora da Coleta (HH:MM:SS)")
plt.tight_layout(rect=[0, 0.03, 1, 0.95])

plt.savefig("giroscopio_plot_tempo_real.png")
print("Gráfico 'giroscopio_plot_tempo_real.png' salvo com sucesso.")

plt.show()
