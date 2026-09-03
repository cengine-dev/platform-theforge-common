#pragma once

// O terceiro irmao das pontes de desenho (ForgeUi = texto, ForgeSpriteUi =
// quads texturizados): um batcher de LINHAS 2D.
//
// Por que linhas e nao sprites rotacionados: os arcades vetoriais (Asteroids,
// Lunar Lander, Tempest) sao desenhados a linha, nao a bitmap — e linha
// dispensa atlas: nao ha arte para produzir, so geometria. Um poligono girado
// e so uma lista de pontos girados; nao existe o problema de "sprite rotacionado
// fica serrilhado" porque nao existe sprite.
//
// Mesma mecanica do batcher de sprites: as cenas desenham em modo imediato, o
// lote acumula num vertex buffer dinamico (um trecho por frame in flight) e sai
// UM draw call. O ciclo de vida (init/load/begin/flush) e do casco da
// plataforma; as cenas so enxergam drawLine/drawPolyline.
//
// Camadas 2D = ordem de chamada: o forgeui::drawText da flush no lote pendente
// antes de gravar texto, entao "linhas primeiro, texto depois" dentro do draw()
// da cena poe o texto por cima.

#include <cstdint>

// The-Forge (o include path do projeto aponta para a raiz do The-Forge).
#include "Common_3/Graphics/Interfaces/IGraphics.h"

namespace forgeline {

/// Ponto em PIXELS de tela (origem no canto superior esquerdo) — a mesma
/// convencao do forgeui::drawText. A projecao para NDC acontece aqui dentro.
struct Point
{
    float x = 0.0f;
    float y = 0.0f;
};

// Configuracao do batcher — fornecida pelo jogo na montagem do casco.
struct LineBatcherDesc
{
    // false DESLIGA o batcher: todas as funcoes viram no-op (jogo que nao
    // desenha linha nao paga pelo pipeline). Mesmo espirito do atlasPath nulo
    // do forgesprite.
    bool enabled = false;

    // Capacidade do lote por quadro; estourou, dropa a linha e loga uma vez.
    uint32_t maxLines = 4096;

    // Frames in flight do casco (trechos do vertex buffer dinamico).
    uint32_t frameCount = 2;
};

// Contadores do quadro ANTERIOR (fechados no begin() seguinte).
struct Stats
{
    uint32_t triangles = 0;
    uint32_t lines = 0;
    uint32_t drawCalls = 0;

    // **O que foi DESCARTADO por lote cheio, e o teto contra o qual medir.**
    //
    // Eles entraram com o Vigil em malha (degrau 24), e o motivo e um defeito de
    // categoria: o estouro sempre existiu, mas so se manifestava como um `LOGF`
    // uma vez por quadro. **Corpo que some da tela sem erro visivel** e o pior
    // jeito de errar -- quem esta jogando conclui que o bicho morreu.
    //
    // O teto vem junto porque **nenhum consumidor pode saber qual e** sem copiar
    // o `maxLines` que ele mesmo passou na montagem, e numero copiado e numero
    // que desencontra. Quem sabe o teto e o batcher.
    //
    // `dropped` conta PRIMITIVAS perdidas (linha ou triangulo), nao vertices: e o
    // numero que responde "sumiu alguma coisa?", que e a pergunta que importa.
    uint32_t dropped = 0;
    uint32_t vertices = 0;
    uint32_t vertexCapacity = 0;
};

// --- ciclo de vida (chamado pelo casco da plataforma) ---

void init(Renderer* renderer, const LineBatcherDesc& desc);
void exit();

void load(const ReloadDesc* reloadDesc, TinyImageFormat colorFormat, SampleCount sampleCount, uint32_t sampleQuality);
void unload(const ReloadDesc* reloadDesc);

void begin(Cmd* cmd, float width, float height, uint32_t frameIndex);

// Desenha o lote acumulado (1 draw call) e reinicia a acumulacao. Chamado pelo
// forgeui::drawText (linhas pendentes ficam SOB o texto) e pelo casco no fim do
// quadro. No-op com lote vazio.
void flush();

// --- consumo pelas cenas ---

/// Um segmento, em pixels, na cor ABGR (mesmo formato do forgeui::color).
void drawLine(Point from, Point to, uint32_t colorAbgr);

/// Uma sequencia de pontos ligados. `closed` fecha o ultimo no primeiro — e o
/// que desenha um poligono (a nave, uma rocha) numa chamada so.
void drawPolyline(const Point* points, uint32_t count, bool closed, uint32_t colorAbgr);

// **UM TRIANGULO CHEIO, no mesmo lote das linhas.**
//
// Ele entrou com o Vigil (degrau 24), e a razao nao e acabamento: wireframe e
// TRANSPARENTE. Com trinta corpos na tela, trinta contornos se sobrepoem e nenhum
// esconde o outro -- a leitura morre. Superficie cheia devolve a informacao mais
// basica de uma cena com profundidade: **quem esta na frente de quem.**
//
// Ele reaproveita tudo o que a linha ja tem -- mesmo buffer, mesmos shaders, mesmo
// vertice (posicao em NDC e cor). O que muda e a topologia, e por isso e um segundo
// pipeline e nao um modulo novo.
//
// **A ordem de chamada continua sendo a profundidade.** Nao ha depth buffer: o que
// se desenha depois cobre o que veio antes, e cabe a cena desenhar do fundo para a
// frente. Alternar triangulo e linha custa um `draw call` a cada troca, porque o
// lote pendente e de uma topologia so.
//
// **Sem descarte de face no pipeline.** Quem chama ja sabe se a face esta virada
// para a camera -- e depois de uma projecao feita na CPU, a orientacao do triangulo
// na tela e um dado que a cena tem e o pipeline teria de adivinhar.
void drawTriangle(Point a, Point b, Point c, uint32_t colorAbgr);

// **O mesmo triangulo, com UMA COR POR VERTICE.**
//
// O vertice do batcher sempre carregou cor propria -- a versao de uma cor so a
// repetia tres vezes. Expor isso e o que permite ao chamador sombrear por VERTICE
// e deixar o rasterizador interpolar, em vez de sombrear por face.
//
// A diferenca nao e sutil: face chapada mostra a mesma malha como um poliedro,
// e sombreado interpolado mostra a superficie que o autor modelou. Trazido pelo
// `vigil` quando as normais por vertice do proprio arquivo passaram a ser lidas.
void drawTriangle(Point a, Point b, Point c, uint32_t colorA, uint32_t colorB, uint32_t colorC);

Stats lastFrameStats();

} // namespace forgeline
