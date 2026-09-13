# 18 - O conjunto se chama `PerFrame` e nao e

- **Status:** **done (0.23.0)** — 2026-09-13
- **Categoria:** Plataforma / 3D — divida das tasks 11 e 13
- **Registrada em:** 2026-09-06, na revisao do degrau 06 do `diorama`
- **Absorve parte da:** task 17 (instancias), que cresceu por causa disto

## O problema

O `mesh.srt.h` declara:

```
BEGIN_SRT_NO_AB(MeshSrtData)
    BEGIN_SRT_SET(PerFrame)
        DECL_CBUFFER(PerFrame, CBUFFER(UniformeDaMalha), gMalha)
```

**Mas o conjunto e ligado por DESENHO**, e o buffer carrega tres frequencias
diferentes:

| campo | frequencia REAL |
|---|---|
| `mvp`, `modelo` | por **objeto** |
| `luzDirecao`, `luzCor`, `ambiente` | por **quadro** |
| `albedo` | por **material** (task 12) |

O nome ja mentia na task 11 -- o `mvp` sempre foi por objeto. A task 13 pos dado
genuinamente por quadro ao lado, e agora a mentira e dupla.

## Por que isso importa, alem do nome

**A luz e copiada para dentro de cada objeto desenhado.** Hoje sao dois objetos e
o custo e irrisorio; o problema nao e o desperdicio, e o que ele impede:

1. **A task 12 nao tem onde por material.** Sem separacao de frequencias, a
   resposta vai ser "mais um campo no mesmo buffer" — e material e por MATERIAL,
   nao por objeto nem por quadro. Seria a terceira frequencia empilhada no mesmo
   lugar.
2. **A task 17 (instancias) cresceu.** Ela previa trocar "um buffer por
   instancia" por offsets dinamicos ou buffer estruturado. Com dado por quadro
   misturado, a troca deixa de ser mecanica: e preciso decidir o que vai para
   onde antes de escolher a mecanica.

## A task 12 chegou, e trouxe a evidencia (2026-09-07)

O material ficou **partido em duas frequencias**:

| parte do material | onde vive | quando e escrita |
|---|---|---|
| os dois mapas | descriptor set `PerBatch` | quando o material muda |
| o `baseColorFactor` | uniforme **por desenho** | **toda vez que se desenha** |

Consequencia pratica: **"material" nao e uma unidade.** Nao da para ligar um
material com uma chamada, e dois objetos com o mesmo material reescrevem o mesmo
fator duas vezes.

Isso deixa de ser previsao e vira um caso concreto: ha dado **por material** no
buffer **por desenho**, ao lado de dado por quadro (a luz) e por objeto (as duas
matrizes). **Tres frequencias num buffer so, mais uma quarta parte do mesmo
conceito noutro conjunto.**

O escopo abaixo ganha um item: o `baseColorFactor` (e o que a task 12 vier a
acrescentar de parametro escalar) muda para o conjunto `PerBatch`, junto dos
mapas.

## Escopo (quando a task 12 chegar)

Separar em conjuntos por frequencia, que e para isso que o SRT do The-Forge tem
`PerFrame`, `PerBatch` e `PerDraw`:

| conjunto | conteudo | ligado |
|---|---|---|
| `PerFrame` | camera + luz + ambiente | uma vez por quadro |
| `PerBatch` | material: os mapas **e o `baseColorFactor`** | quando o material muda |
| `PerDraw` | `mvp` e `modelo` | por objeto |

**Nao fazer antes da task 12.** Com duas frequencias so, a separacao seria uma
adivinhacao sobre onde material cabe — e material e justamente o caso que decide.
Mesma razao pela qual a 15b e a 17 esperam.

## Criterios de Aceite

1. Cada conjunto e ligado na frequencia do nome dele.
2. A luz e escrita **uma vez por quadro**, e nao uma vez por objeto.
3. A API publica (`setCamera`, `setLight`, `draw`) nao muda.
4. Nenhum consumidor 2D e afetado.

## Fechada em 2026-09-13

Tres conjuntos, cada um na cadencia do proprio nome. Conferido no HLSL gerado:

```
CBUFFER(UniformeDoQuadro)    gQuadro   : register(b0, space1)   PerFrame
CBUFFER(UniformeDoMaterial)  gMaterial : register(b0, space2)   PerBatch
Tex2D(float4)                gMapaDeCor              (space2)
Tex2D(float4)                gMapaDeNormal           (space2)
CBUFFER(UniformeDoObjeto)    gObjeto   : register(b0, space3)   PerDraw
```

### O que encolheu

| | antes | depois |
|---|---|---|
| buffers do QUADRO | 128 (dentro de cada objeto) | **2** (um por quadro em voo) |
| buffer POR DESENHO | 192 bytes | **64** (so a matriz de modelo) |
| escritas da luz por quadro | uma por corpo | **uma** |
| escritas do fator de cor | uma por desenho | **uma, na carga do material** |

### O contrato NOVO, e ele precisa ser dito

Camera, luz, ambiente e o modo de conferencia vao a GPU **no primeiro `draw` do
quadro**. Chamar `setCamera`/`setLight`/`setAmbient`/`setModoDeConferencia`
depois disso nao tem efeito naquele quadro.

Antes tinha, e por acidente: cada objeto carregava a propria copia, entao mexer
no meio do quadro pegava a partir do proximo corpo. **Era um comportamento que
ninguem tinha pedido e que ninguem podia explicar.**

Como a mudanca e silenciosa para quem dependia dela, os quatro setters **avisam
uma vez** quando chegam tarde:

```
[forgemesh] setCamera chamado DEPOIS do primeiro draw do quadro: sem efeito ate
o proximo. Estado de quadro (camera, luz, ambiente, modo) vai a GPU no primeiro
desenho.
```

### Uma inversao registrada

O modo de conferencia mudou de lugar E de sentido: era o `w` do fator de cor,
com **zero** ligando; agora e `modoDeConferencia.x`, com **diferente de zero**
ligando. Esta escrito nos dois lados (o `.srt.h` e o `ForgeMesh.h`), porque um
sentido invertido aqui nao da erro: da a imagem errada.

### O que isto libera

**A task 14 (skinning) tem onde por as matrizes de junta:** um array **por
objeto animado** cabe no `PerDraw`, sem empilhar uma quarta cadencia num buffer
que ja misturava tres.

**A task 17 encolheu de novo:** o buffer por desenho e um terco do que era, e
agora e homogeneo — trocar a mecanica (offset dinamico ou buffer estruturado)
voltou a ser a troca mecanica que a task previa antes da task 13.
