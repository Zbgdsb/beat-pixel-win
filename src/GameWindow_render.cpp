/**
 * @file GameWindow_render.cpp
 * @brief GameWindow渲染系统 - V2.0
 * @details 包含：菜单渲染、游戏渲染、UI绘制、结算界面、动画绘制
 */

#include "GameWindow.h"
#include <cstdio>
#include <algorithm>
#include <cmath>

// ========== 主菜单渲染（V2.0新增） ==========

void GameWindow::drawMenuScreen() {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) return;

    // 渐变背景（从深蓝到深紫）
    const int bands = 16;
    for (int i = 0; i < bands; i++) {
        float t = (float)i / bands;
        int r = (int)(8 + 12 * t);
        int g = (int)(8 + 4 * t);
        int b = (int)(20 + 15 * t);
        int y1 = (int)(height * t);
        int y2 = (int)(height * (t + 1.0f / bands));
        sf::RectangleShape band({(float)width, (float)(y2 - y1)});
        band.setPosition({0.0f, (float)y1});
        band.setFillColor(sf::Color(r, g, b));
        g_window->draw(band);
    }

    // 标题呼吸灯效果
    float breathe = 0.7f + 0.3f * std::sin((float)GetTickCount64() / 800.0f);
    int titleR = (int)(0 * 255 * breathe);
    int titleG = (int)(1.0f * 255 * breathe);
    int titleB = (int)(1.0f * 255 * breathe);

    settextcolor(RGB(titleR, titleG, titleB));
    settextstyle(48, 0, "Consolas");
    const char* title = "BEAT PIXEL";
    int titleW = textwidth(title);
    outtextxy((width - titleW) / 2, 60, title);

    // 标题下方装饰线
    int lineW = titleW + 40;
    sf::RectangleShape decorLine({(float)lineW, 2.0f});
    decorLine.setPosition({(float)((width - lineW) / 2), 115.0f});
    decorLine.setFillColor(sf::Color(0, 200, 200, (uint8_t)(120 * breathe)));
    g_window->draw(decorLine);

    // 副标题
    settextcolor(RGB(140, 140, 170));
    settextstyle(16, 0, "Consolas");
    const char* subtitle = "4-Key Rhythm Game";
    int subW = textwidth(subtitle);
    outtextxy((width - subW) / 2, 125, subtitle);

    // 菜单选项
    const char* menuItems[] = {
        "Start Game (Demo)",
        "Import MP3",
        "Settings",
        "Difficulty: Normal",
        "Exit"
    };
    char diffStr[64];
    snprintf(diffStr, sizeof(diffStr), "Difficulty: %s", getDifficultyName());
    menuItems[3] = diffStr;

    int menuY = 190;
    int itemH = 55;

    for (int i = 0; i < 5; i++) {
        int y = menuY + i * itemH;

        if (i == menuSelection) {
            // 选中项：渐变高亮底板
            sf::RectangleShape highlight({300.0f, 40.0f});
            highlight.setPosition({(float)((width - 300) / 2), (float)(y - 5)});
            highlight.setFillColor(sf::Color(0, 180, 180, 40));
            highlight.setOutlineColor(sf::Color(0, 220, 220, 120));
            highlight.setOutlineThickness(1.0f);
            g_window->draw(highlight);

            // 左侧指示三角
            settextcolor(RGB(0, 255, 255));
            settextstyle(22, 0, "Consolas");
            outtextxy((width - 300) / 2 + 10, y + 2, ">");

            settextcolor(RGB(255, 255, 255));
        } else {
            settextcolor(RGB(100, 100, 130));
        }

        settextstyle(22, 0, "Consolas");
        int itemW = textwidth(menuItems[i]);
        outtextxy((width - itemW) / 2 + 15, y + 3, menuItems[i]);
    }

    // 排行榜预览
    int highScore = dataManager.getHighScore("Demo Song", (int)currentDifficulty);
    if (highScore > 0) {
        settextcolor(RGB(255, 215, 0));
        settextstyle(14, 0, "Consolas");
        char lbStr[128];
        snprintf(lbStr, sizeof(lbStr), "Demo Song Best: %d", highScore);
        int lbW = textwidth(lbStr);
        outtextxy((width - lbW) / 2, 420, lbStr);
    }

    // 底部提示
    settextcolor(RGB(70, 70, 90));
    settextstyle(13, 0, "Consolas");
    const char* hint1 = "W/S: Select  |  ENTER: Confirm";
    const char* hint2 = "MP3 files -> songs/ folder";
    outtextxy((width - textwidth(hint1)) / 2, height - 55, hint1);
    outtextxy((width - textwidth(hint2)) / 2, height - 35, hint2);

    // 版本号
    settextcolor(RGB(50, 50, 70));
    settextstyle(11, 0, "Consolas");
    outtextxy(width - 80, height - 18, "V2.0");
}

// ========== 动态背景绘制 ==========
void GameWindow::drawDynamicBackground() {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) return;

    int combo = scoreSystem.getCurrentCombo();

    // 背景色随Combo变化：深蓝 → 紫 → 红
    int baseR = 12, baseG = 12, baseB = 28;
    if (combo >= 100) {
        // 炽热红
        baseR = 40 + (combo / 10 % 10);  // 微弱脉冲
        baseG = 8;
        baseB = 20;
    } else if (combo >= 50) {
        // 紫色
        float t = (combo - 50) / 50.0f;
        baseR = (int)(20 + 20 * t);
        baseG = 10;
        baseB = (int)(35 - 15 * t);
    } else if (combo >= 10) {
        // 深蓝偏紫
        float t = (combo - 10) / 40.0f;
        baseR = (int)(12 + 8 * t);
        baseG = 12;
        baseB = (int)(28 + 7 * t);
    }

    // 从上到下渐变
    int topR = baseR + 5, topG = baseG + 3, topB = baseB + 10;
    int botR = baseR - 5, botG = baseG - 3, botB = baseB - 5;
    if (botR < 0) botR = 0;
    if (botG < 0) botG = 0;
    if (botB < 0) botB = 0;

    // 绘制渐变背景（分16条带）
    const int bands = 16;
    for (int i = 0; i < bands; i++) {
        float t = (float)i / bands;
        int r = (int)(topR + (botR - topR) * t);
        int g = (int)(topG + (botG - topG) * t);
        int b = (int)(topB + (botB - topB) * t);
        int y1 = (int)(height * t);
        int y2 = (int)(height * (t + 1.0f / bands));

        sf::RectangleShape band({(float)width, (float)(y2 - y1)});
        band.setPosition({0.0f, (float)y1});
        band.setFillColor(sf::Color(r, g, b));
        g_window->draw(band);
    }
}

