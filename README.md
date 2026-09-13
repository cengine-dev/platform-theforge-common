# platform-theforge-common

Codigo comum para jogos de estudo que usam The Forge como plataforma grafica.

Este repositorio existe para concentrar adaptadores e utilitarios que sao
reutilizaveis entre jogos, mas que nao pertencem ao `cengine` generico porque
conhecem detalhes concretos do The Forge: renderer, command buffers, resource
loader, shaders FSL, input da plataforma, fontes, texturas e ciclo
`Init/Load/Unload/Draw`.

## Objetivo

Separar responsabilidades entre os projetos:

- `cengine`: loop, portas e mecanismos independentes de plataforma.
- `platform-theforge-common`: infraestrutura reutilizavel especifica do
  The Forge.
- jogos (`8puzzle`, `spaceinvaders`, `asteroids`, ...): dominio, regras,
  cenas concretas e assets de cada jogo.

## Conteudo (0.22.1)

> **A versao que o BUILD le esta no `TheForgeCommon.props`**
> (`TheForgeCommonVersion`), e ela e a fonte. O numero deste titulo e prosa para
> quem le; quando subir a versao, suba os dois.
>
> O consumidor que quiser ser avisado quando o casco se mover declara
> `<TheForgeCommonExpectedVersion>` no `.vcxproj` dele, e o build falha na
> divergencia. Quem nao declara nada nao ganha conferencia — e aditivo.

> **Nota de manutencao:** as secoes abaixo pulam da 0.22.1 para a 0.10.0. As
> versoes 0.11.0 a 0.15.0 existem no repositorio (ver `git log`) mas nunca
> ganharam secao aqui — `Paint-Mask`, o ARRASTAR, o triangulo do `forgeline`, os
> contadores de lote e a cor por vertice. A lacuna e anterior a 0.16.0 e fica
> registrada em vez de continuada em silencio.

### Novo na 0.22.1

Duas correcoes achadas **lendo o codigo**, e nao executando — o degrau 07 ja
tinha passado no teste do dono quando as duas apareceram.

- **O religamento de material ligava `NULL`.** O laco que re-liga materiais no
  `load()` (para reload de shader) nao fazia o fallback para os mapas padrao, e
  um material sem mapa guarda `NULL`. Era inofensivo **por acidente** — o `load`
  roda antes de haver material, e o laco ficava vazio. Codigo que parece certo,
  com comentario explicando por que existe, esperando o primeiro reload.

- **`loadMaterial` falhava calado.** A guarda de "chamado antes do `load()`"
  devolvia `kSemMaterial` sem log, e o objeto sairia branco sem explicacao. Agora
  loga o que fazer.

### Novo na 0.22.0

- **MATERIAL e TEXTURA** (task 12). `forgemesh::MaterialDesc`, `loadMaterial` e
  `draw(malha, modelo, material)`, com um descriptor set **`PerBatch`** de dois
  mapas por material. Consumidor de validacao: o lab **`diorama`** (degrau 07).

  **"Sem mapa" nao e um caso especial.** Sem mapa de cor, o mapa e branco 1x1;
  sem mapa de normal, e plano 1x1. O shader multiplica e transforma como sempre
  — nenhum `if`, nenhuma variante de pipeline, nenhum caminho de codigo que so
  alguns modelos percorrem (e que so alguns modelos testam).

  **A tangente teve de ser pedida ao Blender.** Os `.bin` nao tinham `TANGENT`:
  o `export_tangents` do exportador glTF e `False` por padrao. Corrigido na
  ferramenta do `diorama` e os assets regerados.

  **O swizzle do normal map e `{x,x,x,y}`** — X nos canais de cor, **Y no
  ALFA**, Z reconstruido. Assumir `RGB = XYZ` ilumina para o lado errado, e a
  imagem continua parecendo ter relevo. (Achado do degrau 01 do lab, cobrado
  aqui.)

  **`SRGB` so no mapa de cor:** o de normal guarda direcao, e nao luz.

  **Fora de escopo, e escrito:** `metallic` e `roughness` — os dois so
  significam algo dentro de um BRDF especular, e este shader e Lambert. Campos
  que nenhuma conta le sao enfeite que parece funcionalidade.

- **`setBaseColor` virou `setModoDeConferencia`.** A cor base agora e do
  material; o que sobrou na chamada e so o modo de diagnostico. **Muda a
  assinatura**, mas nenhum consumidor 2D usa `forgemesh`.

### Novo na 0.21.0

