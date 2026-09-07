# 12 - Material e textura

- **Status:** **done (0.22.0)** — 2026-09-07
- **Categoria:** Plataforma
- **Registrada em:** 2026-09-03
- **Depende de:** task 11 (sem shader com SRT nao ha onde ligar uma textura)

## O problema

O levantamento do Vigil, na tabela que fechou o projeto:

| o `AssetPipelineCmd` produz | o jogo consome |
|---|---|
| `.bin` -- geometria e dados por vertice | so a geometria |
| `.dds` (`-pt`) | **nao** |
| material compilado | **nao** |

O dono nomeou o problema depois de comparar as capturas:

> *"No Blender, tudo esta num arquivo so. Quem separa e o The-Forge. O que ele
> nao sabe usar, ele descarta e nao avisa."*

O asset ja existe em disco. Falta quem leia.

## Escopo

1. **Textura:** `forgemesh` ganha carga de `.dds` pelo resource loader e um
   descriptor set por material. O `sampler` sai da desc.
2. **Material:** o `ForgeMaterialCompiler` (`The-Forge/Common_3/Tools/`) produz o
   material; o `mesh.frag.fsl` le base color, normal, metallic e roughness.
3. **O que nao tem textura continua funcionando** -- o Vigil descobriu que a cor
   pode vir do MODELO (atributo de vertice) e nao precisou de textura nenhuma
   para as seis criaturas. Material sem mapa e um valor constante, nao um erro.

Referencia: `Examples_3/Unit_Tests/src/06_MaterialPlayground`.

## Criterios de Aceite

1. Um modelo do Blender com base color texturizada aparece **com a textura**,
   conferido contra a captura de referencia do Blender.
2. Um modelo **sem** textura (so cor de vertice, como as criaturas do Vigil)
   continua desenhando certo pelo mesmo caminho.
3. Normal map ligado muda o sombreado de forma visivel e na direcao certa.
4. Trocar o material de um objeto nao exige recarregar a geometria.

## O que fica FORA, e por que

Replicar o Eevee. O criterio do projeto de aplicacao e **paridade de FORMA**:
geometria, esqueleto, animacao, textura e material presentes e corretos, com a
captura do Blender ao lado. A iluminacao e o PBR do The-Forge -- e **a diferenca
fica escrita**, em vez de perseguida.

## Fechada em 2026-09-07

`MaterialDesc` / `loadMaterial` / `draw(malha, modelo, material)`, com um
descriptor set `PerBatch` de dois mapas por material.

### A descoberta que responde a task 15b: **nao ha variante**

A task 15b esperava esta task para saber se textura e uma **variante de shader**
ou um **uniforme**. A resposta e nenhuma das duas: e um **descritor sempre
ligado, com um padrao**.

Sem mapa de cor, o mapa e **branco 1x1**; sem mapa de normal, e **plano 1x1**. O
shader multiplica e transforma como sempre. Nenhum `if`, nenhuma variante,
nenhum caminho de codigo que so alguns modelos percorrem -- e que so alguns
modelos testam.

> E assim que *"material sem mapa e um valor constante, nao um erro"* -- a frase
> que esta task escreveu antes de a task existir -- se realiza sem ramo.

**A 15b encolheu:** o unico eixo de variante que sobra e o **skinning** (task
14), que muda o vertex layout de verdade.

### A TANGENTE nao existia, e teve de ser pedida ao Blender

Normal map exige espaco tangente, e os `.bin` do banco de provas **nao tinham
`TANGENT`** -- medido lendo o `ShadowData`. O `export_tangents` do exportador
glTF do Blender e `False` por padrao.

Corrigido no lab (`tools/_exportar_gltf.py`), e os assets regerados. Depois:

```
orientacao    POSITION(12), NORMAL(4), TANGENT(4), TEXCOORD0(4)
profundidade  POSITION(12), NORMAL(4), TANGENT(4), TEXCOORD0(4)
textura       POSITION(12), NORMAL(4), TANGENT(4), TEXCOORD0(4)
esqueleto     POSITION(12), NORMAL(4), JOINTS(8), WEIGHTS(16), TEXCOORD0(4)
```

**O `esqueleto` continua sem tangente**, e isso esta certo: o Blender so a produz
para malha com material de normal map. Pedir um atributo ausente nao da erro (o
`continue` de `ResourceLoader.cpp:1793`) e o campo fica zerado -- inofensivo,
porque a tangente so e LIDA quando ha mapa de normal, e sem mapa o padrao e plano
e ela some da conta.

### Tres coisas medidas sobre os formatos

| atributo | formato | como o shader desempacota |
|---|---|---|
| `NORMAL` | `R32_UINT` | `decodeDir(unpackUnorm2x16(...))` |
| `TANGENT` | `R32_UINT` | idem (o `AssetPipeline` trata as duas no mesmo ramo) |
| `TEXCOORD0` | `R32_UINT` | **`unpack2Floats`** (half2) |

A UV **nao** e unorm: coordenada de textura passa de 1 quando a textura repete.
Usar o `unpackUnorm2x16` da normal grampearia as coordenadas em [0,1], e o
defeito so apareceria em modelo com UV fora do quadrado.

### O SWIZZLE do normal map, cobrado do degrau 01

