# 01 - Extrair pontes The Forge reutilizaveis

- **Status:** done (0.1.0 — degraus 1 e 2; degraus 3/4 redefinidos: o
  primeiro consumidor sera o asteroids, ver "Decisoes registradas")
- **Prioridade:** media - a necessidade apareceu com `spaceinvaders` e tende a
  crescer com os proximos jogos 2D.
- **Categoria:** Plataforma
- **Origem:** `spaceinvaders/src/platform/theforge/src/SpaceInvadersForge`
- **Consumidores candidatos:** `spaceinvaders`, `8puzzle`, `asteroids`

## Contexto

O Space Invaders criou pecas de plataforma mais genericas que o jogo em si:

- `ForgeUi`: input, texto, hints e medidas de fonte.
- `ForgeSpriteUi`: sprite batcher 2D com atlas, tint por vertice, vertex buffer
  dinamico e flush por lote.
- convencoes de FSL/SRT, descriptor set, sampler nearest e alpha blending.

Essas pecas nao pertencem ao `cengine`, porque conhecem The Forge diretamente.
Tambem nao deveriam ser copiadas indefinidamente entre jogos. Este repositorio
e o lugar para transformar essas pontes em infraestrutura reutilizavel de
plataforma.

## Objetivo

Extrair primeiro o `ForgeSpriteUi` para uma API configuravel, mantendo o
contrato que funcionou no Space Invaders:

```cpp
drawSprite(region, x, y, scale, tint);
flush();
```

A extracao deve remover suposicoes de jogo, principalmente:

- atlas fixo chamado `atlas.dds`;
- tabela de regioes fixa com invasores/jogador/tiro;
- limite de lote hardcoded sem configuracao;
- nomes ligados a Space Invaders.

## Escopo Proposto

### Degrau 1 - desenho da API

Definir a forma minima de configuracao:

```cpp
struct SpriteRegion {
    float x;
    float y;
    float w;
    float h;
};

struct SpriteBatcherDesc {
    const char* atlasPath;
    uint32_t maxSprites;
    uint32_t frameCount;
};
```

Pontos a decidir:

- namespace do projeto;
- ownership dos recursos The Forge;
- se a API sera global como a PoC ou objeto instanciavel;
- como expor estatisticas de frame;
- como integrar flush automatico antes de texto.

### Decisoes registradas (2026-07-13, 0.1.0)

- **Namespaces mantidos** (`forgeui`, `forgesprite`): sao palavras de
  plataforma, nao de jogo, e preservam a continuidade com as copias
  congeladas nos jogos (documentacao viva — facil de diffar).
- **API global mantida** (como a PoC): o proprio The Forge e um-por-processo
  nesse desenho (WndProc estatico, `gWindow` global do fontstash); um objeto
  instanciavel seria mentira de API. Reavaliar so se um consumidor precisar
  de duas janelas.
- **Escopo alargado alem do ForgeSpriteUi**: `ForgeUi` e o
  `TheForgeWindowManager` inteiro tambem entraram — o casco e a peca com
  mais valor de reuso e ja nasceu configuravel (`TheForgeWindowDesc`).
- **Estado segurado generalizado**: `pushHeldState(moveAxis, fireHeld)` (o
  formato do canhao do spaceinvaders) virou `pushHeldKey(Key, bool)` +
  `isHeld(Key)` + `heldAxis(neg, pos)`; o WndProc rastreia setas + espaco.
  `Key::Space` existe SO no estado segurado (na fila de edges o espaco
  continua chegando como `Key::Char ' '`).
- **Batcher opt-in**: `SpriteBatcherDesc.atlasPath == nullptr` desliga tudo
  (jogo so de texto nao paga pelo pipeline de sprites).
- **Estatisticas/flush**: mantidos como na PoC (`lastFrameStats()`; o
  `drawText` flusha o lote pendente — camadas por ordem de chamada).
- **Degraus 3/4 redefinidos**: 8puzzle e spaceinvaders ficaram congelados
  como historia (nao migram — decisao do dono do projeto, ADR 0003 da
  cengine). O aceite de consumidor real passa para o **asteroids**, primeiro
  jogo a consumir este repo; a validacao visual/regressao acontece la.

### Degrau 2 - extracao mecanica

Copiar a implementacao do `spaceinvaders`, remover nomes de jogo e adaptar para
a API configuravel.

Nao adicionar funcionalidades novas neste degrau.

### Degrau 3 - primeiro consumidor real

Migrar o `spaceinvaders` para consumir este repo. O aceite real e o jogo seguir
renderizando:

- horda, jogador, tiros e bombas;
- texto por cima dos sprites;
- resize/reload;
- um draw call por lote contiguo.

### Degrau 4 - segundo consumidor

Validar com outro jogo ou PoC. Se a API exigir alteracao por causa desse
segundo consumidor, registrar o aprendizado antes de estabilizar.

## Fora do Escopo

- Engine generica independente de The Forge.
- Regras de jogo.
- Tabela de sprites especifica de qualquer jogo.
- ECS, scene graph ou sistema de UI completo.
- Som.
- Rotacao/camera no primeiro corte, salvo se o consumidor real exigir.

## Criterios de Aceite

- [x] README documenta objetivo e fronteiras do repo (+ estrutura e receita
      de consumo).
- [x] API do sprite batcher nao contem vocabulario de jogo (tabela de
      regioes removida; atlas/limite viram `SpriteBatcherDesc`).
- [x] ~~`spaceinvaders` consome o common sem regressao visual~~ — jogos
      congelados como historia; o aceite passa para o `asteroids` (primeiro
      consumidor, valida na task de bootstrap daquele repo).
- [x] Recursos The Forge continuam com ciclo de vida claro:
      `init/load/begin/flush/unload/exit`.
- [x] O sprite batcher e o ForgeUi permanecem isolados do `cengine`; o
      `TheForgeWindowManager` depende do port `IWindowManager` por natureza
      (e o adaptador cengine<->The Forge — decisao registrada acima).

## Riscos

- Extrair cedo demais e congelar a API no formato do Space Invaders.
- Esconder demais o ciclo de vida do The Forge e dificultar debug.
- Misturar input/texto/sprites em uma abstracao grande antes de haver dois
  consumidores reais.

## Relacionado

- `spaceinvaders` task 01 - PoC sprites concluida.
- `cengine` ADR 0002 - filtro anti-deposito: codigo de plataforma vai para um
  pacote de plataforma, nao para a engine generica.
