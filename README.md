# platform-theforge-common

Codigo comum para jogos de estudo que usam The Forge como plataforma grafica.

Este repositorio existe para concentrar adaptadores e utilitarios que sao
reutilizaveis entre jogos, mas que nao pertencem ao `cengine` generico porque
conhecem detalhes concretos do The Forge: renderer, command buffers, resource
loader, shaders FSL, input da plataforma, fontes, texturas e ciclo
`Init/Load/Unload/Draw`.

## Objetivo

Separar responsabilidades entre os projetos:

- `cengine`: loop, portas e mecanismos independentes de plataforma.
- `platform-theforge-common`: infraestrutura reutilizavel especifica do
  The Forge.
- jogos (`8puzzle`, `spaceinvaders`, `asteroids`, ...): dominio, regras,
  cenas concretas e assets de cada jogo.

## Conteudo (0.10.0)

### Novo na 0.10.0

- **O ARRASTAR** (`forgeui::drag()` / `readDrop()` / `cancelDrag()`), com a
  captura de `WM_LBUTTONUP` que faltava no WndProc. Consumidor de validacao: o
  Klondike (degrau 06).

  Vocabulario LOCAL, e nao porta da engine — mesmo criterio que segurou o mouse
  por dois jogos (task 27): um consumidor, e nenhuma evidencia de que esta seja
  a forma certa para o proximo. Se um segundo jogo arrastar, a comparacao
  decide.

  **Duas leituras, porque arrastar nao cabe na fila de cliques.** Um clique e
  um EVENTO (aconteceu, tem uma posicao, acabou); arrastar e um CICLO DE VIDA
  com duas pontas que carregam posicoes DIFERENTES — a origem diz o que se
  pega, o destino diz para onde vai. Entao: `drag()` e estado continuo (para
  desenhar o que esta na mao, todo quadro) e `readDrop()` e edge (um por gesto,
  com as duas posicoes juntas).

  **O casco NAO decide se o gesto foi clique ou arrasto.** Ele entrega as duas
  pontas; quantos pixels de folga ainda contam como toque parado e politica do
  JOGO — depende do tamanho do alvo e do tremor de mao que se perdoa.

- **`SetCapture` no `WM_LBUTTONDOWN`** (e `ReleaseCapture` no UP). Sem isto,
  soltar o botao FORA da janela nao gera `WM_LBUTTONUP` nenhum e o gesto ficava
  pendurado para sempre — a carta grudada no cursor. E o primeiro lugar em que
  o arrastar cobra algo que o clique nunca cobrou. `WM_CAPTURECHANGED` e
  `WM_KILLFOCUS` cancelam o gesto: cancelar e melhor que soltar num destino que
  o jogador nao escolheu.

### Mudou de casa na 0.9.0

- **O vocabulario de mouse SUBIU para a `cengine::input`** (task 27 da engine,
  cengine 0.14.0). `MouseButton` e `MouseClick` agora sao da engine; esta ponte
  guarda a instancia de `cengine::input::Mouse` e continua fazendo o que so ela
  pode fazer — CAPTURAR (o WndProc traduzindo `WM_MOUSEMOVE`/`WM_LBUTTONDOWN`).

  Mesmo caminho que o teclado fez na 0.8.0 da engine: o vocabulario viveu aqui
  enquanto tinha um consumidor so (o Bulwark), e subiu quando o SEGUNDO
  (Tactics) usou a forma do primeiro **sem pedir nenhuma mudanca de API**.

  **Nao quebra nada.** `forgeui::MouseButton` e `forgeui::MouseClick` continuam
  existindo como ALIAS dos tipos da engine, e as funcoes globais
  (`mouseX`/`mouseY`/`readMouseClick`) tem exatamente a mesma assinatura. Quem
  quiser falar com a porta direto tem `forgeui::mouse()`, irmao do
  `forgeui::keyboard()`. Requer cengine >= 0.14.0.

### Corrigido na 0.8.0

- **O X da janela nao forja mais um `Escape`.** O `WM_CLOSE` agora marca um
  pedido de fechamento que a cengine le pelo `IWindowManager::shouldClose()`
  (0.13.0); o loop para no fim do quadro e o `cleanup()` roda normalmente.

  Ate a 0.7.1 o X empurrava um `Escape` FALSO na fila de teclas, para a cena
  rotear para a saida. Funcionava por sorte: so nos jogos em que ESC ja
  significava "sair". Nos que usam ESC para "voltar ao menu" — o Delve e o
  Bulwark — clicar no X levava ao MENU. "O jogador pediu para sair" e "o
  sistema mandou fechar" nao sao a mesma coisa e nao podem dividir o mesmo
  canal. Requer cengine >= 0.13.0.

### Novo na 0.7.0

