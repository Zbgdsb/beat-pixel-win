/**
 * @file AudioManager.cpp
 * @brief 音频管理类实现
 * @details 通过正弦波合成程序化生成所有音频，无需外部文件依赖
 *          BGM根据谱面音符时间点同步生成，确保节奏一致
 *          按键音效复用Sound对象，避免重复创建导致卡顿
 */
#include "AudioManager.h"

static const int SAMPLE_RATE = 44100;
static const float BGM_VOLUME = 0.20f;
static const float SFX_VOLUME = 0.5f;
static const std::vector<sf::SoundChannel> MONO_MAP = {sf::SoundChannel::Mono};

// 轨道对应音高（C大调和弦音，每个轨道不同音高增加层次感）
static const float TRACK_PITCHES[4] = {
    261.63f,  // C4 - 轨道0
    329.63f,  // E4 - 轨道1
    392.00f,  // G4 - 轨道2
    523.25f   // C5 - 轨道3
};

AudioManager::AudioManager() : bgmLoaded(false), sfxLoaded(false) {
}

bool AudioManager::loadBuffer(sf::SoundBuffer& buffer, const std::vector<int16_t>& data) {
    return buffer.loadFromSamples(data.data(), data.size(), 1, SAMPLE_RATE, MONO_MAP);
}

std::vector<int16_t> AudioManager::generateTone(float freq, float duration,
                                                  int sampleRate, float volume) {
    int numSamples = (int)(sampleRate * duration);
    std::vector<int16_t> samples(numSamples);
    for (int i = 0; i < numSamples; i++) {
        float t = (float)i / sampleRate;
        float envelope = 1.0f;
        float attackTime = 0.01f, decayTime = 0.05f, releaseTime = 0.05f;
        if (t < attackTime) envelope = t / attackTime;
        else if (t < attackTime + decayTime) envelope = 1.0f - 0.3f * ((t - attackTime) / decayTime);
        else if (t > duration - releaseTime) envelope = 0.7f * ((duration - t) / releaseTime);
        else envelope = 0.7f;
        float value = volume * envelope * std::sin(2.0f * M_PI * freq * t);
        samples[i] = (int16_t)(value * 32000.0f);
    }
    return samples;
}

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
 * @brief 生成与谱面节奏同步的BGM
 * @details 根据每个音符的时间点和轨道编号，在对应位置生成对应音高的旋律音
 *          这样BGM的节奏和音符下落完全同步
 * @param noteData 音符数据列表，每项为(时间戳ms, 轨道编号)
 */
bool AudioManager::generateSyncedBGM(const std::vector<std::pair<long long, int>>& noteData) {
    if (noteData.empty()) return false;

    // 计算总时长：最后一个音符时间 + 2秒余量
    long long lastNoteTime = noteData.back().first;
    float totalDuration = (lastNoteTime / 1000.0f) + 2.0f;
    int totalSamples = (int)(totalDuration * SAMPLE_RATE);

    // 初始化为静音
    std::vector<int16_t> bgmData(totalSamples, 0);

    // 每个音符在对应时间点写入一个短促的旋律音（0.15秒）
    float noteDuration = 0.15f;

    for (const auto& [timeMs, trackId] : noteData) {
        int track = (trackId >= 0 && trackId < 4) ? trackId : 0;
        float freq = TRACK_PITCHES[track];

        // 计算该音符在PCM数据中的起始位置
        int startSample = (int)((timeMs / 1000.0f) * SAMPLE_RATE);

        // 生成短促旋律音
        auto tone = generateTone(freq, noteDuration, SAMPLE_RATE, BGM_VOLUME);

        // 叠加到BGM数据（避免越界）
        for (size_t i = 0; i < tone.size() && (startSample + (int)i) < totalSamples; i++) {
            int idx = startSample + (int)i;
            int sum = bgmData[idx] + tone[i];
            if (sum > 32767) sum = 32767;
            if (sum < -32768) sum = -32768;
            bgmData[idx] = (int16_t)sum;
        }
    }

    // 在间隙填充低音和弦铺底（营造氛围感）
    float padFreq = 130.81f; // C3低音
    for (int i = 0; i < totalSamples; i++) {
        float t = (float)i / SAMPLE_RATE;
        float pad = BGM_VOLUME * 0.3f * std::sin(2.0f * M_PI * padFreq * t);
        int sum = bgmData[i] + (int16_t)(pad * 15000.0f);
        if (sum > 32767) sum = 32767;
        if (sum < -32768) sum = -32768;
        bgmData[i] = (int16_t)sum;
    }

    if (!loadBuffer(bgmBuffer, bgmData)) return false;
    bgmSound = std::make_unique<sf::Sound>(bgmBuffer);
    bgmSound->setLooping(true);
    bgmLoaded = true;
    return true;
}

bool AudioManager::generateBGM() {
    // 默认BGM（不使用同步模式时的后备方案）
    const float C4 = 261.63f, E4 = 329.63f, G4 = 392.00f, C5 = 523.25f;
    struct Note { float freq; float beats; };
    Note melody[] = {
        {C4, 1}, {E4, 1}, {G4, 1}, {C5, 1},
        {G4, 1}, {E4, 1}, {C4, 1}, {G4, 1},
        {E4, 1}, {G4, 1}, {C5, 1}, {G4, 1},
        {E4, 1}, {C4, 1}, {G4, 2},
    };
    std::vector<int16_t> bgmData;
    float beatDuration = 0.5f;
    for (const auto& note : melody) {
        float dur = note.beats * beatDuration;
        auto tone = generateTone(note.freq, dur, SAMPLE_RATE, BGM_VOLUME);
        bgmData.insert(bgmData.end(), tone.begin(), tone.end());
    }
    if (!loadBuffer(bgmBuffer, bgmData)) return false;
    bgmSound = std::make_unique<sf::Sound>(bgmBuffer);
    bgmSound->setLooping(true);
    bgmLoaded = true;
    return true;
}

bool AudioManager::generateSFX() {
    auto perfectData = generateChord(523.25f, 659.25f, 0.15f, SAMPLE_RATE, SFX_VOLUME);
    if (!loadBuffer(perfectBuffer, perfectData)) return false;
    perfectSound = std::make_unique<sf::Sound>(perfectBuffer);

    auto hitData = generateTone(392.0f, 0.12f, SAMPLE_RATE, SFX_VOLUME);
    if (!loadBuffer(hitBuffer, hitData)) return false;
    hitSound = std::make_unique<sf::Sound>(hitBuffer);

    auto missData = generateTone(150.0f, 0.08f, SAMPLE_RATE, SFX_VOLUME * 0.6f);
    if (!loadBuffer(missBuffer, missData)) return false;
    missSound = std::make_unique<sf::Sound>(missBuffer);

    sfxLoaded = true;
    return true;
}

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

/**
 * @brief 播放按键音效
 * @details 先stop再play，复用同一个Sound对象，避免重复创建导致卡顿
 */
void AudioManager::playHit(bool isPerfect) {
    if (isPerfect && perfectSound) {
        perfectSound->stop();
        perfectSound->play();
    } else if (hitSound) {
        hitSound->stop();
        hitSound->play();
    }
}

void AudioManager::playMiss() {
    if (missSound) {
        missSound->stop();
        missSound->play();
    }
}
