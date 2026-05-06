/**
 * @file AudioManager.h
 * @brief 音频管理类定义
 * @details 负责背景音乐播放、按键音效播放，使用SFML Audio模块
 *          所有音频通过程序化生成，无需外部音频文件
 */
#pragma once
#include <SFML/Audio.hpp>
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
    sf::SoundBuffer bgmBuffer;      // 背景音乐PCM数据
    std::unique_ptr<sf::Sound> bgmSound; // 背景音乐播放器

    // 按键音效
    sf::SoundBuffer hitBuffer;      // Good按键音效
    sf::SoundBuffer perfectBuffer;  // Perfect按键音效
    sf::SoundBuffer missBuffer;     // Miss按键音效
    std::unique_ptr<sf::Sound> hitSound;
    std::unique_ptr<sf::Sound> perfectSound;
    std::unique_ptr<sf::Sound> missSound;

    bool bgmLoaded;                 // 背景音乐是否加载成功
    bool sfxLoaded;                 // 音效是否加载成功

    // 内部工具方法
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

    /**
     * @brief 初始化音频系统
     * @details 生成背景音乐和所有音效，加载到SFML SoundBuffer
     * @return true初始化成功，false失败（游戏仍可运行，只是没声音）
     */
    bool init();

    void playBGM();
    void stopBGM();
    void playHit(bool isPerfect);
    void playMiss();
};
