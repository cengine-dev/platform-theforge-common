# 13 - A luz posicionavel

- **Status:** **done (0.21.0)** — 2026-09-06
- **Categoria:** Plataforma
- **Registrada em:** 2026-09-03
- **Depende de:** ~~task 12 (material)~~ — **a dependencia nao existia**, ver abaixo

## O problema

No Vigil a iluminacao virou **um rig ajustado a mao contra uma captura do
Blender** -- numeros escolhidos ate a imagem parecer certa, calculados na CPU
dentro do `ForgeMalha.h`. Nao ha luz no sentido de cena: nao ha posicao, nao ha
cor, nao ha intensidade, e nao ha como um segundo objeto reagir a ela.

## Escopo

A luz entra no constant buffer do quadro, junto da view-projection (task 11), e
o `mesh.frag.fsl` a le:

```cpp
namespace forgemesh {
struct DirectionalLight { float direction[3]; float color[3]; float intensity; };
struct Ambient          { float color[3];     float intensity; };

void setLight(const DirectionalLight&);
void setAmbient(const Ambient&);
}
```

Uma direcional mais uma ambiente. **Nao** um sistema de luzes -- direcional +
ambiente e o menor conjunto que responde a pergunta "de onde vem a luz?", e e
exatamente o que o Blender chama de Sun + World.

## Criterios de Aceite

1. Mover a luz muda o sombreado, e a face voltada para ela fica mais clara.
2. Os **mesmos numeros** de direcao, cor e intensidade do Sun do Blender,
   colados no lab, produzem um sombreado reconhecivelmente igual ao da captura.
3. Dois objetos na mesma cena reagem a mesma luz, sem cada um carregar o seu rig.
4. `intensity = 0` na direcional deixa so a ambiente, e nada fica preto puro.

## Por que e uma task separada da 12

Porque tem criterio de aceite proprio, e ele e o mais dificil de todos: **o
unico jeito de saber se a luz esta certa e olhar duas imagens lado a lado.**
Misturada com material, a duvida "o que esta errado, o mapa ou a luz?" nao teria
resposta.

## Fechada em 2026-09-06

`setLight`, `setAmbient` e `setBaseColor` no `forgemesh`, tudo por QUADRO como o
`setCamera`. O `mesh.frag.fsl` faz Lambert mais ambiente.

### A dependencia da task 12 nao existia

Esta task dizia *"depende de: task 12 (material)"*, e o plano do `diorama` a
inverteu de proposito — luz no degrau 06, material no 07. **A inversao estava
certa:** luz precisa de uma normal em espaco de mundo e de uma cor base, e cor
base constante e exatamente o que a propria task 12 promete continuar
suportando (*"material sem mapa e um valor constante, nao um erro"*).

E ha um ganho que so a inversao da: com albedo CONSTANTE, o sombreado nao se
confunde com a cor do material. **A pergunta "o que esta errado, o mapa ou a
luz?" nao chega a existir** — que e o motivo pelo qual esta task foi escrita
separada da 12, no mesmo dia em que as duas foram registradas.

### O que a normal precisou

A `UniformeDaMalha` ganhou a matriz de **MODELO separada**: a normal tem de
chegar ao espaco de mundo, que e onde a luz vive. Sem ela, girar o objeto
giraria a iluminacao junto — o corpo ficaria com a mesma face clara para sempre,
e o sintoma pareceria *"a luz esta presa no objeto"*.

**Limitacao registrada:** o certo em geral e a inversa-transposta da modelo. Com
rotacao, translacao e escala UNIFORME — tudo o que este ecossistema desenha hoje
— as duas coincidem a menos de um fator que o `normalize` absorve. Escala
nao-uniforme torceria a normal, e ai a struct ganha um campo, e nao o shader um
remendo.

### O SENTIDO da direcao, e por que ele mora em dois lugares

| onde | convencao |
|---|---|
| `DirectionalLight::direction` (desc) | **para onde a luz VIAJA** — a do Blender, e a que os `.txt` gravam |
| `luzDirecao.xyz` (shader) | **de onde a luz VEM** — o sinal que o `dot(N,L)` espera |

A negacao acontece **uma vez por desenho**, dentro do `draw`, e nao por pixel.
Quem chama nunca ve a segunda convencao.

Trocar o sentido nao da erro nem tela preta: **ilumina exatamente as faces
erradas**, e a imagem continua parecendo uma imagem iluminada. Por isso as duas
estao escritas no ponto de uso, e a metade que o lab faz (conversao de eixos)
tem teste.

### O que NAO bate com o Blender, e fica escrito

Isto e **Lambert**; o Eevee usa Principled BSDF com divisao por pi, mapeamento de
tom e mais. Os valores ABSOLUTOS nao coincidem. O criterio e a direcao e o
contraste RELATIVO entre as faces — qual esta clara, qual escura, e quanto —,
que e o que "paridade de FORMA" quer dizer.

### Um detalhe de tipo que custou um build

No lado C++, o `DATA(float4, ...)` do FSL expande para o **`float4` POD** do
ModifiedSonyMath, e nao para `vec4`. So `float4x4` e remapeado (para `mat4`,
`fsl_srt.h:34`). Os dois tipos existem, nao se convertem, e o erro so aparece na
atribuicao. Esta anotado no ponto de uso.
