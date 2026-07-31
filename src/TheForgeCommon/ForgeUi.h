#pragma once

// Ponte entre o casco da plataforma The-Forge (TheForgeWindowManager) e as
// cenas do jogo (extraida dos jogos de estudo — task 01 deste repo): o casco
// publica aqui o snapshot de input de cada update e o alvo de desenho de cada
// quadro; as cenas consomem via readKey()/isHeld()/drawText*() sem conhecer o
// casco. Teclado em fila (no maximo um evento consumido por input()) e
// desenho em modo imediato.
//
// O VOCABULARIO e o CONTRATO de teclado nao moram mais aqui: subiram para a
// `cengine::input` (task 20 da engine, 0.8.0) — eram a quarta copia do mesmo
// enum no ecossistema. Esta ponte guarda o que sempre foi dela: a CAPTURA (o
// WndProc traduzindo VK_* para Key) e a ergonomia global das cenas.
//
// Os aliases abaixo mantem `Key`/`KeyEvent` sem qualificacao no codigo das
// cenas — nenhum jogo precisou mudar uma linha.

#include <cstdint>
#include <string>

#include <cengine/input/Keyboard.hpp>
#include <cengine/input/Mouse.hpp>

// The-Forge (o include path do projeto aponta para a raiz do The-Forge).
#include "Common_3/Application/Interfaces/IFont.h"
#include "Common_3/Graphics/Interfaces/IGraphics.h"

// Space so aparece no estado SEGURADO (isHeld): na fila de edges o espaco chega
// como Key::Char com character ' ' (via WM_CHAR), como sempre foi.
using Key = cengine::input::Key;
using KeyEvent = cengine::input::KeyEvent;

