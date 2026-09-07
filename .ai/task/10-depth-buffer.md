# 10 - Depth buffer opt-in

- **Status:** **done (0.17.0)** — 2026-09-05
- **Categoria:** Plataforma
- **Registrada em:** 2026-09-03
- **Depende de:** nada (mas so tem consumidor depois da task 11)

## O problema

Hoje o casco liga o render target do quadro **sem profundidade nenhuma**:

```cpp
// TheForgeWindowManager.cpp:498
bindRenderTargets.mDepthStencil = { NULL, LOAD_ACTION_DONTCARE };
```

Com um `NULL` ali, oclusao vira **ordem do pintor** -- ordenar os corpos por
distancia e desenhar de tras para frente. O Vigil fez exatamente isso, e mediu o
custo: **6,8% dos pixels do modelo `prisao` saem errados**, porque a ordem do
pintor nao resolve interpenetracao nem malha que se dobra sobre si mesma.

Nao ha conserto por ordenacao mais esperta. Ordem do pintor e a aproximacao
errada; o depth buffer e a resposta.

## Escopo

`TheForgeWindowDesc` ganha um bloco, no mesmo espirito de `sprites` e `lines`:

```cpp
struct DepthDesc
{
    // false DESLIGA: nenhum render target de profundidade e criado, o
    // mDepthStencil segue NULL, e o jogo 2D nao paga por memoria de video
    // que nao usa. Mesmo espirito do atlasPath nulo do forgesprite e do
    // enabled do forgeline.
    bool  enabled = false;
    float clearDepth = 1.0f;
};
```

O que muda no casco, e so isso:

1. `addGameSwapChain` passa a criar tambem o depth RT quando ligado (mesmas
   dimensoes, `D32_SFLOAT`);
2. `applyPendingResize` recria os **dois** juntos -- um resize que recrie so o
   swapchain deixa o depth com o tamanho velho, e isso e um defeito silencioso
   classico;
3. `mDepthStencil` recebe o RT com `LOAD_ACTION_CLEAR`;
4. barreira do depth RT no `beginCmd`;
5. `forgeframe::target().depthFormat` (task 09) passa a responder de verdade.

## Criterios de Aceite

1. Jogo com `depth.enabled = false` (os 8+ jogos 2D): **zero mudanca** de
   comportamento, e nenhum RT de profundidade alocado.
2. Jogo com `enabled = true`: duas malhas que se interpenetram desenham certo em
   **qualquer ordem de chamada** -- e essa e a prova de que o buffer esta vivo.
3. Redimensionar a janela nao quebra a profundidade (o teste que pega o item 2
   da lista acima).

## O que a spec nao previu, e foi medido montando

**1. A barreira do item 4 nao existe.** A spec pedia "barreira do depth RT no
`beginCmd`". Nao ha: o render target nasce em `RESOURCE_STATE_DEPTH_WRITE`
(`mStartState`) e nada o tira de la, entao nao ha transicao a fazer. E o que o
`01_Transformations` do The-Forge faz — ele so barreira o render target de cor.

**2. Apareceu um item 6 que vale mais que os cinco: o passe de OVERLAY.**

O `cmdDrawTextWithFont` desenha com `pPipelines[mText3D]`, e o pipeline 2D
(indice 0) do fontstash e criado com `mDepthStencilFormat = UNDEFINED` **fixo**
(`FontSystem.cpp:440`). Nenhum desc muda isso — `FontSystemLoadDesc.mDepthFormat`
so alimenta o pipeline de indice 1, o de texto 3D.

Ou seja: **ligar profundidade quebraria o texto**, que e criterio de aceite do
degrau 02 do diorama. O proprio The-Forge resolve desligando o depth antes de
desenhar a UI (`01_Transformations.cpp:1022`). O `forgeframe::enterOverlay()` e
aquela linha, automatizada e chamada pelas tres pontes 2D.

> A spec da task falava em "criar o RT, ligar, redimensionar". O que ela nao
> tinha como prever e que **o depth buffer nao e uma propriedade do quadro, e
> uma propriedade de um PASSE** — e que o casco ja tinha dois passes sem saber.

**3. A task 09 nao era aditiva, e a task 10 pagou a correcao.** Ver a secao
homonima em `09-expor-o-quadro.md`: `.cpp` novo no casco nunca e aditivo, porque
cada jogo enumera os `.cpp` do casco no proprio `.vcxproj`. O `ForgeFrame` virou
so cabecalho (`inline`).

## O que esta task APAGA

No consumidor: a ordenacao por distancia e o `desenhaAntes` do Vigil. Esta e a
primeira das cinco que apagam mais do que acrescentam.
