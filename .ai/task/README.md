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
| 05 | [forgeaudio: backend XAudio2 da porta de audio](05-forge-audio-backend.md) | candidata (gate 2/2 ja disparado; extrai no proximo jogo com som) | Plataforma |
| 06 | [Escritor de DDS compartilhado (tools)](06-atlas-dds-writer.md) | candidata (3 copias identicas; extrai no proximo jogo com atlas) | Ferramentas |

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

## Regra pratica

Este repo nao e a engine. Ele pode conhecer The Forge, mas nao deve conhecer
regras, estados ou entidades de jogos especificos.