// ========== 渲染调度 ==========

void GameWindow::render() {
    BeginBatchDraw();

    if (gameState == MENU) {
        cleardevice();
        if (isInSettings) {
            drawSettingsScreen();
        } else {
            drawMenuScreen();
        }
    } else if (gameState == PLAYING) {
        // 游戏背景图（cover模式，不变形铺满）
        {
            using namespace _easyx_impl;
            sf::Sprite bgSprite(textures.gameBg);
            sf::Vector2u texSize = textures.gameBg.getSize();
            float scaleX = (float)width / texSize.x;
            float scaleY = (float)height / texSize.y;
            float scale = std::max(scaleX, scaleY);  // cover模式
            bgSprite.setScale({scale, scale});
            // 居中裁剪
            float offsetX = (width - texSize.x * scale) / 2.0f;
            float offsetY = (height - texSize.y * scale) / 2.0f;
            bgSprite.setPosition({offsetX, offsetY});
            g_window->draw(bgSprite);
        }
        // 动态背景替代纯黑背景
        drawDynamicBackground();
        for (int i = 0; i < TRACK_COUNT; i++) {
            // V3.0: 传递按键瞬间发光Alpha
            int pressGlow = (int)(keyFeedback[i].glowTimer / KeyFeedback::GLOW_DURATION * 255);
            tracks[i]->draw(textures, keyGlowAlpha[i], pressGlow);
        }
        for (int i = 0; i < TRACK_COUNT; i++) {
            // V3.0: 传递按键缩放参数
            tracks[i]->drawKeyButton(textures, keyPressed[i], keyFeedback[i].pressScale);
        }
        drawAnimations();
        drawParticles();        // V3.0: 判定粒子特效
        drawBPMIndicator();     // V3.0: BPM指示器
        drawNotePreviewBar();   // V3.0: 音符预读条
        drawUI();
        drawProgressBar();      // V3.0: 歌曲进度条
    } else if (gameState == PAUSED) {
        // 先绘制游戏界面作为背景
        drawDynamicBackground();
        for (int i = 0; i < TRACK_COUNT; i++) {
            int pressGlow = (int)(keyFeedback[i].glowTimer / KeyFeedback::GLOW_DURATION * 255);
            tracks[i]->draw(textures, keyGlowAlpha[i], pressGlow);
        }
        for (int i = 0; i < TRACK_COUNT; i++) {
            tracks[i]->drawKeyButton(textures, keyPressed[i], keyFeedback[i].pressScale);
        }
        drawAnimations();
        drawParticles();
        drawBPMIndicator();
        drawNotePreviewBar();
        drawUI();
        drawProgressBar();
        // 叠加暂停界面
        drawPauseScreen();
    } else if (gameState == RESULT) {
        cleardevice();
        drawResultScreen();
    }

    FlushBatchDraw();
}

// ========== 动画绘制 ==========

void GameWindow::drawAnimations() {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) return;

    // 绘制打击动画：音符放大消失
    for (auto& anim : hitAnims) {
        float t = anim.elapsed / anim.duration;
        if (t > 1.0f) t = 1.0f;
        float easedT = easeOutQuad(t);

        float scale = 1.0f + 0.5f * easedT;
        uint8_t alpha = (uint8_t)(255 * (1.0f - t));

        // 使用实际纹理尺寸，保持原始比例
        float texW = (float)anim.texture->getSize().x;
        float texH = (float)anim.texture->getSize().y;
        float displayW = 40.0f;  // V3.0: 缩小音符打击动画
        float displayH = displayW * texH / texW;  // 按比例计算高度

        sf::Sprite sprite(*anim.texture);
        sprite.setOrigin({texW / 2.0f, texH / 2.0f});
        sprite.setScale({displayW / texW * scale, displayH / texH * scale});
        sprite.setPosition({anim.x, anim.y});
        sprite.setColor(sf::Color(255, 255, 255, alpha));
        g_window->draw(sprite);
    }

    // 文字弹出动画
    for (auto& anim : textAnims) {
        float t = anim.elapsed / anim.duration;
        if (t > 1.0f) t = 1.0f;
        float easedT = easeOutQuad(t);

        float scale = 1.0f + 0.2f * easedT;
        uint8_t alpha = (uint8_t)(255 * (1.0f - t));

        // 使用实际纹理尺寸，保持原始比例
        float texW = (float)anim.texture->getSize().x;
        float texH = (float)anim.texture->getSize().y;
        float displayW = 90.0f * scale;  // V3.0: 缩小判定文字显示宽度
        float displayH = displayW * texH / texW;  // 按比例计算高度

        sf::Color bgColor;
        if (anim.texture == &textures.textPerfect)
            bgColor = sf::Color(255, 200, 0);
        else if (anim.texture == &textures.textGood)
            bgColor = sf::Color(80, 220, 80);
        else
            bgColor = sf::Color(255, 60, 60);

        sf::RectangleShape bg({displayW + 6, displayH + 4});
        bg.setPosition({anim.x - displayW/2 - 3, anim.y - displayH/2 - 2});
        bg.setFillColor(sf::Color(bgColor.r, bgColor.g, bgColor.b, (uint8_t)(alpha * 0.7f)));
        bg.setOutlineColor(sf::Color(255, 255, 255, alpha));
        bg.setOutlineThickness(1.5f);
        g_window->draw(bg);

        sf::Sprite sprite(*anim.texture);
        sprite.setOrigin({texW / 2.0f, texH / 2.0f});
        sprite.setScale({displayW / texW, displayH / texH});
        sprite.setPosition({anim.x, anim.y});
        sprite.setColor(sf::Color(255, 255, 255, alpha));
        g_window->draw(sprite);
    }
}

