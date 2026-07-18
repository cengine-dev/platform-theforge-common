#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include <cengine/audio/Player.hpp>

// forgeaudio (task 05): o BACKEND XAudio2 da porta `cengine::audio::Player`,
// extraido das copias identicas do breakout e do mario-bros (conferidas linha a
// linha na revisao pos-cengine-0.9.0) e validado pelo zelda, o 3o consumidor.
//
// O corte da extracao — o mesmo do input (task 03), invertido: la a plataforma
// captura e a cena le; aqui a cena pede (`play(id)`) e a plataforma toca.
//
// - **Sobe (este modulo):** o mecanismo — COM, XAudio2 2.9, mastering voice,
//   pool de vozes round-robin (PCM 16-bit mono 44.1kHz), submit; e os helpers
//   de sintese (`synth`/`concat`) com que os jogos escrevem as receitas.
// - **Fica no jogo:** o enum `Sound` (o catalogo), as RECEITAS (frequencias/
//   duracoes — a identidade sonora) e a decisao de QUANDO tocar (a cena traduz
//   os Events do dominio em plays).
//
// Nota de carta (registrada na task): a task 06 do breakout vetou este repo
// como casa do audio ("o common e coisas do The-Forge, e o The-Forge nao tem
// audio"). A decisao do dono na extracao (2026-07-18) foi o contrario: este
// repo ja e na pratica o CASCO Windows dos jogos (janela, GPU, teclado da
// porta de input), e o backend de audio e o mesmo tipo de peca — backend
// Windows de uma porta da cengine.
//
// Este header NAO puxa Windows nem XAudio2 (pimpl): a ilha Win32 — e a briga
// `_WIN32_WINNT` The-Forge x xaudio2.h — fica confinada ao .cpp. Por isso ele
// tambem nao loga (nao conhece o ILog): DEVOLVE o erro, e quem loga e o
// composition root do jogo.

namespace forgeaudio {

/// Sintetiza um blip de onda QUADRADA (a voz do arcade): frequencia varrendo
/// de `fromHz` a `toHz`, com decaimento exponencial (ataque instantaneo +
/// queda rapida). E o tijolo das receitas dos jogos.
[[nodiscard]] std::vector<int16_t> synth(float fromHz, float toHz, double seconds, float volume, float decay);

/// Cola varios blips em sequencia (fanfarras, descidas de game over).
[[nodiscard]] std::vector<int16_t> concat(const std::vector<std::vector<int16_t>>& parts);

// PCM 16-bit mono. Um formato so para todas as vozes — e o que permite um pool
// unico de vozes reaproveitaveis.
class AudioPlayer final: public cengine::audio::Player
{
public:
    static constexpr uint32_t kSampleRate = 44100;

    /// Vozes simultaneas. Poucas de proposito: quando estouram, a mais antiga e
    /// reciclada — num jogo de arcade, um som novo importa mais que um som
    /// velho terminando.
    static constexpr size_t kVoiceCount = 8;

    AudioPlayer() = default;
    ~AudioPlayer() override;

    /// Abre o device com a TABELA de samples do jogo (indice = SoundId do
    /// catalogo dele; as receitas sao construidas com synth/concat). Falhar
    /// NAO e fatal: sem placa de som, o jogo roda mudo (retorna false, e todo
    /// `play()` vira no-op) — o "mudo e degradacao normal" do contrato da
    /// porta.
    bool init(std::vector<std::vector<int16_t>> samples);
    void shutdown();

    /// A porta: toca o som `id` da tabela. Id fora dela e ignorado. O acucar
    /// de enum da base mantem `play(Sound::Sword)` valendo.
    void play(cengine::audio::SoundId id) override;
    using cengine::audio::Player::play;

    /// HRESULT do ultimo erro (0 se nao houve). Quem loga e o composition root.
    [[nodiscard]] uint32_t lastError() const { return m_lastError; }

private:
    struct Impl;
    Impl* m_impl = nullptr; // pimpl: os headers do XAudio2 nao vazam para as cenas

    bool     m_ownsCom = false; // fomos nos que inicializamos o COM desta thread?
    uint32_t m_lastError = 0;

    std::vector<std::vector<int16_t>> m_samples;
};

} // namespace forgeaudio
