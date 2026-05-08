/**
 * @file Animations.h
 * @brief 动画系统数据结构定义
 * @details 定义三种动态效果的数据结构，由GameWindow统一驱动更新和渲染
 *          所有动画使用sf::Clock计时，easeOut缓动函数
 */
#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>

// ========== easeOut缓动函数 ==========
// 输入: t ∈ [0, 1]，输出: 缓动后的进度值
inline float easeOutQuad(float t) {
    return t * (2.0f - t);
}

inline float easeOutCubic(float t) {
    return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
}

/**
 * @brief 音符打击消失动画
 * @details 音符判定成功后，从当前大小放大到1.5倍，透明度渐变到0，持续200ms
 */
struct HitAnim {
    float x, y;           // 动画中心坐标（屏幕绝对坐标）
    float duration = 0.2f; // 动画时长（秒）
    float elapsed = 0.0f;  // 已过时间
    sf::Texture* texture;  // 对应的音符纹理（判定等级纹理）
};

/**
 * @brief 判定文字弹出动画
 * @details 判定成功时在判定线位置弹出文字：1.0倍→1.2倍放大，透明度255→0，持续300ms
 */
struct TextPopupAnim {
    float x, y;           // 动画中心坐标
    float duration = 0.3f;
    float elapsed = 0.0f;
    sf::Texture* texture;  // 对应的文字纹理
};

/**
 * @brief Combo数字跳动动画
 * @details Combo增加时，数字从1.0倍放大到1.3倍再弹回，颜色从橙色渐变回白色
 */
struct ComboAnim {
    float duration = 0.35f;
    float elapsed = 0.0f;
};
