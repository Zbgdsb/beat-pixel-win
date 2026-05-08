/**
 * @file Note.cpp
 * @brief Note基类与NormalNote派生类实现
 * @details v2.0: 使用SFML Sprite+Texture渲染音符
 *          已判定音符半透明渐隐，穿过判定线后自然消失
 */
#include "Note.h"
#include <cmath>

Note::Note(int track, long long judgeTime, int judgeY, double speed)
    : track(track), judgeY(judgeY), speed(speed),
      isJudged(false), judgeTime(judgeTime), judgedFrames(0) {
    y = judgeY - (int)(speed * 60.0 * 3.0);
}

bool Note::update(long long currentTime) {
    if (isJudged) return false;
    double timeDiff = (judgeTime - currentTime) / 1000.0;
    y = judgeY - (int)(speed * timeDiff * 60.0);
    return y > judgeY + 300;
}

Judgement Note::judge(long long pressTime) {
    if (isJudged) return NONE;
    isJudged = true;
    long long diff = std::abs(pressTime - judgeTime);
    judgeTime = pressTime; // 记录实际判定时间，用于渐隐计时
    if (diff <= 120) return PERFECT;   // 放宽到±120ms
    else if (diff <= 300) return GOOD;  // 放宽到±300ms
    else return MISS;
}

// ========== NormalNote ==========

NormalNote::NormalNote(int track, long long judgeTime, int judgeY, double speed, COLORREF color)
    : Note(track, judgeTime, judgeY, speed), color(color) {
}

/**
 * @brief 绘制音符
 * @details 未判定：全彩纹理。已判定：快速渐隐消失。
 */
void NormalNote::draw(int trackX, const sf::Texture& noteNormalTex) {
    if (y > 700) return; // 离开屏幕停止绘制

    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) return;

    const float W = 80.0f, H = 80.0f;
    const float TEX = 512.0f;
    float cx = trackX + 50.0f;
    float cy = (float)y;

    if (isJudged) {
        judgedFrames++;
        // 20帧（约330ms）内渐隐消失
        float fade = 1.0f - (float)judgedFrames / 20.0f;
        if (fade <= 0) return; // 已完全透明
        uint8_t alpha = (uint8_t)(220 * fade);

        sf::RectangleShape rect({W, H});
        rect.setPosition({cx - W/2, cy - H/2});
        rect.setFillColor(sf::Color(255, 255, 255, alpha));
        g_window->draw(rect);

        sf::Sprite sprite(noteNormalTex);
        sprite.setOrigin({TEX / 2.0f, TEX / 2.0f});
        sprite.setScale({W / TEX, H / TEX});
        sprite.setPosition({cx, cy});
        sprite.setColor(sf::Color(255, 255, 255, alpha));
        g_window->draw(sprite);
        return;
    }

    // 未判定：全彩绘制
    // 白色实心底板（确保可见）
    sf::RectangleShape rect({W, H});
    rect.setPosition({cx - W/2, cy - H/2});
    rect.setFillColor(sf::Color(255, 255, 255, 255));
    g_window->draw(rect);

    // 纹理叠加
    sf::Sprite sprite(noteNormalTex);
    sprite.setOrigin({TEX / 2.0f, TEX / 2.0f});
    sprite.setScale({W / TEX, H / TEX});
    sprite.setPosition({cx, cy});
    g_window->draw(sprite);
}
