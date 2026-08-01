# 07 - Paint.ps1: os helpers de pintura dos tools de atlas

- **Status:** done (0.7.0, 2026-07-28) — emendado na 0.11.0 (2026-07-31) com o
  `Paint-Mask`.
- **Prioridade:** media - ferramenta, nao runtime.
- **Categoria:** Ferramentas

## Contexto

`Set-Pixel` estava escrito a mao em CINCO tools de atlas (breakout, mario-bros,
zelda, starforce, delve) e `Fill-Rect` em tres, sempre igual: escrever RGBA num
`byte[]` com checagem de limites, e preencher retangulo chamando o primeiro.

O precedente era exato — o `Write-Dds.ps1` (task 06) subiu com TRES copias — e a
diferenca era o tamanho: aquele era o header DDS byte a byte, este sao ~15
linhas de `for` aninhado. Por isso ele esperou um consumidor de validacao em vez
de subir sozinho.

**O Bulwark foi esse consumidor** (degrau 07 daquele repo), e a extracao
aconteceu com ele.

## O que mudou na extracao

As copias dependiam de `$script:texW` e `$script:pixels` — variaveis do escopo
do script chamador, o que so funciona por acidente de escopo e quebra em
silencio se o tool renomear a variavel. Aqui isso virou um CANVAS explicito: as
funcoes recebem o alvo em vez de adivinhar onde ele mora.

**A ARTE (paleta, celulas, retangulos) segue sendo identidade de cada jogo** e
fica no tool do jogo. Isto aqui e so o pincel.

Os cinco tools existentes NAO migraram: o atlas deles ja estava gerado e
versionado — mesma regra que valeu na extracao do `Write-Dds`.

## Emenda (0.11.0, 2026-07-31): o `Paint-Mask`

Os DEZ primeiros tools do ecossistema desenharam so RETANGULOS — chao, parede,
tijolo, torre, nave — e `Fill-Rect` deu conta em todos.

**O Klondike foi o primeiro com arte que nao e retangulo:** um naipe de copas
tem dois lobos e um bico, paus tem tres circulos. Escrever isso em retangulos
seria ilegivel no tool e feio na tela, entao os quatro naipes viraram MASCARAS
de texto (`#` pinta, `.` deixa passar) — a unica forma em que a arte do arquivo
se PARECE com a arte da tela.

Ficou registrado como observacao de UMA evidencia, com a formula da casa: *o
proximo jogo com arte que nao seja retangulo EXTRAI, nao copia.*

**O Counter (12o jogo) foi esse segundo**, e pelo mesmo motivo: silhueta de
pessoa e xicara tambem nao sao retangulos. A extracao aconteceu COM ele — o
mesmo caminho do `forgeaudio`, do `Write-Dds` e do proprio `Paint.ps1`.

**O que subiu e o pincel; as MASCARAS ficam no tool de cada jogo**, porque elas
sao a arte. E o Klondike nao migra, pela regra de sempre.

### O que o segundo consumidor teve a dizer sobre a API

**Nada** — usou a forma do primeiro sem pedir mudanca, e so acrescentou o
`Alpha` opcional que o `Set-Pixel` e o `Fill-Rect` ja tinham, por simetria.

E o mesmo sinal que a task 27 da engine registrou como o mais forte que este
filtro consegue dar.

## Criterios de Aceite

- [x] `New-AtlasCanvas`, `Set-Pixel` e `Fill-Rect` com canvas explicito.
- [x] `Paint-Mask` (0.11.0), com o Counter como consumidor de validacao.
- [x] Os tools antigos nao migram.
- [x] A arte (paleta, retangulos, mascaras) continua no tool de cada jogo.
