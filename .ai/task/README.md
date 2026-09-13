# Plano de trabalho - platform-theforge-common

Este diretorio registra as tarefas para extrair codigo comum de plataforma
The Forge usado pelos jogos de estudo.

## Indice

| # | Task | Status | Categoria |
|---|------|--------|-----------|
| 01 | [Extrair pontes The Forge reutilizaveis](01-extract-forge-sprite-ui.md) | done (0.1.0) | Plataforma |
| 02 | [ForgeLineUi: batcher de linhas 2D (wireframe)](02-forge-line-ui.md) | done (0.2.0) | Plataforma |
| 03 | [ForgeUi delega o teclado para a cengine::input](03-keyboard-via-cengine-input.md) | done (0.3.0) | Plataforma |
| 04 | [forgesprite::drawSpriteRect (retangulo arbitrario)](04-draw-sprite-rect.md) | done (0.4.0) | Plataforma |
| 05 | [forgeaudio: backend XAudio2 da porta de audio](05-forge-audio-backend.md) | done (0.5.0) | Plataforma |
| 06 | [Escritor de DDS compartilhado (tools)](06-atlas-dds-writer.md) | done (0.5.0) | Ferramentas |
| 07 | [Paint.ps1: helpers de pintura dos tools](07-paint-helpers.md) | done (0.7.0; `Paint-Mask` na 0.11.0) | Ferramentas |
| 08 | [ForgeUi delega o ARRASTAR para a cengine::input](08-drag-via-cengine-input.md) | done (0.12.0) | Plataforma |
| 09 | [O casco expoe o QUADRO (forgeframe)](09-expor-o-quadro.md) | **done (0.16.0, corrigida na 0.17.0)** | Plataforma / 3D |
| 10 | [Depth buffer opt-in](10-depth-buffer.md) | **done (0.17.0)** | Plataforma / 3D |
| 11 | [forgemesh: a malha na GPU, com shader que le matriz](11-forgemesh.md) | **done (0.18.0)** | Plataforma / 3D |
| 12 | [Material e textura](12-material-e-textura.md) | **done (0.22.0)** | Plataforma / 3D |
| 13 | [A luz posicionavel](13-a-luz.md) | **done (0.21.0)** | Plataforma / 3D |
| 14 | [Esqueleto e animacao, com skinning na GPU](14-esqueleto-e-animacao.md) | todo | Plataforma / 3D |
| 15 | [forgemesh: variantes, e um layout de vertice SO](15-forgemesh-variantes.md) | **15a done (0.20.1); 15b espera a task 12** | Plataforma / 3D |
| 16 | [O casco ganha um ALVO DE BUILD](16-alvo-de-build.md) | **done (0.20.0)** | Plataforma / Infra |
| 17 | [Como o forgemesh entrega a matriz por objeto](17-instancias-do-forgemesh.md) | **adiada, com gatilho** | Plataforma / 3D |
| 18 | [O conjunto se chama `PerFrame` e nao e](18-frequencias-do-uniforme.md) | **todo — a 12 confirmou; pronta** | Plataforma / 3D |
| 19 | [O modo de conferencia nao devia ser um `if` por pixel](19-o-modo-de-conferencia.md) | **todo — espera a 15b** | Plataforma / 3D |
| 20 | [Liberar malha e material](20-liberar-malha-e-material.md) | **todo** — item de spec da 11 nunca feito | Plataforma / 3D |
| 21 | [O casco como BIBLIOTECA](21-o-casco-como-biblioteca.md) | **todo** — proposta escrita (revisao arquitetural 3.1/3.2) | Plataforma / Infra |
| 22 | [A tangente que nao veio](22-a-tangente-que-nao-veio.md) | **todo** — a 2a metade do achado 3.13 | Plataforma / 3D |
| 23 | [O `.props` diz o que o casco TEM, e nao o que ele EXIGE](23-o-props-declara-o-que-exige.md) | **todo** — buraco da task 16 | Plataforma / Infra |

## As tres tasks de DIVIDA (15-17), registradas em 2026-09-06

Sairam de uma pergunta do dono depois do degrau 05 do `diorama`: *"algum ponto
que ficou mal arquitetado?"*. As tres sao consequencia das tasks 09-11, e a
ordem entre elas importa:

**A ordem foi revista em 2026-09-06**, ao separar o que pode mudar de rumo do
que nao pode:

| ordem | | risco de mudar de rumo | quando |
|---|---|---|---|
| **1o** | **16** alvo de build | nenhum | **FEITA (0.20.0)** |
| **2o** | **15a** o layout numa funcao so | nenhum | **FEITA (0.20.1)** |
| — | **15b** a maquina de variantes | **alto** — a task 12 e que define os eixos de verdade | quando a 12 forcar |
| — | **17** instancias | **alto** — depende de o material precisar de descriptor por desenho (task 12) | **adiada, gatilho mensuravel** |
| — | **18** frequencias do uniforme | — | **a 12 chegou e confirmou**: pronta |
| — | **19** o modo de conferencia | — | espera a **15b** (e onde a variante cabe) |