// ========== V3.0: BPM指示器 ==========

void GameWindow::drawBPMIndicator() {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) return;

    // BPM脉冲动画：随节拍跳动
    float beatInterval = 60000.0f / currentBPM;
    float phase = fmod((float)currentTime, beatInterval) / beatInterval;

    // 缓动脉冲：快速放大后缓慢恢复
    float pulseScale = 1.0f;
    if (phase < 0.15f) {
        pulseScale = 1.0f + 0.25f * (phase / 0.15f);
    } else if (phase < 0.5f) {
        float t = (phase - 0.15f) / 0.35f;
        pulseScale = 1.25f - 0.25f * easeOutQuad(t);
    }
    bpmPulseScale = pulseScale;

    float panelX = 10.0f;
    float panelY = 72.0f;
    float panelW = 130.0f;
    float panelH = 42.0f;

    // 底板
    sf::RectangleShape bg({panelW, panelH});
    bg.setPosition({panelX, panelY});
    bg.setFillColor(sf::Color(0, 0, 0, 120));
    bg.setOutlineColor(sf::Color(255, 180, 60, (uint8_t)(80 + 60 * (pulseScale - 1.0f) / 0.25f)));
    bg.setOutlineThickness(1.0f);
    g_window->draw(bg);

    // BPM数字
    char bpmStr[16];
    snprintf(bpmStr, sizeof(bpmStr), "%.0f", currentBPM);

    settextcolor(RGB(255, 200, 80));
    int fontSize = (int)(22 * pulseScale);
    settextstyle(fontSize, 0, "Consolas");
    int textW = textwidth(bpmStr);
    outtextxy((int)(panelX + 42 - textW / 2), (int)(panelY + panelH / 2 - fontSize / 2 - 1), bpmStr);

    // "BPM" 标签
    settextcolor(RGB(180, 150, 100));
    settextstyle(11, 0, "Consolas");
    outtextxy((int)(panelX + 42 - textwidth("BPM") / 2), (int)(panelY + panelH - 13), "BPM");

    // 节拍指示圆点
    float dotRadius = 5.0f * pulseScale;
    sf::CircleShape dot(dotRadius);
    dot.setOrigin({dotRadius, dotRadius});
    dot.setPosition({panelX + 105.0f, panelY + panelH / 2});
    uint8_t dotAlpha = (uint8_t)(120 + 135 * (pulseScale - 1.0f) / 0.25f);
    dot.setFillColor(sf::Color(255, 100, 50, dotAlpha));
    g_window->draw(dot);

    // 外环
    float ringRadius = 9.0f * pulseScale;
    sf::CircleShape ring(ringRadius);
    ring.setOrigin({ringRadius, ringRadius});
    ring.setPosition({panelX + 105.0f, panelY + panelH / 2});
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineColor(sf::Color(255, 100, 50, (uint8_t)(dotAlpha * 0.4f)));
    ring.setOutlineThickness(1.5f);
    g_window->draw(ring);
}

// ========== V3.0: 音符预读条 ==========

void GameWindow::drawNotePreviewBar() {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) return;

    int totalWidth = TRACK_COUNT * TRACK_WIDTH + (TRACK_COUNT - 1) * 10;
    int startX = (width - totalWidth) / 2;
    int barX = startX;
    int barY = PREVIEW_Y;
    int barW = totalWidth;
    int barH = PREVIEW_HEIGHT;
    float previewMs = PREVIEW_DURATION * 1000.0f;

    // 底板
    sf::RectangleShape bg({(float)barW, (float)barH});
    bg.setPosition({(float)barX, (float)barY});
    bg.setFillColor(sf::Color(0, 0, 0, 150));
    bg.setOutlineColor(sf::Color(255, 255, 255, 30));
    bg.setOutlineThickness(1.0f);
    g_window->draw(bg);

    // 轨道分割线
    for (int i = 1; i < TRACK_COUNT; i++) {
        int lineX = barX + i * (TRACK_WIDTH + 10) - 5;
        sf::RectangleShape divider({1.0f, (float)barH});
        divider.setPosition({(float)lineX, (float)barY});
        divider.setFillColor(sf::Color(255, 255, 255, 25));
        g_window->draw(divider);
    }

    // "NOW" 标记线
    sf::RectangleShape nowLine({2.0f, (float)barH});
    nowLine.setPosition({(float)barX, (float)barY});
    nowLine.setFillColor(sf::Color(0, 255, 200, 180));
    g_window->draw(nowLine);

    // 时间刻度
    settextcolor(RGB(100, 100, 100));
    settextstyle(9, 0, "Consolas");
    for (int sec = 1; sec <= 5; sec++) {
        float ratio = (float)sec / PREVIEW_DURATION;
        int tickX = barX + (int)(barW * ratio);
        sf::RectangleShape tick({1.0f, 4.0f});
        tick.setPosition({(float)tickX, (float)(barY + barH - 4)});
        tick.setFillColor(sf::Color(255, 255, 255, 50));
        g_window->draw(tick);
        char timeStr[8];
        snprintf(timeStr, sizeof(timeStr), "%ds", sec);
        outtextxy(tickX - 5, barY + barH - 14, timeStr);
    }

    // 音符预览点
    long long viewStart = currentTime;
    long long viewEnd = currentTime + (long long)previewMs;

    for (int trackId = 0; trackId < TRACK_COUNT; trackId++) {
        int trackLeft = barX + trackId * (TRACK_WIDTH + 10);
        int trackRight = trackLeft + TRACK_WIDTH;
        [[maybe_unused]] int trackCenterX = (trackLeft + trackRight) / 2;

        for (const auto& [noteTime, noteTrack] : noteTimeData) {
            if (noteTrack != trackId) continue;
            if (noteTime < viewStart || noteTime > viewEnd) continue;

            float ratio = (float)(noteTime - viewStart) / previewMs;
            int dotX = barX + (int)(barW * ratio);
            int dotY = barY + barH / 2;

            sf::Color dotColor;
            switch (trackId) {
                case 0: dotColor = sf::Color(0, 200, 200, 200); break;
                case 1: dotColor = sf::Color(200, 80, 80, 200); break;
                case 2: dotColor = sf::Color(80, 200, 80, 200); break;
                case 3: dotColor = sf::Color(200, 200, 80, 200); break;
            }

            float dotR = 3.5f;
            sf::CircleShape noteDot(dotR);
            noteDot.setOrigin({dotR, dotR});
            noteDot.setPosition({(float)dotX, (float)dotY});
            noteDot.setFillColor(dotColor);
            g_window->draw(noteDot);
        }
    }

    // 标签
    settextcolor(RGB(120, 120, 120));
    settextstyle(9, 0, "Consolas");
    outtextxy(barX + 5, barY + 2, "PREVIEW");
}

