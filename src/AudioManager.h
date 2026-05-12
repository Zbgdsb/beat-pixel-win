/**
 * @file AudioManager.h
 * @brief 音频管理类定义
 * @details 负责背景音乐播放、按键音效播放，使用SFML Audio模块
 *          所有音频通过程序化生成，无需外部音频文件
 *          音效复用Sound对象，避免重复创建导致卡顿
 */
#pragma once
#ifdef _WIN32
#ifndef BEATPIXEL_USE_SFML
#include "graphics.h"
#else
// SFML编译时：提供Audio stub，跳过音频
#define NOMINMAX
#include <windows.h>
#include <io.h>
#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <cstring>
#include <cmath>
namespace sf {
    enum class SoundChannel { Mono, FrontLeft, FrontRight, FrontCenter, RearLeft, RearRight };
    class SoundBuffer {
    public:
        // SFML 3.0 signature: data, count, channelCount, sampleRate, channelMap
        bool loadFromSamples(const int16_t*, size_t, unsigned int, unsigned int, const std::vector<sf::SoundChannel>&) { return true; }
        bool loadFromFile(const std::string&) { return true; }
        int64_t getSampleCount() const { return 0; }
        unsigned int getSampleRate() const { return 44100; }
        unsigned int getChannelCount() const { return 1; }
    };
    class Sound {
    public:
        Sound() = default;
        Sound(const SoundBuffer&) {}
        void play() {}
        void pause() {}
        void stop() {}
        void setPlayingOffset(int64_t) {}
        void setVolume(float) {}
        void setLooping(bool) {}
        int getStatus() const { return 0; }
    };
    class Music {
    public:
        Music() = default;
        ~Music() = default;
        Music(const Music&) = delete;
        Music& operator=(const Music&) = delete;
        Music(Music&&) = default;
        Music& operator=(Music&&) = default;
        bool openFromFile(const std::string&) { return true; }
        void play() {}
        void pause() {}
        void stop() {}
        void setVolume(float) {}
        void setLooping(bool) {}
        void setPlayingOffset(int64_t) {}
        int getStatus() const { return 0; }
        int64_t getDuration() const { return 0; }
    };
    class InputSoundFile {
    public:
        bool openFromFile(const std::string&) { return true; }
        size_t read(int16_t* d, size_t max) { if(d) memset(d,0,max*sizeof(int16_t)); return max; }
        int64_t getSampleCount() const { return 0; }
        unsigned int getSampleRate() const { return 44100; }
        unsigned int getChannelCount() const { return 1; }
    };
}
#endif
#else
#include <SFML/Audio.hpp>
#endif
#include <cmath>
#include <vector>
#include <cstdint>
#include <memory>

/**
 * @brief 音频管理器
 * @details 管理游戏中的所有音频：背景音乐和按键音效
 *          音频数据通过正弦波合成程序化生成
 */
class AudioManager {
private:
    // 背景音乐
    sf::SoundBuffer bgmBuffer;
    std::unique_ptr<sf::Sound> bgmSound;
    sf::Music originalMusic;        // V3.3: 原曲播放
    bool usingOriginalMusic = false; // 是否在播放原曲

    // 按键音效 - 复用Sound对象，避免每次按键重新创建
    sf::SoundBuffer hitBuffer;
    sf::SoundBuffer perfectBuffer;
    sf::SoundBuffer missBuffer;
    std::unique_ptr<sf::Sound> hitSound;
    std::unique_ptr<sf::Sound> perfectSound;
    std::unique_ptr<sf::Sound> missSound;

    bool bgmLoaded;
    bool sfxLoaded;

    float m_musicVolume = 1.0f;   // 背景音乐音量
    float m_effectVolume = 1.0f;  // 音效音量