- **A LUZ** (task 13). `forgemesh::setLight`, `setAmbient` e `setBaseColor` —
  uma direcional mais uma ambiente, tudo por QUADRO, como o `setCamera`.
  Consumidor de validacao: o lab **`diorama`** (degrau 06).

  **Nao e um sistema de luzes, e nao vai ser.** Quantas luzes uma cena tem e
  decisao de JOGO. Direcional + ambiente e o menor conjunto que responde "de
  onde vem a luz?", e e exatamente o que o Blender chama de Sun + World.

  **A luz e da CENA, e nao do objeto.** Dois corpos no mesmo quadro reagem a
  mesma luz sem cada um carregar o seu rig — que e o criterio 3 da task, e o que
  o Vigil nao conseguia: la a iluminacao era um punhado de numeros ajustados a
  mao dentro do laco de desenho de um corpo so.

  **A normal agora chega ao espaco de MUNDO.** A `UniformeDaMalha` ganhou a
  matriz de modelo separada; sem ela, girar o objeto giraria a iluminacao junto.
  (Limitacao registrada no `mesh.srt.h`: escala nao-uniforme exigiria a
  inversa-transposta.)

  **O sentido da direcao vive em duas convencoes, de proposito:** o desc recebe
  *"para onde a luz viaja"* (a do Blender, e a dos `.txt` de captura) e o shader
  quer *"de onde ela vem"*. A negacao acontece uma vez por desenho, dentro do
  `draw`. Trocar o sentido nao da erro — **ilumina as faces erradas**, e a
  imagem continua parecendo iluminada.

  **O `1/pi` do Lambert nao e cosmetico.** A `intensity` e IRRADIANCIA (o
  `energy` do Sun), e a radiancia de uma superficie lambertiana e
  `albedo/pi * irradiancia * NdotL`. Sem a divisao, os numeros do rig davam
  **2.39** numa face iluminada e saturavam a partir de `nDotL = 0.384` — toda
  face a menos de 67 graus da luz saia branco puro, e o contraste entre faces
  sumia. Com ela, o pico e 0.85. (A ambiente **nao** divide: para ambiente
  uniforme o `pi` cancela.)

  **O que nao bate com o Blender:** o mapeamento de tom do Eevee (AgX/Filmic).
  A faixa e a mesma; o criterio e o contraste RELATIVO entre as faces.

### Novo na 0.20.1

- **O layout de vertice do `forgemesh` passou a ter UMA definicao** (task 15a).
  Refatoracao pura: nenhuma API mudou, e o executavel do consumidor saiu com o
  mesmo tamanho.

  Ele estava escrito **duas vezes** — uma para montar o pipeline, outra para
  pedir o arranjo ao `ResourceLoader` —, e as duas tinham de concordar.
  Divergirem nao daria erro: o `ResourceLoader` copia o tamanho da ORIGEM, e o
  unico guarda-corpo (`ASSERT(dstFormatSize == srcFormatSize)`,
  `ResourceLoader.cpp:1767`) so existe em Debug.

  **E a assinatura exata do defeito dos cubos PRETOS** do degrau 04 do
  `diorama`: layout pedido diferente do que o arquivo tem, normal invalida,
  `normalize()` devolvendo NaN, alvo UNORM grampeando em zero. Duas copias de
  uma decisao que precisa ser uma so sao esse defeito esperando acontecer.

### Novo na 0.20.0

