/**
 * @file AudioManager.cpp
 * @brief 音频管理类实现
 * @details 通过正弦波合成程序化生成所有音频，无需外部文件依赖
 *          背景音乐：8小节C大调旋律循环播放
 *          按键音效：Perfect（清脆和弦）、Good（单音）、Miss（低沉短音）
 */
#include "AudioManager.h"

// 音频参数常量
static const int SAMPLE_RATE = 44100;
static const float BGM_VOLUME = 0.25f;
static const float SFX_VOLUME = 0.5f;

// SFML 3.0 通道映射（单声道）
static const std::vector<sf::SoundChannel> MONO_MAP = {sf::SoundChannel::Mono};

AudioManager::AudioManager() : bgmLoaded(false), sfxLoaded(false) {
}

/**
 * @brief 将PCM数据加载到SoundBuffer（SFML 3.0兼容）
 */
bool AudioManager::loadBuffer(sf::SoundBuffer& buffer, const std::vector<int16_t>& data) {
    return buffer.loadFromSamples(data.data(), data.size(), 1, SAMPLE_RATE, MONO_MAP);
}

/**
 * @brief 生成单音正弦波PCM数据
 */
std::vector<int16_t> AudioManager::generateTone(float freq, float duration,
                                                  int sampleRate, float volume) {
    int numSamples = (int)(sampleRate * duration);
    std::vector<int16_t> samples(numSamples);

    for (int i = 0; i < numSamples; i++) {
        float t = (float)i / sampleRate;
        // ADSR简易包络
        float envelope = 1.0f;
        float attackTime = 0.01f;
        float decayTime = 0.05f;
        float releaseTime = 0.05f;

        if (t < attackTime) {
            envelope = t / attackTime;
        } else if (t < attackTime + decayTime) {
            envelope = 1.0f - 0.3f * ((t - attackTime) / decayTime);
        } else if (t > duration - releaseTime) {
            envelope = 0.7f * ((duration - t) / releaseTime);
        } else {
            envelope = 0.7f;
        }

        float value = volume * envelope * std::sin(2.0f * M_PI * freq * t);
        samples[i] = (int16_t)(value * 32000.0f);
    }
    return samples;
}

/**
 * @brief 生成双音和弦PCM数据
 */
std::vector<int16_t> AudioManager::generateChord(float freq1, float freq2,
                                                   float duration, int sampleRate, float volume) {
    auto tone1 = generateTone(freq1, duration, sampleRate, volume * 0.6f);
    auto tone2 = generateTone(freq2, duration, sampleRate, volume * 0.4f);

    std::vector<int16_t> result(tone1.size());
    for (size_t i = 0; i < tone1.size(); i++) {
        int sum = tone1[i] + tone2[i];
        if (sum > 32767) sum = 32767;
        if (sum < -32768) sum = -32768;
        result[i] = (int16_t)sum;
    }
    return result;
}

/**
 * @brief 生成背景音乐
 * @details 8小节C大调旋律，BPM=120，简单明快适合音游节奏感
 */
bool AudioManager::generateBGM() {
    const float C4 = 261.63f, D4 = 293.66f, E4 = 329.63f;
    const float F4 = 349.23f, G4 = 392.00f, A4 = 440.00f;
    const float B4 = 493.88f, C5 = 523.25f;

    struct Note { float freq; float beats; };
    Note melody[] = {
        {C4, 1}, {E4, 1}, {G4, 1}, {C5, 1},
        {B4, 1}, {G4, 1}, {E4, 1}, {C4, 1},
        {G4, 0.5f}, {0, 0.5f}, {A4, 0.5f}, {0, 0.5f},
        {G4, 1}, {E4, 1},
        {F4, 0.5f}, {0, 0.5f}, {G4, 0.5f}, {0, 0.5f},
        {A4, 1}, {G4, 1},
        {C5, 1}, {B4, 0.5f}, {A4, 0.5f}, {G4, 1}, {A4, 1},
        {G4, 1}, {F4, 1}, {E4, 1}, {D4, 1},
        {C4, 1}, {E4, 1}, {G4, 1}, {A4, 1},
        {G4, 2}, {C4, 2},
    };

    std::vector<int16_t> bgmData;
    float beatDuration = 0.5f;

    for (const auto& note : melody) {
        if (note.freq > 0) {
            float dur = note.beats * beatDuration;
            auto tone = generateTone(note.freq, dur, SAMPLE_RATE, BGM_VOLUME);
            bgmData.insert(bgmData.end(), tone.begin(), tone.end());
        } else {
            int silentSamples = (int)(note.beats * beatDuration * SAMPLE_RATE);
            bgmData.insert(bgmData.end(), silentSamples, 0);
        }
    }

    if (!loadBuffer(bgmBuffer, bgmData)) {
        return false;
    }

    bgmSound = std::make_unique<sf::Sound>(bgmBuffer);
    bgmSound->setLooping(true);
    bgmLoaded = true;
    return true;
}

/**
 * @brief 生成所有按键音效
 */
bool AudioManager::generateSFX() {
    // Perfect音效：清脆和弦（C5+E5），0.15秒
    auto perfectData = generateChord(523.25f, 659.25f, 0.15f, SAMPLE_RATE, SFX_VOLUME);
    if (!loadBuffer(perfectBuffer, perfectData)) return false;
    perfectSound = std::make_unique<sf::Sound>(perfectBuffer);

    // Good音效：G4单音，0.12秒
    auto hitData = generateTone(392.0f, 0.12f, SAMPLE_RATE, SFX_VOLUME);
    if (!loadBuffer(hitBuffer, hitData)) return false;
    hitSound = std::make_unique<sf::Sound>(hitBuffer);

    // Miss音效：低频短促，0.08秒
    auto missData = generateTone(150.0f, 0.08f, SAMPLE_RATE, SFX_VOLUME * 0.6f);
    if (!loadBuffer(missBuffer, missData)) return false;
    missSound = std::make_unique<sf::Sound>(missBuffer);

    sfxLoaded = true;
    return true;
}

/**
 * @brief 初始化音频系统
 */
bool AudioManager::init() {
    generateBGM();
    generateSFX();
    return true;
}

void AudioManager::playBGM() {
    if (bgmLoaded && bgmSound) bgmSound->play();
}

void AudioManager::stopBGM() {
    if (bgmLoaded && bgmSound) bgmSound->stop();
}

void AudioManager::playHit(bool isPerfect) {
    if (isPerfect && perfectSound) perfectSound->play();
    else if (hitSound) hitSound->play();
}

void AudioManager::playMiss() {
    if (missSound) missSound->play();
}
