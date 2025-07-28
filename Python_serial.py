import serial
import time

# -- CONFIGURACOES --
PORTA_PICO = "COM6"  # << MUDE AQUI para a sua porta COM
ARQUIVO_DESTINO = "dados_pico.csv"


def buscar_dados_com_atalhos():
    print("--- Iniciando comunicação com o Pico usando ATALHOS ---")
    pico = None

    try:
        pico = serial.Serial(PORTA_PICO, 115200, timeout=1)
        print(f"Conectado a {PORTA_PICO}. Aguardando 2 segundos...")
        time.sleep(2)
    except serial.SerialException:
        print(f"--- ERRO CRÍTICO NA CONEXÃO ---")
        print(
            f"Não consegui abrir a porta {PORTA_PICO}. Verifique se a porta está correta e livre."
        )
        return

    # --- INICIO DA COMUNICAÇÃO USANDO SEUS COMANDOS ---

    # Passo 1: Enviar o atalho 'a' para montar o cartão SD
    print("Enviando atalho 'a' para montar o SD card...")
    pico.write(b"a")
    time.sleep(1)  # Dar um tempo para o SD montar

    # Limpa as mensagens de "Montando o SD..." que o Pico envia
    if pico.in_waiting > 0:
        pico.read(pico.in_waiting)
        print("Mensagens de montagem limpas.")

    # Passo 2: Enviar o atalho 'd' para ler o arquivo
    # No seu código C, o comando 'd' chama a função read_file(filename)
    # que lê o "adc_data1.csv" hardcoded. Perfeito!
    print("Enviando atalho 'd' para ler o conteúdo do arquivo...")
    pico.write(b"d")
    time.sleep(1)  # Dar um tempo para o Pico começar a enviar o arquivo

    # Passo 3: Ler a resposta (que será o conteúdo do arquivo)
    linhas_recebidas = []
    print("Aguardando e recebendo dados do arquivo...")
    while True:
        try:
            linha = pico.readline().decode("utf-8").strip()
        except UnicodeDecodeError:
            continue  # Ignora bytes estranhos

        if not linha:
            # Se a linha estiver vazia, o Pico provavelmente terminou
            break

        # Ignora as mensagens de status do seu programa C
        if (
            "Conteúdo do arquivo" in linha
            or "Leitura do arquivo" in linha
            or "Escolha o comando" in linha
        ):
            continue

        # Se a linha contiver o cabeçalho do CSV, começamos a salvar
        if "timestamp_us;ax;ay;az" in linha:
            linhas_recebidas.clear()  # Limpa qualquer lixo que possa ter vindo antes

        print(f"Recebido: {linha}")
        linhas_recebidas.append(linha)

    # Passo 4 (Opcional, mas boa prática): Enviar 'b' para desmontar
    print("Enviando atalho 'b' para desmontar o SD card...")
    pico.write(b"b")
    time.sleep(0.5)

    pico.close()
    print("-" * 40)
    print("Conexão com o Pico fechada.")

    # Passo 5: Salvar os dados coletados no PC
    if linhas_recebidas:
        print(f"Salvando dados recebidos em '{ARQUIVO_DESTINO}'...")
        with open(ARQUIVO_DESTINO, "w") as f:
            for linha in linhas_recebidas:
                f.write(linha + "\n")
        print("--- SUCESSO! Arquivo salvo no seu computador. ---")
    else:
        print(
            "--- FALHA: Nenhum dado de CSV foi recebido. Verifique se o arquivo existe no SD card. ---"
        )


# Roda a função principal
if __name__ == "__main__":
    buscar_dados_com_atalhos()