- **`TheForgeCommon.props` — o casco se descreve** (task 16). Fontes, shaders e
  include path num arquivo so; o consumidor importa numa linha:

  ```xml
  <Import Project="$(CommonRoot)src\TheForgeCommon\TheForgeCommon.props" />
  ```

  **O que isto conserta.** Ate aqui este repo nao tinha alvo de build: cada jogo
  enumerava os `.cpp` do casco no proprio `.vcxproj`. Entao **um `.cpp` novo aqui
  nunca era aditivo** — era sempre uma edicao obrigatoria em 8+ projetos que ja
  existem. Foi assim que a 0.16.0 quebrou o link de todos eles.

  A resposta de entao (tornar os modulos novos "so cabecalho") tratava o
  sintoma: *"nunca acrescente um `.cpp`"* e uma regra sobre o BUILD disfarcada de
  regra sobre codigo, e enquanto ela valia o desenho de cada modulo novo era
  decidido por uma limitacao de `.vcxproj`.

  **A adocao e OPT-IN, e nenhum projeto de referencia foi tocado.** As listas
  explicitas continuam validas — o `.props` e uma segunda forma de dizer a mesma
  coisa. Cada projeto adota quando alguem voltar a toca-lo, se voltar.

  Consumidor de validacao: o lab **`diorama`**. O executavel saiu com o mesmo
  tamanho (`709.632 bytes`), e o criterio central foi **provado e nao deduzido**:
  um `.cpp` novo no casco compilou com o **hash do `.vcxproj` inalterado`**.

  **O que fica liberado:** header-only volta a ser escolha, e nao imposicao.

### Novo na 0.19.0

- **Correcao: o `forgemesh` voltou para `FRONT_FACE_CCW`** (o default, e o do
  glTF). Uma linha, e ela desfaz um engano.

  A 0.18.0 usava `FRONT_FACE_CW`, com a justificativa de que as matrizes do
  The-Forge (`lookAtLH`/`perspectiveLH`) sao canhotas e invertem o enrolamento
  na tela. **O mecanismo estava certo e a conclusao errada:** matriz canhota
  sobre malha destra desenha a cena **ESPELHADA**, e o `CW` era o espelhamento
  se anunciando — nao a resposta.

  Com a `cengine::camera3d` (0.16.0), que emite matrizes **destras**, o
  enrolamento do glTF chega intacto e o `CCW` nativo volta a valer.

  > **O sintoma agora aponta para a causa certa:** quem alimentar o `forgemesh`
  > com matrizes canhotas vera o interior dos corpos, e isso quer dizer *"a
  > camera esta espelhando a cena"* — que e a informacao util.

  Consumidor de validacao: o lab **`diorama`** (degrau 05). Nenhum jogo 2D e
  afetado: a linha so existe com `mesh.enabled = true`, e nenhum a liga.

### Novo na 0.18.0

- **`forgemesh` — o QUARTO irmao das pontes de desenho, e o primeiro em 3D**
  (task 11). `TheForgeWindowDesc` ganhou `mesh.enabled` (default **false**).
  Ligado, o casco carrega `.bin` do `AssetPipelineCmd` direto para a GPU e
  desenha por matriz. Consumidor de validacao: o lab **`diorama`** (degrau 04).

  ```
  forgeui      texto
  forgesprite  quads texturizados
  forgeline    linhas e triangulos 2D
  forgemesh    malha 3D                 <- novo
  ```

  **A diferenca esta na assinatura.** As tres pontes 2D recebem posicao **ja em
  NDC** — quem calcula onde o vertice cai na tela e a CPU, todo quadro, vertice
  a vertice. Esta recebe **uma matriz**: a geometria sobe uma vez e fica, e o
  que viaja por quadro sao 64 bytes por objeto.

  **O custo de nao ter isto foi medido.** O Vigil pedia
  `GEOMETRY_LOAD_FLAG_SHADOWED` para trazer a malha de volta para a CPU —
  ignorando os vertex buffers que o `loadGeometry` ja tinha enchido —,
  reprojetava a cada quadro e empurrava o resultado pelo vertex buffer dinamico
  do `forgeline`. Sao **684 linhas de `ForgeMalha.h`** (projecao, normal por
  face, deteccao de vinco, iluminacao, ordenacao por distancia) que esta ponte
  substitui, e o teto de lote do `forgeline` deixa de valer para malha.

  **Duas regras que ele NAO compartilha com os irmaos:**

  1. **Nao ha lote.** Um `draw` = um `cmdDrawIndexed`. Batching amortiza a
     chamada quando a primitiva e minuscula; malha nao e minuscula, e cada uma
     tem matriz propria.
  2. **Ele desenha no passe com PROFUNDIDADE, e nunca depois do overlay.** As
     pontes 2D chamam `forgeframe::enterOverlay()`; esta nao chama — ela e quem
     usa o passe com depth. Para a cena: malha primeiro, HUD depois.

  **`mesh.enabled` exige `depth.enabled`.** Malha sem profundidade e ordem do
  pintor, que e o que a 0.17.0 substituiu — entao o `forgemesh` recusa e loga,
  em vez de desenhar algo plausivel e errado.

  **O descarte de face e do rasterizador**, com `FRONT_FACE_CW`: o glTF e destro
  e define a face da frente como anti-horaria, mas `lookAtLH`/`perspectiveLH`
  levam a um espaco de recorte canhoto, e a troca de mao inverte o enrolamento
  na tela. (No Vigil o descarte era na CPU e ficou com o sinal invertido por um
  degrau inteiro — *"o jogo desenhava as COSTAS"*.)

- **A desc mora em `ForgeMeshDesc.h`, separada da implementacao.** O
  `TheForgeWindowManager.h` e incluido pelo `main` de todo jogo; se a desc
  morasse no `ForgeMesh.h`, todo consumidor receberia junto o
  `Common_3/Graphics/FSL/defaults.h`, que despeja 20 macros na unidade de
  traducao — entre elas `CAT`, `VARNAME` e `DESCRIPTOR_TABLE`. **Isso nao seria
  adicao pura.** Os irmaos 2D nao tem esta questao porque a implementacao deles
  mora num `.cpp`, onde o `defaults.h` fica confinado.

- **Custo cobrado de quem nao usa:** o `Shaders.list` do casco ganhou
  `mesh.vert`/`mesh.frag`, entao todo consumidor compila dois shaders a mais.
  Nenhum deles e carregado com `mesh.enabled = false`. E o unico preco que esta
  versao cobra de um jogo 2D, e esta escrito aqui para nao ser descoberto.

### Novo na 0.17.0

- **Profundidade opt-in** (task 10). `TheForgeWindowDesc` ganhou
  `depth.enabled` (default **false**) e `depth.clearDepth` (default `1.0f`).
  Ligado, o casco cria um render target `D32_SFLOAT` junto com o swapchain,
  recria os dois juntos no resize, limpa a profundidade a cada quadro e o
  `forgeframe::target().depthFormat` passa a responder de verdade.
  Consumidor de validacao: o lab **`diorama`** (degrau 03).

  **O que isto substitui.** Sem depth buffer, oclusao e ordem do pintor:
  ordenar os corpos por distancia e desenhar de tras para frente. O Vigil fez
  isso e mediu o preco — **6,8% dos pixels do modelo `prisao` errados**. Nao ha
  ordenacao mais esperta que conserte, porque dois corpos que se atravessam nao
  tem "o mais perto": a resposta muda de pixel para pixel.

  **Nenhum jogo 2D e afetado.** Com `enabled = false` nenhum render target de
  profundidade e alocado e o `mDepthStencil` do quadro segue
  `{ NULL, LOAD_ACTION_DONTCARE }` — a mesma linha de antes.

- **O passe de OVERLAY** (`forgeframe::enterOverlay`) — descoberto montando a
  task 10, e **nao previsto na spec dela**.

  O `cmdDrawTextWithFont` desenha com `pPipelines[mText3D]`, e o pipeline 2D
  (indice 0) do fontstash e criado com `mDepthStencilFormat = UNDEFINED`
  **fixo** (`FontSystem.cpp:440`). Nao ha desc que mude isso. Com um depth
  target ligado, o pipeline do texto declara um conjunto de anexos diferente do
  que esta ligado. O proprio The-Forge resolve desligando o depth antes da UI
  (`01_Transformations.cpp:1022`).

  Entao as tres pontes 2D (`forgeui`, `forgesprite`, `forgeline`) chamam
  `forgeframe::enterOverlay()` antes do primeiro desenho do quadro: o casco
  fecha o passe com profundidade e abre um de cor sem depth, carregando o que ja
  foi desenhado. **A regra nova que isso cria:** desenhado o primeiro pixel de
  ponte 2D, o quadro nao volta ao passe com profundidade. As pontes 2D sao
  overlay, e overlay vem por cima — que e a ordem que toda cena deste
  ecossistema ja usa (mundo primeiro, HUD depois). Sem depth ligado, a chamada e
  uma comparacao e volta.

- **Correcao: o `ForgeFrame` virou so cabecalho.** A 0.16.0 o entregou como
  `.h` + `.cpp` e afirmou por escrito que era adicao pura. **Era falso.** O
  `TheForgeWindowManager.cpp` passou a chamar `forgeframe::publish`, e todo jogo
  do ecossistema lista aquele `.cpp` no proprio `.vcxproj` mas nao listava o
  novo — simbolo externo nao resolvido no link, em 8+ consumidores.

  O casco nao e biblioteca com alvo de build: cada jogo enumera os `.cpp` do
  casco no `.vcxproj` dele. **Um `.cpp` novo no casco nunca e aditivo** — e
  sempre uma edicao obrigatoria em todo projeto que ja existe. Por isso o estado
  do `ForgeFrame` agora vive em `inline` (C++17): uma instancia por programa,
  zero unidade de traducao nova, zero `.vcxproj` tocado.

### Novo na 0.16.0

- **`ForgeFrame` — o quadro deixa de ser opaco** (task 09). O casco publica
  `Renderer*`, o `Cmd*` do quadro, o formato/amostragem do render target e o
  indice do frame em voo. Consumidor de validacao: o lab **`diorama`**
  (degrau 02).

  **Por que isto e a coisa mais importante que este repo ganhou.** Ate aqui o
  `TheForgeWindowManager` expunha cinco metodos de ciclo de vida e nada mais;
  `Renderer`, `Cmd` e render target eram estaticos de arquivo. Uma cena nao
  conseguia criar pipeline, shader ou constant buffer, e o unico shader
  disponivel para geometria (o do `forgeline`) recebe posicao **ja em NDC** e
  declara por escrito *"sem SRT: este shader nao le recurso nenhum"*.

  > **Sem lugar onde passar uma matriz, a projecao TEM que ser na CPU.**

  Isso nao e teoria: foi o teto que fechou o Vigil no degrau 24. Ele desenhou
  malha 3D reimplementando projecao, descarte de face, profundidade (ordem do
  pintor, errando 6,8% dos pixels de um modelo) e iluminacao na CPU, dentro de
  um batcher de linhas 2D — **nao por escolha, mas porque nao havia onde passar
  uma matriz**. E o The-Forge ja subia a malha para a GPU; o jogo pedia
  `GEOMETRY_LOAD_FLAG_SHADOWED` para ter copia na CPU e ignorava os buffers
  prontos.

  **O contrato, e ele e curto:** *quem liga render target assume o quadro
  inteiro; o casco nao restaura.* Desenhar com pipeline proprio (`cmdBindPipeline`
  + `cmdDraw`) e o uso pretendido e nao tem esse problema. O `cmd()` e **NULL
  fora do quadro** de proposito — e o unico jeito de um consumidor distinguir
  "estou no meio do quadro" de "estou na carga", e gravar comando num `Cmd`
  fechado nao da erro, da corrupcao.

  **Nenhum jogo 2D e afetado.** Nada foi removido nem renomeado; `forgeui`,
  `forgesprite` e `forgeline` seguem sendo a forma certa de desenhar sem saber o
  que e um `Cmd`. Este cabecalho existe para os MODULOS do casco (o `forgemesh`
  da task 11) e para quem precise de pipeline proprio.

  **E o unico item do pipeline 3D que so ACRESCENTA** — ele existe para que os
  cinco seguintes (depth buffer, malha na GPU, shader com SRT, material,
  skinning) possam apagar mais codigo do que acrescentam.

### Novo na 0.10.0

- **O ARRASTAR** (`forgeui::drag()` / `readDrop()` / `cancelDrag()`), com a
  captura de `WM_LBUTTONUP` que faltava no WndProc. Consumidor de validacao: o
  Klondike (degrau 06).

  Vocabulario LOCAL, e nao porta da engine — mesmo criterio que segurou o mouse
  por dois jogos (task 27): um consumidor, e nenhuma evidencia de que esta seja
  a forma certa para o proximo. Se um segundo jogo arrastar, a comparacao
  decide.

  **Duas leituras, porque arrastar nao cabe na fila de cliques.** Um clique e
  um EVENTO (aconteceu, tem uma posicao, acabou); arrastar e um CICLO DE VIDA
  com duas pontas que carregam posicoes DIFERENTES — a origem diz o que se
  pega, o destino diz para onde vai. Entao: `drag()` e estado continuo (para
  desenhar o que esta na mao, todo quadro) e `readDrop()` e edge (um por gesto,
  com as duas posicoes juntas).

  **O casco NAO decide se o gesto foi clique ou arrasto.** Ele entrega as duas
  pontas; quantos pixels de folga ainda contam como toque parado e politica do
  JOGO — depende do tamanho do alvo e do tremor de mao que se perdoa.

- **`SetCapture` no `WM_LBUTTONDOWN`** (e `ReleaseCapture` no UP). Sem isto,
  soltar o botao FORA da janela nao gera `WM_LBUTTONUP` nenhum e o gesto ficava
  pendurado para sempre — a carta grudada no cursor. E o primeiro lugar em que
  o arrastar cobra algo que o clique nunca cobrou. `WM_CAPTURECHANGED` e
  `WM_KILLFOCUS` cancelam o gesto: cancelar e melhor que soltar num destino que
  o jogador nao escolheu.

### Mudou de casa na 0.9.0

- **O vocabulario de mouse SUBIU para a `cengine::input`** (task 27 da engine,
  cengine 0.14.0). `MouseButton` e `MouseClick` agora sao da engine; esta ponte
  guarda a instancia de `cengine::input::Mouse` e continua fazendo o que so ela
  pode fazer — CAPTURAR (o WndProc traduzindo `WM_MOUSEMOVE`/`WM_LBUTTONDOWN`).

  Mesmo caminho que o teclado fez na 0.8.0 da engine: o vocabulario viveu aqui
  enquanto tinha um consumidor so (o Bulwark), e subiu quando o SEGUNDO
  (Tactics) usou a forma do primeiro **sem pedir nenhuma mudanca de API**.

  **Nao quebra nada.** `forgeui::MouseButton` e `forgeui::MouseClick` continuam
  existindo como ALIAS dos tipos da engine, e as funcoes globais
  (`mouseX`/`mouseY`/`readMouseClick`) tem exatamente a mesma assinatura. Quem
  quiser falar com a porta direto tem `forgeui::mouse()`, irmao do
  `forgeui::keyboard()`. Requer cengine >= 0.14.0.

### Corrigido na 0.8.0

- **O X da janela nao forja mais um `Escape`.** O `WM_CLOSE` agora marca um
  pedido de fechamento que a cengine le pelo `IWindowManager::shouldClose()`
  (0.13.0); o loop para no fim do quadro e o `cleanup()` roda normalmente.

  Ate a 0.7.1 o X empurrava um `Escape` FALSO na fila de teclas, para a cena
  rotear para a saida. Funcionava por sorte: so nos jogos em que ESC ja
  significava "sair". Nos que usam ESC para "voltar ao menu" — o Delve e o
  Bulwark — clicar no X levava ao MENU. "O jogador pediu para sair" e "o
  sistema mandou fechar" nao sao a mesma coisa e nao podem dividir o mesmo
  canal. Requer cengine >= 0.13.0.

### Novo na 0.7.0

- `tools/Paint.ps1` — os helpers de pintura dos tools de atlas, extraidos das
  copias identicas de CINCO jogos (`Set-Pixel` no breakout, mario-bros, zelda,
  starforce e delve; `Fill-Rect` em tres). Consumidor de validacao: o Bulwark
  (degrau 07). Os cinco tools existentes nao migram — o atlas deles ja esta
  gerado e versionado, mesma regra da extracao do `Write-Dds`.

  **O que mudou na extracao:** as copias liam `$script:texW`/`$script:pixels`
  do escopo de quem chamava — funciona por acidente e quebra em silencio se o
  tool renomear a variavel. Virou um CANVAS explicito
  (`New-AtlasCanvas -Width -Height`), passado as funcoes. A ARTE de cada atlas
  segue no tool do jogo; isto e so o pincel.

### Novo na 0.6.0

- **Mouse** (`ForgeUi.h`) — posicao do ponteiro (`mouseX`/`mouseY`, estado
  continuo) e fila de cliques (`readMouseClick`, edges, um por aperto),
  capturados no WndProc (`WM_MOUSEMOVE`, `WM_LBUTTONDOWN`, `WM_RBUTTONDOWN`).
  Primeiro mouse do ecossistema: os nove jogos ate aqui eram 100% teclado.

  **O vocabulario mora AQUI, nao na `cengine::input`.** O enum de teclas so
  subiu para a engine quando ja era a QUARTA copia identica nos jogos; o mouse
  tem UM consumidor (o Bulwark, degrau 05) e nenhuma evidencia de que esta
  forma seja a certa para o proximo. Se um segundo jogo precisar, a comparacao
  decide — mesmo caminho que o teclado fez (ADR 0002 da cengine).

  Duas decisoes de desenho que valem para quem for consumir: **posicao e
  ESTADO, clique e EDGE** (realce sob o cursor precisa de todo quadro;
  construir precisa de uma vez por aperto), e **o clique carrega a posicao DO
  MOMENTO** em que aconteceu — o ponteiro pode andar entre o aperto e o quadro
  em que a cena o le. As coordenadas sao PIXELS da area util, o mesmo espaco
  do `drawText`: esta ponte nao sabe o que e uma celula, e traduzir para o
  mundo do jogo e trabalho do jogo.

### Corrigido na 0.5.1

- **`WM_CHAR` agora filtra auto-repeat**, como o `WM_KEYDOWN` sempre filtrou.
  A fila de edges promete UM EVENTO POR APERTO FISICO, e as duas metades dela
  tinham semanticas diferentes: segurar uma seta mandava um evento, segurar
  uma letra mandava um por quadro. Ninguem tinha batido nisso porque, dos nove
  jogos, o **Bulwark** foi o primeiro a consumir `Key::Char` por esta ponte —
  os outros usam so setas, Enter e ESC. No jogo dele, segurar espaco soltava
  um inimigo por quadro. Validado pelo Bulwark (degrau 03).

### Novo na 0.5.0

- `forgeaudio` (`ForgeAudio.{h,cpp}`) — o backend XAudio2 da porta
  `cengine::audio::Player` (task 24 da cengine), extraido das copias identicas
  do breakout e do mario-bros: COM, pool de 8 vozes round-robin (PCM 16-bit
  mono 44.1kHz) e os helpers de sintese `synth`/`concat`. O jogo entrega a
  TABELA de samples no `init` (o enum `Sound` e as receitas ficam no jogo).
  Sem device, o jogo roda mudo (degradacao normal do contrato da porta).
- `tools/Write-Dds.ps1` — o escritor DDS (RGBA8 sem compressao, header DX10)
  que fechava os tools de atlas dos jogos, agora dot-sourceavel do checkout
  irmao. A arte de cada atlas segue no tool do jogo.

### Novo na 0.4.0

- `forgesprite::drawSpriteRect(region, x, y, w, h, cor)` — desenha a regiao do
  atlas num **retangulo de destino arbitrario**, em vez de uma escala uniforme.

  O `drawSprite(region, x, y, escala, cor)` continua existindo: era o formato de
  que o spaceinvaders precisava, onde o sprite era 1:1 com as unidades do mundo.
  Mas escala uniforme nao da conta de um jogo cujos corpos tem proporcoes
  diferentes entre si (o breakout: tijolo 60x20, raquete 110x16, bola 12x12), nem
  de uma projecao arena->tela que estica X e Y de forma diferente. O quad ja era
  montado a partir de dois cantos — a funcao nova so para de derivar o segundo
  canto de uma escala.

### Novo na 0.3.0

- `ForgeUi` **deixou de ter vocabulario proprio**: `Key`/`KeyEvent` e o contrato
  (fila de edges + estado segurado) subiram para a `cengine::input` (task 20 da
  engine, >= 0.8.0). Esta ponte guarda o que sempre foi dela — a **captura** (o
  WndProc traduzindo `VK_*` para `Key`) — e delega o resto a um
  `cengine::input::Keyboard`.

  Era a **quarta copia** do mesmo enum no ecossistema. Os aliases
  (`using Key = cengine::input::Key;`) mantiveram a ergonomia: o asteroids
  compilou sem mudar uma linha de cena. Quem quiser falar com a porta direto tem
  `forgeui::keyboard()`.

  Consumo: incluir `$(CengineRoot)modules/input/include` e compilar
  `modules/input/src/Keyboard.cpp`.

### Novo na 0.2.0

- `ForgeLineUi` (`forgeline`): **batcher de LINHAS 2D** — `drawLine` e
  `drawPolyline` em pixels, cor por vertice, um draw call por lote. Mesma
  mecanica do batcher de sprites (vertex buffer dinamico com um trecho por frame
  in flight), mas **sem textura, sem sampler e sem SRT**: a cor vem no vertice.
  Ligado por `LineBatcherDesc::enabled` (default `false` — quem nao desenha
  linha nao paga pelo pipeline).

  Trazido pelo `asteroids`, que precisava desenhar corpos que GIRAM. A
  alternativa avaliada era rotacionar sprites; linha ganhou porque os arcades
  vetoriais (Asteroids, Lunar Lander, Tempest) sao desenhados a linha e porque
  **linha dispensa atlas**: nao ha arte para produzir, so geometria. Um poligono
  girado e uma lista de pontos girados.

  **Ele deixou de ser so de linhas em 0.13.0** — `drawTriangle` desenha uma face
  cheia no MESMO lote (mesmo buffer, mesmos shaders, mesmo vertice; o que muda e
  a topologia, e por isso e um segundo pipeline e nao um modulo novo). Trazido
  pelo `vigil` ao por malhas 3D na cena, e nao por acabamento: **wireframe e
  transparente**, entao trinta corpos sao trinta contornos e nenhum esconde o
  outro. Face cheia devolve *quem esta na frente de quem*. Sem depth buffer, a
  ordem de chamada continua sendo a profundidade; alternar linha e triangulo
  custa um draw call por troca.

  **Em 0.15.0 `drawTriangle` ganhou uma cor POR VERTICE.** O vertice do batcher
  sempre carregou a sua; a versao de uma cor so a repetia tres vezes. Expor isso
  deixa o chamador sombrear por vertice e o rasterizador interpolar -- e a
  diferenca entre mostrar a malha como um poliedro e mostrar a superficie que o
  autor modelou.

  **Em 0.14.0 o `Stats` passou a dizer o que foi PERDIDO** (`dropped`) e contra
  que teto (`vertices` / `vertexCapacity`). O estouro de lote sempre existiu e so
  aparecia num `LOGF` — o que, com malha, virou *corpo sumindo da tela sem erro
  visivel*. O teto vem daqui porque **nenhum consumidor pode saber qual e** sem
  copiar o `maxLines` que ele mesmo passou na montagem.

### Base (0.1.0)

Extraido da PoC do Space Invaders (task 01), com o vocabulario de jogo
convertido em configuracao:

- `TheForgeWindowManager`: o casco completo — The Forge como biblioteca atras
  do port `IWindowManager` da cengine (>= 0.5.0). Janela Win32 propria,
  renderer, fontes, swapchain com resize, input via WndProc e o par
  `update()`/`present()` envolvendo as fases do jogo. Configurado por
  `TheForgeWindowDesc` (nome, tamanho, fonte, atlas, cor de clear).
- `ForgeUi`: ponte de texto/input/hints para cenas The Forge. Fila de edges
  (`readKey`) + estado segurado generico por tecla (`isHeld`/`heldAxis` —
  cada jogo compoe seu esquema de controles).
- `ForgeSpriteUi`: sprite batcher 2D com atlas, tint e flush por lote.
  Configurado por `SpriteBatcherDesc` (atlas, capacidade); a tabela de
  regioes e do jogo. `atlasPath` nulo desliga o batcher (jogo so de texto).
- `Shaders/FSL`: shaders do batcher (`sprite.vert/frag.fsl` + `sprite.srt.h`)
  e `Shaders.list` com os rootsigs padrao.
- `Format.h`: utilitarios pequenos de formatacao usados por cenas (std puro).

## Estrutura e consumo

```
src/TheForgeCommon/          <- adicionar ao include path do jogo
  TheForgeWindowManager.h/.cpp   (depende de cengine + The Forge)
  ForgeUi.h/.cpp                 (depende de The Forge)
  ForgeSpriteUi.h/.cpp           (depende de The Forge)
  ForgeLineUi.h/.cpp             (depende de The Forge)
  Format.h                       (std puro)
  Shaders/FSL/                   (Shaders.list para o passo FSL do jogo)
