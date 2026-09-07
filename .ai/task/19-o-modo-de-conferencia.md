# 19 - O modo de conferencia nao devia ser um `if` por pixel

- **Status:** todo — **espera a task 15b** (variantes de pipeline)
- **Categoria:** Plataforma / 3D — divida da task 13
- **Registrada em:** 2026-09-06, na revisao do degrau 06 do `diorama`

## O que foi feito, e por que incomoda

A task 13 pos no casco:

```cpp
struct BaseColor
{
    float color[3] = { 0.75f, 0.75f, 0.75f };
    bool  usarAlbedo = true;   // false = a cor vira a NORMAL
};
```

e, no `mesh.frag.fsl`:

```
float3 base = lerp(n * 0.5f + 0.5f, gMalha.albedo.rgb, gMalha.albedo.a);
```

Duas coisas erradas de uma vez:

**1. E um instrumento de LABORATORIO na API da plataforma.** "Mostrar a normal
como cor" e uma tecnica de depuracao. O casco passou a ter opiniao sobre como se
depura, o que nao e o trabalho dele.

**2. E TRABALHO MORTO por pixel, e nao so um ramo.** O `lerp` avalia **os dois
lados**: a amostragem do normal map e toda a matematica de espaco tangente rodam
para todo fragmento, inclusive quando o resultado e descartado.

Ou seja, nao e "um `if` barato no caminho quente" -- e o caminho inteiro do
material sendo calculado e jogado fora, para escolher entre dois modos que **nao
mudam durante um quadro**. A resposta certa e conhecida: **variante de
pipeline**.

> Esta parte piorou com a task 12 (2026-09-07): antes o lado descartado era um
> `n * 0.5 + 0.5`; agora e duas amostragens de textura mais o Gram-Schmidt.

**3. A struct mistura duas coisas.** `BaseColor` tem uma cor e uma chave de
diagnostico. Sao conceitos de niveis diferentes no mesmo tipo.

## Por que ele foi feito assim mesmo assim, e por que fica

Porque ele **se paga**, e o motivo esta no metodo deste ecossistema: e o que
separa *"a malha esta certa?"* de *"a luz esta certa?"*.

O `diorama` ja pagou duas vezes por nao conseguir separar perguntas — os cubos
pretos do degrau 04 (normal invalida, descoberta pelo dono) e a seta ausente
(transformacao perdida, descoberta pelo dono). Um instrumento que isola a
geometria da iluminacao vale mais do que a limpeza da API.

**O precedente da casa nao cobre exatamente este caso.** O `forgeline::Stats` e o
`forgeui::lastFrameTextCalls()` sao diagnosticas de LEITURA — contadores. Esta
muda a SAIDA, que e outra categoria.

## Escopo (quando a task 15b chegar)

O modo de conferencia vira **variante de pipeline**, e nao campo de material:

```cpp
enum Variante : uint32_t { kSimples = 0, kNormalComoCor = 1 << 0, /* ... */ };
```

Com isso:

- some o `lerp` do caminho quente (dois shaders, cada um sem ramo);
- o `BaseColor` volta a ser so uma cor, e cede o lugar ao material da task 12;
- a escolha e feita onde deve: no pipeline, uma vez por quadro.

**Nao fazer antes da 15b.** A maquina de variantes e que da o lugar; sem ela,
mover isto seria trocar um `if` por pixel por um `if` na criacao do pipeline, que
e a mesma coisa com mais linhas.

## Criterios de Aceite

1. O `mesh.frag.fsl` do caminho normal nao tem ramo de modo de conferencia.
2. `BaseColor` (ou o que a task 12 puser no lugar) so descreve material.
3. O `diorama` continua com a tecla `N` funcionando, e a cena nao ganha
   complexidade por causa da mudanca.
