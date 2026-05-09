/**
 * @file NoteTrack.cpp
 * @brief NoteTrack类实现 - 单个轨道管理
 * @details v2.0: 使用像素素材纹理渲染轨道背景、判定线、按键按钮和音符
 *          支持按键按下状态切换和轨道发光效果
 */
#include "NoteTrack.h"
#include <algorithm>

/**
 * @brief 构造函数
 * @param trackId   轨道编号 0-3
 * @param x         轨道左上角X坐标
 * @param width     轨道宽度
 * @param judgeY    判定线Y坐标
 * @param noteSpeed 音符下落速度 (px/帧)
 * @param key       对应按键字符 (A/S/D/F)
 */
NoteTrack::NoteTrack(int trackId, int x, int width, int judgeY, double noteSpeed, char key)
    : trackId(trackId), x(x), width(width), judgeY(judgeY),
      noteSpeed(noteSpeed), key(key) {
}

/**
 * @brief 向轨道添加一个新音符
 * @param note 音符智能指针，所有权转移到轨道
 */
void NoteTrack::addNote(std::unique_ptr<Note> note) {
    notes.push_back(std::move(note));
}

/**
 * @brief 更新轨道上所有音符状态
 * @details 1. 更新每个音符位置
 *          2. 检测超出屏幕的未判定音符，标记为MISS并计入计分
 *          3. 移除已判定且超出屏幕的音符，释放内存
 *          4. 收集自动Miss音符信息用于播放动画
 * @param currentTime 当前时间戳(ms)
 * @param scoreSystem 计分系统引用，用于记录MISS判定
 * @return 自动Miss的音符信息列表
 */
std::vector<AutoMissInfo> NoteTrack::update(long long currentTime, ScoreSystem& scoreSystem) {
    std::vector<AutoMissInfo> autoMisses;

    // 调试
    for (int i = (int)notes.size() - 1; i >= 0; i--) {
        notes[i]->update(currentTime);

        if (!notes[i]->getIsJudged() && notes[i]->isPastJudgeLine()) {
            // 记录自动Miss信息（用于播放Miss动画）
            AutoMissInfo info;
            info.trackId = trackId;
            info.y = (float)notes[i]->getY();
            info.color = notes[i]->getColor();
            autoMisses.push_back(info);

            scoreSystem.addJudgement(MISS);
            notes[i]->markJudged(); // 标记为已判定，避免重复计分
        }

        // 移除已判定且完全离开屏幕的音符
        if (notes[i]->getIsJudged() && notes[i]->isOffScreen()) {
            notes.erase(notes.begin() + i);
        }
    }
    return autoMisses;
}

/**
 * @brief 绘制轨道（纹理版本）
 * @details 绘制顺序：轨道背景纹理 → 轨道发光叠加 → 判定线纹理 → 音符
 * @param tex 纹理管理器引用
 * @param glowAlpha 轨道发光透明度（0-255）
 */
void NoteTrack::draw(const TextureManager& tex, int glowAlpha, int pressGlowAlpha) {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) return;

    // 1. 先画深色底色，再叠加纹理背景
    sf::RectangleShape darkBg({(float)width, (float)getheight()});
    darkBg.setPosition({(float)x, 0.0f});
    darkBg.setFillColor(sf::Color(25, 25, 35, 255));
    g_window->draw(darkBg);

    // 叠加纹理背景
    sf::Sprite bgSprite(tex.trackBg);
    sf::Vector2u bgSize = tex.trackBg.getSize();
    float bgScaleX = (float)width / bgSize.x;
    float bgScaleY = (float)getheight() / bgSize.y;
    bgSprite.setScale({bgScaleX, bgScaleY});
    bgSprite.setPosition({(float)x, 0.0f});
    g_window->draw(bgSprite);

    // 2. 轨道发光效果（按键按下时叠加半透明白色发光层）
    if (glowAlpha > 0) {
        sf::RectangleShape glow({(float)width, (float)getheight()});
        glow.setPosition({(float)x, 0.0f});
        glow.setFillColor(sf::Color(255, 255, 255, (uint8_t)(glowAlpha * 0.15f)));
        g_window->draw(glow);
    }

    // V3.0: 按键瞬间发光边框（淡蓝色）
    if (pressGlowAlpha > 0) {
        // 左边框
        sf::RectangleShape leftEdge({3.0f, (float)getheight()});
        leftEdge.setPosition({(float)x, 0.0f});
        leftEdge.setFillColor(sf::Color(100, 200, 255, (uint8_t)pressGlowAlpha));
        g_window->draw(leftEdge);
        // 右边框
        sf::RectangleShape rightEdge({3.0f, (float)getheight()});
        rightEdge.setPosition({(float)(x + width - 3), 0.0f});
        rightEdge.setFillColor(sf::Color(100, 200, 255, (uint8_t)pressGlowAlpha));
        g_window->draw(rightEdge);
        // 内部淡蓝色叠加
        sf::RectangleShape innerGlow({(float)width, (float)getheight()});
        innerGlow.setPosition({(float)x, 0.0f});
        innerGlow.setFillColor(sf::Color(100, 200, 255, (uint8_t)(pressGlowAlpha * 0.12f)));
        g_window->draw(innerGlow);
    }

    // 3. 绘制判定线（纹理+后备实线双保险）
    // 先画一条白色实线作为后备
    sf::RectangleShape judgeLine({(float)width, 4.0f});
    judgeLine.setPosition({(float)x, (float)judgeY - 2.0f});
    judgeLine.setFillColor(sf::Color(255, 255, 255, 230));
    g_window->draw(judgeLine);
    // 叠加纹理
    sf::Sprite judgeSprite(tex.judgeLine);
    sf::Vector2u jlSize = tex.judgeLine.getSize();
    float jlScaleX = (float)width / jlSize.x;
    float jlScaleY = 32.0f / jlSize.y;
    judgeSprite.setScale({jlScaleX, jlScaleY});
    judgeSprite.setPosition({(float)x, (float)judgeY - 16.0f});
    g_window->draw(judgeSprite);
    // 判定区域高亮
    sf::RectangleShape judgeZone({(float)width, 60.0f});
    judgeZone.setPosition({(float)x, (float)judgeY - 30.0f});
    judgeZone.setFillColor(sf::Color(255, 255, 255, 15));
    g_window->draw(judgeZone);

    // 5. 判定线闪光效果（打击时触发，逐帧衰减）
    if (judgeLineFlash > 0.01f) {
        // 闪光强度衰减
        judgeLineFlash *= 0.85f;  // 每帧衰减15%，约10帧(160ms)降到可忽略
        if (judgeLineFlash < 0.01f) judgeLineFlash = 0.0f;

        // 绘制闪光层：判定线位置白色高亮
        uint8_t flashAlpha = (uint8_t)(180 * judgeLineFlash);
        sf::RectangleShape flashRect({(float)width, 8.0f});
        flashRect.setPosition({(float)x, (float)judgeY - 4.0f});
        flashRect.setFillColor(sf::Color(255, 255, 255, flashAlpha));
        g_window->draw(flashRect);

        // 上方扩散光晕
        uint8_t glowAlpha = (uint8_t)(60 * judgeLineFlash);
        sf::RectangleShape glowRect({(float)width, 40.0f});
        glowRect.setPosition({(float)x, (float)judgeY - 40.0f});
        glowRect.setFillColor(sf::Color(255, 255, 255, glowAlpha));
        g_window->draw(glowRect);
    }

    // 4. 绘制该轨道的所有音符
    for (auto& note : notes) {
        note->draw(x, tex.noteNormal);
    }
}