```

As camadas 2D sao a ORDEM DE CHAMADA, atravessando as tres pontes: o
`forgeui::drawText` da flush nos lotes pendentes de sprites e linhas antes de
gravar texto. Entao, dentro do `draw()` da cena, "geometria primeiro, texto
depois" poe o HUD por cima do jogo.

O consumo segue a receita dos jogos (vcxproj MSBuild, layout de checkouts
irmaos na mesma pasta — `The-Forge`, `cengine`, este repo e o jogo):

1. incluir `src/TheForgeCommon` no include path (os includes internos sao
   relativos: `"ForgeUi.h"`, `"Shaders/FSL/sprite.srt.h"`);
2. compilar os quatro `.cpp` junto do jogo;
3. apontar o passo FSL para `src/TheForgeCommon/Shaders/FSL/Shaders.list`
   (o `#include` do `defaults.h` assume o layout de checkouts irmaos) — ou
   copiar/estender a lista se o jogo tiver shaders proprios;
4. o jogo continua dono de `PathStatement.txt`, `gpu.cfg`, fontes e atlas
   (`TheForgeWindowDesc` recebe os caminhos).

Nada deve ser promovido para ca apenas por parecer reutilizavel. O codigo deve
entrar quando existir um consumidor real alem do jogo original ou quando uma
nova PoC precisar da mesma ponte com pouca variacao.

> **Jogos estacionados:** `8puzzle` e `spaceinvaders` NAO migram para este
> repo — ficaram congelados como documentacao viva (decisao registrada no
> ADR 0003 da cengine). As copias deles sao a evidencia de duplicacao que
> justificou a extracao; o primeiro consumidor real e o `asteroids`.

## Principios

- Codigo deste repo pode depender do The Forge.
- Codigo deste repo nao deve conter regra de jogo.
- APIs devem receber configuracao do jogo, como atlas, regioes, cores e limites
  de lote, em vez de fixar nomes como `atlas.dds`.
- O ciclo de vida deve continuar explicito para casar com The Forge:
  `init/load/begin/flush/unload/exit`.
- Cenas dos jogos continuam sendo donas da intencao: o common so fornece a
  ponte de plataforma.

## Plano

As tarefas ficam em [`.ai/task/`](.ai/task/). Comece por
[`01-extract-forge-sprite-ui.md`](.ai/task/01-extract-forge-sprite-ui.md).
