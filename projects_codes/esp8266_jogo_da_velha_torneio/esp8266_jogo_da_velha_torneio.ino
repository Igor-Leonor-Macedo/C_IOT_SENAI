/*
 * ============================================================
 *  TORNEIO DE JOGO DA VELHA — ESP8266 (modo Station)
 * ============================================================
 *  O ESP conecta no seu modem/roteador. Os alunos acessam
 *  http://IP_DO_ESP no navegador do celular, registram o nome
 *  e disputam um torneio eliminatório com pontuação crescente.
 *
 *  Bibliotecas necessárias (Gerenciador de Bibliotecas):
 *   - "WebSockets" (Markus Sattler / Links2004)
 *  (ESP8266WiFi, ESP8266WebServer e ESP8266mDNS já vêm com o core)
 *
 *  Pontuação: vitória = rodada x 10 pts (R1=10, semi=20, final=30)
 *  Empate ("deu velha") = replay, invertendo quem começa.
 *  Número ímpar de jogadores: o último da chave recebe "bye".
 * ============================================================
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WebSocketsServer.h>

// ---------- CONFIGURE AQUI ----------
const char* WIFI_SSID = "NOME_DA_SUA_REDE";
const char* WIFI_PASS = "SENHA_DA_SUA_REDE";
const char* ADMIN_PIN = "1234";          // PIN do professor
// ------------------------------------

#define MAX_P 8                          // limite prático do ESP8266

ESP8266WebServer server(80);
WebSocketsServer ws(81);

// ---------- Estado dos jogadores ----------
struct Player {
  char    name[13];   // até 12 caracteres
  int     score;
  uint8_t wsId;       // 255 = desconectado
  bool    used;
};
Player players[MAX_P];

// ---------- Estado do torneio ----------
// phase: 0 = lobby, 1 = torneio, 2 = encerrado
uint8_t phase = 0;
uint8_t alive[MAX_P];     uint8_t aliveCount = 0;   // jogadores da rodada atual
uint8_t winners[MAX_P];   uint8_t winnersCount = 0; // classificados
uint8_t matchPos = 0;                               // par atual dentro da rodada
uint8_t roundNum = 1;

// ---------- Partida atual ----------
// mState: 0 = sem partida, 1 = jogando, 2 = exibindo resultado
uint8_t mState = 0;
uint8_t pA = 0, pB = 0;       // índices dos jogadores da partida
uint8_t turnP = 0;            // de quem é a vez
uint8_t starterP = 0;         // quem começou (para inverter no replay)
char    board[10] = ".........";
int8_t  lastWinner = -2;      // -2 nada, -1 empate, >=0 vencedor
int8_t  champion = -1;
unsigned long resultUntil = 0;
bool    replayPending = false;

const uint8_t WINS[8][3] = {
  {0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}
};

// ============================================================
//  PÁGINA WEB (fica na flash via PROGMEM, não gasta RAM)
// ============================================================
const char PAGE[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="pt-BR"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Torneio da Velha</title>
<style>
:root{
  --bg:#0b0f1a; --panel:#121a2b; --line:#22304d;
  --x:#ff4d6d; --o:#38e8ff; --gold:#ffd166; --txt:#dbe4ff; --dim:#7a89ad;
}
*{box-sizing:border-box;margin:0;padding:0}
body{background:var(--bg);color:var(--txt);font-family:'Courier New',monospace;
  min-height:100vh;display:flex;flex-direction:column;align-items:center;padding:14px}
h1{font-size:1.15rem;letter-spacing:.35em;text-transform:uppercase;margin:8px 0 2px;text-align:center}
h1 b{color:var(--x)} h1 i{color:var(--o);font-style:normal}
#sub{color:var(--dim);font-size:.72rem;letter-spacing:.2em;text-transform:uppercase;margin-bottom:14px}
.card{background:var(--panel);border:1px solid var(--line);border-radius:10px;
  padding:16px;width:100%;max-width:420px;margin-bottom:12px}
input,button{font-family:inherit;font-size:1rem}
input{width:100%;padding:10px;background:#0d1422;border:1px solid var(--line);
  border-radius:6px;color:var(--txt);margin-bottom:10px}
button{width:100%;padding:11px;border:0;border-radius:6px;cursor:pointer;
  background:var(--o);color:#04222b;font-weight:bold;letter-spacing:.12em;text-transform:uppercase}
button.sec{background:transparent;border:1px solid var(--line);color:var(--dim);margin-top:8px}
#msg{color:var(--x);font-size:.8rem;min-height:1.1em;margin-bottom:6px;text-align:center}
#status{text-align:center;font-size:.95rem;margin:6px 0 12px;min-height:1.3em}
#stage{color:var(--gold);letter-spacing:.25em;text-transform:uppercase;font-size:.8rem;text-align:center;margin-bottom:4px}
#vs{text-align:center;font-size:1rem;margin-bottom:10px}
#vs .nA{color:var(--x);font-weight:bold} #vs .nB{color:var(--o);font-weight:bold}
#vs .turn{text-decoration:underline}
#grid{display:grid;grid-template-columns:repeat(3,88px);grid-template-rows:repeat(3,88px);
  gap:6px;justify-content:center;margin:0 auto 6px}
.cell{background:#0d1422;border:1px solid var(--line);border-radius:8px;
  display:flex;align-items:center;justify-content:center;font-size:2.6rem;font-weight:bold;
  cursor:default;user-select:none;transition:box-shadow .15s}
.cell.free.my{cursor:pointer}
.cell.free.my:hover{box-shadow:0 0 0 2px var(--o) inset}
.cell.X{color:var(--x);text-shadow:0 0 12px var(--x)}
.cell.O{color:var(--o);text-shadow:0 0 12px var(--o)}
#rank td{padding:4px 6px;font-size:.9rem}
#rank tr.me td{color:var(--gold)}
#rank td.pts{text-align:right;color:var(--gold)}
#rank td.out{color:var(--dim);text-decoration:line-through}
table{width:100%;border-collapse:collapse}
.lbl{color:var(--dim);font-size:.72rem;letter-spacing:.2em;text-transform:uppercase;margin-bottom:8px}
#champ{text-align:center;font-size:1.3rem;color:var(--gold);
  text-shadow:0 0 18px var(--gold);margin:10px 0;letter-spacing:.1em}
details{margin-top:4px} summary{color:var(--dim);font-size:.75rem;cursor:pointer;
  letter-spacing:.15em;text-transform:uppercase}
.hide{display:none}
@media(max-width:360px){#grid{grid-template-columns:repeat(3,76px);grid-template-rows:repeat(3,76px)}}
</style></head><body>
<h1><b>X</b> Torneio da Velha <i>O</i></h1>
<div id="sub">aula de programacao em jogos</div>

<div class="card" id="sJoin">
  <div class="lbl">Registro do jogador</div>
  <div id="msg"></div>
  <input id="nome" maxlength="12" placeholder="Seu nome (max 12)">
  <button onclick="reg()">Entrar no torneio</button>
  <button class="sec" onclick="telaoOn()">Modo telão (projetor)</button>
</div>

<div class="card hide" id="sMain">
  <div id="stage"></div>
  <div id="vs"></div>
  <div id="grid"></div>
  <div id="status"></div>
  <div id="champ" class="hide"></div>
</div>

<div class="card">
  <div class="lbl">Ranking</div>
  <table id="rank"></table>
</div>

<div class="card">
  <details><summary>Área do professor</summary>
    <input id="pin" type="password" placeholder="PIN" style="margin-top:8px">
    <button id="btnStart" onclick="cmd('START')">Iniciar torneio</button>
    <button class="sec" onclick="cmd('RESET')">Novo torneio (zera tudo)</button>
  </details>
</div>

<script>
let ws, st=null, myIdx=-1, telao=false;

function connect(){
  ws=new WebSocket('ws://'+location.hostname+':81/');
  ws.onopen=()=>{const n=localStorage.getItem('velhaNome');
    if(n && !telao) ws.send('REG|'+n);};
  ws.onclose=()=>setTimeout(connect,1500);
  ws.onmessage=e=>{
    const j=JSON.parse(e.data);
    if(j.a=='id'){myIdx=j.i; document.getElementById('msg').textContent='';}
    if(j.a=='err'){myIdx=-1; localStorage.removeItem('velhaNome');
      document.getElementById('msg').textContent=j.m;}
    if(j.a=='st'){st=j; render();}
  };
}
function reg(){
  const n=document.getElementById('nome').value.trim();
  if(!n) return;
  localStorage.setItem('velhaNome',n);
  ws.send('REG|'+n);
}
function telaoOn(){telao=true; render();}
function cmd(c){ws.send(c+'|'+document.getElementById('pin').value);}
function mv(i){
  if(st && st.ph==1 && st.ms==1 && myIdx==st.mt && st.bd[i]=='.')
    ws.send('MV|'+i);
}
function stageName(n){return n==2?'FINAL':(n<=4?'SEMIFINAL':'RODADA '+st.rnd);}

function render(){
  if(!st) return;
  const joined = myIdx>=0;
  document.getElementById('sJoin').classList.toggle('hide', joined||telao);
  document.getElementById('sMain').classList.toggle('hide', !(joined||telao));
  document.getElementById('btnStart').classList.toggle('hide', st.ph!=0);

  // ranking
  const rk=document.getElementById('rank');
  const idx=st.pl.map((p,i)=>i).sort((a,b)=>st.pl[b][1]-st.pl[a][1]);
  rk.innerHTML = st.pl.length? idx.map(i=>{
    const p=st.pl[i], out=(st.ph>0 && !p[2]);
    return '<tr class="'+(i==myIdx?'me':'')+'"><td class="'+(out?'out':'')+'">'
      +p[0]+'</td><td class="pts">'+p[1]+' pts</td></tr>';
  }).join('') : '<tr><td style="color:var(--dim)">ninguém ainda…</td></tr>';

  const stage=document.getElementById('stage'), vs=document.getElementById('vs'),
        grid=document.getElementById('grid'), status=document.getElementById('status'),
        champ=document.getElementById('champ');

  if(st.ph==0){
    stage.textContent='LOBBY';
    vs.innerHTML=''; grid.innerHTML='';
    status.textContent=st.pl.length<2 ? 'Aguardando jogadores…'
      : st.pl.length+' jogadores prontos. Professor pode iniciar!';
    champ.classList.add('hide');
    return;
  }
  if(st.ph==2){
    stage.textContent='TORNEIO ENCERRADO';
    vs.innerHTML=''; grid.innerHTML='';
    status.textContent='';
    champ.classList.remove('hide');
    champ.textContent='🏆 CAMPEÃO: '+st.pl[st.ch][0]+' 🏆';
    return;
  }
  // ph==1 — torneio rodando
  champ.classList.add('hide');
  stage.textContent=stageName(st.na);
  const A=st.pl[st.ma][0], B=st.pl[st.mb][0];
  vs.innerHTML='<span class="nA '+(st.mt==st.ma?'turn':'')+'">'+A+' (X)</span>'
    +' &nbsp;vs&nbsp; <span class="nB '+(st.mt==st.mb?'turn':'')+'">'+B+' (O)</span>';
  grid.innerHTML=[...st.bd].map((c,i)=>{
    const my=(st.ms==1 && myIdx==st.mt);
    return '<div class="cell '+(c=='.'?'free':'')+' '+(c!='.'?c:'')+' '+(my?'my':'')
      +'" onclick="mv('+i+')">'+(c=='.'?'':c)+'</div>';
  }).join('');
  if(st.ms==2){
    status.textContent = st.w==-1 ? '😮 Deu velha! Replay já já…'
      : '⭐ '+st.pl[st.w][0]+' venceu! +'+(st.rnd*10)+' pts';
  } else if(myIdx==st.ma||myIdx==st.mb){
    status.textContent = myIdx==st.mt ? '👉 SUA VEZ!' : 'Aguarde a jogada…';
  } else {
    status.textContent='Vez de '+st.pl[st.mt][0];
  }
}
connect();
</script></body></html>)rawliteral";

// ============================================================
//  FUNÇÕES DO SERVIDOR
// ============================================================

int findByName(const char* n) {
  for (int i = 0; i < MAX_P; i++)
    if (players[i].used && strcasecmp(players[i].name, n) == 0) return i;
  return -1;
}
int findByWs(uint8_t id) {
  for (int i = 0; i < MAX_P; i++)
    if (players[i].used && players[i].wsId == id) return i;
  return -1;
}
int playerCount() {
  int c = 0;
  for (int i = 0; i < MAX_P; i++) if (players[i].used) c++;
  return c;
}
bool isAlive(uint8_t idx) {
  for (int i = 0; i < aliveCount; i++) if (alive[i] == idx) return true;
  for (int i = 0; i < winnersCount; i++) if (winners[i] == idx) return true;
  return false;
}

// Monta e transmite o estado para todos os clientes
void sendState() {
  String s = F("{\"a\":\"st\",\"ph\":");
  s += phase;
  s += F(",\"rnd\":"); s += roundNum;
  s += F(",\"na\":");  s += aliveCount;           // p/ nomear a fase (final, semi…)
  s += F(",\"pl\":[");
  bool first = true;
  for (int i = 0; i < MAX_P; i++) {
    if (!players[i].used) continue;
    if (!first) s += ',';
    first = false;
    s += F("[\""); s += players[i].name; s += F("\",");
    s += players[i].score; s += ',';
    s += (phase == 0 || isAlive(i)) ? 1 : 0;
    s += ']';
  }
  s += F("],\"ma\":"); s += pA;
  s += F(",\"mb\":");  s += pB;
  s += F(",\"mt\":");  s += turnP;
  s += F(",\"ms\":");  s += mState;
  s += F(",\"w\":");   s += lastWinner;
  s += F(",\"ch\":");  s += champion;
  s += F(",\"bd\":\""); s += board; s += F("\"}");
  ws.broadcastTXT(s);
}

void sendId(uint8_t num, int idx) {
  String s = F("{\"a\":\"id\",\"i\":");
  s += idx; s += '}';
  ws.sendTXT(num, s);
}
void sendErr(uint8_t num, const __FlashStringHelper* m) {
  String s = F("{\"a\":\"err\",\"m\":\"");
  s += m; s += F("\"}");
  ws.sendTXT(num, s);
}

// Inicia a partida do par atual (resolve "byes" automaticamente)
void startNextMatch() {
  // bye: jogador sem par avança direto
  while (matchPos < aliveCount && matchPos + 1 >= aliveCount) {
    winners[winnersCount++] = alive[matchPos];
    matchPos += 2;
  }
  if (matchPos >= aliveCount) {           // rodada terminou
    memcpy(alive, winners, winnersCount);
    aliveCount = winnersCount;
    winnersCount = 0;
    matchPos = 0;
    if (aliveCount == 1) {                // temos um campeão!
      phase = 2;
      champion = alive[0];
      mState = 0;
      sendState();
      return;
    }
    roundNum++;
    startNextMatch();
    return;
  }
  pA = alive[matchPos];
  pB = alive[matchPos + 1];
  starterP = pA;
  turnP = starterP;
  strcpy(board, ".........");
  lastWinner = -2;
  mState = 1;
  sendState();
}

void handleMove(int idx, int cell) {
  if (phase != 1 || mState != 1) return;
  if (idx != turnP) return;
  if (cell < 0 || cell > 8 || board[cell] != '.') return;

  board[cell] = (idx == pA) ? 'X' : 'O';

  // vitória?
  for (int w = 0; w < 8; w++) {
    char a = board[WINS[w][0]];
    if (a != '.' && a == board[WINS[w][1]] && a == board[WINS[w][2]]) {
      lastWinner = idx;
      players[idx].score += roundNum * 10;   // pontuação cresce por rodada
      winners[winnersCount++] = idx;
      mState = 2;
      replayPending = false;
      resultUntil = millis() + 3500;
      sendState();
      return;
    }
  }
  // empate ("deu velha")?
  if (!strchr(board, '.')) {
    lastWinner = -1;
    mState = 2;
    replayPending = true;                    // replay invertendo quem começa
    resultUntil = millis() + 3000;
    sendState();
    return;
  }
  turnP = (turnP == pA) ? pB : pA;
  sendState();
}

void resetAll() {
  for (int i = 0; i < MAX_P; i++) players[i].used = false;
  phase = 0; roundNum = 1; aliveCount = 0; winnersCount = 0;
  matchPos = 0; mState = 0; champion = -1; lastWinner = -2;
  strcpy(board, ".........");
  sendState();
}

// ============================================================
//  EVENTOS WEBSOCKET
// ============================================================
void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t len) {
  if (type == WStype_DISCONNECTED) {
    int i = findByWs(num);
    if (i >= 0) players[i].wsId = 255;       // permite reconectar depois
    return;
  }
  if (type == WStype_CONNECTED) { sendState(); return; }
  if (type != WStype_TEXT) return;

  String msg((char*)payload);
  int sep = msg.indexOf('|');
  String cmd = (sep < 0) ? msg : msg.substring(0, sep);
  String arg = (sep < 0) ? ""  : msg.substring(sep + 1);

  // ---------- REGISTRO / RECONEXÃO ----------
  if (cmd == "REG") {
    arg.trim();
    arg.replace("\"", ""); arg.replace("\\", ""); arg.replace("|", "");
    if (arg.length() == 0 || arg.length() > 12) {
      sendErr(num, F("Nome invalido (1 a 12 caracteres).")); return;
    }
    int ex = findByName(arg.c_str());
    if (ex >= 0) {                            // nome já existe
      if (players[ex].wsId == 255) {          // dono caiu → reconecta
        players[ex].wsId = num;
        sendId(num, ex); sendState();
      } else sendErr(num, F("Nome ja esta em uso."));
      return;
    }
    if (phase != 0) { sendErr(num, F("Torneio em andamento. Aguarde o proximo!")); return; }
    if (playerCount() >= MAX_P) { sendErr(num, F("Sala cheia (max 8).")); return; }
    for (int i = 0; i < MAX_P; i++) {
      if (!players[i].used) {
        players[i].used = true;
        players[i].score = 0;
        players[i].wsId = num;
        strncpy(players[i].name, arg.c_str(), 12);
        players[i].name[12] = 0;
        sendId(num, i); sendState();
        return;
      }
    }
  }
  // ---------- COMANDOS DO PROFESSOR ----------
  else if (cmd == "START") {
    if (arg != ADMIN_PIN) { sendErr(num, F("PIN incorreto.")); return; }
    if (phase != 0 || playerCount() < 2) return;
    aliveCount = 0;
    for (int i = 0; i < MAX_P; i++)
      if (players[i].used) alive[aliveCount++] = i;
    winnersCount = 0; matchPos = 0; roundNum = 1; phase = 1; champion = -1;
    startNextMatch();
  }
  else if (cmd == "RESET") {
    if (arg != ADMIN_PIN) { sendErr(num, F("PIN incorreto.")); return; }
    resetAll();
  }
  // ---------- JOGADA ----------
  else if (cmd == "MV") {
    int i = findByWs(num);
    if (i >= 0) handleMove(i, arg.toInt());
  }
}

// ============================================================
//  SETUP / LOOP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(100);
  strcpy(board, ".........");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print(F("\nConectando ao WiFi"));
  while (WiFi.status() != WL_CONNECTED) { delay(400); Serial.print('.'); }

  Serial.println();
  Serial.println(F("=============================================="));
  Serial.print(F("  Alunos devem acessar:  http://"));
  Serial.println(WiFi.localIP());
  Serial.println(F("  (ou http://velha.local em alguns aparelhos)"));
  Serial.println(F("=============================================="));

  MDNS.begin("velha");

  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.begin();

  ws.begin();
  ws.onEvent(onWsEvent);
}

void loop() {
  server.handleClient();
  ws.loop();
  MDNS.update();

  // avança automaticamente após exibir o resultado da partida
  if (mState == 2 && millis() > resultUntil) {
    if (replayPending) {                      // empate → replay
      replayPending = false;
      starterP = (starterP == pA) ? pB : pA;  // inverte quem começa
      turnP = starterP;
      strcpy(board, ".........");
      lastWinner = -2;
      mState = 1;
      sendState();
    } else {                                  // vitória → próxima partida
      matchPos += 2;
      startNextMatch();
    }
  }
}
