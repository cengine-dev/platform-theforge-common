// SRT do forgemesh -- a ponte 3D (task 11).
//
// **A diferenca entre esta ponte e as outras tres esta neste arquivo.** O
// `line.vert.fsl` diz por escrito *"sem SRT: este shader nao le recurso
// nenhum"* e recebe posicao ja em NDC, calculada na CPU. Este declara um
// constant buffer, e por ele passa a matriz -- entao a projecao acontece na GPU
// e a geometria pode ficar parada na memoria de video.
//
// Foi a ausencia deste arquivo que obrigou o Vigil a reprojetar vertice a
// vertice, todo quadro, dentro de um batcher de linhas 2D.
//
// Compartilhado entre o FSL (via #include nos .fsl) e o C++ (via #include no
// ForgeMesh.h, depois do defaults.h): cada lado expande as macros do seu jeito,
// e e isso que mantem shader e descriptor set em acordo.

#pragma once

STRUCT(UniformeDaMalha)
{
    // `projecao * vista * modelo`, composta na CPU e enviada como UMA matriz.
    //
    // Coluna-maior, como o `mat4` do The-Forge -- por isso o shader faz
    // `mul(matriz, vetor)` com a matriz a ESQUERDA. Inverter a ordem produz uma
    // imagem plausivel e errada.
    DATA(float4x4, mvp, None);

    // **A matriz de MODELO, separada** (task 13). Ela leva a normal do espaco de
    // modelo para o de MUNDO, que e onde a luz vive. Sem ela, girar o objeto
    // giraria a iluminacao junto -- o corpo ficaria com a mesma face clara para
    // sempre, e isso e um defeito que parece "a luz esta presa no objeto".
    //
    // **Limitacao registrada:** para normal, o certo em geral e a
    // INVERSA-TRANSPOSTA da modelo. Com rotacao, translacao e escala UNIFORME
    // (que e o caso de tudo o que este ecossistema desenha hoje) as duas
    // coincidem a menos de um fator, que o `normalize` do fragment absorve.
    // Escala nao-uniforme torceria a normal -- e ai esta struct ganha um campo,
    // e nao um remendo no shader.
    DATA(float4x4, modelo, None);

    // A luz DIRECIONAL: para onde apontar, de que cor, com que forca.
    //
    // `xyz` = a direcao **de onde a luz VEM** (unitaria, em espaco de mundo).
    // Guardar "de onde vem" e nao "para onde vai" poupa uma negacao por pixel e,
    // mais importante, e o sinal que o `dot(N, L)` espera -- trocar o sentido
    // ilumina exatamente as faces erradas, que e uma imagem plausivel.
    //
    // **`w` nao e usado**, e e so o alinhamento de 16 bytes do constant buffer.
    // A primeira versao guardava a intensidade aqui E no `luzCor.a`; dois campos
    // com o mesmo valor e ninguem conferindo sao um desencontro esperando.
    DATA(float4, luzDirecao, None);

    // `rgb` a cor, `a` a INTENSIDADE (irradiancia, como o `energy` do Sun).
    DATA(float4, luzCor, None);

    // A ambiente: `xyz` a cor, `w` a intensidade. E o que impede o lado escuro
    // de ser preto puro -- no Blender e o World, e aqui e a mesma ideia.
    DATA(float4, ambiente, None);

    // O FATOR de cor base, multiplicado pelo mapa (task 12). Sem mapa, o mapa e
    // branco 1x1 e sobra so o fator -- que e como *"material sem mapa e um valor
    // constante, nao um erro"* se realiza sem um `if`.
    //
    // `w` liga o modo de conferencia: 1 = usar o material, 0 = usar a NORMAL
    // como cor. O segundo nao e enfeite; e o que permite conferir a malha e a
    // iluminacao **separadamente**, que e o metodo deste ecossistema. (Ele deve
    // virar variante de pipeline -- task 19 do casco.)
    DATA(float4, albedo, None);
};

// Um trecho por (quadro em voo x instancia). Escrever no do quadro N e seguro
// porque a fence do cmd ring garante que a GPU ja consumiu o de `frameCount`
// quadros atras -- mesma mecanica dos batchers 2D do casco.
BEGIN_SRT_NO_AB(MeshSrtData)
    BEGIN_SRT_SET(PerFrame)
        DECL_CBUFFER(PerFrame, CBUFFER(UniformeDaMalha), gMalha)
    END_SRT_SET(PerFrame)

    // **O MATERIAL, no proprio conjunto** (task 12), ligado quando o material
    // muda -- e nao por objeto nem por quadro. E a primeira frequencia deste
    // modulo que esta no conjunto com o nome certo, e ela mostra na pratica o
    // que a task 18 vai fazer com as outras duas.
    //
    // **Nao ha sampler declarado**: o `defaults.h` ja publica onze samplers
    // ESTATICOS (`gSamplerTrilinearWrap` e companhia, em `space 100`). Declarar
    // um proprio seria um recurso a mais para ligar por material sem nada a
    // decidir por material.
    BEGIN_SRT_SET(PerBatch)
        DECL_TEXTURE(PerBatch, Tex2D(float4), gMapaDeCor)
        DECL_TEXTURE(PerBatch, Tex2D(float4), gMapaDeNormal)
    END_SRT_SET(PerBatch)
END_SRT(MeshSrtData)