A `-pt --normalmap` grava `swizzle = {x,x,x,y}`: **X nos canais de cor, Y no
ALFA, Z reconstruido**. Assumir `RGB = XYZ` -- que e o que quase todo tutorial
diz -- ilumina para o lado errado, e a imagem continua parecendo ter relevo.

### `SRGB` so no mapa de COR

O mapa de normal guarda **direcao**, e nao luz. Interpreta-lo como sRGB distorce
os valores por uma gama de 2.2, e o relevo sai errado de um jeito plausivel.

## O defeito que impediu o executavel de abrir

O primeiro build do degrau 07 passou e **o `.exe` nao subiu**. No log, logo apos
`Adding D3D12 swapchain`:

```
Direct3D12.c:3755  ERR| D3D12MA_CreateResource(...): FAILED with HRESULT: 0x80070057
```

`0x80070057` e `E_INVALIDARG`, e o recurso era a **textura 1x1 padrao**.

**Causa:** o `TextureDesc` foi valor-inicializado e eu nao preenchi
`mSampleCount`. O enum `SampleCount` **comeca em 1** (`IGraphics.h:321`), entao
zero nao e "o padrao": e um pedido de zero amostras, que o D3D12 recusa.

> **A licao e sobre struct valor-inicializada.** `= {}` parece seguro e e, para
> tudo que tem zero como valor util. Para um enum que comeca em 1, o zero e um
> valor **invalido** — e o compilador nao tem como saber a diferenca.
>
> Aqui o defeito foi BARULHENTO, o que e raro neste pipeline: falhou na criacao,
> com HRESULT e linha. As outras texturas do casco (o atlas do `forgesprite`)
> vem de arquivo, e o loader preenche o desc — por isso ninguem tinha esbarrado.

## O que ficou de FORA, e por que

**`metallic` e `roughness`.** A spec desta task os listava, e eles nao entraram:
os dois so significam alguma coisa dentro de um **BRDF especular**, e o shader
deste modulo e Lambert (task 13). Campos numa struct que nenhuma conta le sao
enfeite -- e enfeite que parece funcionalidade.

Entram quando entrar um BRDF que os use, e isso e uma task propria.

**A HANDEDNESS da tangente.** O glTF guarda a tangente como `float4` com
`w = +-1` dizendo de que lado fica a bitangente; o empacotamento octaedral do
`AssetPipeline` carrega so a direcao. A bitangente e reconstruida como
`cross(N, T)`, o que assume `w = +1` e **erra o sinal em malha com UV
espelhada**. Registrado aqui; nao ha consumidor com UV espelhada hoje.

## Criterios de Aceite -- resultado

1. **Textura aparece, conferida contra a captura.** ✅ construido; a conferencia
   e do dono (degrau 07 do `diorama`).
2. **Modelo sem textura desenha pelo MESMO caminho.** ✅ e nao ha o que mostrar
   no codigo: nao existe ramo.
3. **Normal map muda o sombreado.** ✅ a tecla `B` do lab troca entre o material
   com e sem o mapa.
4. **Trocar material nao recarrega geometria.** ✅ o material e um handle
   proprio, e o `draw` liga outro slot do descriptor set.

## A revisao pos-entrega (2026-09-07) — duas correcoes e tres registros

O dono perguntou *"algum ponto que ficou mal arquitetado?"* depois de o degrau 07
passar. Cinco achados; dois viraram correcao na hora.

### Corrigido: o religamento de material ligava `NULL`

O laco que re-liga materiais no `load()` (para o reload de shader) usava
`gTexturas[...]` **direto**, sem o fallback para os mapas padrao. Um material sem
mapa guarda `NULL` ali -- e o caso legitimo do "so fator" --, entao o
religamento poria `NULL` no descriptor set.

Hoje era inofensivo **por acidente**: o `load` roda uma vez, no `init()`, quando
ainda nao ha material carregado, e o laco fica vazio.

> **A pior categoria de defeito: codigo que parece certo, tem comentario
> explicando por que existe, e esta errado — esperando o primeiro reload.**
> Nenhuma execucao teria pego; so a leitura.

### Corrigido: o `loadMaterial` falhava calado

A guarda `gConjuntoDeMaterial == NULL` devolvia `kSemMaterial` sem log. Chamar
antes do `load()` do casco daria um objeto **branco**, plausivel e sem
explicacao.

Num repo cuja licao central e *"o pipeline sai com sucesso sem fazer o
trabalho"*, era mais uma. Todo outro caminho de erro deste modulo loga.

### Registrado: o material esta partido em duas frequencias

O `baseColorFactor` mora no uniforme **por desenho**, e os mapas no conjunto
**por material**. "Material" deixou de ser uma unidade. Virou evidencia concreta
para a **task 18**, que ate aqui era previsao.

### Registrado: nao ha `unload`

A spec da task 11 prometia `void unload(MeshHandle)`, e nunca foi implementado --
nem para malha nem para material. **Item de spec descartado em silencio.** Virou
a **task 20**, com as tres perguntas de desenho que ela precisa responder.

### Registrado: a task 19 piorou

O modo de conferencia e um `lerp`, e `lerp` avalia **os dois lados**. Depois
desta task, o lado descartado passou a incluir duas amostragens de textura e o
Gram-Schmidt do espaco tangente. Nao e um ramo barato: e o caminho do material
inteiro calculado e jogado fora, por pixel.
