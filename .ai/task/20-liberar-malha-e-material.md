# 20 - Liberar malha e material

- **Status:** todo
- **Categoria:** Plataforma / 3D — **item de spec descartado em silencio**
- **Registrada em:** 2026-09-07, na revisao do degrau 07 do `diorama`

## O que aconteceu

A spec da **task 11** listava, por escrito:

```cpp
[[nodiscard]] MeshHandle load(const char* binPath);
void unload(MeshHandle);
```

**O `unload` nunca foi implementado**, e eu nao sinalizei na hora. A task 12
repetiu o padrao: `loadMaterial` existe, `unloadMaterial` nao.

Hoje as duas so sao liberadas no `forgemesh::exit()`, no fim do processo.

## Por que passou despercebido

Porque o unico consumidor e um **lab de um objeto**. Carregar tudo no `onEnter` e
soltar no fim e exatamente o certo para ele, e o defeito nao tem como aparecer.

> **Nao ha consumidor errado aqui — ha uma promessa nao cumprida.** A diferenca
> importa: o problema nao e que falta funcionalidade, e que a spec dizia que
> existia e ninguem conferiu.

## Onde isso morde

Um jogo que carregue por FASE. Cada troca de nivel acumula `Geometry` e `Texture`
ate o fim do processo, e o sintoma e memoria de video subindo sem parar — sem
erro, sem log, e diagnosticado como "vazamento em algum lugar".

Nenhum consumidor faz isso hoje. **O ecossistema tem 14 jogos 2D que carregam
tudo na entrada**, e o lab tambem.

## Escopo

```cpp
void unloadMesh(Handle);
void unloadMaterial(MaterialHandle);
```

E as tres perguntas que o desenho precisa responder, e que sao a razao de isto
ser uma task e nao um remendo:

**1. O slot e reaproveitado?** Hoje `gMalhaCount`/`gMaterialCount` so crescem, e
o handle e o indice. Liberar sem reaproveitar e um `unload` que nao libera
handle; reaproveitar exige uma lista de livres e **invalida handles antigos em
silencio** se alguem guardou um.

**2. Quem garante que a GPU terminou?** Um `removeResource` de recurso ainda em
uso por um quadro em voo e corrupcao. O casco tem `frameCount` quadros em voo;
liberar exige `waitQueueIdle` ou uma fila de destruicao atrasada.

**3. O que acontece com um `draw` de handle liberado?** Hoje `h >= gMalhaCount`
devolve sem desenhar, calado. Com reaproveitamento, um handle liberado pode
apontar para OUTRA malha — e ai o `draw` desenha a coisa errada em vez de nao
desenhar nada. **Isso e pior do que o comportamento atual.**

## Criterios de Aceite

1. `unloadMesh`/`unloadMaterial` liberam de verdade (medivel: contador de
   recursos vivos, ou o proprio `Stats`).
2. Liberar durante o quadro nao corrompe: ou espera, ou adia.
3. Desenhar com handle liberado **nao desenha outra coisa** — e a propriedade que
   o reaproveitamento ameaca, e ela vale mais do que economizar slots.
4. O `diorama` nao muda: ele carrega no `onEnter` e nao libera.