// ========== V3.0: 判定粒子特效绘制 ==========

void GameWindow::drawParticles() {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) return;

    // 绘制粒子
    for (const auto& p : particles) {
        float t = p.elapsed / p.lifetime;
        if (t > 1.0f) t = 1.0f;
        uint8_t alpha = (uint8_t)(255 * (1.0f - easeOutQuad(t)));
        float currentSize = p.size * (1.0f - 0.5f * t);  // 逐渐缩小

        sf::CircleShape circle(currentSize);
        circle.setOrigin({currentSize, currentSize});
        circle.setPosition({p.x, p.y});
        circle.setFillColor(sf::Color(p.color.r, p.color.g, p.color.b, alpha));
        g_window->draw(circle);
    }

    // 绘制Miss叉号
    for (const auto& cross : missCrossAnims) {
        float t = cross.elapsed / cross.duration;
        if (t > 1.0f) t = 1.0f;
        // 闪烁效果：快速明暗交替
        float blink = fabsf(sinf(t * 3.14159f * 4.0f));
        uint8_t alpha = (uint8_t)(255 * (1.0f - t) * blink);

        float size = 12.0f;
        float thickness = 2.5f;

        // 两条交叉线组成X
        sf::RectangleShape line1({size * 2.0f, thickness});
        line1.setOrigin({size, thickness / 2.0f});
        line1.setPosition({cross.x, cross.y});
        line1.setRotation(45.0f);
        line1.setFillColor(sf::Color(255, 60, 60, alpha));
        g_window->draw(line1);

        sf::RectangleShape line2({size * 2.0f, thickness});
        line2.setOrigin({size, thickness / 2.0f});
        line2.setPosition({cross.x, cross.y});
        line2.setRotation(-45.0f);
        line2.setFillColor(sf::Color(255, 60, 60, alpha));
        g_window->draw(line2);
    }
}

// ========== V3.0: 歌曲进度条绘制 ==========

void GameWindow::drawProgressBar() {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) return;

    float barH = 10.0f;
    float barY = (float)height - barH;
    float barW = (float)width;

    // 计算进度
    float progress = 0.0f;
    if (lastNoteTime > 0) {
        progress = (float)currentTime / (float)(lastNoteTime + 2000);
        if (progress > 1.0f) progress = 1.0f;
    }

    // 底板（半透明深色）
    sf::RectangleShape bg({barW, barH});
    bg.setPosition({0.0f, barY});
    bg.setFillColor(sf::Color(0, 0, 0, 160));
    g_window->draw(bg);

    // 进度填充（青色渐变）
    if (progress > 0.0f) {
        sf::RectangleShape fill({barW * progress, barH});
        fill.setPosition({0.0f, barY});
        fill.setFillColor(sf::Color(0, 200, 200, 200));
        g_window->draw(fill);

        // 进度头部光点
        sf::CircleShape head(4.0f);
        head.setOrigin({4.0f, 4.0f});
        head.setPosition({barW * progress, barY + barH / 2});
        head.setFillColor(sf::Color(0, 255, 255, 255));
        g_window->draw(head);
    }

    // 时间文字（右下角）
    int currentSec = (int)(currentTime / 1000);
    int totalSec = (int)((lastNoteTime + 2000) / 1000);
    int curMin = currentSec / 60;
    int curSecRem = currentSec % 60;
    int totMin = totalSec / 60;
    int totSecRem = totalSec % 60;

    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d / %02d:%02d", curMin, curSecRem, totMin, totSecRem);

    settextcolor(RGB(180, 180, 180));
    settextstyle(11, 0, "Consolas");
    int tw = textwidth(timeStr);
    outtextxy(width - tw - 10, (int)(barY - 14), timeStr);
}

// ========== V3.1: 暂停界面绘制 ==========
void GameWindow::drawPauseScreen() {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) return;

    // 半透明黑色遮罩
    sf::RectangleShape mask({(float)width, (float)height});
    mask.setPosition({0, 0});
    mask.setFillColor(sf::Color(0, 0, 0, 180));
    g_window->draw(mask);

    // 面板
    float panelW = 450.0f;
    float panelH = 380.0f;
    float panelX = (width - panelW) / 2.0f;
    float panelY = (height - panelH) / 2.0f;

    sf::RectangleShape panel({panelW, panelH});
    panel.setPosition({panelX, panelY});
    panel.setFillColor(sf::Color(20, 20, 30, 240));
    panel.setOutlineColor(sf::Color(100, 200, 255, 200));
    panel.setOutlineThickness(2.0f);
    g_window->draw(panel);

    // 标题
    settextcolor(RGB(100, 200, 255));
    settextstyle(48, 0, "Consolas");
    const char* title = "游戏暂停";
    int titleW = textwidth(title);
    outtextxy((int)(panelX + (panelW - titleW) / 2), (int)(panelY + 30), title);

