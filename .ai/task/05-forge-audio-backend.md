# 05 - forgeaudio: backend XAudio2 da porta de audio da cengine

- **Status:** candidata (NAO implementar ainda — o gate de evidencias JA
  disparou, falta o consumidor de validacao: o proximo jogo com som)
- **Categoria:** Ponte de plataforma (Windows)
- **Registrada em:** 2026-07-17 (revisao pos-cengine-0.9.0)

## A candidata

A cengine 0.9.0 promoveu a PORTA de audio (`cengine::audio::Player`,
`play(id)`). O BACKEND ficou nos jogos — e esta duplicado: o `AudioPlayer` do
breakout e o do mario-bros sao IDENTICOS em todo o mecanismo (conferido linha a
linha em 2026-07-17):

- ilha Win32 pura (a briga `_WIN32_WINNT` The-Forge x xaudio2.h), pimpl;
- `CoInitializeEx` + XAudio2 2.9 + mastering voice + pool de 8 vozes
  round-robin, PCM 16-bit mono 44.1kHz;
- sintese de onda quadrada com envelope exponencial (`square`/`synth`/`concat`);
- mudo = degradacao normal (devolve HRESULT, quem loga e o composition root);
- shutdown na ordem certa (vozes -> master -> engine -> COM).

So diferem: o enum `Sound` e as RECEITAS (frequencias/duracoes) — que sao a
identidade sonora de cada jogo e ficam nos jogos.

E o discriminador do input de novo, agora no nivel da plataforma: duas copias
identicas do MESMO mecanismo. A copia do mario foi deliberada (media o gate da
task 24 da cengine) — e mediu este tambem.

## A tensao de carta (registrar, nao esconder)

A task 06 do breakout VETOU este repo como casa do audio: "o common e coisas do
The-Forge, e o The-Forge nao tem modulo de audio". O argumento contrario, hoje:
este repo ja e na pratica o CASCO Windows dos jogos (janela, GPU, captura de
teclado da porta de input) — e o backend de audio e exatamente o mesmo tipo de
peca (backend Windows de uma porta da cengine, como o teclado na task 03).
Decisao do dono na hora de extrair: `forgeaudio` aqui, ou um repo proprio.

## O corte (quando extrair)

- **Sobe:** a classe backend implementando `cengine::audio::Player` (COM, pool,
  submit) + os helpers de sintese (`synth`/`concat`), parametrizada pela TABELA
  de samples.
- **Fica no jogo:** o enum `Sound`, as receitas, e a decisao de QUANDO tocar
  (a cena traduz `Events` em plays).

## Gate para comecar

O proximo jogo com som (seria a 3a copia). Em vez de copiar de novo, extrai —
e o jogo novo e o consumidor de validacao, com breakout e mario ficando como
estao (padrao do ecossistema: jogo antigo nao migra, o novo valida).
