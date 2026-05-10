/**
 * @file AudioManager.h
 * @brief 音频管理类定义
 * @details 负责背景音乐播放、按键音效播放，使用SFML Audio模块
 *          所有音频通过程序化生成，无需外部音频文件
 *          音效复用Sound对象，避免重复创建导致卡顿
 */
#pragma once
#ifdef _WIN32
#include "graphics.h"
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
    bool loadBuffer(sf::SoundBuffer& buffer, const std::vector<int16_t>& data);
    bool generateBGM();
    bool generateSFX();

public:
    AudioManager();
    ~AudioManager() = default;

    bool init();
    void playBGM();
    void pauseBGM();  // 暂停BGM
    void resumeBGM(); // 恢复BGM
    void stopBGM();

    // 设置音量（0.0~1.0）
    void setMusicVolume(float volume) {
        m_musicVolume = std::max(0.0f, std::min(1.0f, volume));
        if (bgmSound) bgmSound->setVolume(m_musicVolume * 100.0f);
    }

    void setEffectVolume(float volume) {
        m_effectVolume = std::max(0.0f, std::min(1.0f, volume));
        if (hitSound) hitSound->setVolume(m_effectVolume * 100.0f);
        if (perfectSound) perfectSound->setVolume(m_effectVolume * 100.0f);
        if (missSound) missSound->setVolume(m_effectVolume * 100.0f);
    }

    float getMusicVolume() const { return m_musicVolume; }
    float getEffectVolume() const { return m_effectVolume; }

    /**
     * @brief 播放按键音效
     * @details 复用Sound对象，先stop再play，避免重复触发导致卡顿
     */
    void playHit(bool isPerfect);
    void playMiss();

    /**
     * @brief 生成与谱面节奏同步的BGM
     * @details 根据音符时间点和轨道生成对应的旋律音高，确保BGM和谱面节奏一致
     */
    bool generateSyncedBGM(const std::vector<std::pair<long long, int>>& noteData);
};
