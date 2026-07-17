# 04 - forgesprite::drawSpriteRect (retangulo de destino arbitrario)

- **Status:** done (0.4.0, commit `51c1342`) — registro retroativo (2026-07-17):
  a mudanca saiu junto do degrau 3 do mario-bros e nao tinha task aqui.
- **Categoria:** Ponte de plataforma
- **Consumidor real:** mario-bros (animacao/tiles), breakout (ja usava a forma
  por celula).

## O que aconteceu

O `forgesprite` da 0.1.0 desenhava celulas de tamanho fixo. O mario precisou
desenhar uma REGIAO arbitraria do atlas num RETANGULO arbitrario de destino
(sprites de 16x16 escalados, tiles, mastro empilhavel): `drawSpriteRect(regiao,
destino, tint)`. A tabela de regioes continua sendo DO JOGO (`MarioSprites.h`,
`BreakoutSprites.h`) — este repo so desenha o que lhe mandam.

## Criterios de Aceite

- [x] Regiao em pixels do atlas -> retangulo em pixels da tela, com tint.
- [x] Nenhuma tabela de sprites neste repo (regioes vivem nos jogos).
- [x] Consumidor real validado jogando (mario degrau 3+).
