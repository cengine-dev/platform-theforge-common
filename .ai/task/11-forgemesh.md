# 11 - forgemesh: a malha na GPU, com shader que le matriz

- **Status:** **done (0.18.0)** — 2026-09-05
- **Categoria:** Plataforma (o coracao do pipeline 3D)
- **Registrada em:** 2026-09-03
- **Depende de:** tasks 09 (o quadro exposto) e 10 (depth buffer)

## O problema

O `AssetPipelineCmd` do The-Forge produz `.bin` com geometria e dados por
vertice, e o `loadGeometry` **ja sobe tudo para a GPU**: `Geometry` traz
`pIndexBuffer` e os vertex buffers prontos.

O Vigil pede `GEOMETRY_LOAD_FLAG_SHADOWED` para ter uma copia na CPU e **ignora
os buffers de GPU** -- porque nao tem pipeline onde desenha-los. Entao reprojeta
vertice a vertice, na CPU, **a cada quadro**, e empurra o resultado pelo vertex
buffer dinamico do `forgeline`. Dai vem o teto de lote (`Stats::dropped`), que so
existe por causa disso.

## Escopo

O **quarto irmao** das pontes de desenho (`forgeui` = texto, `forgesprite` =
quads, `forgeline` = linhas): `forgemesh`, com o mesmo ciclo de vida
`init/load/begin/flush/unload/exit` chamado pelo casco, e a mesma desc opt-in.

```cpp
namespace forgemesh {

struct MeshBatcherDesc { bool enabled = false; uint32_t maxInstances = 512;
                         uint32_t frameCount = 2; };

// Carga (uma vez, fora do quadro).
[[nodiscard]] MeshHandle load(const char* binPath);
void unload(MeshHandle);

// Desenho (modo imediato, dentro do quadro -- como os outros tres).
void setCamera(const float viewProj[16]);
void draw(MeshHandle, const float model[16]);

} // namespace forgemesh
```

O shader (`mesh.vert.fsl` / `mesh.frag.fsl`) **tem SRT** -- e essa e a diferenca
que muda tudo. Um constant buffer por quadro com a view-projection, e a matriz
de modelo por objeto (push constant ou buffer indexado por instancia).

Referencia no The-Forge: `Examples_3/Unit_Tests/src/28_Skinning/Shaders/FSL/`
(o `Global.srt.h` e o padrao a copiar).

## Criterios de Aceite

1. Um `.bin` do `AssetPipelineCmd` desenha na tela **sem uma linha de projecao na
   CPU** e sem `GEOMETRY_LOAD_FLAG_SHADOWED`.
2. O mesmo modelo desenhado 100 vezes com matrizes diferentes nao muda o custo
   por CPU (a matriz vai no buffer, a geometria nao volta a passar).
3. O descarte de face e do **rasterizador**, e nao da CPU. (No Vigil o sinal
   ficou invertido por um degrau inteiro -- o jogo desenhava as COSTAS.)
4. `forgeline` e `forgesprite` continuam funcionando no mesmo quadro, por cima.

## O que mudou da spec para o que foi construido

**1. `maxInstances` caiu de 512 para 64.** A spec supunha um buffer compartilhado
indexado por instancia; o que foi construido usa **um constant buffer por
(quadro em voo x instancia)**, seguindo a mecanica ja provada dos batchers. 512
seriam 1024 alocacoes para uma porta que nenhum consumidor usa com mais de um
punhado de objetos. O teto e conferivel pelo `Stats::instanceCapacity`.

**2. A API ficou `setCamera` + `draw`, com `mat4` e nao `float[16]`.** A ponte e
inerentemente 3D; esconder isso atras de um ponteiro de float so adiaria o erro
de convencao para o momento em que ele nao da mensagem.

**3. Apareceu um `ForgeMeshDesc.h`.** O `TheForgeWindowDesc` precisa da
`MeshBatcherDesc` por valor, e o `TheForgeWindowManager.h` chega ao `main` de
todo jogo. Com a desc dentro do `ForgeMesh.h`, **todo consumidor 2D receberia o
`Common_3/Graphics/FSL/defaults.h`** e suas 20 macros (`CAT`, `VARNAME`,
`DESCRIPTOR_TABLE`...). Mesma familia de problema da task 09: uma mudanca que
parece aditiva no codigo e obriga o consumidor a mudar.

Os irmaos 2D nunca tiveram esta questao porque a implementacao deles mora num
`.cpp`. **Modulo header-only paga esse pedagio**, e a regra que fica e: separar
a desc da implementacao sempre que a desc for para um cabecalho publico.

**4. O sombreado nao usa material.** A cor e `normal * 0.5 + 0.5`, e nao a cor do
Blender — material e a task 12. Por acaso o resultado tem a mesma propriedade
que o banco de provas do lab procurava: um cubo sai com seis cores distintas,
complementares aos pares.

## O que esta task APAGA

Toda a matematica de `ForgeMalha.h` do Vigil (projecao, descarte de face,
iluminacao a mao), e **o teto de lote do `forgeline` deixa de existir para
malha**, porque a geometria para de passar pelo vertex buffer por quadro.
