/**
 * @file TextureManager.h
 * @brief 纹理资源管理器 - 集中加载和管理所有游戏纹理
 * @details 使用SFML Texture加载像素素材，提供统一的纹理访问接口
 *          所有纹理在init()中一次性加载，运行时通过引用访问，避免重复IO
 */
#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <cstdio>

class TextureManager {
public:
    // ========== 音符纹理 ==========
    sf::Texture noteNormal;    // 普通音符 (note_normal.png)
    sf::Texture notePerfect;   // Perfect判定音符 (note_perfect.png)
    sf::Texture noteGood;      // Good判定音符 (note_good.png)
    sf::Texture noteMiss;      // Miss判定音符 (note_miss.png)

    // ========== 按键纹理 (A/S/D/F) ==========
    sf::Texture keyNormal[4];  // 按键正常状态
    sf::Texture keyPressed[4]; // 按键按下状态

    // ========== 判定文字纹理 ==========
    sf::Texture textPerfect;   // "PERFECT!"文字素材
    sf::Texture textGood;      // "GOOD"文字素材
    sf::Texture textMiss;      // "MISS"文字素材

    // ========== 场景纹理 ==========
    sf::Texture trackBg;       // 轨道背景 (track_background.png)
    sf::Texture judgeLine;     // 判定线 (judge_line.png)

    /**
     * @brief 加载所有纹理资源
     * @param assetPath 素材目录路径（末尾不带斜杠）
     * @return 全部加载成功返回true
     */
    bool loadAll(const std::string& assetPath) {
        bool ok = true;
        printf("[TextureManager] 尝试加载路径: %s\n", assetPath.c_str());
        fflush(stdout);
        // 音符
        ok &= load(noteNormal,   assetPath + "/note_normal.jpg");
        ok &= load(notePerfect,  assetPath + "/note_perfect.jpg");
        ok &= load(noteGood,     assetPath + "/note_good.jpg");
        ok &= load(noteMiss,     assetPath + "/note_miss.jpg");
        // 按键 A S D F
        const char* keyNames[4] = {"A", "S", "D", "F"};
        for (int i = 0; i < 4; i++) {
            ok &= load(keyNormal[i],  assetPath + "/key_" + keyNames[i] + "_normal.jpg");
            ok &= load(keyPressed[i], assetPath + "/key_" + keyNames[i] + "_pressed.jpg");
        }
        // 判定文字
        ok &= load(textPerfect, assetPath + "/text_perfect.jpg");
        ok &= load(textGood,    assetPath + "/text_good.jpg");
        ok &= load(textMiss,    assetPath + "/text_miss.jpg");
        // 场景
        ok &= load(trackBg,    assetPath + "/track_background.jpg");
        ok &= load(judgeLine,  assetPath + "/judge_line.jpg");

        if (ok) printf("[TextureManager] 全部 %d 张纹理加载成功\n", 19);
        return ok;
    }

private:
    bool load(sf::Texture& tex, const std::string& path) {
        if (!tex.loadFromFile(path)) {
            fprintf(stderr, "[TextureManager] 加载失败: %s\n", path.c_str());
            return false;
        }
        tex.setSmooth(false); // 像素风格，关闭抗锯齿
        return true;
    }
};