    static std::vector<int16_t> generateTone(float freq, float duration,
                                              int sampleRate, float volume);
    static std::vector<int16_t> generateChord(float freq1, float freq2,
                                               float duration, int sampleRate, float volume);
    // V3.5: 打击乐音效
    static std::vector<int16_t> generateKick(float duration, int sampleRate, float volume);
    static std::vector<int16_t> generateSnare(float duration, int sampleRate, float volume);
    static std::vector<int16_t> generateHihat(float duration, int sampleRate, float volume);
    static std::vector<int16_t> generateTom(float freq, float duration, int sampleRate, float volume);
    int soundPack = 0; // 0=叮咚 1=打击乐
    std::string m_resourceDir; // 资源目录（exeDir）
    // V3.5: 4轨道独立鼓声音效
    sf::SoundBuffer trackBuffers[6];
    std::unique_ptr<sf::Sound> trackSounds[6];
    // V3.6: 通鼓循环系统（高中低3个通鼓共用J键，算法自动轮换）
    sf::SoundBuffer tomBuffers[3];
    std::unique_ptr<sf::Sound> tomSounds[3];
    int tomCycleIndex = 0; // 通鼓轮换索引 0=高 1=中 2=低
    long long lastTomHitTime = 0;  // 上次tom时间，>500ms重置轮换
    // V3.6: Ride镲
    sf::SoundBuffer rideBuffer;
    std::unique_ptr<sf::Sound> rideSound;
    bool loadBuffer(sf::SoundBuffer& buffer, const std::vector<int16_t>& data);
    bool generateBGM();
    bool generateSFX();
    // 尝试从文件加载打击乐采样
    bool loadPercussionFromFile(sf::SoundBuffer& buf, std::unique_ptr<sf::Sound>& snd,
                                 const char* filename, float volume);

public:
    AudioManager();
    ~AudioManager() = default;

    bool init();
    void setResourceDir(const std::string& dir) { m_resourceDir = dir; }
    void playBGM();
    void pauseBGM();  // 暂停BGM
    void resumeBGM(); // 恢复BGM
    void stopBGM();

    // 设置音量（0.0~1.0）
    void setMusicVolume(float volume) {
        m_musicVolume = std::max(0.0f, std::min(1.0f, volume));
        if (bgmSound) bgmSound->setVolume(m_musicVolume * 100.0f);
        if (usingOriginalMusic) originalMusic.setVolume(m_musicVolume * 100.0f);
    }

    void setEffectVolume(float volume) {
        m_effectVolume = std::max(0.0f, std::min(1.0f, volume));
        if (hitSound) hitSound->setVolume(m_effectVolume * 100.0f);
        if (perfectSound) perfectSound->setVolume(m_effectVolume * 100.0f);
        if (missSound) missSound->setVolume(m_effectVolume * 100.0f);
        for (int i = 0; i < 6; i++)
            if (trackSounds[i]) trackSounds[i]->setVolume(m_effectVolume * 100.0f);
        for (int i = 0; i < 3; i++)
            if (tomSounds[i]) tomSounds[i]->setVolume(m_effectVolume * 100.0f);
        if (rideSound) rideSound->setVolume(m_effectVolume * 100.0f);
    }

    float getMusicVolume() const { return m_musicVolume; }
    float getEffectVolume() const { return m_effectVolume; }

    // V3.5: 音效包
    void setSoundPack(int pack) { soundPack = pack; generateSFX(); }
    int getSoundPack() const { return soundPack; }

    /**
     * @brief 播放按键音效
     * @details 复用Sound对象，先stop再play，避免重复触发导致卡顿
     */
    void playHit(bool isPerfect, int track = -1);
    void playMiss();
    // V3.6: 通鼓自动轮换播放（高中低）
    void playTom();
    // V3.6: Ride镲播放
    void playRide();

    /**
     * @brief 生成与谱面节奏同步的BGM
     * @details 根据音符时间点和轨道生成对应的旋律音高，确保BGM和谱面节奏一致
     */
    bool generateSyncedBGM(const std::vector<std::pair<long long, int>>& noteData);

    /**
     * @brief V3.3: 播放原曲MP3
     */
    bool playOriginalSong(const std::string& filePath);
};
