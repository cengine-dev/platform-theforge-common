# 15 - forgemesh: variantes, e um layout de vertice SO

- **Status:** **15a done (0.20.1)** — 2026-09-06. **15b todo**, esperando a task 12
- **Categoria:** Plataforma / 3D — **divida da task 11**
- **Registrada em:** 2026-09-06, na revisao do degrau 05 do `diorama`
- **Bloqueia:** tasks 12 e 14
- **Dividida em duas metades, e elas tem prazos diferentes** — ver "Quando fazer
  cada metade", abaixo. **Depende da task 16** (o alvo de build), que deve vir
  primeiro.

## O erro de desenho

A task 11 chamou o `forgemesh` de **"o quarto irmao das pontes de desenho"**.
Ele nao e irmao delas, e isso agora e demonstravel.

`forgeui`, `forgesprite` e `forgeline` fazem **uma coisa fixa cada**: texto,
quad, linha. Por isso um shader e um pipeline bastam, e por isso a metafora de
"ponte" funciona para os tres.

O `forgemesh` tem trabalho **aberto**, e as tasks que ja estao escritas dizem o
quanto:

| task | o que exige | o que quebra hoje |
|---|---|---|
| 12 | *"um descriptor set por material"* + *"o que nao tem textura continua funcionando pelo mesmo caminho"* | duas variantes de shader, no minimo |
| 14 | *"o `mesh.vert.fsl` ganha os indices e pesos de junta"* | **outro vertex layout inteiro** |

Hoje ha exatamente **um** shader, **um** layout, **um** pipeline, **um**
descriptor set.

## O defeito que ja existe

O `VertexLayout` esta escrito **duas vezes** no `ForgeMesh.h`:

| linha | para que |
|---|---|
| ~246 | o pipeline (`addPipeline`) |
| ~358 | a carga da geometria (`GeometryLoadDesc`) |

As duas **tem** de concordar. Se divergirem, o `ResourceLoader` copia errado sem
reclamar — e o sintoma e o mesmo dos **cubos pretos** do degrau 04: o
`ASSERT(dstFormatSize == srcFormatSize)` de `ResourceLoader.cpp:1767` so existe
em Debug.

**Nao e risco hipotetico: e a repeticao exata de um defeito que ja custou um
teste do dono.**

## Quando fazer cada metade

Revisto em 2026-09-06, respondendo a pergunta do dono: *"pode mudar o rumo de
alguma solucao depois de algumas tasks acontecerem?"*. **Pode, e desta task e a
metade que mais pode.**

| metade | risco de mudar de rumo | quando |
|---|---|---|
| **15a — o layout numa funcao so** | **nenhum**: e remover duplicacao de algo que ja PRECISA concordar | **agora**, depois da task 16 |
| **15b — a maquina de variantes** | **alto** | quando a task 12 a FORCAR |

**Por que a 15b espera.** Montar o enum agora e projetar contra requisitos que
ainda nao existem. Eu suporia que os eixos sao "texturada" e "com esqueleto" —
mas a propria task 12 diz *"material sem mapa e um valor constante, nao um
erro"*, o que sugere que textura pode **nao ser variante de shader**, e sim um
uniforme. E o skinning da task 14 precisa de um buffer estruturado de matrizes
de junta, que mexe no **SRT**, e nao so no layout.

> Desenhar a abstracao antes do segundo caso e o ponto cego conhecido deste
> metodo — *"enviesado para EXTRACAO, e por isso estruturalmente cego para
> abstracao que so paga se desenhada antes de existir"*. A diferenca e que aqui
> o segundo caso esta a UMA task de distancia, e escrito. Vale esperar por ele.

**O que a 15a entrega sozinha:** o defeito do layout duplicado fecha, e o lugar
onde a variante vai caber (uma funcao com um parametro) fica pronto. A 15b so
preenche.

## Escopo

**1. O layout vira UMA funcao, derivada da variante.** *(metade 15a)*

```cpp
enum Variante : uint32_t { kSimples = 0, kTexturada = 1 << 0, kComEsqueleto = 1 << 1 };

// A UNICA definicao de layout do modulo. Chamada pelo pipeline E pela carga.
VertexLayout layoutDe(uint32_t variante);
```

Isto sozinho ja fecha o defeito acima, e e a parte que vale fazer mesmo que o
resto espere.

**2. O pipeline passa a ser por variante, criado sob demanda e cacheado.** *(15b)*