> **A 20 saiu da revisao do degrau 07** (2026-09-07), junto com duas correcoes
> feitas na hora: o religamento de material que ligava `NULL` em vez do mapa
> padrao (**bug real, ainda nao disparado**), e o `loadMaterial` que falhava
> calado. A revisao tambem confirmou o diagnostico da 18 e piorou o da 19.
>
> **As 18 e 19 sairam da revisao do degrau 06**, junto com duas correcoes que
> foram feitas na hora (a divisao por pi que faltava no Lambert, e a intensidade
> guardada em dois campos). As tres adiadas convergem na task 12: e ela que traz
> a frequencia de MATERIAL, e sem ela qualquer separacao seria adivinhacao.

**A 16 vem primeiro e nao e detalhe de ordem.** Enquanto a regra "nunca
acrescente um `.cpp`" valer, todo modulo novo nasce header-only por imposicao de
build, e nao por desenho — inclusive a implementacao da 15. Fazer a 15 antes e
escrever codigo que a 16 vai querer mover.

A 15 tambem fecha um defeito que **ja existe**: o `VertexLayout` escrito duas
vezes no `ForgeMesh.h`, com a mesma assinatura de falha silenciosa que produziu
os cubos pretos do degrau 04.

## O pipeline 3D (tasks 09-14), registrado em 2026-09-03

