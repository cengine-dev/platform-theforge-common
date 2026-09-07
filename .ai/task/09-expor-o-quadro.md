# 09 - O casco expoe o QUADRO (forgeframe)

- **Status:** **done (0.16.0, 2026-09-03)** — consumidor de validacao: o lab
  `diorama`, degrau 02 (o primeiro do ecossistema a montar pipeline proprio).
- **Categoria:** Plataforma (pre-requisito de todo o pipeline 3D)
- **Registrada em:** 2026-09-03, a partir do teto medido pelo Vigil (degrau 24)

## O problema, medido e nao suposto

O `TheForgeWindowManager` expoe cinco metodos de ciclo de vida -- `init`,
`update`, `present`, `shouldClose`, `cleanup` -- e **nada mais**. Nem
`Renderer*`, nem o `Cmd` do quadro, nem o render target. Todos vivem como
estaticos de arquivo em `TheForgeWindowManager.cpp:49-52` e `:489`.

A consequencia foi medida no Vigil: uma cena **nao consegue** criar pipeline,
shader, constant buffer ou depth target. O shader do batcher de linhas recebe
posicao **ja em NDC** (`Shaders/FSL/line.vert.fsl`) e declara por escrito *"sem
SRT: este shader nao le recurso nenhum"*.

> **Sem lugar onde passar uma matriz, a projecao TEM que ser na CPU.** Nao foi
> escolha de design -- foi consequencia.

E dai desceu todo o resto: descarte de face na CPU (com o sinal invertido por um
degrau inteiro), profundidade virando ordem do pintor (que ainda erra 6,8% dos
pixels do `prisao`), e iluminacao virando um rig ajustado a mao contra uma
captura do Blender.

Ironia final: o The-Forge **ja sobe a malha para a GPU** -- `Geometry` traz
`pIndexBuffer` e os vertex buffers prontos. O Vigil pede
`GEOMETRY_LOAD_FLAG_SHADOWED` para ter uma copia na CPU e **ignora os buffers de
GPU**, porque nao tem como desenha-los.

## Escopo

Um cabecalho novo, `ForgeFrame.h`, que publica o que o casco ja tem em maos
durante o quadro:

```cpp
namespace forgeframe {

// Validos entre o begin e o fim do quadro (o par update()/present()).
[[nodiscard]] Renderer* renderer();
[[nodiscard]] Cmd*      cmd();

// O alvo do quadro, para quem precisa montar pipeline (formato, MSAA).
struct Target { TinyImageFormat colorFormat; TinyImageFormat depthFormat;
                SampleCount sampleCount; uint32_t sampleQuality;
                uint32_t width; uint32_t height; };
[[nodiscard]] const Target& target();

// O indice do frame in flight -- quem tem buffer dinamico precisa dele.
[[nodiscard]] uint32_t frameIndex();

} // namespace forgeframe
```

## O risco, nomeado antes da primeira linha

**Expor o `Cmd` cru convida o jogo a fazer qualquer coisa com ele.** Um jogo que
chame `cmdBindRenderTargets` no meio do quadro quebra o casco em silencio.

O contorno **nao** e esconder: e dizer por escrito quem e o consumidor
pretendido. O `forgeframe` existe para **modulos do casco** (`forgemesh`, task
11) e para o jogo que precise de um pipeline proprio -- que e exatamente o caso
que hoje nao existe. A regra fica no cabecalho: *quem liga render target assume
o quadro inteiro; o casco nao restaura*.

Precedente da casa: o `forgeui::mouse()` e o `forgeui::keyboard()` ja expoem a
porta crua "para quem quiser falar direto", ao lado da fachada global. Mesma
forma.

## Criterios de Aceite

1. `forgeframe::renderer()` e `cmd()` devolvem os mesmos ponteiros que o casco
   usa, e sao **nulos fora do quadro** (antes do `update`, depois do `present`).
2. Os **8+ jogos 2D nao mudam uma linha** e continuam compilando e rodando.
3. Um teste manual: um `cmdDraw` cru vindo de fora do casco aparece na tela.
4. O cabecalho declara, por escrito, o contrato de quem liga render target.

## O que esta task NAO faz

Depth buffer (task 10), malha (11), material (12), luz (13), esqueleto (14).
Ela so **abre a porta**. E a unica das seis que nao apaga codigo nenhum -- todas
as outras apagam mais do que acrescentam.

## Correcao na 0.17.0: um `.cpp` novo nunca e aditivo

Esta task foi entregue como `ForgeFrame.h` + `ForgeFrame.cpp`, e afirmou por
escrito que era **adicao pura, verificavel lendo o diff**. Era falso, e o diff
mostrava: o `TheForgeWindowManager.cpp` passou a chamar `forgeframe::publish`, e
**todo jogo do ecossistema lista aquele `.cpp` no `.vcxproj` mas nao listava o
novo.** Link quebrado por simbolo externo nao resolvido, em 8+ consumidores.

Medido: `vigil`, `cue` e `fold` listam `TheForgeWindowManager.cpp` e nao listam
`ForgeFrame.cpp`. Nada chegou a ser commitado, mas a quebra estava viva em disco.

**A causa e estrutural, e vale para toda task futura deste repo:** o casco nao e
uma biblioteca com alvo de build — cada jogo **enumera os `.cpp` do casco no
proprio `.vcxproj`**. Entao um arquivo `.cpp` novo aqui e sempre uma edicao
obrigatoria em todo projeto que ja existe, por mais aditivo que o CODIGO seja.

Correcao: o `ForgeFrame` virou so cabecalho, com o estado em variavel `inline`
(C++17) — uma instancia por programa, zero unidade de traducao nova.

> **A licao e sobre o metodo, e ela dobra a regra que ja existia.** Eu li o diff
> procurando simbolo REMOVIDO e assinatura MUDADA. A quebra veio de uma
> referencia **ACRESCENTADA**. Ler o diff so prova compatibilidade se a leitura
> incluir *o que o casco passou a EXIGIR de quem o compila* — simbolos novos e
> arquivos novos, nao so o que sumiu.
