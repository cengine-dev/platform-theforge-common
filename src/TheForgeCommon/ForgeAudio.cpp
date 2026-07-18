#include "ForgeAudio.h"

#include <algorithm>
#include <cmath>
#include <utility>

// XAudio2 2.9: vem no Windows 10 SDK e nao exige COM inicializado (ao contrario
// da 2.7, que era um objeto COM de verdade). Sem redistribuivel, sem terceiros.
// NENHUM header do The-Forge entra aqui — de proposito. O Config.h dele fixa
// `_WIN32_WINNT` em **Windows 7**, e o `xaudio2.h` do SDK recusa qualquer alvo
// anterior ao Windows 8 (a versao antiga vivia no DirectX SDK). Misturar os dois
// da erro de compilacao ou, pior, um device que nao abre em silencio.
//
// Entao este arquivo e uma ilha Win32 pura: ele NAO loga (nao conhece o ILog) —
// ele DEVOLVE o erro, e quem loga e o composition root do jogo.
//
// (Mecanica extraida das copias identicas do breakout e do mario-bros — ver o
// header. As RECEITAS nao estao aqui: cada jogo monta a sua tabela.)
#define WIN32_LEAN_AND_MEAN

#include <objbase.h>
#include <windows.h>
#include <xaudio2.h>

namespace forgeaudio {

namespace {

constexpr float kTwoPi = 6.28318530718f;

/// Onda QUADRADA — a voz do arcade. Senoide soaria macia demais para um jogo
/// de 8 bits.
float square(const float phase)
{
    return std::fmod(phase, kTwoPi) < kTwoPi * 0.5f ? 1.0f : -1.0f;
}

} // namespace

std::vector<int16_t> synth(const float fromHz, const float toHz, const double seconds, const float volume,
                           const float decay)
{
    const auto count = static_cast<size_t>(AudioPlayer::kSampleRate * seconds);

    std::vector<int16_t> samples(count);
    float                phase = 0.0f;

    for (size_t i = 0; i < count; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(count); // 0..1
        const float hz = fromHz + (toHz - fromHz) * t;

        phase += kTwoPi * hz / static_cast<float>(AudioPlayer::kSampleRate);

        const float envelope = std::exp(-decay * t);
        const float value = square(phase) * envelope * volume;

        samples[i] = static_cast<int16_t>(std::clamp(value, -1.0f, 1.0f) * 32767.0f);
    }

    return samples;
}

std::vector<int16_t> concat(const std::vector<std::vector<int16_t>>& parts)
{
    std::vector<int16_t> all;
    for (const auto& part: parts)
    {
        all.insert(all.end(), part.begin(), part.end());
    }
    return all;
}

struct AudioPlayer::Impl
{
    IXAudio2*               engine = nullptr;
    IXAudio2MasteringVoice* master = nullptr;
    IXAudio2SourceVoice*    voices[kVoiceCount] = {};
    size_t                  nextVoice = 0;
};

AudioPlayer::~AudioPlayer() { shutdown(); }

bool AudioPlayer::init(std::vector<std::vector<int16_t>> samples)
{
    // A tabela vem PRONTA do jogo (receitas via synth/concat): o mecanismo nao
    // sabe o que e um "pulo" ou uma "espada" — so toca o indice pedido.
    m_samples = std::move(samples);

    auto* impl = new Impl();

    // O XAudio2 exige apartamento COM inicializado nesta thread. O The-Forge nao
    // o faz por nos, e o erro que isso produz e mudo (o device simplesmente nao
    // abre) — o breakout tropecou nisso primeiro (breakout task 06).
    // RPC_E_CHANGED_MODE = alguem ja inicializou com outro modelo, o que serve.
    const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    m_ownsCom = SUCCEEDED(com);

    if (const HRESULT hr = XAudio2Create(&impl->engine, 0, XAUDIO2_DEFAULT_PROCESSOR); FAILED(hr))
    {
        m_lastError = static_cast<uint32_t>(hr);
        delete impl;
        return false; // sem audio: o jogo roda mudo, e isso NAO e um erro fatal
    }

    if (const HRESULT hr = impl->engine->CreateMasteringVoice(&impl->master); FAILED(hr))
    {
        m_lastError = static_cast<uint32_t>(hr);
        impl->engine->Release();
        delete impl;
        return false;
    }

    WAVEFORMATEX format = {};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 1;
    format.nSamplesPerSec = kSampleRate;
    format.wBitsPerSample = 16;
    format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

    // Um pool de vozes com o MESMO formato: qualquer som pode tocar em qualquer
    // voz, e sons simultaneos (espada + dano) nao se atropelam.
    for (auto& voice: impl->voices)
    {
        if (FAILED(impl->engine->CreateSourceVoice(&voice, &format)))
        {
            voice = nullptr;
        }
    }

    m_impl = impl;
    return true;
}

void AudioPlayer::shutdown()
{
    if (!m_impl)
    {
        return;
    }

    for (auto& voice: m_impl->voices)
    {
        if (voice)
        {
            voice->Stop();
            voice->DestroyVoice();
            voice = nullptr;
        }
    }

    if (m_impl->master)
    {
        m_impl->master->DestroyVoice();
    }
    if (m_impl->engine)
    {
        m_impl->engine->Release();
    }

    delete m_impl;
    m_impl = nullptr;

    if (m_ownsCom)
    {
        CoUninitialize();
        m_ownsCom = false;
    }
}

void AudioPlayer::play(const cengine::audio::SoundId id)
{
    if (!m_impl || id >= m_samples.size())
    {
        return; // mudo (sem device) ou id fora da tabela: no-op seguro
    }

    const auto& samples = m_samples[id];
    if (samples.empty())
    {
        return;
    }

    // Round-robin: a proxima voz do pool. Se ela ainda estiver tocando, o
    // FlushSourceBuffers corta o som velho — num arcade, o som NOVO importa mais.
    IXAudio2SourceVoice* voice = m_impl->voices[m_impl->nextVoice];
    m_impl->nextVoice = (m_impl->nextVoice + 1) % kVoiceCount;

    if (!voice)
    {
        return;
    }

    voice->Stop();
    voice->FlushSourceBuffers();

    XAUDIO2_BUFFER buffer = {};
    buffer.AudioBytes = static_cast<UINT32>(samples.size() * sizeof(int16_t));
    buffer.pAudioData = reinterpret_cast<const BYTE*>(samples.data());
    buffer.Flags = XAUDIO2_END_OF_STREAM;

    // O buffer aponta para m_samples, que vive enquanto o AudioPlayer viver — o
    // XAudio2 le direto dali, sem copiar. Por isso o player NAO pode morrer antes
    // das vozes pararem (o shutdown() cuida disso, nesta ordem).
    if (SUCCEEDED(voice->SubmitSourceBuffer(&buffer)))
    {
        voice->Start();
    }
}

} // namespace forgeaudio
