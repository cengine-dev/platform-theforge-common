#pragma once

// ForgeFrame — o casco deixa de ser opaco (task 09).
//
// Ate aqui o `TheForgeWindowManager` expunha cinco metodos de ciclo de vida
// (`init`, `update`, `present`, `shouldClose`, `cleanup`) e NADA MAIS. O
// `Renderer*`, o `Cmd*` do quadro e o render target viviam como estaticos de
// arquivo, invisiveis de fora.
//
// A consequencia foi MEDIDA, e ela fechou um projeto: o Vigil chegou a desenhar
// malha 3D sem ter onde passar uma matriz, entao a projecao TEVE que ser na CPU
// -- e dai desceu descarte de face na CPU, profundidade virando ordem do pintor
// e iluminacao virando um rig ajustado a mao. Nao foi escolha de design; foi
// consequencia de nao existir este cabecalho.
//
//   > Sem lugar onde passar uma matriz, a projecao TEM que ser na CPU.
//
// Este e o unico item do pipeline 3D que so ACRESCENTA. Ele existe para que os
// cinco seguintes (depth buffer, malha na GPU, shader com SRT, material,
// skinning) possam APAGAR.
//
// ## O contrato, e ele e curto
//
// **Quem liga render target assume o quadro inteiro; o casco nao restaura.**
//
// O casco liga o render target do swapchain no `update()` e o desliga no
// `present()`. Entre os dois, quem chamar `cmdBindRenderTargets` esta trocando o
// alvo para todo mundo -- inclusive para o `forgeui::drawText` e para os
// batchers, que desenham depois. Se voce trocar, volte a ligar o alvo do quadro
// antes de devolver o controle.
//
// Desenhar (pipeline proprio, `cmdDraw`) nao tem esse problema e e o uso
// pretendido.
//
// ## Para quem isto existe
//
// Para MODULOS do casco (o `forgemesh`, task 11) e para o consumidor que precise
// de um pipeline proprio -- que e exatamente o caso que hoje nao existe e que o
// `diorama` inaugura. Nao e uma porta de uso geral, e o jogo 2D nao tem motivo
// para tocar aqui: `forgeui`, `forgesprite` e `forgeline` continuam sendo a
// forma certa de desenhar sem saber o que e um `Cmd`.
//
// Precedente da casa: `forgeui::keyboard()` e `forgeui::mouse()` ja expoem a
// porta crua "para quem quiser falar direto", ao lado da fachada global. Mesma
// forma, mesma razao.
//
// ## Por que isto e SO um cabecalho (corrigido na 0.17.0)
//
// A 0.16.0 entregou este modulo como `.h` + `.cpp`, e afirmou por escrito que
// era "adicao pura, verificavel lendo o diff". **Era falso, e o diff mostrava:**
// o `TheForgeWindowManager.cpp` passou a chamar `forgeframe::publish` — e todo
// jogo do ecossistema lista aquele `.cpp` no `.vcxproj` mas nao listava este.
// Simbolo externo nao resolvido no link, em 8+ consumidores.
//
// O casco nao e uma biblioteca com alvo de build: cada jogo enumera os `.cpp`
// do casco no proprio `.vcxproj`. **Entao um `.cpp` NOVO nunca e aditivo** — e
// sempre uma edicao obrigatoria em todo projeto que ja existe.
//
// Dai o estado viver aqui em `inline` (variavel inline, C++17): uma instancia
// por programa, zero unidade de traducao nova, zero `.vcxproj` tocado.
//
// > A licao e sobre o metodo, nao sobre o modulo: eu li o diff procurando
// > simbolo REMOVIDO e assinatura MUDADA, e a quebra veio de uma referencia
// > ACRESCENTADA. Ler o diff so prova compatibilidade se a leitura incluir o
// > que o casco passou a exigir de quem o compila.

#include <cstdint>

// The-Forge (o include path do projeto aponta para a raiz do The-Forge).
#include "Common_3/Graphics/Interfaces/IGraphics.h"

