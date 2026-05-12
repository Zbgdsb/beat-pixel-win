#include <chrono>
/**
 * @file AudioManager.cpp
 * @brief 音频管理类实现
 * @details 通过正弦波合成程序化生成所有音频，无需外部文件依赖
 *          BGM根据谱面音符时间点同步生成，确保节奏一致
 *          按键音效复用Sound对象，避免重复创建导致卡顿
 */
#include "AudioManager.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#include <windows.h>

// 优先用exe同目录下的ffmpeg.exe，没有则回退到系统PATH
static std::string resolveFfmpegPath(const std::string& resourceDir) {
    std::string bundled = resourceDir + "/ffmpeg.exe";
    FILE* fp = fopen(bundled.c_str(), "rb");
    if (fp) { fclose(fp); return bundled; }
    return "ffmpeg";
}
#endif

static const int SAMPLE_RATE = 44100;
static const float BGM_VOLUME = 0.20f;
static const float SFX_VOLUME = 0.5f;

// 轨道对应音高（C大调和弦音，每个轨道不同音高增加层次感）
static const float TRACK_PITCHES[6] = {
    261.63f,  // C4 - 轨道0
    329.63f,  // E4 - 轨道1
    392.00f,  // G4 - 轨道2
    523.25f,  // C5 - 轨道3
    440.00f,  // A4 - 轨道4 (Tom)
    587.33f   // D5 - 轨道5 (Ride)
};

AudioManager::AudioManager() : bgmLoaded(false), sfxLoaded(false) {
}

