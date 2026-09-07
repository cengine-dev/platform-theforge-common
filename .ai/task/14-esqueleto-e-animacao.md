# 14 - Esqueleto e animacao, com skinning na GPU

- **Status:** todo
- **Categoria:** Plataforma (a ultima do pipeline, e a mais cara)
- **Registrada em:** 2026-09-03
- **Depende de:** task 11 (obrigatoriamente)

## A licao que reordenou esta task

O plano do Vigil tinha "animacao esqueletica" como degrau C, cedo. Ao fechar, o
degrau 24 registrou o erro:

> **Animacao sem skinning na GPU nao faz sentido** -- o que mostra que o degrau
> C estava mal colocado no plano desde que foi escrito.

Deformar a malha na CPU a cada quadro, para uma malha que ja esta na GPU, e
pagar duas vezes pelo mesmo trabalho e perder as duas. Por isso esta task e a
**ultima**, e depende da 11 de forma dura, nao de conveniencia.

## O problema

| o `AssetPipelineCmd` produz | o jogo consome |
|---|---|
| `skeleton.ozz` + `<nome>.ozz` (`-pa`) | **nao** |

O `-pa` do `AssetPipelineCmd` ja gera o esqueleto e os clipes em formato `ozz`.
O The-Forge ja tem o sistema inteiro montado em
`Common_3/Resources/AnimationSystem/Animation/`: `Rig`, `Clip`,
`ClipController`, `AnimatedObject`, `SkeletonBatcher`.

Nada disso precisa ser escrito. Precisa ser **ligado**.

## Escopo

1. **Carga:** `forgemesh` aceita `skeleton.ozz` e clipes ao lado do `.bin`.
2. **Amostragem:** `ClipController` avanca no `dt` da cengine, `AnimatedObject`
   produz as matrizes de junta.
3. **Skinning na GPU:** as matrizes vao num buffer estruturado; o
   `mesh.vert.fsl` ganha os indices e pesos de junta e deforma no vertex shader.
   Referencia direta: `28_Skinning/Shaders/FSL/skinning.vert.fsl`.
4. **O esqueleto visivel:** `SkeletonBatcher` desenhando as juntas por cima da
   malha -- e ferramenta de conferencia, nao enfeite. E o unico jeito de ver que
   a hierarquia entrou certa antes de a deformacao entrar.

## O que a cengine PODE ganhar aqui, e o que nao pode

**Nao pode:** amostragem de clipe esqueletico, blending, o formato `ozz`. Isso e
mecanismo do The-Forge, e reimplementar seria o oposto do que este projeto e.

**Pode, e ja existe:** o `cengine::anim` de hoje e clipe de QUADROS (2D). Um
`ClipController` -- tempo normalizado, `loop`, `finished` -- ja e da engine
(`ClipDesc::loop` e `Animator::finished`, 0.12.0). Se o controle de tempo do
clipe esqueletico for a mesma coisa, e **extracao**; se for diferente, a
diferenca e o achado. A pergunta fica registrada, sem resposta antecipada.

## Criterios de Aceite

1. Um modelo do Blender com esqueleto e uma acao toca a animacao, com a malha
   deformando -- e a deformacao acontece **no vertex shader**.
2. As juntas desenhadas pelo `SkeletonBatcher` batem com a hierarquia do
   Blender (numero de ossos, pai de cada um, pose de repouso).
3. Um quadro escolhido da animacao, parado, bate com o mesmo quadro no Blender.
4. Dois objetos com o mesmo esqueleto e clipes diferentes rodam ao mesmo tempo.