```cpp
Pipeline* pipelineDe(uint32_t variante);   // lazy, guardado num array pequeno
```

Os shaders seguem a convencao que o proprio The-Forge usa para variantes
(`imgui_SAMPLE_COUNT_1.frag`, `imgui_SAMPLE_COUNT_2.frag`...): `mesh.vert`,
`mesh_skinned.vert`, e assim por diante.

**3. A malha LEMBRA a variante dela.** *(15b)*

```cpp
Handle loadMesh(const char* binPath, uint32_t variante = kSimples);
```

A variante e decidida na carga porque e ela que escolhe o layout pedido ao
`ResourceLoader`. O `draw` usa a variante da malha para escolher o pipeline —
entao **nao ha como desenhar uma malha com o pipeline errado**, que e a
propriedade que hoje depende de haver so um.

## O que este escopo NAO faz

Nao transforma o `forgemesh` num sistema de material. O material em si (base
color, mapas, parametros) e a task 12; aqui so nasce o **lugar** onde ele vai
caber.

E nao renomeia o modulo. O nome pode continuar; o que muda e a documentacao
parar de dizer que ele e irmao das pontes 2D — **ele carrega, guarda e desenha,
e isso e outra categoria de coisa.**

## Criterios de Aceite

1. O layout de vertice aparece **uma vez so** no arquivo.
2. Duas variantes coexistem, cada uma com o pipeline dela, e desenhar uma malha
   carregada como `kSimples` continua idêntico ao de hoje.
3. Um consumidor 2D (`mesh.enabled = false`) nao paga nada: nenhum pipeline a
   mais criado.
4. O `diorama` no degrau 06 nao muda uma linha por causa desta task.

## 15a fechada em 2026-09-06

`layoutDe(uint32_t variante)` — **a unica definicao de layout de vertice do
modulo**. As duas copias (a do pipeline, a da carga) sumiram; `grep` por
`mAttribs[0].mSemantic = SEMANTIC_POSITION` acha **um** lugar.

Junto veio o `enum Variante` com **um valor so** (`kSimples`). Ele nao faz nada
hoje: e o parametro onde a 15b vai encaixar os eixos que a task 12 definir.

### Um detalhe que o compilador ensinou

O layout nao pode ser `const` nos dois lugares:

| | tipo do campo |
|---|---|
| `GeometryLoadDesc::pVertexLayout` | `const VertexLayout*` |
| `GraphicsPipelineDesc::pVertexLayout` | `VertexLayout*` (**nao**-const) |

A carga aceita const; o pipeline nao. Esta escrito no ponto de uso, porque e o
tipo de assimetria que faz alguem "consertar" o const de volta e quebrar o build.

### Os criterios, medidos

1. **O layout aparece uma vez so** no arquivo. ✅
2. Duas variantes coexistindo — **isto e da 15b**, e nao foi feito.
3. Consumidor 2D (`mesh.enabled = false`) nao paga nada: nenhum pipeline a mais.
   ✅ (a funcao so e chamada dentro do `load`/`loadMesh`, que ja eram guardados
   por `gEnabled`).
4. **O `diorama` nao mudou uma linha.** ✅ E o executavel saiu com o mesmo
   tamanho de antes (`709.632 bytes`), que e a evidencia de que a refatoracao
   nao mudou comportamento.

### O que a 15b ainda espera

A task 12, que define se textura e uma VARIANTE de shader ou um UNIFORME — a
propria spec dela sugere o segundo (*"material sem mapa e um valor constante,
nao um erro"*). Com a 15a feita, ela encaixa em `layoutDe` e num
`pipelineDe`; sem a 12, seria um enum inventado.

**E a 16 ja passou**, entao a 15b nao precisa mais caber num cabecalho.

## A task 12 respondeu, e a 15b ENCOLHEU (2026-09-07)

A pergunta era: textura e variante de shader ou uniforme? **Nenhuma das duas.**
E um descritor sempre ligado, com um padrao -- branco 1x1 no mapa de cor, normal
plana 1x1 no de normal. Sem `if`, sem variante.

Sobra **um eixo so** para a 15b: o **skinning** (task 14), que muda o vertex
layout de verdade (`JOINTS` e `WEIGHTS`) e exige outro vertex shader.

Ou seja: a 15b deixou de ser "uma maquina de variantes" e virou "duas variantes".
Vale reavaliar se um `enum` com dois valores merece maquinaria, ou se dois
pipelines nomeados bastam -- **e essa pergunta so se responde fazendo a task 14**.
