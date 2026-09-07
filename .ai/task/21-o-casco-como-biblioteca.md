# 21 - o casco como BIBLIOTECA: instancia, canal de erro e versao

- **Status:** ABERTA — precisa de decisao do dono (e a maior reforma pendente do casco)
- **Categoria:** Divida de plataforma
- **Registrada em:** 2026-09-07, na revisao arquitetural (achados 3.1, 3.2 e a metade do 1.1 que sobrou)

## Os tres fatos, e eles sao o mesmo fato

### 1. O casco nao e um objeto

`TheForgeWindowManager` guarda tres campos (`m_desc`, `m_width`, `m_height`). O
estado de verdade — `pRenderer`, `pSwapChain`, `gHwnd`, `gCloseRequested`, o
quadro do `forgeframe`, os tres batchers — vive em globais de arquivo ou `inline`.

E o **construtor ja escreve nos globais** (`TheForgeWindowManager.cpp:344-350`:
`gDepthEnabled`, `gClearDepth`, `gClearColor`), antes de qualquer `init()`.

Consequencias que se leem no codigo, sem executar:

- dois managers no mesmo processo se atropelam, e nada avisa;
- o `cleanup()` nao zera `gCloseRequested` nem `gResizePending`: um segundo
  `start()` no mesmo processo sairia no primeiro quadro;
- `RegisterClassExW` falha na segunda janela (a classe ja existe) e o codigo
  responde com `exit(EXIT_FAILURE)`;
- nada disso e testavel sem GPU.

> A escolha original esta escrita e e defensavel: *"UMA janela por processo (o
> WndProc e estatico e o fontstash ja depende do global `gWindow`)"*. O problema
> nao e o singleton — e o singleton **nao declarado**, que se comporta como
> objeto na assinatura e como global no efeito.

### 2. O casco mata o processo

`exit(EXIT_FAILURE)` em seis lugares (`init`, `applyPendingResize`). O
`initGraphics()` ate devolve `bool` corretamente — e o chamador chama `exit()`
assim mesmo.

Isso pula todos os destrutores (inclusive o `exitMemAlloc` do `main`, cuja ordem
o `diorama` acabou de corrigir) e torna o casco impossivel de usar dentro de um
teste ou de uma ferramenta.

A justificativa esta escrita: *"IWindowManager::init() nao tem canal de erro;
numa falha de plataforma o processo loga e encerra — escopo dos jogos de
estudo"*. Ela e honesta, e e uma decisao da CENGINE (a porta sem canal de erro)
sendo paga aqui.

### 3. O casco nao tem versao

Unica dependencia do ecossistema sem nenhuma forma de pinagem: `CommonRoot =
$(GameRoot)..\platform-theforge-common\` nos 14 `.vcxproj`. A versao existe em
UMA linha do `README.md` (`## Conteudo (0.22.1)`), que nenhum build le.

A cengine acabou de ganhar o `VersionTest` para o caso analogo — e la havia duas
fontes que divergiram. Aqui ha uma fonte que ninguem consulta, o que e a mesma
familia de problema um passo antes.

## Por que os tres sao um so

Todos os tres sao a mesma ausencia: **o casco nao declara o que ele e.** Nao diz
que e um singleton, nao diz como falha, nao diz que versao tem. Cada consumidor
descobre por tentativa, e o que ele descobre nao esta escrito em lugar nenhum que
o build consulte.

E por isso que a task 16 (o `.props`) foi o primeiro passo certo, e por isso ela
nao bastou: ela fez o casco dizer QUAIS ARQUIVOS tem. Faltam as outras tres
perguntas.

> A correcao do achado 3.4 (o `ForgeAudio.cpp` ausente da `.props`) ja mostrou o
> limite: acrescentar a linha QUEBRAVA todo consumidor, porque aquela unidade
> exige um include da cengine que a `.props` se proibiu de fornecer. **A lista de
> arquivos do casco nao e plana** — e essa e a mesma descoberta, de novo.

## As saidas, e elas sao independentes

### Para o (1): declarar o singleton em vez de escondê-lo

Mais barato, e ja paga: mover a escrita dos globais do construtor para o
`init()`; zerar no `cleanup()` tudo o que e de sessao; e uma guarda de instancia
unica que transforme o segundo manager em erro ALTO.

Nao vira objeto — assume que e singleton, com todas as letras. **Nenhum
consumidor muda uma linha.**

A alternativa (um contexto por instancia) exigiria resolver o `gWindow` global do
fontstash, que e do The-Forge e esta fora da fronteira.

### Para o (2): a decisao mora na CENGINE, nao aqui

`IWindowManager::init()` ganhar canal de erro e mudanca da engine (task nova la),
e ela alcanca todo consumidor. As opcoes:

- `init()` devolver `bool` — muda a assinatura de uma interface pura, entao TODO
  implementador muda (breaking de verdade);
- o casco lancar `std::runtime_error` e o `main` decidir — nao muda assinatura
  nenhuma, e o `EngineManager` ja lanca em dois lugares. **Parece a saida certa,
  e ela nem precisa da engine.**

### Para o (3): a versao que o build le

Uma propriedade no `.props` (`TheForgeCommonVersion`), mais a conferencia contra
o `README.md` no mesmo espirito do `VersionTest` da cengine 0.17.0. Casa com a
task 30 da engine, que decide o mecanismo de pinagem do ecossistema — **esta task
nao deve escolher sozinha**.

## A pergunta que fica para o dono

> O (1) e o (2) podem entrar agora, sem esperar ninguem — os dois sao aditivos
> para os 14 consumidores. Entram?
>
> O (3) espera a task 30 da cengine, que decide o mecanismo. Confirma?

## Criterios de aceite

1. Construir dois `TheForgeWindowManager` no mesmo processo falha ALTO, e nao com
   comportamento estranho.
2. Uma falha de plataforma no `init()` chega ao `main` como excecao, e os
   destrutores rodam.
3. `cleanup()` deixa o casco no estado em que o encontrou: um segundo `init()` no
   mesmo processo funciona ou recusa — nao "abre e fecha sozinho".
4. A prova continua sendo por DIFF e por COMPILACAO (o link do `DioramaForge` nao
   fecha sem o The-Forge, que e projeto de terceiro).
