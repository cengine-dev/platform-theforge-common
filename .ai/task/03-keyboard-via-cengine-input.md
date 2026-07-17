# 03 - ForgeUi delega o teclado para a cengine::input

- **Status:** done (0.3.0, commit `f799d28`) — registro retroativo (2026-07-17):
  a mudanca saiu junto da task 20 da cengine e nao tinha task aqui.
- **Categoria:** Ponte de plataforma
- **Consumidor real:** breakout (o jogo que validou a cengine 0.8.0).

## O que aconteceu

A task 20 da cengine promoveu o vocabulario de input (`cengine::input::Keyboard`
+ enum `Key`, 4 copias identicas nos jogos — o "discriminador do input"). O
corte: o VOCABULARIO subiu para a engine; a CAPTURA (InputSystem do The-Forge,
mapeamento de teclas) ficou aqui, no `ForgeUi`, que passou a empurrar eventos
para o teclado da porta em vez de manter um teclado proprio.

E o mesmo desenho que a porta de audio repetiu depois (cengine 0.9.0, task 24):
contrato na engine, backend na plataforma.

## Criterios de Aceite

- [x] `forgeui::keyboard()` devolve `cengine::input::Keyboard&` (a porta).
- [x] Nenhum enum de tecla proprio neste repo — o vocabulario e o da cengine.
- [x] Consumidor real validado jogando (breakout na cengine 0.8.0).