// 按钮
    const char* buttonTexts[] = {"继续游戏", "重新开始", "返回主菜单"};
    COLORREF buttonColors[] = {
        RGB(100, 255, 100),   // 继续：绿色
        RGB(255, 200, 0),     // 重新开始：黄色
        RGB(255, 100, 100)    // 返回：红色
    };

    float buttonW = 300.0f;
    float buttonH = 60.0f;
    float buttonY = panelY + 100.0f;
    float buttonX = panelX + (panelW - buttonW) / 2.0f;

    for (int i = 0; i < 3; i++) {
        float currentY = buttonY + i * (buttonH + 20.0f);
        // 按钮背景
        sf::RectangleShape button({buttonW, buttonH});
        button.setPosition({buttonX, currentY});
        if (i == pauseMenuSelection) {
            button.setFillColor(sf::Color(80, 80, 100, 255));
            if (i == 0) button.setOutlineColor(sf::Color(100, 255, 100, 255));
            else if (i == 1) button.setOutlineColor(sf::Color(255, 200, 0, 255));
            else button.setOutlineColor(sf::Color(255, 100, 100, 255));
            button.setOutlineThickness(2.0f);
        } else {
            button.setFillColor(sf::Color(50, 50, 70, 255));
            button.setOutlineColor(sf::Color(100, 100, 120, 100));
            button.setOutlineThickness(1.0f);
        }
        g_window->draw(button);

        // 按钮文字
        settextcolor(buttonColors[i]);
        settextstyle(32, 0, "Consolas");
        int textW = textwidth(buttonTexts[i]);
        outtextxy((int)(buttonX + (buttonW - textW) / 2), (int)(currentY + 12), buttonTexts[i]);
    }

    // 提示文字
    settextcolor(RGB(150, 150, 170));
    settextstyle(16, 0, "Consolas");
    const char* hint = "W/S切换选项，ENTER确认，ESC继续游戏";
    int hintW = textwidth(hint);
    outtextxy((int)(panelX + (panelW - hintW) / 2), (int)(panelY + panelH - 35), hint);
}

// ========== UI绘制 ==========

void GameWindow::drawUI() {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) return;

    // --- 左上角：分数面板 ---
    // 半透明底板
    sf::RectangleShape scorePanel({180.0f, 55.0f});
    scorePanel.setPosition({10.0f, 10.0f});
    scorePanel.setFillColor(sf::Color(0, 0, 0, 120));
    scorePanel.setOutlineColor(sf::Color(255, 255, 255, 30));
    scorePanel.setOutlineThickness(1.0f);
    g_window->draw(scorePanel);

    settextcolor(RGB(255, 255, 255));
    settextstyle(24, 0, "Consolas");
    char scoreStr[64];
    snprintf(scoreStr, sizeof(scoreStr), "SCORE: %d", scoreSystem.getTotalScore());
    outtextxy(20, 18, scoreStr);

    // 歌曲名和难度
    settextcolor(RGB(140, 140, 180));
    settextstyle(13, 0, "Consolas");
    char songStr[128];
    snprintf(songStr, sizeof(songStr), "%s [%s]",
             currentSongName.c_str(), getDifficultyName());
    outtextxy(20, 46, songStr);

    // --- 右上角：Combo面板 ---
    // V3.0: Combo断连红色闪烁
    if (comboBreakFlash > 0.0f) {
        float breakT = comboBreakFlash / COMBO_BREAK_DURATION;
        uint8_t breakAlpha = (uint8_t)(120 * breakT);
        sf::RectangleShape breakOverlay({(float)width, (float)height});
        breakOverlay.setPosition({0.0f, 0.0f});
        breakOverlay.setFillColor(sf::Color(255, 50, 50, breakAlpha));
        g_window->draw(breakOverlay);
    }

    if (scoreSystem.getCurrentCombo() > 0) {
        int baseComboSize = 28 + std::min(scoreSystem.getCurrentCombo() / 10, 5) * 4;

        float comboScale = 1.0f;
        if (comboAnim.elapsed < comboAnim.duration) {
            float t = comboAnim.elapsed / comboAnim.duration;
            comboScale = 1.0f + 0.3f * std::cos(t * 3.14159f * 2.0f) * (1.0f - t);
        }
        int comboSize = (int)(baseComboSize * comboScale);

        // V3.0: Combo等级动态变色
        int combo = scoreSystem.getCurrentCombo();
        int r, g, b;
        bool isGold = false;

        if (combo >= 100) {
            // 金色 + 发光
            r = 255; g = 215; b = 0;
            isGold = true;
        } else if (combo >= 50) {
            // 绿色
            r = 100; g = 255; b = 100;
        } else if (combo >= 10) {
            // 淡蓝色
            r = 100; g = 200; b = 255;
        } else {
            // 白色
            r = 255; g = 255; b = 255;
        }

        // V3.0: Combo断连时短暂变红
        if (comboBreakFlash > 0.0f) {
            float breakT = comboBreakFlash / COMBO_BREAK_DURATION;
            r = (int)(r + (255 - r) * breakT);
            g = (int)(g * (1.0f - breakT));
            b = (int)(b * (1.0f - breakT));
        }

        settextstyle(comboSize, 0, "Consolas");
        settextcolor(RGB(r, g, b));
        char comboStr[32];
        snprintf(comboStr, sizeof(comboStr), "%d COMBO", combo);
        int comboW = textwidth(comboStr);

        // Combo底板
        sf::RectangleShape comboPanel({(float)(comboW + 20), (float)(comboSize + 10)});
        comboPanel.setPosition({(float)(width - comboW - 30), 10.0f});
        comboPanel.setFillColor(sf::Color(0, 0, 0, 100));
        if (isGold) {
            // V3.0: 金色Combo发光底板
            comboPanel.setOutlineColor(sf::Color(255, 215, 0, 80));
            comboPanel.setOutlineThickness(2.0f);
        }
        g_window->draw(comboPanel);

        // V3.0: 金色发光效果
        if (isGold) {
            float glowPulse = 0.6f + 0.4f * sinf((float)currentTime / 200.0f);
            sf::RectangleShape glowPanel({(float)(comboW + 28), (float)(comboSize + 16)});
            glowPanel.setPosition({(float)(width - comboW - 34), 7.0f});
            glowPanel.setFillColor(sf::Color::Transparent);
            glowPanel.setOutlineColor(sf::Color(255, 215, 0, (uint8_t)(60 * glowPulse)));
            glowPanel.setOutlineThickness(3.0f);
            g_window->draw(glowPanel);
        }

        outtextxy(width - comboW - 20, 15, comboStr);

        // Combo加成倍率
        const char* comboLevel = scoreSystem.getComboLevel();
        if (comboLevel[0] != '\0') {
            settextstyle(18, 0, "Consolas");
            settextcolor(RGB(255, 165, 0));
            char levelStr[32];
            snprintf(levelStr, sizeof(levelStr), "BONUS %s", comboLevel);
            int levelW = textwidth(levelStr);
            outtextxy(width - levelW - 20, 15 + comboSize + 5, levelStr);
        }
    }

    // --- 底部：判定统计面板 ---
    sf::RectangleShape statPanel({280.0f, 48.0f});
    statPanel.setPosition({10.0f, (float)(height - 58)});
    statPanel.setFillColor(sf::Color(0, 0, 0, 100));
    g_window->draw(statPanel);

    settextstyle(14, 0, "Consolas");
    settextcolor(RGB(180, 180, 180));
    char statStr[128];
    snprintf(statStr, sizeof(statStr), "P:%d  G:%d  M:%d  MaxCombo:%d",
             scoreSystem.getPerfectCount(),
             scoreSystem.getGoodCount(),
             scoreSystem.getMissCount(),
             scoreSystem.getMaxCombo());
    outtextxy(20, height - 52, statStr);

    // 操作提示
    settextcolor(RGB(100, 100, 100));
    settextstyle(12, 0, "Consolas");
    outtextxy(20, height - 28, "ESC: Menu  |  A S D F");
}