As seis nascem juntas, do teto que o **Vigil** mediu ao fechar o degrau 24 (ver
`vigil/.ai/task/24-a-cena-em-3d.md`, secao *"Por que este degrau fechou o
projeto"*). Elas nao sao seis ideias: sao **uma ordem**, e a ordem foi
descoberta, nao escolhida.

> **O bloqueio nunca foi o formato do asset: e a superficie deste casco.** O
> `AssetPipelineCmd` ja produz `.bin`, `skeleton.ozz`, `<nome>.ozz` e `.dds`; o
> The-Forge ja sobe a malha para a GPU. O Vigil pedia `SHADOWED` para ter copia
> na CPU e **ignorava os buffers de GPU**, porque nao tinha onde desenha-los.

Cinco das seis **apagam mais codigo do que acrescentam** -- so a 09 e pura
adicao, e ela existe para que as outras cinco possam apagar. Consumidor de
validacao: o lab **`diorama`**, um degrau por task.

**Nenhum jogo 2D e afetado.** As tres portas novas (`depth`, `forgemesh`, e o
`forgeframe`) sao opt-in por desc, exatamente como `sprites` e `lines` ja sao.

### A regra que protege os consumidores, e por que ela e mais forte aqui

Este casco **nao e dependencia versionada: e um checkout IRMAO em disco.** Todo
consumidor compila contra o que estiver em `../platform-theforge-common`, sem
pinagem. Nao ha `>= 0.12.0` que o build respeite -- ha o que esta na pasta.

Consequencia: **evoluir este repo muda o insumo de build de todos os jogos, sem
tocar em nenhum deles.** O ADR 0003 da cengine congela consumidores como
documentacao viva; aqui nao existe mecanismo que faca esse congelamento valer.

Dai duas regras para as tasks 10-14:

1. **Toda porta nova nasce DESLIGADA por desc.** O consumidor que nao pediu nada
   tem de compilar e rodar identico. A task 10 (depth buffer) e o caso critico:
   ela mexe em `addGameSwapChain` e `applyPendingResize`, que todo jogo 2D
   executa -- com `depth.enabled = false` nenhum render target e criado e o
   `mDepthStencil` segue `NULL`, como sempre foi.
2. **Compatibilidade se prova pelo DIFF, nao buildando projeto alheio.** Nenhum
   simbolo removido, nenhuma assinatura mudada, nenhum default alterado. Se a
   compatibilidade nao se ler no diff, a mudanca deixou de ser aditiva -- e o
   problema e esse, nao a falta do build.

Os outros projetos do workspace sao **referencia**: servem para ler, citar e
aprender. O lab que valida estas tasks e o `diorama`, e e o unico que este repo
tem o direito de buildar.

## Candidatas (mesma disciplina da ADR 0002 da cengine)

Duplicacao detectada na revisao pos-cengine-0.9.0 (2026-07-17), varrendo os 5
jogos. As duas ja passam do gate de evidencias — o que falta e o consumidor de
validacao (o proximo jogo que precisar delas), para nao extrair sem ninguem
exercitando o resultado:

- **05 (forgeaudio)** — o backend XAudio2 da porta `cengine::audio::Player` e
  IDENTICO no breakout e no mario (sintese, pool de vozes, COM, shutdown); so
  as receitas e o enum `Sound` sao de cada jogo.
- **06 (escritor de DDS)** — o bloco que escreve o header DDS/DX10 byte a byte
  e IDENTICO nos tools de atlas de spaceinvaders, breakout e mario; a ARTE de
  cada atlas e do jogo.

- **07 (helpers de pintura dos tools de atlas)** — `Set-Pixel` esta escrito a
  mao em CINCO tools (breakout, mario-bros, zelda, starforce, delve) e
  `Fill-Rect` em tres, sempre identicos: escrever RGBA num `byte[]` com
  checagem de limites, e preencher retangulo. Registrado na revisao pos-Delve
  (2026-07-27). Passa o gate de evidencias com folga e o precedente e exato —
  o `Write-Dds.ps1` (task 06) foi extraido com TRES copias. A diferenca e o
  tamanho: aquele era o header DDS byte a byte, este sao ~15 linhas de `for`
  aninhado, entao o valor e baixo. Mesma formula das tasks 05/06: **o proximo
  jogo que escrever um atlas EXTRAI, nao copia**; os cinco tools existentes
  NAO migram (o atlas deles ja esta gerado e versionado, mesma regra que valeu
  na extracao do Write-Dds). A PALETA e os retangulos de cada jogo seguem
  sendo identidade do jogo.

## Observacoes (nao sao candidatas ainda — 1a evidencia so)

> **A mascara ASCII SAIU desta lista em 2026-07-31: virou o `Paint-Mask` da
> 0.11.0**, com o Counter como segundo consumidor. Ficou aqui por um jogo
> exatamente como a formula manda — *o proximo jogo com arte que nao seja
> retangulo EXTRAI, nao copia* — e o proximo veio. Ver a task 07.

> **O vocabulario de DRAG SAIU desta lista em 2026-08-03: subiu para a
> `cengine::input::Mouse` (task 28 da engine, 0.15.0), e este casco passou a
> DELEGAR na 0.12.0.** Ficou aqui por dois jogos exatamente como o caminho do
> mouse mandava (task 03 deste repo -> task 27 da cengine), e o gate foi
> cumprido ao pe da letra: o 2o consumidor (**cue**, degrau 06 — a tacada por
> arrasto) usou as duas leituras para o que elas foram feitas, leu os quatro
> campos de cada uma e **nao pediu nenhuma mudanca de API**.
>
> `forgeui::DragState` e `forgeui::Drop` continuam existindo como ALIAS, e
> `drag()`/`readDrop()` continuam sendo a ergonomia global das cenas — nenhum
> jogo mudou uma linha, igual a promocao do mouse.
>
> **O `SetCapture`/`WM_CAPTURECHANGED` ficou, e nunca foi candidato**: e Win32, e
> o WndProc E o casco. O que subiu foi o `cancelDrag()`, que e a resposta em
> vocabulario de PORTA ao que este casco descobre em vocabulario de JANELA.
>
> E a folga de "isto foi clique ou arrasto?" tambem ficou fora, e o segundo
> consumidor provou por que: o Klondike usa 8 pixels para uma carta de 90, o cue
> usa 12 para uma bola de 20, e os dois medem perguntas diferentes ("errou o
> alvo?" contra "quis mesmo tacar?"). **Foi a primeira vez que uma recusa a
> opinar do casco foi TESTADA por um segundo caso, em vez de so declarada.**

- **`forgesprite` e um atlas SO** (`ForgeSpriteUi.cpp`: `Texture* gAtlasTexture`
  global, setado uma vez em `init()`). O Star Force (2026-07-22, tela de menu
  com arte gerada fora do pipeline pixel-art da gameplay: logo/botoes/fundo)
  bateu nesse teto — resolveu SEM mexer aqui, crescendo o proprio atlas do
  jogo (128x128 -> 800x788) e usando `drawSpriteRect` (task 04, ja promovida)
  pra esticar regioes de tamanhos bem diferentes no mesmo desenho. Registrado
  so pra memoria: se um segundo jogo precisar de duas fontes de arte que NAO
  cabem bem no mesmo atlas (paletas/resolucoes muito diferentes), a questao
  "multiplos atlas/texturas" volta — por ora, 1 evidencia, resolvida sem
  atrito real, nao e candidata.

- **`forgeui::drawText` tem um TETO de chamadas por quadro** — ~143 num mesmo
  quadro estouram o renderizador de fonte do The-Forge (glifos "fantasma"
  acesos com a cor errada em posicoes fixas, independentes do que o jogo
  pediu); 108 e 115 passam. Medido no Delve (2026-07-25), que desenhava uma
  grade 13x11 celula a celula. Junto veio o segundo limite, independente do
  The-Forge: **a fonte nao e monoespacada**, entao agrupar celulas iguais numa
  string so — a saida obvia para o primeiro limite — desalinha em silencio.
  Nao e candidata a nada: nao ha mecanismo novo para extrair, e a saida certa
  ja existe (`forgesprite`, que desenha o lote em uma chamada). **A acao foi
  DOCUMENTAR**, no `ForgeUi.h`, junto da declaracao do `drawText` — o percurso
  custou dois rascunhos errados e um bug que parecia certo, e o proximo jogo
  nao precisa repetir.

## Regra pratica

Este repo nao e a engine. Ele pode conhecer The Forge, mas nao deve conhecer
regras, estados ou entidades de jogos especificos.
