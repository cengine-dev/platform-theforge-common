// SRT do forgemesh -- a ponte 3D (task 11), separada por FREQUENCIA (task 18).
//
// **A diferenca entre esta ponte e as tres 2D esta neste arquivo.** O
// `line.vert.fsl` diz por escrito *"sem SRT: este shader nao le recurso
// nenhum"* e recebe posicao ja em NDC, calculada na CPU. Este declara constant
// buffers, e por eles passam as matrizes -- entao a projecao acontece na GPU e
// a geometria pode ficar parada na memoria de video.
//
// Foi a ausencia deste arquivo que obrigou o Vigil a reprojetar vertice a
// vertice, todo quadro, dentro de um batcher de linhas 2D.
//
// ## Tres conjuntos, e cada um muda numa cadencia
//
// Ate a 0.22.1 havia UM conjunto, chamado `PerFrame`, ligado **por desenho**, e
// ele carregava tres coisas que mudam em ritmos diferentes: a camera e a luz
// (por quadro), as matrizes do objeto (por desenho) e o fator de cor (por
// material). O nome mentia, e a luz era **copiada para dentro de cada objeto
// desenhado**.
//
// | conjunto | o que tem | ligado |
// |---|---|---|
// | `PerFrame` | camera, luz, ambiente, modo de conferencia | uma vez por quadro |
// | `PerBatch` | o material: fator de cor + os dois mapas | quando o material muda |
// | `PerDraw` | a matriz de modelo do objeto | por objeto |
//
// **O ganho nao e so o nome.** O buffer por desenho encolheu de 192 para 64
// bytes, e a luz passou a ser escrita uma vez por quadro em vez de uma vez por
// corpo. E, mais importante para o que vem: a task 14 (skinning) precisa de um
// array de matrizes de junta **por objeto animado**, e agora ha onde poe-lo sem
// empilhar uma quarta cadencia no mesmo buffer.
//
// Este header e compartilhado entre o FSL (via #include nos .fsl) e o C++ (via
// #include no ForgeMesh.h, depois do defaults.h): cada lado expande as macros
// do seu jeito, e e isso que mantem shader e descriptor set em acordo.

#pragma once

// -----------------------------------------------------------------------------
// PerFrame -- o que vale para o quadro inteiro
// -----------------------------------------------------------------------------
STRUCT(UniformeDoQuadro)
{
    // `projecao * vista`. **Sem o modelo**: ele vive no `PerDraw`, e o vertex
    // shader faz as duas multiplicacoes.
    //
    // Coluna-maior, como o `mat4` do The-Forge -- por isso o shader usa
    // `mul(matriz, vetor)` com a matriz a ESQUERDA. Inverter a ordem produz uma
    // imagem plausivel e errada.
    DATA(float4x4, viewProj, None);

    // A luz DIRECIONAL.
    //
    // `xyz` = a direcao **de onde a luz VEM** (unitaria, em espaco de mundo).
    // Guardar "de onde vem" e nao "para onde vai" poupa uma negacao por pixel
    // e, mais importante, e o sinal que o `dot(N, L)` espera -- trocar o
    // sentido ilumina exatamente as faces erradas, que e uma imagem plausivel.
    //
    // **`w` nao e usado**, e e so o alinhamento de 16 bytes.
    DATA(float4, luzDirecao, None);

    // `rgb` a cor, `a` a INTENSIDADE (irradiancia, como o `energy` do Sun).
    DATA(float4, luzCor, None);

    // A ambiente: `xyz` a cor, `w` a intensidade. E o que impede o lado escuro
    // de ser preto puro -- no Blender e o World, e aqui e a mesma ideia.
    DATA(float4, ambiente, None);

    // `x != 0` liga o modo de CONFERENCIA: a cor vira a normal do vertice, sem
    // iluminacao. Ele e do QUADRO, e nao do material -- e uma chave de cena.
    //
    // (Ele devia ser variante de pipeline, e nao um ramo -- task 19.)
    DATA(float4, modoDeConferencia, None);
};

// -----------------------------------------------------------------------------
// PerBatch -- o MATERIAL, ligado quando o material muda
// -----------------------------------------------------------------------------
STRUCT(UniformeDoMaterial)
{
    // O FATOR de cor base, multiplicado pelo mapa. Sem mapa, o mapa e branco
    // 1x1 e sobra so o fator -- que e como *"material sem mapa e um valor
    // constante, nao um erro"* se realiza sem um `if`.
    //
    // Ele morava no buffer POR DESENHO ate a 0.22.1: dois objetos com o mesmo
    // material reescreviam o mesmo numero duas vezes, e "material" nao era uma
    // unidade que se pudesse ligar de uma vez.
    DATA(float4, baseColor, None);
};

// -----------------------------------------------------------------------------
// PerDraw -- o objeto
// -----------------------------------------------------------------------------
STRUCT(UniformeDoObjeto)
{
    // A matriz de MODELO. Ela posiciona o corpo e leva a normal ao espaco de
    // MUNDO, que e onde a luz vive. Sem ela, girar o objeto giraria a
    // iluminacao junto -- um defeito que parece "a luz esta presa no objeto".
    //
    // **Limitacao registrada:** para normal, o certo em geral e a
    // INVERSA-TRANSPOSTA da modelo. Com rotacao, translacao e escala UNIFORME
    // (o caso de tudo o que este ecossistema desenha hoje) as duas coincidem a
    // menos de um fator, que o `normalize` do fragment absorve. Escala
    // nao-uniforme torceria a normal -- e ai esta struct ganha um campo, e nao
    // um remendo no shader.
    DATA(float4x4, modelo, None);
};

BEGIN_SRT_NO_AB(MeshSrtData)
    BEGIN_SRT_SET(PerFrame)
        DECL_CBUFFER(PerFrame, CBUFFER(UniformeDoQuadro), gQuadro)
    END_SRT_SET(PerFrame)

    // **Nao ha sampler declarado**: o `defaults.h` ja publica onze samplers
    // ESTATICOS (`gSamplerTrilinearWrap` e companhia, em `space 100`). Declarar
    // um proprio seria um recurso a mais para ligar por material sem nada a
    // decidir por material.
    BEGIN_SRT_SET(PerBatch)
        DECL_CBUFFER(PerBatch, CBUFFER(UniformeDoMaterial), gMaterial)
        DECL_TEXTURE(PerBatch, Tex2D(float4), gMapaDeCor)
        DECL_TEXTURE(PerBatch, Tex2D(float4), gMapaDeNormal)
    END_SRT_SET(PerBatch)

    BEGIN_SRT_SET(PerDraw)
        DECL_CBUFFER(PerDraw, CBUFFER(UniformeDoObjeto), gObjeto)
    END_SRT_SET(PerDraw)
END_SRT(MeshSrtData)
