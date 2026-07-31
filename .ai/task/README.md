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

- **Mascara ASCII no `Paint.ps1`** — os dez tools de atlas anteriores
  desenharam so RETANGULOS, e `Fill-Rect` deu conta em todos. O Klondike
  (2026-07-31) foi o primeiro com arte que NAO e retangulo: um naipe de copas
  tem dois lobos e um bico, paus tem tres circulos. Escrever isso em
  `Fill-Rect` seria ilegivel no tool e feio na tela, entao os quatro naipes
  viraram MASCARAS de texto (`#` pinta, `.` deixa passar) em
  `tools/make-atlas-klondike-dds.ps1` — a unica forma em que a arte do arquivo
  se PARECE com a arte da tela. **1 evidencia so**, e por isso fica no tool do
  jogo. O criterio de mecanismo puro passa com folga (um `string[]` e uma cor),
  e da para exercitar sem jogo e sem GPU. Mesma formula das tasks 05 e 06: **o
  proximo jogo com arte que nao seja retangulo EXTRAI, nao copia.**

- **Vocabulario de DRAG (0.10.0), LOCAL no `forgeui`** — `drag()` (estado) +
  `readDrop()` (edge), nascidos com o Klondike. Ficam aqui, e o caminho e o que
  o mouse ja percorreu (task 03 deste repo -> task 27 da cengine): viveu no
  `forgeui` por dois jogos e so subiu para a engine quando o 2o consumidor usou
  a forma do 1o sem pedir mudanca de API. **Um consumidor** — nenhuma evidencia
  de que esta forma seja a certa para o proximo. O que NAO espera nada e o
  `SetCapture`/`WM_CAPTURECHANGED` no `TheForgeWindowManager`: ele ja subiu por
  construcao, porque o WndProc E o casco, e era um buraco que existia desde
  sempre (sem capturar o ponteiro, soltar o botao fora da janela nao gera
  `WM_LBUTTONUP` e o gesto fica pendurado para sempre).

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