/**
 * @brief 绘制按键按钮
 * @details 在轨道判定线下方显示按键素材，按下时切换到pressed纹理
 *          按键尺寸80x80，在100px宽的轨道内居中显示
 * @param tex 纹理管理器引用
 * @param isPressed 按键是否按下
 */
void NoteTrack::drawKeyButton(const TextureManager& tex, bool isPressed, float scale) {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) return;

    // 先获取纹理引用
    const sf::Texture& keyTex = isPressed ? tex.keyPressed[trackId] : tex.keyNormal[trackId];

    const float SIZE = 80.0f;
    const float SRC_W = (float)keyTex.getSize().x;  // 使用实际纹理宽度
    const float SRC_H = (float)keyTex.getSize().y;  // 使用实际纹理高度
    float baseScale = SIZE / SRC_W;  // 按宽度缩放，保持原始比例
    float finalScale = baseScale * scale;  // 应用V3.0缩放参数
    float cx = x + width / 2.0f;
    float cy = judgeY + 50.0f + SIZE / 2.0f;

    // 直接绘制按键精灵，无底板矩形，透明背景
    sf::Sprite keySprite(keyTex);
    keySprite.setOrigin({SRC_W / 2.0f, SRC_H / 2.0f});
    keySprite.setScale({finalScale, finalScale});
    keySprite.setPosition({cx, cy});
    g_window->draw(keySprite);
}

/**
 * @brief 处理该轨道按键按下事件
 * @details 三层判定逻辑：
 *          1. ±300ms内有音符 -> 正常判定(Perfect/Good/Miss)
 *          2. ±500ms内有音符但不在判定窗口 -> 按早了/按晚了，扣分(MISS)
 *          3. 500ms内无音符 -> 乱按，扣分(MISS)
 *          防止玩家无脑同时按住所有键刷分
 * @param pressTime 按键按下的时间戳(ms)
 * @return 判定结果
 */
NoteTrack::JudgeResult NoteTrack::handlePress(long long pressTime) {
    JudgeResult result;
    Note* closestNote = nullptr;
    long long minDiff = LLONG_MAX;
    bool hasNearbyNote = false;

    for (auto& note : notes) {
        if (note->getIsJudged()) continue;
        long long diff = std::abs(pressTime - note->getJudgeTime());
        if (diff <= 500) hasNearbyNote = true;
        if (diff <= 300 && diff < minDiff) {
            minDiff = diff;
            closestNote = note.get();
        }
    }

    // 情况1：判定窗口内有音符，正常判定
    if (closestNote) {
        result.noteY = (float)closestNote->getY();
        result.color = closestNote->getColor();
        result.judgement = closestNote->judge(pressTime);
        // 打击成功时触发判定线闪光
        if (result.judgement == PERFECT || result.judgement == GOOD) {
            judgeLineFlash = 1.0f;
        }
        return result;
    }

    // 情况2：500ms内有音符但不在判定窗口 -> MISS
    if (hasNearbyNote) {
        Note* nearest = nullptr;
        long long nearestDiff = LLONG_MAX;
        for (auto& note : notes) {
            if (note->getIsJudged()) continue;
            long long diff = std::abs(pressTime - note->getJudgeTime());
            if (diff < nearestDiff) { nearestDiff = diff; nearest = note.get(); }
        }
        if (nearest) {
            result.noteY = (float)nearest->getY();
            result.color = nearest->getColor();
            nearest->markJudged();
        }
        result.judgement = MISS;
        return result;
    }

    // 情况3：500ms内无音符 -> 乱按
    result.noteY = (float)judgeY; // 使用判定线位置作为后备
    result.judgement = MISS;
    return result;
}
