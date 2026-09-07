# 16 - O casco ganha um ALVO DE BUILD

- **Status:** **done (0.20.0)** — 2026-09-06
- **Categoria:** Plataforma / Infra — **divida estrutural, exposta pela task 09**
- **Registrada em:** 2026-09-06, na revisao do degrau 05 do `diorama`

## O problema, e ele nao e sobre `.cpp`

**Este repo nao tem alvo de build.** Cada jogo enumera os `.cpp` do casco no
proprio `.vcxproj`:

```xml
<ClCompile Include="$(CommonRoot)src\TheForgeCommon\TheForgeWindowManager.cpp" />
<ClCompile Include="$(CommonRoot)src\TheForgeCommon\ForgeUi.cpp" />
...
```

Consequencia, medida na task 09: **um `.cpp` novo aqui nunca e aditivo.** Ele e
sempre uma edicao obrigatoria em 8+ projetos que ja existem, por mais aditivo
que o codigo seja. A 0.16.0 quebrou o link de todos eles ao acrescentar
`ForgeFrame.cpp`, e o diff nao acusou porque eu procurava simbolo REMOVIDO.

## A resposta que dei, e por que ela e remendo

Tornei o `ForgeFrame` e o `ForgeMesh` **so cabecalho**, com estado em variavel
`inline`, e transformei isso em regra do repo.

Funciona, e **trata o sintoma**. O custo ja apareceu duas vezes:

- o `ForgeMeshDesc.h` so existe porque header-only vazaria o
  `Common_3/Graphics/FSL/defaults.h` — e suas 20 macros (`CAT`, `VARNAME`,
  `DESCRIPTOR_TABLE`) — para todo consumidor 2D;
- o `ForgeMesh.h` ja tem ~400 linhas de implementacao num cabecalho, e a task 14
  (skinning com `ozz`) piora isso substancialmente.

> **A regra "nunca acrescente um `.cpp`" e uma regra sobre o build disfarcada de
> regra sobre codigo.** Enquanto ela valer, o desenho de cada modulo novo e
> decidido por uma limitacao de `.vcxproj`.

## Escopo

**Um `TheForgeCommon.props`** neste repo, que declara o que hoje cada jogo
repete:

```xml
<!-- src/TheForgeCommon/TheForgeCommon.props -->
<ItemGroup>
  <ClCompile Include="$(MSBuildThisFileDirectory)TheForgeWindowManager.cpp" />
  <ClCompile Include="$(MSBuildThisFileDirectory)ForgeUi.cpp" />
  ...
</ItemGroup>
<ItemDefinitionGroup>
  <ClCompile>
    <AdditionalIncludeDirectories>$(MSBuildThisFileDirectory);%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
  </ClCompile>
</ItemDefinitionGroup>
```

O consumidor troca a lista inteira por uma linha:

```xml
<Import Project="$(CommonRoot)src\TheForgeCommon\TheForgeCommon.props" />
```

## A ADOCAO e opt-in, e isso e o ponto

**Nenhum projeto de referencia e tocado.** As listas explicitas continuam
funcionando exatamente como hoje — o `.props` e uma segunda forma de dizer a
mesma coisa, nao uma substituicao forcada.

| projeto | quando adota |
|---|---|
| `diorama` | nesta task (e o unico que posso buildar) |
| os 14 jogos | quando alguem voltar a tocar em cada um, se voltar |

Isso preserva a fronteira do workspace (`vigil`, os jogos e o `creative-lab` sao
**referencia**: ler, citar, nunca buildar nem modificar) e ainda assim
desbloqueia o repo.

## O que isto libera

Depois desta task, **acrescentar um `.cpp` ao casco volta a ser aditivo** para
quem adotou o `.props`. Header-only deixa de ser obrigatorio e volta a ser uma
escolha — que ainda pode ser a certa para modulos pequenos como o `ForgeFrame`,
mas por conveniencia, e nao por imposicao.

## Criterios de Aceite

1. O `DioramaForge.vcxproj` importa o `.props` e nao lista mais `.cpp` do casco.
2. O build do `diorama` produz o mesmo executavel (mesmos simbolos linkados).
3. **Um `.cpp` novo no casco entra sem tocar no `.vcxproj` do diorama** — a prova
   e acrescentar um arquivo vazio e ver o build pega-lo.
4. Um projeto que NAO adotou (lista explicita) continua compilando — verificavel
   **lendo o diff**: nada foi removido de `src/TheForgeCommon/`.

## Fechada em 2026-09-06

`src/TheForgeCommon/TheForgeCommon.props`, e o `DioramaForge.vcxproj` importando
numa linha. O `.props` declara tres coisas e **nenhuma configuracao**:

| | |
|---|---|
| `ClCompile` | os quatro `.cpp` do casco |
| `FSLShader` | o `Shaders.list` do casco |
| `AdditionalIncludeDirectories` | a pasta dos cabecalhos |

**Ele e auto-localizavel** (`$(MSBuildThisFileDirectory)`), e nao depende de o
consumidor ter definido `$(CommonRoot)` — que, num `.vcxproj` tipico, so existe
DEPOIS dos property sheets. O `Import` precisa vir antes do
`ItemDefinitionGroup` do projeto, para que o `%(AdditionalIncludeDirectories)`
de la herde.

### O que NAO entrou, e por que

**Nada de configuracao.** Sem flags, sem defines, sem link. Este arquivo diz
QUAIS ARQUIVOS o casco tem, e so — misturar as duas coisas transformaria um
`.props` de fontes num `.props` com opiniao, e o consumidor perderia o controle
de como compila.

**Nada da cengine.** Outro repo, com modulos opt-in proprios; quem escolhe quais
linkar e o consumidor.

### Os criterios, medidos

1. **O `.vcxproj` nao lista mais `.cpp` do casco.** Os quatro compilam pelo
   `.props` (visiveis no log do build).
2. **O executavel e o mesmo:** `709.632 bytes`, identico ao da build anterior.
3. **Um `.cpp` novo entra sem tocar no `.vcxproj`.** Provado, e nao deduzido: um
   `_ProvaDaTask16.cpp` foi acrescentado ao casco e ao `.props`, o build o
   compilou, e o **hash do `.vcxproj` ficou inalterado**. Depois foi removido.
4. **Quem NAO adotou continua compilando** — verificavel no `git status`:
   nenhum arquivo foi removido de `src/TheForgeCommon/`. As listas explicitas
   dos 14 jogos seguem validas.

### O que isto libera

**Header-only deixa de ser imposicao e volta a ser escolha.** O `ForgeFrame.h` e
o `ForgeMesh.h` podem continuar como estao (para um modulo pequeno ainda e a
forma certa), mas a task 15 e as 12/13/14 nao precisam mais se contorcer para
caber num cabecalho.

E o `ForgeMeshDesc.h`, que existe so para o `defaults.h` do FSL nao vazar aos
consumidores 2D, deixa de ser necessario quando o `ForgeMesh` tiver um `.cpp` —
**mas isso e da task 15, e nao desta.**
