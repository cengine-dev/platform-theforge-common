# 23 - O `.props` diz o que o casco TEM, e nao o que ele EXIGE

- **Status:** todo
- **Categoria:** Plataforma / Infra — **buraco da task 16**
- **Registrada em:** 2026-09-13
- **Task irma:** `cengine/.ai/task/30-dependencia-partida.md`

## O buraco

O `TheForgeCommon.props` (task 16) declara tres coisas: os `.cpp` do casco, o
`Shaders.list` e o include path **do proprio casco**. E diz, com todas as letras:

> **Nada da cengine.** Ela e outro repo, com os modulos opt-in dela; quem escolhe
> quais linkar e o consumidor, e o casco nao tem opiniao sobre isso.

A intencao estava certa — o casco nao deve escolher modulos da cengine pelo
consumidor. **A execucao ficou pela metade**, e a medicao mostra por que:

```
ForgeUi.h               -> <cengine/input/Keyboard.hpp>, <cengine/input/Mouse.hpp>
ForgeAudio.h            -> <cengine/audio/Player.hpp>
TheForgeWindowManager.h -> <cengine/core/IWindowManager.hpp>
```

**Os cabecalhos PUBLICOS do casco incluem a cengine.** Entao um consumidor que
importe o `.props` e nada mais **nao compila**: os proprios arquivos do casco
nao acham os cabecalhos de que dependem.

Hoje ninguem tropeca porque todo `.vcxproj` do ecossistema ja supre o
`$(CengineRoot)...\include` a mao, herdado do molde original. **O `.props` esta
sendo carregado por uma linha que ele nao declara.**

## A distincao que faltou

Ha duas coisas diferentes, e a task 16 tratou as duas como uma:

| | quem decide |
|---|---|
| **quais modulos da cengine LINKAR** | o consumidor — e o `.props` esta certo em nao opinar |
| **de que cabecalhos o CASCO precisa para compilar** | o **casco**, porque sao os `#include` dele |

O segundo nao e opiniao sobre o consumidor: e a descricao de um requisito
proprio. Um `.props` que lista os arquivos de um modulo e omite as dependencias
deles descreve metade do modulo.

> **A regra da task 16 continua valendo, e e ela que corrige a task 16:** *quem
> tem os arquivos e quem os descreve*. Os `#include` sao do casco.

## Escopo

O `.props` passa a declarar o que o casco **exige**, e nao so o que ele tem:

```xml
<!-- O casco inclui <cengine/...> nos cabecalhos PUBLICOS. Isto nao escolhe
     modulos pelo consumidor -- diz de que include path os arquivos DESTE repo
     precisam para compilar. -->
<ItemDefinitionGroup>
  <ClCompile>
    <AdditionalIncludeDirectories>$(CengineDir)core\include;$(CengineDir)modules\input\include;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
  </ClCompile>
</ItemDefinitionGroup>
```

`core` e `input` sao **incondicionais** (o `TheForgeWindowManager.h` e o
`ForgeUi.h` os incluem sempre). O `audio` entra no mesmo `ItemGroup` opt-in que
ja liga o `ForgeAudio.cpp`.

### De onde vem o `$(CengineDir)`

Duas saidas, e a escolha depende da **task 30 da cengine**:

- se a cengine ganhar um `cengine.props`, ele define `$(CengineDir)` e este
  arquivo so o importa;
- ate la, o `.props` do casco **falha alto** quando a propriedade nao existe, em
  vez de emitir um caminho vazio e deixar o erro aparecer como "cabecalho nao
  encontrado" trinta linhas depois:

```xml
<Target Name="TheForgeCommonConfere" BeforeTargets="ClCompile">
  <Error Condition="'$(CengineDir)' == ''"
         Text="TheForgeCommon.props: defina CengineDir. O casco inclui &lt;cengine/...&gt; nos cabecalhos publicos." />
</Target>
```

## Por que isto e uma task e nao um remendo

Porque ha uma pergunta de desenho embaixo: **o casco deveria depender da cengine
nos cabecalhos PUBLICOS?**

Hoje depende, e com razao — `TheForgeWindowManager` implementa
`cengine::core::IWindowManager`, que e o ponto inteiro do casco. Mas isso
significa que **nao existe consumidor do casco que nao seja consumidor da
cengine**, e o `.props` finge que existe.

Declarar a exigencia torna isso visivel. Escondê-la mantem a ficcao.

## Criterios de Aceite

1. Um `.vcxproj` que importe **so** o `.props` (sem include de cengine proprio)
   compila os arquivos do casco.
2. Esquecer o `CengineDir` da um erro que **diz o que fazer**, e nao um
   "cabecalho nao encontrado".
3. O `DioramaForge.vcxproj` deixa de listar `$(CengineRoot)core\include` e
   `modules\input\include` a mao — os dois passam a vir do `.props`.
4. Os modulos que o consumidor escolhe (`camera3d`, `routing`...) continuam
   sendo dele. O casco declara o que o CASCO precisa, e nada mais.