// ========== 结算界面（V3.1新版：带准确率、评级、Full Combo/All Perfect标记） ==========

void GameWindow::drawResultScreen() {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) return;

    // 绘制半透明黑色遮罩
    sf::RectangleShape mask({(float)width, (float)height});
    mask.setPosition({0, 0});
    mask.setFillColor(sf::Color(0, 0, 0, 200));
    g_window->draw(mask);

    // 主面板
    float panelW = 600.0f;
    float panelH = 580.0f;
    float panelX = (width - panelW) / 2.0f;
    float panelY = (height - panelH) / 2.0f;

    sf::RectangleShape panel({panelW, panelH});
    panel.setPosition({panelX, panelY});
    panel.setFillColor(sf::Color(20, 20, 30, 240));
    panel.setOutlineColor(sf::Color(100, 200, 255, 200));
    panel.setOutlineThickness(2.0f);
    g_window->draw(panel);

    // 顶部标题
    settextcolor(RGB(100, 200, 255));
    settextstyle(48, 0, "Consolas");
    const char* title = "演奏完成";
    int titleW = textwidth(title);
    outtextxy((int)(panelX + (panelW - titleW) / 2), (int)(panelY + 30), title);

    // 歌曲名和难度
    settextcolor(RGB(150, 150, 180));
    settextstyle(16, 0, "Consolas");
    char songStr[128];
    snprintf(songStr, sizeof(songStr), "%s [%s]",
             currentSongName.c_str(), getDifficultyName());
    int songW = textwidth(songStr);
    outtextxy((int)(panelX + (panelW - songW) / 2), (int)(panelY + 85), songStr);

    // 最终分数（金色大号）
    settextcolor(RGB(255, 215, 0));
    settextstyle(56, 0, "Consolas");
    char finalScoreStr[64];
    snprintf(finalScoreStr, sizeof(finalScoreStr), "%d", scoreSystem.getTotalScore());
    int scoreW = textwidth(finalScoreStr);
    outtextxy((int)(panelX + (panelW - scoreW) / 2), (int)(panelY + 110), finalScoreStr);

    settextcolor(RGB(200, 160, 0));
    settextstyle(20, 0, "Consolas");
    const char* scoreLabel = "Final Score";
    int labelW = textwidth(scoreLabel);
    outtextxy((int)(panelX + (panelW - labelW) / 2), (int)(panelY + 170), scoreLabel);

    // 最高Combo
    settextstyle(24, 0, "Consolas");
    settextcolor(RGB(255, 215, 0));
    char maxComboStr[32];
    snprintf(maxComboStr, sizeof(maxComboStr), "Max Combo: %d", scoreSystem.getMaxCombo());
    int maxComboW = textwidth(maxComboStr);
    outtextxy((int)(panelX + (panelW - maxComboW) / 2), (int)(panelY + 205), maxComboStr);

    // 计算准确率和评级
    int perfect = scoreSystem.getPerfectCount();
    int good = scoreSystem.getGoodCount();
    int miss = scoreSystem.getMissCount();
    int total = perfect + good + miss;
    float accuracy = total > 0 ? (float)(perfect * 100 + good * 70) / total : 0.0f;

    // 获取评级和颜色
    const char* rating;
    COLORREF ratingColor;
    if (accuracy >= 95.0f) {
        rating = "S";
        ratingColor = RGB(255, 215, 0); // 金色
    } else if (accuracy >= 85.0f) {
        rating = "A";
        ratingColor = RGB(180, 0, 255); // 紫色
    } else if (accuracy >= 70.0f) {
        rating = "B";
        ratingColor = RGB(0, 180, 255); // 蓝色
    } else if (accuracy >= 60.0f) {
        rating = "C";
        ratingColor = RGB(80, 255, 80); // 绿色
    } else {
        rating = "D";
        ratingColor = RGB(150, 150, 150); // 灰色
    }

    // 评级显示
    settextstyle(80, 0, "Consolas");
    settextcolor(ratingColor);
    int ratingW = textwidth(rating);
    outtextxy((int)(panelX + 50), (int)(panelY + 240), rating);

    // 准确率显示
    settextstyle(32, 0, "Consolas");
    settextcolor(RGB(220, 220, 220));
    char accStr[32];
    snprintf(accStr, sizeof(accStr), "%.1f%%", accuracy);
    int accW = textwidth(accStr);
    outtextxy((int)(panelX + 320), (int)(panelY + 260), accStr);

    settextcolor(RGB(150, 150, 150));
    settextstyle(16, 0, "Consolas");
    const char* accLabel = "Accuracy";
    int accLabelW = textwidth(accLabel);
    outtextxy((int)(panelX + 320 + accW / 2 - accLabelW / 2), (int)(panelY + 300), accLabel);

    // 判定统计
    settextstyle(18, 0, "Consolas");
    int statsX = (int)(panelX + 40);
    int statsY = (int)(panelY + 340);
    int lineH = 28;

    settextcolor(RGB(255, 215, 0));
    char perfectStr[64];
    snprintf(perfectStr, sizeof(perfectStr), "Perfect:  %d", perfect);
    outtextxy(statsX, statsY, perfectStr);

    settextcolor(RGB(100, 255, 100));
    char goodStr[64];
    snprintf(goodStr, sizeof(goodStr), "Good:     %d", good);
    outtextxy(statsX, statsY + lineH, goodStr);

    settextcolor(RGB(255, 80, 80));
    char missStr[64];
    snprintf(missStr, sizeof(missStr), "Miss:     %d", miss);
    outtextxy(statsX, statsY + lineH * 2, missStr);

    // 特殊标记：Full Combo / All Perfect
    bool fullCombo = miss == 0 && total > 0;
    bool allPerfect = good == 0 && miss == 0 && total > 0;
    int badgeY = statsY;

    if (allPerfect) {
        settextcolor(RGB(255, 215, 0));
        settextstyle(24, 0, "Consolas");
        const char* apBadge = "🏆 All Perfect";
        int apW = textwidth(apBadge);
        outtextxy((int)(panelX + panelW - apW - 40), badgeY, apBadge);
        badgeY += lineH + 5;
    }

    if (fullCombo && !allPerfect) {
        settextcolor(RGB(255, 215, 0));
        settextstyle(24, 0, "Consolas");
        const char* fcBadge = "⭐ Full Combo";
        int fcW = textwidth(fcBadge);
        outtextxy((int)(panelX + panelW - fcW - 40), badgeY, fcBadge);
    }

    // 排行榜显示
    int lbY = (int)(panelY + 430);
    settextcolor(RGB(0, 255, 255));
    settextstyle(16, 0, "Consolas");
    const char* lbTitle = "--- Leaderboard ---";
    int lbTitleW = textwidth(lbTitle);
    outtextxy((int)(panelX + (panelW - lbTitleW) / 2), lbY, lbTitle);

    auto leaderboard = dataManager.getLeaderboard(currentSongName, (int)currentDifficulty, 2);
    int entryY = lbY + 25;
    int rank = 1;
    for (const auto& entry : leaderboard) {
        settextcolor(rank == 1 ? RGB(255, 215, 0) : RGB(180, 180, 180));
        settextstyle(14, 0, "Consolas");
        char entryStr[128];
        snprintf(entryStr, sizeof(entryStr), "#%d  Score: %d  Combo: %d",
                 rank, entry.highScore, entry.maxCombo);
        int entryW = textwidth(entryStr);
        outtextxy((int)(panelX + (panelW - entryW) / 2), entryY, entryStr);
        entryY += 20;
        rank++;
    }

    if (leaderboard.empty()) {
        settextcolor(RGB(100, 100, 100));
        settextstyle(14, 0, "Consolas");
        const char* noData = "No records yet";
        int noDataW = textwidth(noData);
        outtextxy((int)(panelX + (panelW - noDataW) / 2), entryY, noData);
    }

    // 底部按钮
    const char* buttonTexts[] = {"重新开始", "返回主菜单"};
    COLORREF buttonColors[] = {
        RGB(100, 255, 100),   // 绿色
        RGB(255, 100, 100)    // 红色
    };

    float buttonW = 200.0f;
    float buttonH = 40.0f;
    float buttonY = panelY + panelH - 70.0f;
    float buttonSpacing = 50.0f;
    float startX = panelX + (panelW - buttonW * 2 - buttonSpacing) / 2;

    // 处理菜单选择（这里只显示，输入处理在handleResultInput）
    for (int i = 0; i < 2; i++) {
        float x = startX + i * (buttonW + buttonSpacing);
        sf::RectangleShape button({buttonW, buttonH});
        button.setPosition({x, buttonY});
        if (i == resultMenuSelection) {
            button.setFillColor(sf::Color(80, 80, 100, 255));
            if (i == 0) button.setOutlineColor(sf::Color(100, 255, 100, 255));
            else button.setOutlineColor(sf::Color(255, 100, 100, 255));
            button.setOutlineThickness(2.0f);
        } else {
            button.setFillColor(sf::Color(50, 50, 70, 255));
            button.setOutlineColor(sf::Color(100, 100, 120, 100));
            button.setOutlineThickness(1.0f);
        }
        g_window->draw(button);

        settextcolor(buttonColors[i]);
        settextstyle(22, 0, "Consolas");
        int textW = textwidth(buttonTexts[i]);
        outtextxy((int)(x + (buttonW - textW) / 2), (int)(buttonY + 5), buttonTexts[i]);
    }

    // 提示
    settextcolor(RGB(120, 120, 120));
    settextstyle(14, 0, "Consolas");
    const char* hint1 = "W/S Switch | ENTER Confirm";
    int hintW = textwidth(hint1);
    outtextxy((int)(panelX + (panelW - hintW) / 2), (int)(panelY + panelH - 25), hint1);
}