namespace forgeui {

// O teclado da cengine por tras da fachada global — exposto para quem quiser
// falar com a porta direto (uma cena que receba `cengine::input::Keyboard&` em
// vez de chamar as funcoes globais).
[[nodiscard]] cengine::input::Keyboard& keyboard();

// O ponteiro por tras da fachada global, para quem quiser falar com a porta
// direto (uma cena que receba `cengine::input::Mouse&` em vez de chamar as
// funcoes globais). Mesmo desenho do `keyboard()`.
[[nodiscard]] cengine::input::Mouse& mouse();

// Paleta (ABGR, formato do FontDrawDesc::mFontColor): mesma intencao de cores
// das plataformas de terminal dos jogos de estudo (ciano para titulos, ambar
// para destaques).
namespace color {
inline constexpr uint32_t kTitle = 0xffffb300;   // ciano
inline constexpr uint32_t kAccent = 0xff00b3ff;  // ambar
inline constexpr uint32_t kValue = 0xff00d7ff;   // dourado (valores/stats)
inline constexpr uint32_t kSuccess = 0xff00c800; // verde
inline constexpr uint32_t kText = 0xffffffff;
inline constexpr uint32_t kDim = 0xff9a9a9a;
inline constexpr uint32_t kFaint = 0xff5a5a5a;
} // namespace color

// --- mouse ---
//
// O VOCABULARIO do mouse SUBIU para a `cengine::input` (task 27 / 0.14.0).
// Ele viveu aqui por dois jogos de proposito: o enum de teclas so subiu na
// quarta copia identica, e nao havia por que apressar o ponteiro. Subiu agora
// porque o SEGUNDO consumidor (tactics, degrau 06) usou a forma do primeiro
// (bulwark, degrau 05) sem mudar nada — que e o sinal mais forte de que a
// forma esta certa.
//
// A ponte guarda o que sempre foi dela: a CAPTURA (o WndProc traduzindo
// WM_MOUSEMOVE/WM_LBUTTONDOWN) e a ergonomia global das cenas. Os nomes abaixo
// continuam existindo como ALIAS — quem ja escrevia `forgeui::MouseClick` nao
// precisa mudar nada.

using MouseButton = cengine::input::MouseButton;
using MouseClick = cengine::input::MouseClick;

// --- arrastar ---
//
// O vocabulario do ARRASTAR mora AQUI, e nao na `cengine::input` — pelo mesmo
// criterio que segurou o mouse por dois jogos (task 27 da engine): ele tem UM
// consumidor (o Klondike, degrau 06) e nenhuma evidencia de que a forma abaixo
// seja a certa para o proximo. Se um segundo jogo arrastar, a comparacao
// decide; e ai o caminho e o mesmo que o teclado e o mouse fizeram.
//
// ## Por que arrastar nao cabe na fila de cliques
//
// Um clique e um EVENTO: aconteceu, tem uma posicao, acabou. Arrastar e um
// CICLO DE VIDA com duas pontas que carregam posicoes DIFERENTES — a origem
// diz O QUE se pega, o destino diz PARA ONDE vai — e um meio em que o jogo
// precisa desenhar o que esta na mao.
//
// Por isso sao duas leituras, como no teclado:
//
// - **estado** (`drag()`): "o botao esta segurado, comecou ali, esta aqui
//   agora". Serve para desenhar o que se arrasta, todo quadro.
// - **edge** (`readDrop()`): "soltou". Um evento por gesto fisico, com as duas
//   posicoes juntas.
//
// ## O que este casco NAO decide
//
// Se um gesto foi "clique" ou "arrasto". Ele entrega onde apertou e onde
// soltou; **quantos pixels de folga ainda contam como clique parado e politica
// do JOGO** — depende do tamanho do alvo e da tolerancia que o jogo quer ter.

/// O gesto em andamento. `active == false` quando nada esta sendo arrastado.
struct DragState
{
    bool  active = false;
    float startX = 0.0f; ///< onde o botao foi apertado
    float startY = 0.0f;
    float x = 0.0f; ///< onde o ponteiro esta agora
    float y = 0.0f;
};

/// Um gesto que TERMINOU: as duas pontas, juntas.
struct Drop
{
    bool  happened = false;
    float startX = 0.0f;
    float startY = 0.0f;
    float x = 0.0f;
    float y = 0.0f;
};

// --- ciclo de vida (chamado pelo casco da plataforma) ---

// Enfileira um evento de tecla vindo do WndProc do casco (WM_KEYDOWN/
// WM_CHAR): edges, um evento por aperto fisico.
void pushKey(KeyEvent event);

// Publica o estado segurado de UMA tecla (WM_KEYDOWN/WM_KEYUP rastreados
// pelo WndProc do casco). O estado persiste ate o proximo push da mesma
// tecla — o casco nao precisa republicar a cada quadro.
void pushHeldKey(Key key, bool held);

// Solta todas as teclas seguradas (perda de foco: o WM_KEYUP nunca chegara).
void clearHeldKeys();

// Publica a posicao do ponteiro (WM_MOUSEMOVE do casco), em pixels da area
// util.
void pushMousePosition(float x, float y);

// Enfileira um clique (WM_LBUTTONDOWN/WM_RBUTTONDOWN): edges, um evento por
// aperto — mesma promessa da fila de teclas.
void pushMouseClick(MouseClick click);

// Comeca um gesto de arrastar (WM_LBUTTONDOWN do casco).
void pushMouseDown(float x, float y);

// Termina o gesto e enfileira o `Drop` (WM_LBUTTONUP do casco).
void pushMouseUp(float x, float y);

// Cancela o gesto em andamento sem gerar `Drop`: a janela perdeu o foco, e o
// WM_LBUTTONUP pode nunca chegar. Irmao do `clearHeldKeys`.
void cancelDrag();

// Publica o alvo de desenho do quadro (chamado no update do casco, antes
// das fases do jogo).
void beginDraw(Cmd* cmd, float width, float height, uint32_t fontID);

// --- consumo pelas cenas ---

// Consome no maximo um evento de tecla por chamada (fila esvazia 1/quadro).
KeyEvent readKey();

// Estado continuo (teclas SEGURADAS): a fila de edges acima nao serve para
// movimento — mover uma nave exige saber se a tecla esta pressionada AGORA,
// todo quadro.
[[nodiscard]] bool isHeld(Key key);

// Conveniencia para eixos: -1 quando so `negative` esta segurada, +1 quando
// so `positive`, 0 nos demais casos. Ex.: heldAxis(Key::Left, Key::Right).
[[nodiscard]] float heldAxis(Key negative, Key positive);

// Onde o ponteiro esta AGORA, em pixels da area util. Estado continuo: serve
// para realce sob o cursor, que precisa da posicao todo quadro.
[[nodiscard]] float mouseX();
[[nodiscard]] float mouseY();

// Consome no maximo um clique por chamada (fila esvazia 1/quadro), igual ao
// `readKey`. Sem clique pendente, devolve `MouseButton::None`.
MouseClick readMouseClick();

// O gesto em andamento (estado continuo): serve para desenhar o que esta
// sendo arrastado, todo quadro.
[[nodiscard]] DragState drag();

// Consome no maximo um `Drop` por chamada. Sem gesto terminado, devolve
// `happened == false`.
Drop readDrop();

float screenWidth();
float screenHeight();

[[nodiscard]] float textWidth(const std::string& text, float fontSize);

// DOIS LIMITES medidos no Delve (2026-07-25, task 02 daquele repo), que
// custaram dois rascunhos errados e um bug que PARECIA certo. Valem para
// qualquer jogo que desenhe muita coisa em texto imediato:
//
// 1. **teto de chamadas por quadro.** ~143 chamadas de `drawText` num mesmo
//    quadro estouram o renderizador de fonte do The-Forge: aparecem glifos
//    "fantasma" acesos com a COR ERRADA em posicoes fixas da tela,
//    independentes do que o jogo pediu (reproduzido por screenshot). 108 e
//    115 chamadas passam; o ponto exato entre 115 e 143 nao foi medido. Uma
//    grade grande desenhada celula a celula chega la sem esforco — o Delve
//    tinha 13x11. Se precisar de muitos elementos, use o `forgesprite`: o
//    batcher desenha o lote inteiro em UMA chamada.
//
// 2. **a fonte NAO e monoespacada.** Uma string de N caracteres nao ocupa N
//    vezes a largura de um: no Delve o avanco do '#' era ~15px onde a grade
//    reservava 26. Agrupar tiles iguais numa string so (a saida obvia para o
//    limite 1) desalinha em silencio — a corrida de 10 paredes terminava
//    perto do pixel 150 em vez do 260, e o vao resultante era
//    indistinguivel de chao na tela enquanto o jogo o tratava como parede.
//    Para grade de passo fixo, a posicao tem que vir da aritmetica da grade,
//    nunca do avanco da fonte.
void drawText(const std::string& text, float x, float y, float fontSize, uint32_t colorAbgr);
void drawTextCentered(const std::string& text, float y, float fontSize, uint32_t colorAbgr);

// Rodape padrao com as dicas de tecla da cena.
void drawHints(const std::string& text);

} // namespace forgeui
