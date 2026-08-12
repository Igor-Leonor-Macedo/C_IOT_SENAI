# Torneio de Jogo da Velha — ESP8266

Servidor de torneio eliminatório de jogo da velha rodando inteiro num ESP8266
conectado ao modem da sala. Os alunos jogam pelo navegador do celular.

## Como funciona

1. O ESP conecta no seu roteador (modo Station) e imprime o IP no Serial Monitor.
2. Alunos entram na mesma rede WiFi e acessam `http://IP_DO_ESP` no celular.
3. Cada um se registra com um nome (máx. 12 caracteres, até 8 jogadores).
4. O professor abre "Área do professor" na página, digita o PIN e clica
   em **Iniciar torneio**.
5. O ESP monta a chave eliminatória: as partidas rodam **uma de cada vez** e
   todo mundo assiste ao vivo. Se o número de jogadores for ímpar, o último
   da chave avança direto ("bye").
6. Pontuação: vitória vale `rodada × 10` pontos (rodada 1 = 10, semifinal = 20,
   final = 30). Empate ("deu velha") gera replay invertendo quem começa.
7. Ao final aparece a tela de campeão. **Novo torneio** (com PIN) zera tudo.

Dica de aula: abra a página num computador ligado ao projetor e clique em
**Modo telão** — a turma inteira acompanha a chave, o tabuleiro e o ranking.

## Preparando o Arduino IDE

1. Instale o core do ESP8266 (Preferências → URLs adicionais →
   `http://arduino.esp8266.com/stable/package_esp8266com_index.json`).
2. No Gerenciador de Bibliotecas, instale **"WebSockets" de Markus Sattler**
   (também aparece como Links2004/arduinoWebSockets).
3. Abra `jogo_da_velha_torneio.ino` e edite no topo:
   - `WIFI_SSID` e `WIFI_PASS` → rede do seu modem
   - `ADMIN_PIN` → PIN do professor (padrão `1234` — troque!)
4. Placa: NodeMCU 1.0 ou LOLIN(WEMOS) D1 mini. Grave e abra o Serial
   Monitor em **115200** para ver o IP.

## Dicas e limites

- **IP fixo**: no painel do modem, reserve o IP do ESP (DHCP reservation)
  para não mudar entre as aulas. Escreva o IP no quadro.
- `http://velha.local` funciona em iPhone/Mac/Windows recentes; a maioria dos
  Android não resolve mDNS — use o IP direto com eles.
- Limite de **8 jogadores** por ESP (RAM e estabilidade do WebSocket). Turma
  grande? Use vários ESPs, um por grupo, cada um com seu IP.
- Se um aluno cair da rede, basta acessar de novo e entrar **com o mesmo
  nome** — a vaga dele é recuperada e a partida continua de onde parou.
- O jogo espera indefinidamente pela jogada; se alguém sumir de vez, o
  professor pode usar "Novo torneio" para destravar.

## Ideias de extensão para os alunos

- Placar persistente entre aulas (salvar em LittleFS/EEPROM)
- Timer de jogada com derrota por W.O.
- Partidas simultâneas em vez de sequenciais
- Sons e animação de vitória no cliente (só mexe no HTML/JS!)
- Migrar para ESP32 e subir o limite de jogadores
