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

float screenWidth();
float screenHeight();

[[nodiscard]] float textWidth(const std::string& text, float fontSize);

void drawText(const std::string& text, float x, float y, float fontSize, uint32_t colorAbgr);
void drawTextCentered(const std::string& text, float y, float fontSize, uint32_t colorAbgr);

// Rodape padrao com as dicas de tecla da cena.
void drawHints(const std::string& text);

} // namespace forgeui
