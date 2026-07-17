# 06 - Escritor de DDS compartilhado (tools/)

- **Status:** candidata (NAO implementar ainda — 3 copias identicas ja existem,
  falta o consumidor de validacao: o proximo jogo com atlas)
- **Categoria:** Ferramentas (nao entra em lib linkada; e um .ps1 dot-sourced)
- **Registrada em:** 2026-07-17 (revisao pos-cengine-0.9.0)

## A candidata

Os tools de atlas dos jogos (`make-atlas-dds.ps1` do spaceinvaders e do
breakout, `make-atlas-mario-dds.ps1` do mario) terminam todos com o MESMO bloco
byte a byte: o escritor do arquivo DDS (magic `DDS `, DDS_HEADER de 124 bytes,
DDS_PIXELFORMAT fourCC `DX10`, DDS_HEADER_DXT10 com
`DXGI_FORMAT_R8G8B8A8_UNORM`), que o tinydds do ResourceLoader le direto.
Tres copias identicas parametrizadas so por largura/altura/pixels.

A ARTE de cada atlas (paleta, retangulos, espelhos) e a IDENTIDADE do jogo e
fica no tool do jogo — como as tabelas de regioes (`*Sprites.h`) ficam nos
jogos.

## O corte (quando extrair)

- **Sobe (aqui, em `tools/`):** `Write-Dds.ps1` — funcao `Write-Dds($path,
  $width, $height, [byte[]]$pixels)` com o header DX10. Os tools dos jogos
  passam a dot-sourcear do checkout irmao, como os vcxproj ja fazem com
  `$(CengineRoot)`.
- **Fica no jogo:** todo o desenho (Set-Pixel/Fill-Rect/paleta/celulas).

## Gate para comecar

O proximo jogo com atlas (seria a 4a copia). Em vez de copiar de novo, extrai —
o jogo novo valida; os tools existentes dos 3 jogos NAO migram (o atlas deles
ja esta gerado e versionado; regenerar sem motivo e risco sem ganho).
