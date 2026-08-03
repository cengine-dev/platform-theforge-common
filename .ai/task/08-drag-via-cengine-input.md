# 08 - ForgeUi delega o ARRASTAR para a cengine::input

- **Status:** done (0.12.0, 2026-08-03)
- **Prioridade:** media - o segundo consumidor chegou e nao pediu mudanca.
- **Categoria:** Plataforma
- **Depende de:** task 03 (o mesmo movimento, com o teclado).

## Contexto

O vocabulario de arrastar nasceu aqui na 0.12.0 do jogo... nao: nasceu aqui na
**0.10.0**, com o Klondike, e ficou LOCAL de proposito.

O ledger deste repo registrava o caminho antes de ele acontecer:

> Ficam aqui, e o caminho e o que o mouse ja percorreu (task 03 deste repo ->
> task 27 da cengine): viveu no `forgeui` por dois jogos e so subiu para a
> engine quando o 2o consumidor usou a forma do 1o sem pedir mudanca de API.

O segundo consumidor chegou — **cue, degrau 06, a tacada por arrasto** — e o gate
foi cumprido ao pe da letra.

## O que mudou aqui

O `ForgeUi.cpp` perdeu **o estado, a fila e o teto** (`gDrag`, `gDrops`,
`kDropQueueMax`) e passou a delegar para a instancia de `cengine::input::Mouse`
que ele ja guardava desde a 0.9.0.

`forgeui::DragState` e `forgeui::Drop` viraram **alias** dos tipos da engine, e
`drag()`/`readDrop()`/`pushMouseDown`/`pushMouseUp`/`cancelDrag` continuam
existindo com a mesma assinatura.

**Nenhum jogo mudou uma linha.** Exatamente como na task 03 (teclado) e na
promocao do mouse (0.9.0).

## O que ficou aqui, e nunca foi candidato

**O `SetCapture` e o `WM_CAPTURECHANGED`.** Sem capturar o ponteiro no aperto,
soltar o botao fora da janela nao gera `WM_LBUTTONUP` nenhum e o gesto fica
pendurado para sempre. Mas isso e Win32, e **o WndProc E o casco** — nao ha o que
subir.

O que subiu foi o `cancelDrag()`: ele e a resposta em vocabulario de PORTA ao que
este casco descobre em vocabulario de JANELA.

## A recusa a opinar deste casco foi TESTADA

O `ForgeUi.h` sempre disse que decidir "isto foi um clique ou um arrasto?" e
politica do JOGO, *porque depende do tamanho do alvo e de quanto tremor de mao se
perdoa*.

Ate aqui isso era uma boa razao. Agora e um **fato medido**:

| jogo | folga | tamanho do alvo | o que a folga mede |
|---|---|---|---|
| klondike | 8 px | carta de 90 px | errou o alvo? |
| cue | 12 px | bola de 20 px | quis mesmo tacar? |

Um casco que tivesse fixado 8 estaria errado no segundo jogo — e nem e a mesma
pergunta. **E a primeira vez que uma recusa a decidir e validada por um segundo
caso, em vez de so declarada.**

## Criterios de Aceite

- [x] `ForgeUi.cpp` sem estado local de arrasto; tudo delegado a porta.
- [x] `forgeui::DragState`/`Drop` como alias; assinaturas inalteradas.
- [x] O `SetCapture` e o `cancelDrag` do WndProc intactos.
- [x] Klondike e cue buildam e rodam sem mudar uma linha.
- [x] Ledger deste repo e da cengine atualizados.
