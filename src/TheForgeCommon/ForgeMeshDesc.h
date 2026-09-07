#pragma once

// A DESC da ponte 3D, separada do `ForgeMesh.h` de proposito.
//
// O `TheForgeWindowDesc` precisa da `MeshBatcherDesc` por valor, e o
// `TheForgeWindowManager.h` e incluido pelo `main` de todo jogo do ecossistema.
// Se a desc morasse no `ForgeMesh.h`, todo consumidor receberia junto o
// `Common_3/Graphics/FSL/defaults.h` -- que despeja 20 macros na unidade de
// traducao, entre elas `CAT`, `VARNAME` e `DESCRIPTOR_TABLE`.
//
// **Isso nao seria uma adicao pura.** Macro generica em cabecalho de uso geral e
// a mesma familia de problema que fechou a task 09: uma mudanca que parece
// aditiva no codigo e obriga o consumidor a mudar. Os irmaos 2D nao tem esta
// questao porque a implementacao deles mora num `.cpp`, e la o `defaults.h`
// fica confinado.
//
// Quem chama `loadMesh`/`draw` inclui o `ForgeMesh.h` e aceita o `defaults.h`
// junto -- escolha de quem desenha 3D, e nao imposicao a quem desenha 2D.

#include <cstdint>

namespace forgemesh {



/// Identificador de uma malha carregada. `kSemMalha` e o "nao carregou".
using Handle = uint32_t;
inline constexpr Handle kSemMalha = 0xffffffffu;

// Configuracao da ponte — fornecida pelo jogo na montagem do casco.
struct MeshBatcherDesc
{
    /// false DESLIGA: nenhum shader, pipeline ou buffer e criado, e todas as
    /// funcoes viram no-op. Mesmo espirito do `atlasPath` nulo do forgesprite e
    /// do `enabled` do forgeline.
    ///
    /// **Ligar isto exige `depth.enabled` tambem.** Malha 3D sem profundidade e
    /// ordem do pintor, que e exatamente o que a task 10 substituiu -- o `load`
    /// recusa e loga, em vez de desenhar algo plausivel e errado.
    bool enabled = false;

    /// Quantos objetos podem ser desenhados por quadro.
    ///
    /// **64 e nao 512** (o numero que a spec da task supunha): ha um constant
    /// buffer por (quadro em voo x instancia), entao 512 seriam 1024 alocacoes
    /// para uma porta que nenhum consumidor usa com mais de um punhado de
    /// objetos. Estourou, dropa o desenho e loga uma vez -- e o `Stats::dropped`
    /// diz quanto sumiu, porque **corpo que some sem erro visivel e o pior jeito
    /// de errar**.
    uint32_t maxInstances = 64;

    /// Quantas malhas distintas podem estar carregadas ao mesmo tempo.
    uint32_t maxMeshes = 16;

    /// Quantos materiais distintos. O `+1` do padrao (mapas brancos/planos)
    /// e reservado pelo modulo, e nao conta aqui.
    uint32_t maxMaterials = 16;

    /// Frames in flight do casco. Preenchido pelo casco, nao pelo jogo.
    uint32_t frameCount = 2;
};

/// A luz DIRECIONAL da cena (task 13) -- o Sun do Blender.
///
/// **`direction` e para onde a luz VIAJA**, que e a convencao do Blender (o Sun
/// emite ao longo do `-Z` local dele) e a que os `.txt` de captura do `diorama`
/// gravam. O `forgemesh` nega e normaliza internamente, uma vez por quadro;
/// quem chama nao precisa saber que o shader quer o sentido oposto.
///
/// Trocar o sentido nao da erro: **ilumina exatamente as faces erradas**, e a
/// imagem continua parecendo uma imagem iluminada.
struct DirectionalLight
{
    float direction[3] = { 0.0f, -1.0f, 0.0f }; // de cima para baixo
    float color[3] = { 1.0f, 1.0f, 1.0f };
    float intensity = 1.0f;
};

/// A luz AMBIENTE -- o World do Blender.
///
/// E o que impede o lado escuro de ser preto puro. Uma direcional sozinha deixa
/// metade de qualquer corpo em preto absoluto, e preto absoluto nao acontece em
/// lugar nenhum fora de um vacuo.
struct Ambient
{
    float color[3] = { 0.1f, 0.1f, 0.12f };
    float intensity = 1.0f;
};

/// Identificador de um material carregado. `kSemMaterial` usa os mapas padrao
/// (cor branca, normal plana) -- que NAO e um caso de erro, e sim o material
/// mais simples que existe.
using MaterialHandle = uint32_t;
inline constexpr MaterialHandle kSemMaterial = 0xffffffffu;

/// Um material: dois mapas e um fator (task 12).
///
/// **Caminho de arquivo COM extensao**, relativo ao `RD_TEXTURES` do
/// PathStatement do jogo -- ex.: `"textura/albedo.tex"`. O
/// `AssetPipelineCmd -pt` produz `.tex` (contentor DDS), e nao `.dds`; e o
/// `TextureLoadDesc` do The-Forge usa o nome como esta (o precedente e o
/// `28_Skinning`, com `"Stormtrooper_D.tex"`).
///
/// **Nulo nao e erro.** Sem `baseColorPath`, o mapa e branco 1x1 e sobra o
/// `baseColorFactor`; sem `normalPath`, o mapa e plano 1x1 e sobra a normal do
/// vertice. Nenhum `if` no shader, nenhuma variante de pipeline -- e assim que
/// *"material sem mapa e um valor constante, nao um erro"* se realiza.
struct MaterialDesc
{
    const char* baseColorPath = nullptr;
    const char* normalPath = nullptr;

    /// Multiplica o mapa de cor. Sozinho, e o material sem textura.
    float baseColorFactor[3] = { 1.0f, 1.0f, 1.0f };
};

/// O modo de CONFERENCIA, ate a task 19 transforma-lo em variante de pipeline.
///
/// `usarMaterial = false` faz a cor voltar a ser a normal do VERTICE
/// (`n * 0.5 + 0.5`), que e o instrumento com que os degraus 04, 05 e 06 do
/// `diorama` conferiram malha, orientacao e iluminacao. Ele existe para separar
/// as perguntas: *"a malha esta certa?"* e *"o material esta certo?"* nao podem
/// depender uma da outra.
struct ModoDeConferencia
{
    bool usarMaterial = true;
};

// Contadores do quadro ANTERIOR (fechados no `begin()` seguinte).
struct Stats
{
    uint32_t draws = 0;
    uint32_t dropped = 0; // objetos perdidos por lote cheio
    uint32_t instanceCapacity = 0;
};

} // namespace forgemesh