namespace forgeframe {

/// O alvo do quadro, para quem precisa MONTAR um pipeline (o `addPipeline`
/// quer formato e amostragem) e para quem precisa da area util em pixels.
///
/// `depthFormat` e `TinyImageFormat_UNDEFINED` quando o consumidor nao ligou
/// `desc.depth.enabled` (task 10) -- que e o caso de todos os jogos 2D. Um
/// pipeline com `pDepthState` mas sem depth target nao desenha, entao consultar
/// este campo e a maneira certa de saber se da para pedir profundidade, em vez
/// de supor.
struct Target
{
    TinyImageFormat colorFormat = TinyImageFormat_UNDEFINED;
    TinyImageFormat depthFormat = TinyImageFormat_UNDEFINED;
    SampleCount     sampleCount = SAMPLE_COUNT_1;
    uint32_t        sampleQuality = 0;
    uint32_t        width = 0;
    uint32_t        height = 0;
};

// --- leitura (o consumidor) ---

/// O renderer do casco. Valido do `init()` ao `cleanup()`; NULL fora disso.
///
/// E o unico dos quatro que vale FORA do quadro -- e tem de ser, porque criar
/// shader, pipeline e buffer acontece na carga, e nao no meio do desenho.
[[nodiscard]] Renderer* renderer();

/// O command buffer do quadro em andamento. **NULL fora do quadro** (antes do
/// `update()` do casco e depois do `present()`).
///
/// O NULL nao e defensivo: e o unico jeito de um consumidor distinguir "estou no
/// meio do quadro" de "estou na carga". Gravar comando num `Cmd` fechado nao da
/// erro -- da corrupcao.
[[nodiscard]] Cmd* cmd();

/// O alvo do quadro. Valido junto com o `cmd()`.
[[nodiscard]] const Target& target();

/// Qual dos `frameCount()` trechos usar neste quadro. Quem tem buffer dinamico
/// (um por frame in flight) indexa por aqui -- mesma mecanica dos batchers.
[[nodiscard]] uint32_t frameIndex();

/// Quantos quadros em voo o casco mantem. Valido depois do `init()`.
[[nodiscard]] uint32_t frameCount();

// --- o passe de OVERLAY (task 10) ---

/// Fecha o passe 3D e abre o passe de overlay: **cor sem profundidade**,
/// carregando o que ja foi desenhado (`LOAD_ACTION_LOAD`). Idempotente no
/// quadro, e no-op quando nao ha depth buffer.
///
/// ## Por que isto precisa existir
///
/// Nao foi escolha de arquitetura -- foi imposicao do fontstash. O
/// `cmdDrawTextWithFont` desenha com `pPipelines[mText3D]`, e o pipeline 2D
/// (indice 0) e criado com `mDepthStencilFormat = UNDEFINED` **fixo**
/// (`FontSystem.cpp:440`). Nao ha desc que mude isso.
///
/// Ou seja: com um depth target ligado, o pipeline do texto declara um contrato
/// de anexos diferente do que esta ligado. O proprio The-Forge resolve assim --
/// `01_Transformations.cpp:1022` desliga o depth (`mDepthStencil = { NULL,
/// LOAD_ACTION_DONTCARE }`) logo antes de desenhar a UI. Este metodo e aquela
/// linha, automatizada.
///
/// **A consequencia, e ela e uma regra nova:** desenhado o primeiro pixel de
/// `forgeui`/`forgesprite`/`forgeline`, o quadro saiu do passe com profundidade
/// e nao volta. As tres pontes 2D sao **overlay**, e overlay vem por cima. Quem
/// quiser 3D com profundidade desenha antes -- que e a ordem que toda cena deste
/// ecossistema ja usa (mundo primeiro, HUD depois).
void enterOverlay();

/// true depois que o quadro trocou para o passe de overlay.
[[nodiscard]] bool inOverlay();

// --- ciclo de vida (chamado pelo casco da plataforma) ---

/// Publica o renderer e o numero de quadros em voo (fim do `init()` do casco).
void publish(Renderer* renderer, uint32_t frameCount);

/// Abre o quadro: `cmd`, alvo e indice do frame passam a valer.
///
/// `color` e o render target do swapchain e `depth` o de profundidade (NULL
/// quando desligado) -- o `ForgeFrame` guarda os dois porque e ele quem troca de
/// passe no `enterOverlay()`.
void beginFrame(Cmd* cmd, const Target& target, uint32_t frameIndex, RenderTarget* color, RenderTarget* depth);

/// Fecha o quadro: `cmd()` volta a ser NULL.
void endFrame();

/// Esquece o renderer (teardown do casco).
void unpublish();

namespace detalhe {

inline Renderer* gRenderer = NULL;
inline uint32_t  gFrameCount = 0;

// Estado do quadro em andamento. O `gCmd` volta a NULL no `endFrame()` de
// proposito -- ver o comentario do `cmd()` no cabecalho.
inline Cmd*                gCmd = NULL;
inline forgeframe::Target  gTarget = {};
inline uint32_t            gFrameIndex = 0;

// Os render targets do quadro, guardados para a troca de passe do
// `enterOverlay()`. `gDepth` NULL significa "sem profundidade" -- e ai a troca
// de passe nao existe, porque nao ha o que desligar.
inline RenderTarget* gColor = NULL;
inline RenderTarget* gDepth = NULL;
inline bool          gInOverlay = false;

} // namespace detalhe

inline Renderer* renderer() { return detalhe::gRenderer; }

inline Cmd* cmd() { return detalhe::gCmd; }

inline const Target& target() { return detalhe::gTarget; }

inline uint32_t frameIndex() { return detalhe::gFrameIndex; }

inline uint32_t frameCount() { return detalhe::gFrameCount; }

inline void publish(Renderer* renderer, const uint32_t frameCount)
{
    detalhe::gRenderer = renderer;
    detalhe::gFrameCount = frameCount;
}

inline void beginFrame(Cmd* cmd, const Target& target, const uint32_t frameIndex, RenderTarget* color, RenderTarget* depth)
{
    detalhe::gCmd = cmd;
    detalhe::gTarget = target;
    detalhe::gFrameIndex = frameIndex;
    detalhe::gColor = color;
    detalhe::gDepth = depth;
    detalhe::gInOverlay = false;
}

inline bool inOverlay() { return detalhe::gInOverlay; }

inline void enterOverlay()
{
    // Sem quadro, sem depth ou ja trocado: nada a fazer. O caso "sem depth" e o
    // dos 8+ jogos 2D, e por ele esta funcao custa uma comparacao por chamada.
    if (detalhe::gCmd == NULL || detalhe::gDepth == NULL || detalhe::gInOverlay)
    {
        return;
    }
    detalhe::gInOverlay = true;

    BindRenderTargetsDesc bind = {};
    bind.mRenderTargetCount = 1;
    // LOAD, e nao CLEAR: o que a cena 3D desenhou tem de continuar la.
    bind.mRenderTargets[0] = { detalhe::gColor, LOAD_ACTION_LOAD };
    bind.mDepthStencil = { NULL, LOAD_ACTION_DONTCARE };
    cmdBindRenderTargets(detalhe::gCmd, &bind);

    // Viewport e scissor se perdem no rebind -- sem estas duas linhas o overlay
    // desenha num retangulo indefinido, que costuma sair como "o texto sumiu".
    cmdSetViewport(detalhe::gCmd, 0.0f, 0.0f, (float)detalhe::gTarget.width, (float)detalhe::gTarget.height, 0.0f, 1.0f);
    cmdSetScissor(detalhe::gCmd, 0, 0, detalhe::gTarget.width, detalhe::gTarget.height);
}

inline void endFrame()
{
    detalhe::gCmd = NULL;
    detalhe::gColor = NULL;
    detalhe::gDepth = NULL;
    detalhe::gInOverlay = false;
    // O `gTarget` e o `gFrameIndex` NAO sao zerados: eles descrevem o alvo, que
    // e o mesmo do proximo quadro, e ha uso legitimo em ler o formato fora do
    // quadro (montar pipeline na carga). O que nao pode sobreviver ao quadro e
    // o `Cmd`, porque usa-lo depois nao da erro -- da corrupcao.
}

inline void unpublish()
{
    detalhe::gCmd = NULL;
    detalhe::gColor = NULL;
    detalhe::gDepth = NULL;
    detalhe::gInOverlay = false;
    detalhe::gRenderer = NULL;
    detalhe::gFrameCount = 0;
    detalhe::gTarget = {};
}

} // namespace forgeframe