- `tools/Paint.ps1` — os helpers de pintura dos tools de atlas, extraidos das
  copias identicas de CINCO jogos (`Set-Pixel` no breakout, mario-bros, zelda,
  starforce e delve; `Fill-Rect` em tres). Consumidor de validacao: o Bulwark
  (degrau 07). Os cinco tools existentes nao migram — o atlas deles ja esta
  gerado e versionado, mesma regra da extracao do `Write-Dds`.

  **O que mudou na extracao:** as copias liam `$script:texW`/`$script:pixels`
  do escopo de quem chamava — funciona por acidente e quebra em silencio se o
  tool renomear a variavel. Virou um CANVAS explicito
  (`New-AtlasCanvas -Width -Height`), passado as funcoes. A ARTE de cada atlas
  segue no tool do jogo; isto e so o pincel.

### Novo na 0.6.0

- **Mouse** (`ForgeUi.h`) — posicao do ponteiro (`mouseX`/`mouseY`, estado
  continuo) e fila de cliques (`readMouseClick`, edges, um por aperto),
  capturados no WndProc (`WM_MOUSEMOVE`, `WM_LBUTTONDOWN`, `WM_RBUTTONDOWN`).
  Primeiro mouse do ecossistema: os nove jogos ate aqui eram 100% teclado.

  **O vocabulario mora AQUI, nao na `cengine::input`.** O enum de teclas so
  subiu para a engine quando ja era a QUARTA copia identica nos jogos; o mouse
  tem UM consumidor (o Bulwark, degrau 05) e nenhuma evidencia de que esta
  forma seja a certa para o proximo. Se um segundo jogo precisar, a comparacao
  decide — mesmo caminho que o teclado fez (ADR 0002 da cengine).

  Duas decisoes de desenho que valem para quem for consumir: **posicao e
  ESTADO, clique e EDGE** (realce sob o cursor precisa de todo quadro;
  construir precisa de uma vez por aperto), e **o clique carrega a posicao DO
  MOMENTO** em que aconteceu — o ponteiro pode andar entre o aperto e o quadro
  em que a cena o le. As coordenadas sao PIXELS da area util, o mesmo espaco
  do `drawText`: esta ponte nao sabe o que e uma celula, e traduzir para o
  mundo do jogo e trabalho do jogo.

### Corrigido na 0.5.1

- **`WM_CHAR` agora filtra auto-repeat**, como o `WM_KEYDOWN` sempre filtrou.
  A fila de edges promete UM EVENTO POR APERTO FISICO, e as duas metades dela
  tinham semanticas diferentes: segurar uma seta mandava um evento, segurar
  uma letra mandava um por quadro. Ninguem tinha batido nisso porque, dos nove
  jogos, o **Bulwark** foi o primeiro a consumir `Key::Char` por esta ponte —
  os outros usam so setas, Enter e ESC. No jogo dele, segurar espaco soltava
  um inimigo por quadro. Validado pelo Bulwark (degrau 03).

### Novo na 0.5.0

- `forgeaudio` (`ForgeAudio.{h,cpp}`) — o backend XAudio2 da porta
  `cengine::audio::Player` (task 24 da cengine), extraido das copias identicas
  do breakout e do mario-bros: COM, pool de 8 vozes round-robin (PCM 16-bit
  mono 44.1kHz) e os helpers de sintese `synth`/`concat`. O jogo entrega a
  TABELA de samples no `init` (o enum `Sound` e as receitas ficam no jogo).
  Sem device, o jogo roda mudo (degradacao normal do contrato da porta).
- `tools/Write-Dds.ps1` — o escritor DDS (RGBA8 sem compressao, header DX10)
  que fechava os tools de atlas dos jogos, agora dot-sourceavel do checkout
  irmao. A arte de cada atlas segue no tool do jogo.

### Novo na 0.4.0

- `forgesprite::drawSpriteRect(region, x, y, w, h, cor)` — desenha a regiao do
  atlas num **retangulo de destino arbitrario**, em vez de uma escala uniforme.

  O `drawSprite(region, x, y, escala, cor)` continua existindo: era o formato de
  que o spaceinvaders precisava, onde o sprite era 1:1 com as unidades do mundo.
  Mas escala uniforme nao da conta de um jogo cujos corpos tem proporcoes
  diferentes entre si (o breakout: tijolo 60x20, raquete 110x16, bola 12x12), nem
  de uma projecao arena->tela que estica X e Y de forma diferente. O quad ja era
  montado a partir de dois cantos — a funcao nova so para de derivar o segundo
  canto de uma escala.

### Novo na 0.3.0

- `ForgeUi` **deixou de ter vocabulario proprio**: `Key`/`KeyEvent` e o contrato
  (fila de edges + estado segurado) subiram para a `cengine::input` (task 20 da
  engine, >= 0.8.0). Esta ponte guarda o que sempre foi dela — a **captura** (o
  WndProc traduzindo `VK_*` para `Key`) — e delega o resto a um
  `cengine::input::Keyboard`.

  Era a **quarta copia** do mesmo enum no ecossistema. Os aliases
  (`using Key = cengine::input::Key;`) mantiveram a ergonomia: o asteroids
  compilou sem mudar uma linha de cena. Quem quiser falar com a porta direto tem
  `forgeui::keyboard()`.

  Consumo: incluir `$(CengineRoot)modules/input/include` e compilar
  `modules/input/src/Keyboard.cpp`.