// ========== V3.1: 设置界面绘制 ==========

void GameWindow::drawSettingsScreen() {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) return;

    // 半透明黑色遮罩
    sf::RectangleShape mask({(float)width, (float)height});
    mask.setPosition({0, 0});
    mask.setFillColor(sf::Color(0, 0, 0, 220));
    g_window->draw(mask);

    // 主面板
    float panelW = 500.0f;
    float panelH = 500.0f;
    float panelX = (width - panelW) / 2.0f;
    float panelY = (height - panelH) / 2.0f;

    sf::RectangleShape panel({panelW, panelH});
    panel.setPosition({panelX, panelY});
    panel.setFillColor(sf::Color(20, 20, 30, 240));
    panel.setOutlineColor(sf::Color(100, 200, 255, 200));
    panel.setOutlineThickness(2.0f);
    g_window->draw(panel);

    // 标题
    settextcolor(RGB(100, 200, 255));
    settextstyle(40, 0, "Consolas");
    const char* title = "Settings";
    int titleW = textwidth(title);
    outtextxy((int)(panelX + (panelW - titleW) / 2), (int)(panelY + 30), title);

    // 设置选项
    const char* optionNames[] = {
        "Music Volume",
        "Effect Volume",
        "Key 1 (Track 1)",
        "Key 2 (Track 2)",
        "Key 3 (Track 3)",
        "Key 4 (Track 4)",
        "Reset to Default Keys",
        "Save & Back to Menu"
    };

    COLORREF optionTextColors[] = {
        RGB(200, 200, 255),
        RGB(200, 255, 200),
        RGB(255, 220, 100),
        RGB(255, 220, 100),
        RGB(255, 220, 100),
        RGB(255, 220, 100),
        RGB(255, 150, 150),
        RGB(255, 200, 100)
    };

    float optionY = panelY + 90.0f;
    float optionH = 45.0f;
    float barWidth = 200.0f;
    float barHeight = 15.0f;

    for (int i = 0; i < 8; i++) {
        float currentY = optionY + i * optionH;

        // 选中效果
        if (i == settingsMenuSelection) {
            sf::RectangleShape highlight({400.0f, 35.0f});
            highlight.setPosition({panelX + (panelW - 400.0f) / 2.0f, currentY - 5.0f});
            highlight.setFillColor(sf::Color(80, 80, 100, 100));
            if (i < 2) highlight.setOutlineColor(sf::Color(100, 150, 255, 200));
            else if (i < 6) highlight.setOutlineColor(sf::Color(255, 200, 100, 200));
            else if (i == 6) highlight.setOutlineColor(sf::Color(255, 100, 100, 200));
            else highlight.setOutlineColor(sf::Color(100, 255, 100, 200));
            highlight.setOutlineThickness(1.5f);
            g_window->draw(highlight);
        }

        // 选项名称
        settextcolor(optionTextColors[i]);
        settextstyle(24, 0, "Consolas");
        outtextxy((int)(panelX + 50), (int)(currentY), optionNames[i]);

        // 音量条（前2个选项）
        if (i < 2) {
            float value = (i == 0) ? musicVolume : effectVolume;

            // 背景条
            sf::RectangleShape bgBar({barWidth, barHeight});
            bgBar.setPosition({panelX + panelW - barWidth - 50, currentY + 2});
            bgBar.setFillColor(sf::Color(50, 50, 70, 255));
            bgBar.setOutlineColor(sf::Color(100, 100, 120, 255));
            bgBar.setOutlineThickness(1.0f);
            g_window->draw(bgBar);

            // 填充条
            sf::RectangleShape fillBar({barWidth * value, barHeight});
            fillBar.setPosition({panelX + panelW - barWidth - 50, currentY + 2});
            fillBar.setFillColor(i == 0 ? sf::Color(100, 150, 255, 255) : sf::Color(100, 255, 100, 255));
            g_window->draw(fillBar);

            // 百分比文字
            settextcolor(RGB(255, 255, 255));
            settextstyle(18, 0, "Consolas");
            char percentStr[16];
            snprintf(percentStr, sizeof(percentStr), "%d%%", (int)(value * 100.0f));
            outtextxy((int)(panelX + panelW - 50 + 10), (int)(currentY), percentStr);
        }
        // 按键设置（2-5选项）
        else if (i < 6) {
            int keyIdx = i - 2;
            const char* keyName = getKeyName(customKeys[keyIdx]);
            settextcolor(currentKeySettingIndex == keyIdx ? RGB(255, 100, 100) : RGB(255, 255, 255));
            settextstyle(24, 0, "Consolas");
            int keyNameW = textwidth(keyName);
            const char* displayStr = keyName;
            if (currentKeySettingIndex == keyIdx) {
                displayStr = keySettingWaitingRelease ? "[PRESS KEY]" : "[RELEASE KEYS...]";
            }
            int displayW = textwidth(displayStr);
            outtextxy((int)(panelX + panelW - displayW - 50), (int)(currentY), displayStr);
        }
    }

    // 正在设置按键的提示
    if (currentKeySettingIndex >= 0) {
        settextcolor(RGB(255, 100, 100));
        settextstyle(22, 0, "Consolas");
        const char* hint = keySettingWaitingRelease ? "Press any key to set (ESC to cancel)" : "Release all keys first...";
        int hintW = textwidth(hint);
        outtextxy((int)(panelX + (panelW - hintW) / 2), (int)(panelY + panelH - 60), hint);
    } else {
        // 普通提示
        settextcolor(RGB(150, 150, 170));
        settextstyle(16, 0, "Consolas");
        const char* hint = "W/S: Select | A/D: Adjust | ENTER: Confirm";
        int hintW = textwidth(hint);
        outtextxy((int)(panelX + (panelW - hintW) / 2), (int)(panelY + panelH - 30), hint);
    }
}

// ========== 主循环 ==========

void GameWindow::run() {
    const int FRAME_TIME = 1000 / fps;

    while (isRunning) {
        long long frameStart = GetTickCount64();

        if (gameState == MENU) {
            if (isInSettings) {
                handleSettingsInput();
            } else {
                handleMenuInput();
            }
        } else if (gameState == PLAYING) {
            handleInput();
            if (!isRunning) break;
            if (gameState == PLAYING) update();
        } else if (gameState == PAUSED) {
            handlePauseInput();
        } else if (gameState == RESULT) {
            handleResultInput();
            if (!isRunning) break;
        }

        render();

        long long elapsed = GetTickCount64() - frameStart;
        if (elapsed < FRAME_TIME) {
            Sleep((DWORD)(FRAME_TIME - elapsed));
        }
    }

    audioManager.stopBGM();
    closegraph();
}
