#pragma once

// ForgeMesh — **o quarto irmao das pontes de desenho** (task 11), e o primeiro
// que desenha em TRES dimensoes.
//
//   forgeui      texto
//   forgesprite  quads texturizados
//   forgeline    linhas e triangulos 2D
//   forgemesh    malha 3D vinda de um `.bin` do AssetPipelineCmd   <- este
//
// ## A diferenca que muda tudo
//
// As tres pontes 2D recebem posicao **ja em NDC**: quem calcula onde o vertice
// cai na tela e a CPU, todo quadro, vertice a vertice. O `line.vert.fsl` diz
// isso por escrito -- *"sem SRT: este shader nao le recurso nenhum"*.
//
// Esta ponte recebe **uma matriz**. A geometria sobe uma vez para a GPU, fica
// la, e o que viaja por quadro sao 64 bytes por objeto.
//
// **O custo de nao ter isto foi medido.** O Vigil pedia
// `GEOMETRY_LOAD_FLAG_SHADOWED` para trazer a malha de volta para a CPU --
// ignorando os vertex buffers que o `loadGeometry` ja tinha enchido --,
// reprojetava tudo a cada quadro e empurrava o resultado pelo vertex buffer
// dinamico do `forgeline`. Dai vinham a projecao a mao, o descarte de face a
// mao, a iluminacao a mao e o teto de lote (`Stats::dropped`), que so existia
// por causa disso. Sao 684 linhas em `ForgeMalha.h` que esta ponte substitui.
//
// ## A LUZ e da CENA, e nao do objeto (task 13)
//
// `setLight` e `setAmbient` valem para o quadro inteiro, como o `setCamera`.
// Dois corpos na mesma cena reagem a mesma luz **sem cada um carregar o seu
// rig** -- que e exatamente o que o Vigil nao conseguia fazer, porque a
// iluminacao dele era um conjunto de numeros ajustados a mao dentro do laco de
// desenho de um corpo so.
//
// Uma direcional mais uma ambiente, e nao um sistema de luzes: quantas luzes
// uma cena tem e decisao de JOGO, e o casco nao opina sobre isso.
//
// ## Duas regras que ele NAO compartilha com os irmaos 2D
//
// **1. Nao ha lote.** Um `draw` = um `cmdDrawIndexed`. Batching existe para
// amortizar a chamada quando cada primitiva e minuscula; uma malha nao e
// minuscula, e cada uma tem matriz propria. As duas coisas resolvem problemas
// opostos.
//
// **2. Ele desenha ANTES do overlay, e nunca depois.** As pontes 2D chamam
// `forgeframe::enterOverlay()` no primeiro desenho do quadro, e a partir dali o
// quadro nao tem mais profundidade (ver `ForgeFrame.h`). Esta ponte **nao**
// chama -- ela e quem usa o passe com depth. Consequencia pratica para a cena:
// desenhe a malha primeiro, o HUD depois. Que e a ordem que toda cena deste
// ecossistema ja usa.
//
// ## Por que isto e SO um cabecalho
//
// Mesma razao do `ForgeFrame.h`, e ela agora e regra do repo: **o casco nao e
// biblioteca com alvo de build.** Cada jogo enumera os `.cpp` do casco no
// proprio `.vcxproj`, entao um `.cpp` novo aqui nao e aditivo -- e uma edicao
// obrigatoria em 8+ projetos que ja existem. Foi assim que a 0.16.0 quebrou o
// link de todo mundo. Estado em variavel `inline` (C++17): uma instancia por
// programa, zero unidade de traducao nova.

#include <cstdint>

// The-Forge (o include path do projeto aponta para a raiz do The-Forge).
#include "Common_3/Graphics/Interfaces/IGraphics.h"
#include "Common_3/Resources/ResourceLoader/Interfaces/IResourceLoader.h"
#include "Common_3/Utilities/Interfaces/ILog.h"
#include "Common_3/Utilities/Math/MathTypes.h"

#include "ForgeFrame.h"
#include "ForgeMeshDesc.h"

// O defaults.h do FSL TEM de vir antes do .srt.h: e ele que define as macros
// (BEGIN_SRT_NO_AB, DECL_CBUFFER, SRT_SET_DESC...) que o lado C++ expande.
#include "Common_3/Graphics/FSL/defaults.h"

#include "Shaders/FSL/mesh.srt.h"