### Novo na 0.2.0

- `ForgeLineUi` (`forgeline`): **batcher de LINHAS 2D** — `drawLine` e
  `drawPolyline` em pixels, cor por vertice, um draw call por lote. Mesma
  mecanica do batcher de sprites (vertex buffer dinamico com um trecho por frame
  in flight), mas **sem textura, sem sampler e sem SRT**: a cor vem no vertice.
  Ligado por `LineBatcherDesc::enabled` (default `false` — quem nao desenha
  linha nao paga pelo pipeline).

  Trazido pelo `asteroids`, que precisava desenhar corpos que GIRAM. A
  alternativa avaliada era rotacionar sprites; linha ganhou porque os arcades
  vetoriais (Asteroids, Lunar Lander, Tempest) sao desenhados a linha e porque
  **linha dispensa atlas**: nao ha arte para produzir, so geometria. Um poligono
  girado e uma lista de pontos girados.

### Base (0.1.0)

Extraido da PoC do Space Invaders (task 01), com o vocabulario de jogo
convertido em configuracao:

- `TheForgeWindowManager`: o casco completo — The Forge como biblioteca atras
  do port `IWindowManager` da cengine (>= 0.5.0). Janela Win32 propria,
  renderer, fontes, swapchain com resize, input via WndProc e o par
  `update()`/`present()` envolvendo as fases do jogo. Configurado por
  `TheForgeWindowDesc` (nome, tamanho, fonte, atlas, cor de clear).
- `ForgeUi`: ponte de texto/input/hints para cenas The Forge. Fila de edges
  (`readKey`) + estado segurado generico por tecla (`isHeld`/`heldAxis` —
  cada jogo compoe seu esquema de controles).
- `ForgeSpriteUi`: sprite batcher 2D com atlas, tint e flush por lote.
  Configurado por `SpriteBatcherDesc` (atlas, capacidade); a tabela de
  regioes e do jogo. `atlasPath` nulo desliga o batcher (jogo so de texto).
- `Shaders/FSL`: shaders do batcher (`sprite.vert/frag.fsl` + `sprite.srt.h`)
  e `Shaders.list` com os rootsigs padrao.
- `Format.h`: utilitarios pequenos de formatacao usados por cenas (std puro).

## Estrutura e consumo

```
src/TheForgeCommon/          <- adicionar ao include path do jogo
  TheForgeWindowManager.h/.cpp   (depende de cengine + The Forge)
  ForgeUi.h/.cpp                 (depende de The Forge)
  ForgeSpriteUi.h/.cpp           (depende de The Forge)
  ForgeLineUi.h/.cpp             (depende de The Forge)
  Format.h                       (std puro)
  Shaders/FSL/                   (Shaders.list para o passo FSL do jogo)
```

As camadas 2D sao a ORDEM DE CHAMADA, atravessando as tres pontes: o
`forgeui::drawText` da flush nos lotes pendentes de sprites e linhas antes de
gravar texto. Entao, dentro do `draw()` da cena, "geometria primeiro, texto
depois" poe o HUD por cima do jogo.

O consumo segue a receita dos jogos (vcxproj MSBuild, layout de checkouts
irmaos na mesma pasta — `The-Forge`, `cengine`, este repo e o jogo):

1. incluir `src/TheForgeCommon` no include path (os includes internos sao
   relativos: `"ForgeUi.h"`, `"Shaders/FSL/sprite.srt.h"`);
2. compilar os quatro `.cpp` junto do jogo;
3. apontar o passo FSL para `src/TheForgeCommon/Shaders/FSL/Shaders.list`
   (o `#include` do `defaults.h` assume o layout de checkouts irmaos) — ou
   copiar/estender a lista se o jogo tiver shaders proprios;
4. o jogo continua dono de `PathStatement.txt`, `gpu.cfg`, fontes e atlas
   (`TheForgeWindowDesc` recebe os caminhos).

Nada deve ser promovido para ca apenas por parecer reutilizavel. O codigo deve
entrar quando existir um consumidor real alem do jogo original ou quando uma
nova PoC precisar da mesma ponte com pouca variacao.

> **Jogos estacionados:** `8puzzle` e `spaceinvaders` NAO migram para este
> repo — ficaram congelados como documentacao viva (decisao registrada no
> ADR 0003 da cengine). As copias deles sao a evidencia de duplicacao que
> justificou a extracao; o primeiro consumidor real e o `asteroids`.

## Principios

- Codigo deste repo pode depender do The Forge.
- Codigo deste repo nao deve conter regra de jogo.
- APIs devem receber configuracao do jogo, como atlas, regioes, cores e limites
  de lote, em vez de fixar nomes como `atlas.dds`.
- O ciclo de vida deve continuar explicito para casar com The Forge:
  `init/load/begin/flush/unload/exit`.
- Cenas dos jogos continuam sendo donas da intencao: o common so fornece a
  ponte de plataforma.

## Plano

As tarefas ficam em [`.ai/task/`](.ai/task/). Comece por
[`01-extract-forge-sprite-ui.md`](.ai/task/01-extract-forge-sprite-ui.md).
