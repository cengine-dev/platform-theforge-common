# 22 - a tangente que nao veio: detectar atributo AUSENTE no `.bin`

- **Status:** ABERTA — nasceu de uma correcao que se descobriu impossivel como escrita
- **Categoria:** Plataforma (conferencia de asset)
- **Registrada em:** 2026-09-07, na revisao arquitetural (achado 3.13, metade nao resolvida)

## O problema

`forgemesh::layoutDe()` pede `SEMANTIC_TANGENT` sempre. Quando o `.bin` nao tem
— e era o caso de TODOS ate o degrau 07 do `diorama`, e continua sendo o do
`esqueleto.bin` — o `ResourceLoader` pula em silencio:

```
ResourceLoader.cpp:1793   continue;   // atributo ausente: nao copia, nao avisa
```

O campo fica com o que estiver na alocacao. Hoje isso e inofensivo **por
acidente**: o mapa de normal padrao e plano (`nxy ~ 0`), entao a tangente
praticamente some da conta. No dia em que alguem aplicar um normal map de
verdade numa malha sem tangente, sai relevo errado — e **a imagem continua
parecendo uma imagem com relevo**, que e a pior classe de defeito deste
ecossistema.

## Por que a correcao obvia nao funciona

A revisao propunha duas coisas. A primeira foi feita:

- **conferir o passo do vertice na carga** contra `kPassoEsperado`. Feito, e util
  — mas ele NAO detecta este defeito. O passo sai do layout PEDIDO, e nao do
  arquivo; ele so pega o layout ter mudado sem o cabecalho ser relido.

A segunda **nao e implementavel como escrita**:

> *"`loadMaterial` recusar `normalPath` numa malha cuja tangente nao veio"*

Dois motivos, e o segundo e o que fecha a porta:

1. `loadMaterial` nao sabe com que malha o material sera usado — materiais e
   malhas sao handles independentes, e o cruzamento so acontece no `draw`;
2. **o `Geometry` nao guarda quais semanticas o arquivo tinha.** Ele tem
   `mVertexStrides[]`, `mVertexBufferCount` e as contagens
   (`IResourceLoader.h:173-207`). A lista de atributos do arquivo vive em
   `GeometryData::ShadowData::pAttributes[MAX_SEMANTICS]` (linha 232) — que so
   existe com `GEOMETRY_LOAD_FLAG_SHADOWED`.

E a ausencia dessa flag **e o ponto da task 11**: pedi-la significa trazer a
malha de volta para a CPU, que e exatamente o que esta ponte tornou
desnecessario.

## As saidas

### A. Ler o cabecalho do `.bin` no `loadMesh`

Abrir o arquivo e conferir a lista de atributos antes de entregar ao loader. O
formato e do `AssetPipelineCmd` e esta no The-Forge.

- **A favor:** responde exatamente a pergunta, e uma vez por carga.
- **Contra:** o casco passa a depender do FORMATO do `.bin`, e nao so da API do
  loader. Se o The-Forge mudar o formato, quebra aqui.

### B. Um sidecar do pipeline de asset

O `blender_para_assets.py` do `diorama` grava, ao lado do `.bin`, o que ele
exportou. O `loadMesh` le e confere.

- **A favor:** nao acopla o casco ao formato binario; o dado vem de quem o
  produziu.
- **Contra:** e mais um arquivo que pode faltar, e "faltou o sidecar" viraria um
  aviso que se aprende a ignorar. E o pipeline de asset e do lab, nao do casco.

### C. Assumir e DECLARAR, em vez de detectar

O `MaterialDesc` ganha `exigeTangente` (ou o `draw` recebe a informacao), e quem
carrega a malha declara se ela tem. O casco confere a declaracao contra o uso —
nao contra o arquivo.

- **A favor:** barato, e move a responsabilidade para quem sabe (o lab sabe quais
  `.blend` tem normal map, e o `_exportar_gltf.py` ja liga `export_tangents`).
- **Contra:** declaracao pode mentir, e o defeito volta. E "transcricao sem quem
  a confira e deriva esperando acontecer" — a licao da task 05b do lab.

## O gatilho

**Nao ha consumidor sofrendo hoje.** O unico `.bin` com normal map do banco de
provas (`textura.bin`) TEM tangente, porque o degrau 07 corrigiu o exportador.

Entao esta task espera o segundo modelo com normal map — provavelmente o modelo
do dono, no degrau 09. Abri-la agora e registrar que o buraco existe e que a
correcao obvia nao cabe, para nao ser redescoberto.

## Criterio de aceite

Uma malha SEM tangente, com um material que tem `normalPath`, produz um aviso
alto na carga ou no primeiro desenho — e nao uma superficie com relevo plausivel
e errado.