bool AudioManager::loadBuffer(sf::SoundBuffer& buffer, const std::vector<int16_t>& data) {
    std::vector<sf::SoundChannel> channelMap = {sf::SoundChannel::Mono};
    return buffer.loadFromSamples(data.data(), data.size(), 1, SAMPLE_RATE, channelMap);
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

// V3.5: 打击乐音效生成
std::vector<int16_t> AudioManager::generateKick(float duration, int sampleRate, float volume) {
    int numSamples = (int)(sampleRate * duration);
    std::vector<int16_t> samples(numSamples);
    for (int i = 0; i < numSamples; i++) {
        float t = (float)i / sampleRate;
        float envelope = std::exp(-t * 18.0f);
        // 频率从80Hz快速降到40Hz，典型的bass drum下扫
        float freq = 80.0f * std::exp(-t * 8.0f) + 40.0f;
        float value = volume * envelope * std::sin(2.0f * M_PI * freq * t);
        samples[i] = (int16_t)(value * 32000.0f);
    }
    return samples;
}

std::vector<int16_t> AudioManager::generateSnare(float duration, int sampleRate, float volume) {
    int numSamples = (int)(sampleRate * duration);
    std::vector<int16_t> samples(numSamples);
    unsigned int seed = 12345;
    for (int i = 0; i < numSamples; i++) {
        float t = (float)i / sampleRate;
        float envelope = std::exp(-t * 20.0f);
        seed = seed * 1103515245 + 12345;
        float noise = ((seed >> 16) & 0x7FFF) / 32768.0f - 0.5f;
        // snare: 噪声为主 + 200Hz body tone
        float tone = std::sin(2.0f * M_PI * 200.0f * t) * 0.4f;
        float value = volume * envelope * (noise * 0.8f + tone);
        samples[i] = (int16_t)(value * 32000.0f);
    }
    return samples;
}

std::vector<int16_t> AudioManager::generateHihat(float duration, int sampleRate, float volume) {
    int numSamples = (int)(sampleRate * duration);
    std::vector<int16_t> samples(numSamples);
    unsigned int seed = 67890;
    for (int i = 0; i < numSamples; i++) {
        float t = (float)i / sampleRate;
        float envelope = std::exp(-t * 50.0f); // 极快衰减，金属质感
        seed = seed * 1103515245 + 12345;
        float noise = ((seed >> 16) & 0x7FFF) / 32768.0f - 0.5f;
        // 高通滤波效果：减去低频成分
        float prev = (i > 0) ? ((seed * 1103515245 + 12345 >> 16 & 0x7FFF) / 32768.0f - 0.5f) : noise;
        float hp = noise - prev * 0.3f;
        float value = volume * envelope * hp;
        samples[i] = (int16_t)(value * 32000.0f);
    }
    return samples;
}

std::vector<int16_t> AudioManager::generateTom(float freq, float duration, int sampleRate, float volume) {
    int numSamples = (int)(sampleRate * duration);
    std::vector<int16_t> samples(numSamples);
    for (int i = 0; i < numSamples; i++) {
        float t = (float)i / sampleRate;
        float envelope = std::exp(-t * 10.0f);
        // tom: 频率微降，有pitch bend效果
        float f = freq * (1.0f - 0.15f * t);
        float value = volume * envelope * std::sin(2.0f * M_PI * f * t);
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
        int track = (trackId >= 0 && trackId < 6) ? trackId : 0;
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

// V3.6: 从文件加载打击乐采样（支持多路径fallback）
bool AudioManager::loadPercussionFromFile(sf::SoundBuffer& buf, std::unique_ptr<sf::Sound>& snd,
                                           const char* filename, float volume) {
    std::vector<std::string> searchDirs = {
        "assets/sounds",
        "../assets/sounds",
    };
    if (!m_resourceDir.empty()) {
        searchDirs.push_back(m_resourceDir + "/assets/sounds");
        searchDirs.push_back(m_resourceDir + "/../assets/sounds");
    }
    for (const auto& dir : searchDirs) {
        std::string path = dir + "/" + filename;
        if (buf.loadFromFile(path)) {
            snd = std::make_unique<sf::Sound>(buf);
            snd->setVolume(m_effectVolume * volume * 100.0f);
            printf("[SFX] loaded %s from %s (%llu samples)\n", filename, path.c_str(),
                   (unsigned long long)buf.getSampleCount()); fflush(stdout);
            return true;
        }
    }
    return false;
}

// V3.6: 通鼓自动轮换播放（高→中→低循环，模拟真实鼓手手法）
void AudioManager::playTom() {
    long long now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    // Tom Fill 联动：间隔<500ms自动轮换(高→中→低→高)，否则重置为高Tom
    if (now - lastTomHitTime > 500) {
        tomCycleIndex = 0;  // 独立Tom，重置为高
    }
    lastTomHitTime = now;
    
    if (tomSounds[tomCycleIndex]) {
        tomSounds[tomCycleIndex]->stop();
        tomSounds[tomCycleIndex]->play();
    }
    tomCycleIndex = (tomCycleIndex + 1) % 3;
}

// V3.6: Ride镲播放
void AudioManager::playRide() {
    if (rideSound) {
        rideSound->stop();
        rideSound->play();
    }
}

bool AudioManager::generateSFX() {
    if (soundPack == 1) {
        // 打击乐音效包 - 6轨道真实采样
        // 轨道映射: 0=kick, 1=hihat, 2=hihat, 3=snare, 4=tom(占位), 5=ride(占位)
        struct SfxEntry { const char* filename; int track; float volume; };
        SfxEntry entries[] = {
            {"kick.wav",   0, 0.9f},
            {"hihat.wav",  1, 0.7f},
            {"crash.wav",  2, 0.85f},
            {"snare.wav",  3, 0.85f},
        };
        for (auto& e : entries) {
            if (!loadPercussionFromFile(trackBuffers[e.track], trackSounds[e.track],
                                         e.filename, e.volume)) {
                printf("[SFX] WARN: %s not found, using synthetic fallback\n", e.filename); fflush(stdout);
                std::vector<int16_t> data;
                switch (e.track) {
                    case 0: data = generateKick(0.15f, SAMPLE_RATE, SFX_VOLUME * e.volume); break;
                    case 1: case 2: data = generateHihat(0.06f, SAMPLE_RATE, SFX_VOLUME * e.volume); break;
                    case 3: data = generateSnare(0.12f, SAMPLE_RATE, SFX_VOLUME * e.volume); break;
                }
                if (!loadBuffer(trackBuffers[e.track], data)) return false;
                trackSounds[e.track] = std::make_unique<sf::Sound>(trackBuffers[e.track]);
                trackSounds[e.track]->setVolume(m_effectVolume * e.volume * 100.0f);
            }
        }

        // V3.6: 通鼓（3个）加载到tomBuffers
        const char* tomFiles[] = {"tom_hi.wav", "tom_mid.wav", "tom_lo.wav"};
        for (int i = 0; i < 3; i++) {
            if (!loadPercussionFromFile(tomBuffers[i], tomSounds[i], tomFiles[i], 0.8f)) {
                float freqs[] = {400.0f, 300.0f, 200.0f};
                auto data = generateTom(freqs[i], 0.15f, SAMPLE_RATE, SFX_VOLUME * 0.8f);
                if (!loadBuffer(tomBuffers[i], data)) return false;
                tomSounds[i] = std::make_unique<sf::Sound>(tomBuffers[i]);
                tomSounds[i]->setVolume(m_effectVolume * 0.8f * 100.0f);
            }
        }
        tomCycleIndex = 0;

        // V3.6: Ride镲
        if (!loadPercussionFromFile(rideBuffer, rideSound, "ride.wav", 0.75f)) {
            auto data = generateHihat(0.12f, SAMPLE_RATE, SFX_VOLUME * 0.75f);
            if (!loadBuffer(rideBuffer, data)) return false;
            rideSound = std::make_unique<sf::Sound>(rideBuffer);
            rideSound->setVolume(m_effectVolume * 0.75f * 100.0f);
        }

        // Perfect/Miss保留作为通用音效
        auto perfectData = generateSnare(0.10f, SAMPLE_RATE, SFX_VOLUME * 0.95f);
        if (!loadBuffer(perfectBuffer, perfectData)) return false;
        perfectSound = std::make_unique<sf::Sound>(perfectBuffer);
        perfectSound->setVolume(m_effectVolume * 100.0f);

        auto missData = generateHihat(0.05f, SAMPLE_RATE, SFX_VOLUME * 0.4f);
        if (!loadBuffer(missBuffer, missData)) return false;
        missSound = std::make_unique<sf::Sound>(missBuffer);
        missSound->setVolume(m_effectVolume * 100.0f);
    } else {
        // 默认叮咚音效
        auto perfectData = generateChord(523.25f, 659.25f, 0.15f, SAMPLE_RATE, SFX_VOLUME);
        if (!loadBuffer(perfectBuffer, perfectData)) return false;
        perfectSound = std::make_unique<sf::Sound>(perfectBuffer);

        auto hitData = generateTone(392.0f, 0.12f, SAMPLE_RATE, SFX_VOLUME);
        if (!loadBuffer(hitBuffer, hitData)) return false;
        hitSound = std::make_unique<sf::Sound>(hitBuffer);

        auto missData = generateTone(150.0f, 0.08f, SAMPLE_RATE, SFX_VOLUME * 0.6f);
        if (!loadBuffer(missBuffer, missData)) return false;
        missSound = std::make_unique<sf::Sound>(missBuffer);
    }
    sfxLoaded = true;
    return true;
}

bool AudioManager::init() {
    generateBGM();
    generateSFX();
    return true;
}

void AudioManager::playBGM() {
    if (usingOriginalMusic) {
        originalMusic.play();
    } else if (bgmLoaded && bgmSound) {
        bgmSound->play();
    }
}

void AudioManager::stopBGM() {
    if (usingOriginalMusic) {
        originalMusic.stop();
        usingOriginalMusic = false;
    }
    if (bgmLoaded && bgmSound) bgmSound->stop();
}

void AudioManager::pauseBGM() {
    if (usingOriginalMusic) {
        originalMusic.pause();
    } else if (bgmLoaded && bgmSound) {
        bgmSound->pause();
    }
}

void AudioManager::resumeBGM() {
    if (usingOriginalMusic) {
        originalMusic.play();
    } else if (bgmLoaded && bgmSound) {
        bgmSound->play();
    }
}

bool AudioManager::playOriginalSong(const std::string& filePath) {
    // 先尝试直接加载
    if (originalMusic.openFromFile(filePath)) {
        originalMusic.setVolume(m_musicVolume * 100.0f);
        originalMusic.setLooping(false);
        usingOriginalMusic = true;
        printf("[AudioManager] 原曲加载成功\n");
        fflush(stdout);
        return true;
    }

    // SFML不支持该格式，用ffmpeg转换
    printf("[AudioManager] 格式不支持，用ffmpeg转换...\n");
    fflush(stdout);

#ifdef _WIN32
    char tmpPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tmpPath);
    strcat(tmpPath, "beatpixel_bgm_XXXXXX.wav");
    _mktemp_s(tmpPath, strlen(tmpPath) + 1);
#else
    char tmpPath[] = "/tmp/beatpixel_bgm_XXXXXX.wav";
    int tmpFd = mkstemps(tmpPath, 4);
    if (tmpFd < 0) return false;
    close(tmpFd);
#endif

#ifdef _WIN32
    std::string ffp = resolveFfmpegPath(m_resourceDir);
    std::string cmd = ffp + " -y -i \"" + filePath + "\" -ar 44100 -ac 2 -f wav \"" + tmpPath + "\" >NUL 2>&1";
#else
    std::string cmd = "ffmpeg -y -i '" + filePath + "' -ar 44100 -ac 2 -f wav '" + tmpPath + "' 2>/dev/null";
#endif
    int ret = system(cmd.c_str());
    if (ret != 0) {
        remove(tmpPath);
        printf("[AudioManager] ffmpeg转换失败\n");
        fflush(stdout);
        return false;
    }

    bool ok = originalMusic.openFromFile(tmpPath);
    if (ok) {
        originalMusic.setVolume(m_musicVolume * 100.0f);
        originalMusic.setLooping(false);
        usingOriginalMusic = true;
        printf("[AudioManager] 原曲转换并加载成功\n");
    } else {
        printf("[AudioManager] 转换后仍无法加载\n");
    }
    fflush(stdout);
    // 不删除临时文件，因为Music是流式播放，需要文件存在
    return ok;
}

/**
 * @brief 播放按键音效
 * @details 先stop再play，复用同一个Sound对象，避免重复创建导致卡顿
 */
void AudioManager::playHit(bool isPerfect, int track) {
    if (soundPack == 1 && track >= 0 && track < 6) {
        // 打击乐模式：任何判定都用对应音效
        if (track == 4) {
            playTom();   // V3.6: 通鼓自动轮换
        } else if (track == 5) {
            playRide();  // V3.6: Ride镲
        } else if (trackSounds[track]) {
            trackSounds[track]->stop();
            trackSounds[track]->play();
        }
    } else if (isPerfect && perfectSound) {
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
