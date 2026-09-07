# 17 - Como o forgemesh entrega a matriz por objeto

- **Status:** **adiada, com gatilho escrito** (ver abaixo)
- **Categoria:** Plataforma / 3D — divida da task 11
- **Registrada em:** 2026-09-06, na revisao do degrau 05 do `diorama`

## O que existe hoje

Um constant buffer por **(quadro em voo x instancia)**. Com o default
(`frameCount = 2`, `maxInstances = 64`) sao **128 objetos `Buffer`**, e o
`draw` indexa o descriptor set por `quadro * maxInstances + usadas`.

Funciona, esta documentado, e foi a escolha certa para a task 11: e a mecanica
que os batchers 2D ja usavam e que ja estava provada.

## Por que ela nao escala

Ela cresce **linearmente com o teto**, e nao com o uso. Um consumidor com 500
objetos por quadro pediria 1000 alocacoes de buffer — e o desenho normal para
isso e outro:

| alternativa | como |
|---|---|
| offset dinamico | UM buffer por quadro, `maxInstances * alinhado`, com o descriptor apontando para o trecho |
| buffer estruturado | um array de matrizes indexado por `SV_InstanceID` ou por constante de raiz |

**E substituicao, nao extensao.** A API publica (`setCamera` + `draw`) nao muda;
o que muda e tudo por baixo dela — o que e uma boa noticia e a razao de dar para
adiar.

## Por que ADIADA, e nao "todo"

Nenhum consumidor chega perto do teto. O `diorama` desenha **dois** objetos, e o
plano dos degraus 06 a 09 nao aumenta esse numero: eles trocam o SOMBREADO do
mesmo modelo, nao a quantidade.

Trocar agora seria otimizar contra um consumidor imaginario — exatamente o ponto
cego que o ecossistema evita com *"cada modulo entra no degrau que o forca, e
nunca antes"*.

## O GATILHO

> Esta task sai de "adiada" quando **um consumidor real desenhar mais de ~50
> objetos por quadro**, ou quando `Stats::dropped` for diferente de zero num
> consumidor de verdade.

Os dois numeros sao observaveis pelo `forgemesh::lastFrameStats()`, que ja
existe e ja reporta `draws`, `dropped` e `instanceCapacity`. **O gatilho e
mensuravel sem instrumentacao nova** — que e a condicao para um adiamento ser
honesto em vez de esquecimento.

## Criterios de Aceite (quando disparar)

1. A API publica nao muda: `setCamera` + `draw` continuam iguais.
2. O numero de objetos `Buffer` deixa de crescer com `maxInstances`.
3. `Stats` continua reportando o teto real.

## Esta task CRESCEU na revisao do degrau 06 (2026-09-06)

A task 13 pos dado **por quadro** (luz e ambiente) dentro do mesmo buffer que ja
tinha dado **por objeto** (`mvp`, `modelo`). Com isso, trocar a mecanica de
instancias deixou de ser uma troca mecanica: **e preciso decidir o que vai para
onde antes de escolher como entregar.**

Essa decisao virou a **task 18** (frequencias do uniforme), e ela vem primeiro.
O gatilho desta aqui continua valendo, mas a ordem agora e `12 -> 18 -> 17`.