namespace forgemesh {

/// **O formato dos atributos EMPACOTADOS, e ele NAO e float3.**
///
/// O passo do vertice que o layout desta ponte PEDE: 12 bytes de posicao mais
/// tres atributos empacotados de 4. E o numero contra o qual o `.bin` e
/// conferido na carga -- ver `loadMesh`.
inline constexpr uint32_t kPassoEsperado = 3u * sizeof(float) + 3u * sizeof(uint32_t);

/// O `AssetPipelineCmd` grava a normal EMPACOTADA em 4 bytes: octaedral, dois
/// `unorm` de 16 bits (`encodeDir` + `packUnorm2x16`). Medido nos quatro `.bin`
/// do banco de provas do `diorama`, lendo o `ShadowData` do arquivo:
///
///     POSITION stride=12   NORMAL/TANGENT/TEXCOORD0 stride=4
///
/// **Pedir `R32G32B32_SFLOAT` aqui nao da erro nenhum num build Release** -- e
/// foi assim que a primeira versao desta ponte desenhou dois cubos PRETOS:
///
///   - `ResourceLoader.cpp:1767` tem um `ASSERT(dstFormatSize == srcFormatSize)`
///     que so existe em Debug;
///   - `ResourceLoader.cpp:1793` pula a copia de um atributo ausente com um
///     `continue` silencioso;
///   - o `memcpy` de `1821` copia `mVertexStrides[i]` bytes -- os 4 do arquivo
///     -- para um campo de 12, deixando 8 bytes indefinidos. A normal chega
///     invalida, `normalize()` devolve NaN e o alvo UNORM grampeia em zero.
///
/// **Este valor e o shader sao UMA decisao em dois arquivos.** O
/// `mesh.vert.fsl` declara `DATA(uint, Normal, NORMAL)` e desempacota com
/// `decodeDir(unpackUnorm2x16(...))`. Mudar um sem o outro devolve o preto.
/// Precedente do proprio The-Forge: `28_Skinning` faz exatamente isto.
///
/// Vale para NORMAL, TANGENT e TEXCOORD0 -- os tres cabem em 4 bytes, e os tres
/// foram medidos assim nos `.bin` do banco de provas. O que muda e como o shader
/// DESEMPACOTA: `decodeDir(unpackUnorm2x16(...))` para as duas direcoes,
/// `unpack2Floats` (half2) para a UV.
inline constexpr TinyImageFormat kFormatoEmpacotado = TinyImageFormat_R32_UINT;

/// A variante de um pipeline de malha.
///
/// **Hoje tem um valor so, e isso e deliberado.** A task 12 (material e textura)
/// e a 14 (skinning) e que definem os eixos de verdade — e a propria task 12 ja
/// sugere que textura pode NAO ser variante de shader, e sim um uniforme
/// (*"material sem mapa e um valor constante, nao um erro"*). Desenhar o enum
/// antes do segundo caso seria projetar contra um requisito imaginado.
///
/// O que a task 15a entrega e o LUGAR onde a variante vai caber, mais o fim da
/// duplicacao que existia. A 15b preenche.
enum Variante : uint32_t
{
    kSimples = 0,
};

/// **A UNICA definicao de layout de vertice deste modulo** (task 15a).
///
/// Ela existe porque havia DUAS, e as duas tinham de concordar: uma montava o
/// pipeline (`addPipeline`), a outra pedia o arranjo ao `ResourceLoader`
/// (`GeometryLoadDesc`). Divergirem nao daria erro — o `ResourceLoader` copia o
/// tamanho da ORIGEM e o unico guarda-corpo, o
/// `ASSERT(dstFormatSize == srcFormatSize)` de `ResourceLoader.cpp:1767`, so
/// existe em Debug.
///
/// **E a assinatura exata do defeito que produziu os cubos PRETOS** no degrau 04
/// do `diorama`: layout pedido diferente do que o arquivo tem, normal invalida,
/// `normalize()` devolvendo NaN, alvo UNORM grampeando em zero. Duas copias de
/// uma decisao que precisa ser uma so e esse defeito esperando acontecer.
///
/// O parametro `variante` existe hoje com um valor so. **Ele e o lugar onde a
/// task 15b vai caber** — os eixos de verdade (textura? esqueleto? uniforme em
/// vez de variante?) sao definidos pela task 12, e desenha-los antes dela seria
/// projetar contra um requisito imaginado.
[[nodiscard]] inline VertexLayout layoutDe(const uint32_t variante)
{
    (void)variante; // 15b: e por aqui que a variante escolhe atributos

    VertexLayout layout = {};
    layout.mBindingCount = 1;
    // O PASSO, explicito. O irmao `forgesprite` sempre o declarou; aqui ele
    // ficava zero, e zero significa "loader, deduza". Deduzir da o mesmo numero
    // hoje -- e e mais uma decisao morando implicita num modulo cujo defeito
    // assinatura e justamente layout que nao bate com o arquivo.
    layout.mBindings[0].mStride = 3 * sizeof(float) + 3 * sizeof(uint32_t);
    layout.mAttribCount = 4;

    layout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
    layout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
    layout.mAttribs[0].mBinding = 0;
    layout.mAttribs[0].mLocation = 0;
    layout.mAttribs[0].mOffset = 0;

    layout.mAttribs[1].mSemantic = SEMANTIC_NORMAL;
    layout.mAttribs[1].mFormat = kFormatoEmpacotado;
    layout.mAttribs[1].mBinding = 0;
    layout.mAttribs[1].mLocation = 1;
    layout.mAttribs[1].mOffset = 3 * sizeof(float);

    // A TANGENTE usa a MESMA codificacao octaedral da normal -- o
    // `AssetPipeline` trata as duas no mesmo ramo (`AssetPipeline.cpp:1894`).
    //
    // **Ela nao existe em todo `.bin`:** o exportador do Blender so a produz
    // para malha com material de normal map. Pedir um atributo ausente nao da
    // erro (o `continue` de `ResourceLoader.cpp:1793`), e o campo fica zerado.
    // Isso e inofensivo aqui porque a tangente so e LIDA quando ha mapa de
    // normal, e sem mapa o padrao e plano -- a tangente some da conta.
    layout.mAttribs[2].mSemantic = SEMANTIC_TANGENT;
    layout.mAttribs[2].mFormat = kFormatoEmpacotado;
    layout.mAttribs[2].mBinding = 0;
    layout.mAttribs[2].mLocation = 2;
    layout.mAttribs[2].mOffset = 3 * sizeof(float) + sizeof(uint32_t);

    // A UV sao dois `half` (4 bytes), e **nao** um par unorm: coordenada de
    // textura passa de 1 quando a textura repete. O shader desempacota com
    // `unpack2Floats`, e nao com o `unpackUnorm2x16` da normal.
    layout.mAttribs[3].mSemantic = SEMANTIC_TEXCOORD0;
    layout.mAttribs[3].mFormat = kFormatoEmpacotado;
    layout.mAttribs[3].mBinding = 0;
    layout.mAttribs[3].mLocation = 3;
    layout.mAttribs[3].mOffset = 3 * sizeof(float) + 2 * sizeof(uint32_t);

    return layout;
}

namespace detalhe {

inline Renderer*      gRenderer = NULL;
inline MeshBatcherDesc gDesc = {};

/// A ponte DESENHA? Vira false quando o `load()` recusa (sem depth target).
inline bool gEnabled = false;

/// A ponte ALOCOU? Separado do `gEnabled` de proposito.
///
/// O `init()` aloca quando habilitada; o `load()` pode DESABILITAR depois (mesh
/// sem depth). O `exit()` saia cedo em `!gEnabled` -- e os tres blocos do
/// `init()` nunca eram liberados. Vazamento no caminho de FALHA, que e o
/// caminho que menos se olha.
inline bool gAlocado = false;

inline Shader*        gShader = NULL;
inline Pipeline*      gPipeline = NULL;
inline DescriptorSet* gConjunto = NULL;
inline Buffer**       gUniformes = NULL; // (frameCount x maxInstances)

inline Geometry** gMalhas = NULL;
inline uint32_t   gMalhaCount = 0;

// Os materiais. O indice 0 do descriptor set e sempre o PADRAO (mapa de cor
// branco 1x1, mapa de normal plano 1x1); os carregados vem depois. Por isso o
// handle publico `h` mora no slot `h + 1`.
inline DescriptorSet* gConjuntoDeMaterial = NULL;
inline Texture**      gTexturas = NULL; // 2 por material carregado
inline Texture*       gCorPadrao = NULL;
inline Texture*       gNormalPadrao = NULL;
inline float4*        gFatores = NULL; // baseColorFactor por material
inline uint32_t       gMaterialCount = 0;
inline MaterialHandle gMaterialDoQuadro = kSemMaterial;

// quadro em andamento
inline Cmd*     gCmd = NULL;
inline uint32_t gFrameIndex = 0;
inline uint32_t gUsadas = 0; // instancias ja gastas neste quadro
inline mat4     gViewProj = mat4::identity();
inline bool     gOverflowLogged = false;

// Os dois irmaos do `gOverflowLogged`, e pela mesma razao: o log e uma vez por
// EXECUCAO, porque `draw` acontece 60 vezes por segundo. Quem conta e o
// `Stats::dropped`, que e por quadro.
inline bool gHandleInvalidoLogado = false;
inline bool gMaterialInvalidoLogado = false;

// A luz do quadro. Vale ate a proxima chamada de `setLight`/`setAmbient`, como
// a camera -- e, como ela, e estado de CENA e nao de objeto: dois corpos no
// mesmo quadro reagem a mesma luz sem cada um carregar o seu rig (criterio 3 da
// task 13).
inline DirectionalLight  gLuz = {};
inline Ambient           gAmbiente = {};
inline ModoDeConferencia gModo = {};

inline Stats gCurrent = {};
inline Stats gLastFrame = {};

} // namespace detalhe

namespace detalhe {

/// Uma textura 1x1 de um pixel so, para ser o material PADRAO.
///
/// **E ela que faz "sem mapa" deixar de ser um caso especial.** Com um branco
/// 1x1 no lugar do mapa de cor e um plano 1x1 no de normal, o shader multiplica
/// e transforma como sempre, e o resultado e o material constante -- sem `if`,
/// sem variante de pipeline, sem um caminho de codigo que so alguns modelos
/// percorrem (e que so alguns modelos testam).
inline Texture* criarTextura1x1(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a, const char* nome)
{
    TextureDesc desc = {};
    desc.mWidth = 1;
    desc.mHeight = 1;
    desc.mDepth = 1;
    desc.mArraySize = 1;
    desc.mMipLevels = 1;
    // **`SAMPLE_COUNT_1`, e nao o zero do valor-inicializado.** O enum comeca em
    // 1 (`IGraphics.h:321`), entao um `TextureDesc` zerado pede zero amostras --
    // e o D3D12 recusa com `E_INVALIDARG` (0x80070057) na criacao do recurso.
    // Faltar este campo derrubou o executavel do degrau 07 antes da janela abrir.
    desc.mSampleCount = SAMPLE_COUNT_1;
    desc.mFormat = TinyImageFormat_R8G8B8A8_UNORM;
    desc.mStartState = RESOURCE_STATE_SHADER_RESOURCE;
    desc.mDescriptors = DESCRIPTOR_TYPE_TEXTURE;
    desc.pName = nome;

    Texture*        textura = NULL;
    TextureLoadDesc carga = {};
    carga.ppTexture = &textura;
    carga.pDesc = &desc;
    addResource(&carga, NULL);
    waitForAllResourceLoads();

    if (textura == NULL)
    {
        return NULL;
    }

    const uint8_t      pixel[4] = { r, g, b, a };
    TextureUpdateDesc atualizacao = { textura, 0, 1, 0, 1, RESOURCE_STATE_SHADER_RESOURCE };
    beginUpdateResource(&atualizacao);
    TextureSubresourceUpdate linha = atualizacao.getSubresourceUpdateDesc(0, 0);
    memcpy(linha.pMappedData, pixel, sizeof(pixel));
    endUpdateResource(&atualizacao);
    waitForAllResourceLoads();
    return textura;
}

/// Liga os dois mapas de um slot do descriptor set de material.
inline void ligarMaterial(const uint32_t slot, Texture* cor, Texture* normal)
{
    DescriptorData mapas[2] = {};
    mapas[0].mIndex = SRT_RES_IDX(MeshSrtData, PerBatch, gMapaDeCor);
    mapas[0].ppTextures = &cor;
    mapas[1].mIndex = SRT_RES_IDX(MeshSrtData, PerBatch, gMapaDeNormal);
    mapas[1].ppTextures = &normal;
    updateDescriptorSet(gRenderer, slot, gConjuntoDeMaterial, 2, mapas);
}

} // namespace detalhe

// --- ciclo de vida (chamado pelo casco da plataforma) ---

inline void init(Renderer* renderer, const MeshBatcherDesc& desc)
{
    detalhe::gRenderer = renderer;
    detalhe::gDesc = desc;
    detalhe::gEnabled = desc.enabled && renderer != NULL;
    if (!detalhe::gEnabled)
    {
        return;
    }
    detalhe::gAlocado = true;

    detalhe::gMalhas = (Geometry**)tf_calloc(desc.maxMeshes, sizeof(Geometry*));
    detalhe::gMalhaCount = 0;

    detalhe::gTexturas = (Texture**)tf_calloc(desc.maxMaterials * 2u, sizeof(Texture*));
    detalhe::gFatores = (float4*)tf_calloc(desc.maxMaterials, sizeof(float4));
    detalhe::gMaterialCount = 0;
}

inline void exit()
{
    // **`gAlocado`, e nao `gEnabled`.** Ver a declaracao dos dois: uma ponte que
    // o `load()` desligou continua tendo o que liberar.
    if (!detalhe::gAlocado)
    {
        return;
    }
    // As malhas sao liberadas aqui: quem chamou `loadMesh` durante a carga da
    // cena nao tem como saber a ordem de teardown do casco, e vazar Geometry no
    // fim do processo esconderia vazamento de verdade em ferramenta de leak.
    for (uint32_t i = 0; i < detalhe::gMalhaCount; ++i)
    {
        if (detalhe::gMalhas[i])
        {
            removeResource(detalhe::gMalhas[i]);
        }
    }
    tf_free(detalhe::gMalhas);
    detalhe::gMalhas = NULL;
    detalhe::gMalhaCount = 0;

    for (uint32_t i = 0; i < detalhe::gMaterialCount * 2u; ++i)
    {
        if (detalhe::gTexturas[i])
        {
            removeResource(detalhe::gTexturas[i]);
        }
    }
    tf_free(detalhe::gTexturas);
    detalhe::gTexturas = NULL;
    tf_free(detalhe::gFatores);
    detalhe::gFatores = NULL;
    detalhe::gMaterialCount = 0;
    detalhe::gRenderer = NULL;
    detalhe::gEnabled = false;
    detalhe::gAlocado = false;
}

/// Cria shader, descriptor set, constant buffers e pipeline.
///
/// `depthFormat` vem do casco (`forgeframe::target().depthFormat`). Se vier
/// `UNDEFINED`, a ponte se desliga e loga: um pipeline com `pDepthState` e sem
/// depth target nao desenha, e o sintoma seria a tela vazia.
inline void load(const ReloadDesc* reloadDesc, const TinyImageFormat colorFormat, const SampleCount sampleCount,
                 const uint32_t sampleQuality, const TinyImageFormat depthFormat)
{
    if (!detalhe::gEnabled)
    {
        return;
    }

    if (depthFormat == TinyImageFormat_UNDEFINED)
    {
        LOGF(eERROR, "[forgemesh] mesh.enabled sem depth.enabled - a ponte 3D fica desligada");
        detalhe::gEnabled = false;
        return;
    }

    const uint32_t trechos = detalhe::gDesc.frameCount * detalhe::gDesc.maxInstances;

    if (reloadDesc->mType & RELOAD_TYPE_SHADER)
    {
        ShaderLoadDesc shaderDesc = {};
        shaderDesc.mVert.pFileName = "mesh.vert";
        shaderDesc.mFrag.pFileName = "mesh.frag";
        addShader(detalhe::gRenderer, &shaderDesc, &detalhe::gShader);

        DescriptorSetDesc conjuntoDesc = SRT_SET_DESC(MeshSrtData, PerFrame, trechos, 0);
        addDescriptorSet(detalhe::gRenderer, &conjuntoDesc, &detalhe::gConjunto);

        detalhe::gUniformes = (Buffer**)tf_calloc(trechos, sizeof(Buffer*));
        for (uint32_t i = 0; i < trechos; ++i)
        {
            BufferLoadDesc cbDesc = {};
            cbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            cbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
            cbDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
            cbDesc.mDesc.mSize = sizeof(UniformeDaMalha);
            cbDesc.mDesc.pName = "ForgeMesh CB";
            cbDesc.ppBuffer = &detalhe::gUniformes[i];
            addResource(&cbDesc, NULL);
        }
        waitForAllResourceLoads();

        for (uint32_t i = 0; i < trechos; ++i)
        {
            DescriptorData parametro = {};
            parametro.mIndex = SRT_RES_IDX(MeshSrtData, PerFrame, gMalha);
            parametro.ppBuffers = &detalhe::gUniformes[i];
            updateDescriptorSet(detalhe::gRenderer, i, detalhe::gConjunto, 1, &parametro);
        }

        // O conjunto de MATERIAL (task 12). `+1` porque o slot 0 e o padrao.
        DescriptorSetDesc materialDesc =
            SRT_SET_DESC(MeshSrtData, PerBatch, detalhe::gDesc.maxMaterials + 1u, 0);
        addDescriptorSet(detalhe::gRenderer, &materialDesc, &detalhe::gConjuntoDeMaterial);

        // Branco 1x1 e normal plana 1x1. A normal plana e `(0,0,1)` codificada
        // no swizzle `{x,x,x,y}` da `-pt --normalmap`: X e Y no meio da faixa
        // (128) e Z reconstruido como 1.
        detalhe::gCorPadrao = detalhe::criarTextura1x1(255, 255, 255, 255, "ForgeMesh cor padrao");
        detalhe::gNormalPadrao = detalhe::criarTextura1x1(128, 128, 128, 128, "ForgeMesh normal padrao");
        detalhe::ligarMaterial(0, detalhe::gCorPadrao, detalhe::gNormalPadrao);

        // Os materiais que ja tinham sido carregados voltam a ser ligados: o
        // `load` roda de novo em reload de shader, e o descriptor set e novo.
        //
        // **O fallback para os padroes tem de estar AQUI tambem.** Um material
        // sem mapa guarda `NULL` em `gTexturas` (e o caso legitimo do "so
        // fator"), e ligar `NULL` no descriptor set nao e o mesmo que ligar o
        // padrao. A primeira versao deste laco nao fazia o fallback: era codigo
        // errado que nunca tinha rodado, porque hoje o `load` acontece antes de
        // qualquer `loadMaterial` e o laco fica vazio.
        for (uint32_t i = 0; i < detalhe::gMaterialCount; ++i)
        {
            Texture* cor = detalhe::gTexturas[i * 2u];
            Texture* normal = detalhe::gTexturas[i * 2u + 1u];
            detalhe::ligarMaterial(i + 1u, cor ? cor : detalhe::gCorPadrao, normal ? normal : detalhe::gNormalPadrao);
        }
    }

    if (reloadDesc->mType & (RELOAD_TYPE_SHADER | RELOAD_TYPE_RENDERTARGET))
    {
        // **O descarte de face e do RASTERIZADOR** (criterio 3 da task 11). No
        // Vigil ele era feito na CPU e ficou com o sinal invertido por um degrau
        // inteiro -- o jogo desenhava as COSTAS dos corpos.
        //
        // `FRONT_FACE_CCW`, que e o default e e o do glTF: a face da frente e
        // anti-horaria, e as matrizes que esta ponte espera **preservam a mao**.
        //
        // > **Isto era `FRONT_FACE_CW` na 0.18.0, e a mudanca nao e cosmetica.**
        // > Naquela versao o consumidor montava a camera com o `lookAtLH`/
        // > `perspectiveLH` do The-Forge -- matrizes CANHOTAS sobre malha
        // > DESTRA. A troca de mao inverte o enrolamento na tela, e por isso o
        // > `CW` funcionava... desenhando a cena ESPELHADA. Espelhado continua
        // > parecendo certo enquanto nao ha com o que comparar.
        // >
        // > A `cengine::camera3d` (task 29) emite matrizes destras, e com elas o
        // > enrolamento do glTF chega intacto. **Quem alimentar esta ponte com
        // > matrizes canhotas vera o interior dos corpos** -- e esse e o sintoma
        // > certo, porque diz que a camera esta espelhando a cena.
        RasterizerStateDesc rasterizador = {};
        rasterizador.mCullMode = CULL_MODE_BACK;
        rasterizador.mFrontFace = FRONT_FACE_CCW;

        DepthStateDesc profundidade = {};
        profundidade.mDepthTest = true;
        profundidade.mDepthWrite = true;
        // LEQUAL porque o casco limpa a profundidade em 1.0 (Z padrao). Ver
        // TheForgeDepthDesc::clearDepth -- e uma decisao em tres lugares.
        profundidade.mDepthFunc = CMP_LEQUAL;

        // Sem `const`: o `GraphicsPipelineDesc::pVertexLayout` e um ponteiro
        // NAO-const, ao contrario do `GeometryLoadDesc::pVertexLayout` da carga.
        VertexLayout layout = layoutDe(kSimples);

        PipelineDesc desc = {};
        desc.mType = PIPELINE_TYPE_GRAPHICS;
        PIPELINE_LAYOUT_DESC(desc, NULL, SRT_LAYOUT_DESC(MeshSrtData, PerFrame),
                             SRT_LAYOUT_DESC(MeshSrtData, PerBatch), NULL);
        GraphicsPipelineDesc& grafico = desc.mGraphicsDesc;
        grafico.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        grafico.mRenderTargetCount = 1;
        grafico.pColorFormats = (TinyImageFormat*)&colorFormat;
        grafico.mSampleCount = sampleCount;
        grafico.mSampleQuality = sampleQuality;
        grafico.mDepthStencilFormat = depthFormat;
        grafico.pShaderProgram = detalhe::gShader;
        grafico.pVertexLayout = &layout;
        grafico.pRasterizerState = &rasterizador;
        grafico.pDepthState = &profundidade;
        addPipeline(detalhe::gRenderer, &desc, &detalhe::gPipeline);
    }
}

inline void unload(const ReloadDesc* reloadDesc)
{
    if (!detalhe::gEnabled)
    {
        return;
    }

    if (reloadDesc->mType & (RELOAD_TYPE_SHADER | RELOAD_TYPE_RENDERTARGET))
    {
        if (detalhe::gPipeline)
        {
            removePipeline(detalhe::gRenderer, detalhe::gPipeline);
            detalhe::gPipeline = NULL;
        }
    }

    if (reloadDesc->mType & RELOAD_TYPE_SHADER)
    {
        const uint32_t trechos = detalhe::gDesc.frameCount * detalhe::gDesc.maxInstances;
        if (detalhe::gUniformes)
        {
            for (uint32_t i = 0; i < trechos; ++i)
            {
                removeResource(detalhe::gUniformes[i]);
            }
            tf_free(detalhe::gUniformes);
            detalhe::gUniformes = NULL;
        }
        if (detalhe::gConjunto)
        {
            removeDescriptorSet(detalhe::gRenderer, detalhe::gConjunto);
            detalhe::gConjunto = NULL;
        }
        if (detalhe::gConjuntoDeMaterial)
        {
            removeDescriptorSet(detalhe::gRenderer, detalhe::gConjuntoDeMaterial);
            detalhe::gConjuntoDeMaterial = NULL;
        }
        // As texturas PADRAO morrem com o conjunto; as dos materiais carregados
        // sobrevivem (sao dado, e nao recurso de pipeline) e sao religadas no
        // proximo `load`.
        if (detalhe::gCorPadrao)
        {
            removeResource(detalhe::gCorPadrao);
            detalhe::gCorPadrao = NULL;
        }
        if (detalhe::gNormalPadrao)
        {
            removeResource(detalhe::gNormalPadrao);
            detalhe::gNormalPadrao = NULL;
        }
        if (detalhe::gShader)
        {
            removeShader(detalhe::gRenderer, detalhe::gShader);
            detalhe::gShader = NULL;
        }
    }
}

inline void begin(Cmd* cmd, const uint32_t frameIndex)
{
    if (!detalhe::gEnabled)
    {
        return;
    }
    detalhe::gCmd = cmd;
    detalhe::gFrameIndex = frameIndex;
    detalhe::gUsadas = 0;

    detalhe::gLastFrame = detalhe::gCurrent;
    detalhe::gCurrent = {};
}

inline void end() { detalhe::gCmd = NULL; }

// --- carga de malha (chamada pela CENA, fora do quadro) ---

/// Carrega um `.bin` do `AssetPipelineCmd`, relativo ao `RD_MESHES` do
/// PathStatement do jogo.
///
/// **Sem `GEOMETRY_LOAD_FLAG_SHADOWED`** -- e essa ausencia e o ponto da task.
/// A flag existe para trazer uma copia de volta para a CPU; pedi-la significa
/// que alguem vai mexer nos vertices por conta propria, que e exatamente o que
/// esta ponte torna desnecessario.
[[nodiscard]] inline Handle loadMesh(const char* binPath)
{
    if (!detalhe::gEnabled)
    {
        return kSemMalha;
    }
    if (detalhe::gMalhaCount >= detalhe::gDesc.maxMeshes)
    {
        LOGF(eERROR, "[forgemesh] maxMeshes (%u) estourado carregando '%s'", detalhe::gDesc.maxMeshes, binPath);
        return kSemMalha;
    }

    // **O MESMO layout que montou o pipeline** (task 15a). Ele diz ao loader
    // como ARRUMAR o dado na GPU, e o loader converte o que estiver no arquivo;
    // pedir um arranjo diferente do que o pipeline espera nao da erro, da lixo.
    const VertexLayout layout = layoutDe(kSimples);

    Geometry*        geometria = NULL;
    GeometryLoadDesc desc = {};
    desc.pFileName = binPath;
    desc.pVertexLayout = &layout;
    desc.ppGeometry = &geometria;
    desc.mFlags = GEOMETRY_LOAD_FLAG_NONE; // <- o ponto da task
    addResource(&desc, NULL);
    waitForAllResourceLoads();

    if (geometria == NULL)
    {
        LOGF(eERROR, "[forgemesh] '%s' nao carregou", binPath);
        return kSemMalha;
    }

    const Handle h = detalhe::gMalhaCount++;
    detalhe::gMalhas[h] = geometria;

    // **O passo, CONFERIDO e nao so registrado.** Ele sai do LAYOUT pedido, e
    // nao do arquivo, entao ele nao detecta o desencontro descrito em
    // `kFormatoEmpacotado` -- mas ele detecta o layout ter mudado sem este
    // cabecalho ser relido, que e a outra metade do mesmo defeito.
    //
    // A pergunta que pega esta classe e sempre *"o que este numero DEVERIA
    // ser?"*, e ela so protege se alguem a fizer.
    if (geometria->mVertexStrides[0] != kPassoEsperado)
    {
        LOGF(eERROR, "[forgemesh] '%s': passo %u B, esperado %u B - o layout e o shader divergiram", binPath,
             geometria->mVertexStrides[0], kPassoEsperado);
    }

    LOGF(eINFO, "[forgemesh] '%s': %u indices, %u vertices, %u draw args, passo %u B", binPath,
         geometria->mIndexCount, geometria->mVertexCount, geometria->mDrawArgCount, geometria->mVertexStrides[0]);
    return h;
}

/// Carrega um material: dois mapas, opcionais.
///
/// **Caminho COM extensao**, relativo ao `RD_TEXTURES` (ex.: `"textura/albedo.tex"`).
/// A `-pt` do `AssetPipelineCmd` produz `.tex`, e nao `.dds`; o
/// `TextureLoadDesc` usa o nome como esta.
///
/// Um caminho nulo NAO e erro: fica o mapa padrao (branco 1x1 ou normal plana
/// 1x1), e o resultado e o material constante. **Um mapa que FALHA em carregar
/// tambem cai no padrao, mas com log** -- a diferenca entre "nao pedi" e "pedi e
/// nao veio" tem de aparecer, senao o defeito vira uma cor levemente diferente.
[[nodiscard]] inline MaterialHandle loadMaterial(const MaterialDesc& material)
{
    if (!detalhe::gEnabled)
    {
        return kSemMaterial;
    }
    if (detalhe::gConjuntoDeMaterial == NULL)
    {
        // **Falha com voz.** Sem este log, chamar antes do `load()` do casco
        // devolveria "sem material" e o objeto sairia BRANCO -- plausivel, e sem
        // nada dizendo por que. Todo outro caminho de erro deste modulo loga; a
        // primeira versao deste esqueceu.
        LOGF(eERROR, "[forgemesh] loadMaterial antes do load() do casco - carregue material no onEnter da cena");
        return kSemMaterial;
    }
    if (detalhe::gMaterialCount >= detalhe::gDesc.maxMaterials)
    {
        LOGF(eERROR, "[forgemesh] maxMaterials (%u) estourado", detalhe::gDesc.maxMaterials);
        return kSemMaterial;
    }

    const auto carregar = [](const char* caminho, const bool ehCor) -> Texture* {
        if (caminho == NULL)
        {
            return NULL;
        }
        Texture*        textura = NULL;
        TextureLoadDesc carga = {};
        carga.pFileName = caminho;
        carga.ppTexture = &textura;
        // **SRGB so na cor.** O mapa de normal guarda DIRECAO, e nao luz;
        // interpreta-lo como sRGB distorce os valores por uma gama de 2.2 e o
        // relevo sai errado -- de um jeito plausivel, que e o pior.
        carga.mCreationFlag = ehCor ? TEXTURE_CREATION_FLAG_SRGB : TEXTURE_CREATION_FLAG_NONE;
        addResource(&carga, NULL);
        waitForAllResourceLoads();
        if (textura == NULL)
        {
            LOGF(eERROR, "[forgemesh] textura '%s' nao carregou - fica o mapa padrao", caminho);
        }
        return textura;
    };

    const MaterialHandle h = detalhe::gMaterialCount++;
    Texture*             cor = carregar(material.baseColorPath, true);
    Texture*             normal = carregar(material.normalPath, false);

    detalhe::gTexturas[h * 2u] = cor;
    detalhe::gTexturas[h * 2u + 1u] = normal;
    detalhe::gFatores[h] = float4(material.baseColorFactor[0], material.baseColorFactor[1],
                                  material.baseColorFactor[2], 1.0f);

    detalhe::ligarMaterial(h + 1u, cor ? cor : detalhe::gCorPadrao, normal ? normal : detalhe::gNormalPadrao);

    LOGF(eINFO, "[forgemesh] material %u: cor='%s'%s normal='%s'%s", h,
         material.baseColorPath ? material.baseColorPath : "(padrao)", cor ? "" : " [FALTOU]",
         material.normalPath ? material.normalPath : "(padrao)", normal ? "" : " [FALTOU]");
    return h;
}

/// Numero de vertices de uma malha carregada (0 para handle invalido).
[[nodiscard]] inline uint32_t vertexCount(const Handle h)
{
    if (!detalhe::gEnabled || h >= detalhe::gMalhaCount)
    {
        return 0;
    }
    return detalhe::gMalhas[h]->mVertexCount;
}

// --- desenho (modo imediato, dentro do quadro) ---

/// A camera do quadro: `projecao * vista`. Vale ate a proxima chamada.
inline void setCamera(const mat4& viewProj)
{
    if (detalhe::gEnabled)
    {
        detalhe::gViewProj = viewProj;
    }
}

/// A luz direcional da cena. Vale ate a proxima chamada.
///
/// `direction` e **para onde a luz viaja** (a convencao do Blender); a negacao
/// e a normalizacao acontecem aqui dentro, uma vez por chamada, e nao por pixel.
inline void setLight(const DirectionalLight& luz)
{
    if (detalhe::gEnabled)
    {
        detalhe::gLuz = luz;
    }
}

/// A luz ambiente da cena. Vale ate a proxima chamada.
inline void setAmbient(const Ambient& ambiente)
{
    if (detalhe::gEnabled)
    {
        detalhe::gAmbiente = ambiente;
    }
}

/// O modo de conferencia (`usarMaterial = false` devolve a normal do vertice
/// como cor). Ate a task 19 transforma-lo em variante de pipeline.
inline void setModoDeConferencia(const ModoDeConferencia& modo)
{
    if (detalhe::gEnabled)
    {
        detalhe::gModo = modo;
    }
}

/// Desenha uma malha com a matriz de modelo dada.
///
/// O `viewProj * model` e composto aqui, na CPU, e enviado como UMA matriz --
/// e a unica conta por objeto que a CPU faz. Comparar com o Vigil, que fazia
/// uma conta por VERTICE.
inline void draw(const Handle h, const mat4& model, const MaterialHandle material = kSemMaterial)
{
    if (!detalhe::gEnabled || detalhe::gCmd == NULL || detalhe::gPipeline == NULL)
    {
        return;
    }
    if (h >= detalhe::gMalhaCount)
    {
        // **Nao e no-op: e um corpo que sumiu.** O caminho normal para chegar
        // aqui e um `loadMesh` que falhou e devolveu `kSemMalha` -- e a cena
        // segue desenhando o quadro inteiro sem ele. Este modulo prega em tres
        // comentarios que falhar calado e o pior jeito de errar; este era um
        // `return;` seco.
        //
        // O log e uma vez por execucao (a cena chama isto 60 vezes por segundo);
        // o `dropped` e o numero, e ele e o mesmo contador do lote cheio,
        // porque a pergunta que o consumidor faz e uma so: *sumiu alguma coisa?*
        if (!detalhe::gHandleInvalidoLogado)
        {
            LOGF(eWARNING, "[forgemesh] draw com malha invalida (%u de %u carregadas) - o loadMesh falhou?", h,
                 detalhe::gMalhaCount);
            detalhe::gHandleInvalidoLogado = true;
        }
        ++detalhe::gCurrent.dropped;
        return;
    }
    if (material != kSemMaterial && material >= detalhe::gMaterialCount)
    {
        // Material invalido NAO dropa o desenho: o corpo aparece com o material
        // padrao, que e uma imagem util. Mas ele tem de se distinguir de
        // `kSemMaterial` ("nao pedi"), senao um `loadMaterial` que falhou vira
        // uma cor levemente diferente e mais nada.
        if (!detalhe::gMaterialInvalidoLogado)
        {
            LOGF(eWARNING, "[forgemesh] draw com material invalido (%u de %u carregados) - fica o padrao", material,
                 detalhe::gMaterialCount);
            detalhe::gMaterialInvalidoLogado = true;
        }
    }
    if (detalhe::gUsadas >= detalhe::gDesc.maxInstances)
    {
        if (!detalhe::gOverflowLogged)
        {
            LOGF(eWARNING, "[forgemesh] lote cheio (%u instancias) - malha dropada", detalhe::gDesc.maxInstances);
            detalhe::gOverflowLogged = true;
        }
        ++detalhe::gCurrent.dropped;
        return;
    }

    const uint32_t trecho = detalhe::gFrameIndex * detalhe::gDesc.maxInstances + detalhe::gUsadas;
    ++detalhe::gUsadas;

    UniformeDaMalha uniforme = {};
    uniforme.mvp = detalhe::gViewProj * model;
    // A modelo vai SEPARADA porque a normal precisa chegar ao espaco de mundo.
    uniforme.modelo = model;

    // **A negacao acontece aqui**, uma vez por desenho, e nao por pixel: o desc
    // guarda "para onde a luz viaja" (convencao do Blender) e o shader quer "de
    // onde ela vem", que e o sinal que o `dot(N, L)` espera.
    //
    // `float4` e nao `vec4`: o `DATA(float4, ...)` do FSL expande, no lado C++,
    // para o `float4` POD do ModifiedSonyMath -- so `float4x4` e remapeado (para
    // `mat4`, em `fsl_srt.h:34`). Os dois tipos existem e nao se convertem.
    const vec3  viaja = vec3(detalhe::gLuz.direction[0], detalhe::gLuz.direction[1], detalhe::gLuz.direction[2]);
    const float comprimento = length(viaja);
    const vec3  vem = (comprimento > 1e-6f) ? (-viaja / comprimento) : vec3(0.0f, 1.0f, 0.0f);

    uniforme.luzDirecao = float4(vem.getX(), vem.getY(), vem.getZ(), 0.0f); // w nao usado
    uniforme.luzCor =
        float4(detalhe::gLuz.color[0], detalhe::gLuz.color[1], detalhe::gLuz.color[2], detalhe::gLuz.intensity);
    uniforme.ambiente = float4(detalhe::gAmbiente.color[0], detalhe::gAmbiente.color[1], detalhe::gAmbiente.color[2],
                               detalhe::gAmbiente.intensity);
    // O FATOR do material (o mapa vem pelo descriptor set), e a chave do modo de
    // conferencia no `w`.
    const bool   temMaterial = material < detalhe::gMaterialCount;
    const float4 fator = temMaterial ? detalhe::gFatores[material] : float4(1.0f, 1.0f, 1.0f, 1.0f);
    uniforme.albedo = float4(fator.x, fator.y, fator.z, detalhe::gModo.usarMaterial ? 1.0f : 0.0f);

    BufferUpdateDesc atualizacao = { detalhe::gUniformes[trecho] };
    beginUpdateResource(&atualizacao);
    memcpy(atualizacao.pMappedData, &uniforme, sizeof(uniforme));
    endUpdateResource(&atualizacao);

    Geometry* g = detalhe::gMalhas[h];
    cmdBindPipeline(detalhe::gCmd, detalhe::gPipeline);
    cmdBindDescriptorSet(detalhe::gCmd, trecho, detalhe::gConjunto);

    // O material e o SLOT `h + 1`; o 0 e o padrao. Trocar o material de um
    // objeto e trocar este indice -- **a geometria nao volta a passar**, que e o
    // criterio 4 da task 12.
    cmdBindDescriptorSet(detalhe::gCmd, temMaterial ? (material + 1u) : 0u, detalhe::gConjuntoDeMaterial);
    cmdBindVertexBuffer(detalhe::gCmd, 1, &g->pVertexBuffers[0], g->mVertexStrides, NULL);
    cmdBindIndexBuffer(detalhe::gCmd, g->pIndexBuffer, g->mIndexType, 0);
    cmdDrawIndexed(detalhe::gCmd, g->mIndexCount, 0, 0);

    ++detalhe::gCurrent.draws;
}

inline Stats lastFrameStats()
{
    // O teto e respondido na LEITURA, e nao acumulado: assim o primeiro quadro
    // ja devolve um teto valido em vez de zero. Mesma escolha do forgeline.
    Stats stats = detalhe::gLastFrame;
    stats.instanceCapacity = detalhe::gDesc.maxInstances;
    return stats;
}

} // namespace forgemesh
