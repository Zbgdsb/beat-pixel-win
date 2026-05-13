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
    const char* title = "节拍像素";
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
    const char* subtitle = "6键节奏游戏";
    int subW = textwidth(subtitle);
    outtextxy((width - subW) / 2, 125, subtitle);

    // 菜单选项
    const char* menuItems[] = {
        "歌曲列表",
        "成就",
        "游玩说明",
        "设置",
        "难度: 普通",
        "退出"
    };
    char diffStr[64];
    snprintf(diffStr, sizeof(diffStr), "难度: %s", getDifficultyName());
    menuItems[4] = diffStr;

    int menuY = 170;
    int itemH = 42;

    float menuItemX = (float)((width - 300) / 2);
    float menuItemW = 300.0f;
    for (int i = 0; i < 6; i++) {
        int y = menuY + i * itemH;
        
        // 悬浮检测
        bool hovered = (mouseX >= menuItemX && mouseX <= menuItemX + menuItemW &&
                        mouseY >= y - 5 && mouseY <= y + 35);

        if (hovered) {
            // 悬浮高亮 + 自动选中
            menuSelection = i;
            sf::RectangleShape highlight({menuItemW, 40.0f});
            highlight.setPosition({menuItemX, (float)(y - 5)});
            highlight.setFillColor(sf::Color(0, 180, 180, 40));
            highlight.setOutlineColor(sf::Color(0, 220, 220, 120));
            highlight.setOutlineThickness(1.0f);
            g_window->draw(highlight);

            settextcolor(RGB(0, 255, 255));
            settextstyle(22, 0, "Consolas");
            outtextxy((int)(menuItemX + 10), y + 2, ">");

            settextcolor(RGB(255, 255, 255));
        } else if (i == menuSelection) {
            // 选中项（键盘选中，鼠标不在上面）
            sf::RectangleShape highlight({menuItemW, 40.0f});
            highlight.setPosition({menuItemX, (float)(y - 5)});
            highlight.setFillColor(sf::Color(0, 120, 120, 25));
            highlight.setOutlineColor(sf::Color(0, 160, 160, 80));
            highlight.setOutlineThickness(1.0f);
            g_window->draw(highlight);

            settextcolor(RGB(255, 255, 255));
        } else {
            settextcolor(RGB(100, 100, 130));
        }

        settextstyle(22, 0, "Consolas");
        int itemW = textwidth(menuItems[i]);
        outtextxy((width - itemW) / 2, y + 3, menuItems[i]);
    }

    // 排行榜预览（放在菜单下方）
    int highScore = dataManager.getHighScore("示例歌曲", (int)currentDifficulty);
    if (highScore > 0) {
        settextcolor(RGB(255, 215, 0));
        settextstyle(14, 0, "Consolas");
        char lbStr[128];
        snprintf(lbStr, sizeof(lbStr), "示例最佳: %d", highScore);
        int lbW = textwidth(lbStr);
        outtextxy((width - lbW) / 2, 530, lbStr);
    }

    // 底部提示
    settextcolor(RGB(70, 70, 90));
    settextstyle(13, 0, "Consolas");
    const char* hint1 = "W/S: 选择  |  回车: 确认";
    const char* hint2 = "MP3文件放入 songs/ 文件夹";
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
        band.setFillColor(sf::Color(r, g, b, 180)); // 半透明，露出背景图
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
        } else if (showAchievements) {
            drawAchievementsScreen();
        } else if (showTutorial) {
            drawTutorialScreen();
        } else if (isShowingSongList) {
            drawSongListScreen();
        } else {
            drawMenuScreen();
        }
    } else if (gameState == ANALYZING) {
        cleardevice();
        drawAnalysisScreen();
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
    } else if (gameState == TRACK_DELEGATE) {
        cleardevice();
        drawTrackDelegateScreen();
    } else if (gameState == RESULT) {
        cleardevice();
        drawResultScreen();
    }

    // V3.3: 成就弹窗覆盖层（所有界面都显示）
    updateAchievementPopups(1.0f / 60.0f);
    drawAchievementPopups();

    FlushBatchDraw();
    static int renderCount = 0;
    if (++renderCount % 60 == 0) debugLog("render: 60 frames rendered");
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
    float panelY = 85.0f;
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

    // "当前" 标记线
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
    outtextxy(barX + 5, barY + 2, "预览");
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
        line1.setRotation(sf::degrees(45.0f));
        line1.setFillColor(sf::Color(255, 60, 60, alpha));
        g_window->draw(line1);

        sf::RectangleShape line2({size * 2.0f, thickness});
        line2.setOrigin({size, thickness / 2.0f});
        line2.setPosition({cross.x, cross.y});
        line2.setRotation(sf::degrees(-45.0f));
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

    // V3.3: 偏移量显示 + F1/F2提示
    if (gameOffsetMs != 0.0f) {
        settextcolor(RGB(255, 200, 50));
    } else {
        settextcolor(RGB(100, 100, 130));
    }
    settextstyle(12, 0, "Consolas");
    char offsetStr[64];
    snprintf(offsetStr, sizeof(offsetStr), "偏移: %.0fms  [F1:-10ms  F2:+10ms]", gameOffsetMs);
    outtextxy(20, 63, offsetStr);

    // V3.7: 自动演示模式指示
    if (autoPlay) {
        settextcolor(RGB(0, 255, 100));
        settextstyle(14, 0, "Consolas");
        outtextxy(20, 135, "[自动演示 - P键切换]");
    }

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
    sf::RectangleShape statPanel({290.0f, 54.0f});
    statPanel.setPosition({10.0f, (float)(height - 60)});
    statPanel.setFillColor(sf::Color(0, 0, 0, 100));
    g_window->draw(statPanel);

    settextstyle(12, 0, "Consolas");
    settextcolor(RGB(165, 165, 165));
    char statStr[128];
    snprintf(statStr, sizeof(statStr), "P:%d  G:%d  M:%d  MaxCombo:%d",
             scoreSystem.getPerfectCount(),
             scoreSystem.getGoodCount(),
             scoreSystem.getMissCount(),
             scoreSystem.getMaxCombo());
    outtextxy(20, height - 52, statStr);

    // 操作提示
    settextcolor(RGB(110, 110, 110));
    outtextxy(20, height - 28, "A S D F J K  |  P:自动  |  F1/F2:偏移  |  ESC:菜单");
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
    const char* scoreLabel = "最终得分";
    int labelW = textwidth(scoreLabel);
    outtextxy((int)(panelX + (panelW - labelW) / 2), (int)(panelY + 170), scoreLabel);

    // 最高Combo
    settextstyle(24, 0, "Consolas");
    settextcolor(RGB(255, 215, 0));
    char maxComboStr[32];
    snprintf(maxComboStr, sizeof(maxComboStr), "最大连击: %d", scoreSystem.getMaxCombo());
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
    const char* accLabel = "准确率";
    int accLabelW = textwidth(accLabel);
    outtextxy((int)(panelX + 320 + accW / 2 - accLabelW / 2), (int)(panelY + 300), accLabel);

    // 判定统计
    settextstyle(18, 0, "Consolas");
    int statsX = (int)(panelX + 40);
    int statsY = (int)(panelY + 340);
    int lineH = 28;

    settextcolor(RGB(255, 215, 0));
    char perfectStr[64];
    snprintf(perfectStr, sizeof(perfectStr), "完美:  %d", perfect);
    outtextxy(statsX, statsY, perfectStr);

    settextcolor(RGB(100, 255, 100));
    char goodStr[64];
    snprintf(goodStr, sizeof(goodStr), "良好:  %d", good);
    outtextxy(statsX, statsY + lineH, goodStr);

    settextcolor(RGB(255, 80, 80));
    char missStr[64];
    snprintf(missStr, sizeof(missStr), "失误:  %d", miss);
    outtextxy(statsX, statsY + lineH * 2, missStr);

    // 特殊标记：Full Combo / All Perfect
    bool fullCombo = miss == 0 && total > 0;
    bool allPerfect = good == 0 && miss == 0 && total > 0;
    int badgeY = statsY;

    if (allPerfect) {
        settextcolor(RGB(255, 215, 0));
        settextstyle(24, 0, "Consolas");
        const char* apBadge = "🏆 全完美";
        int apW = textwidth(apBadge);
        outtextxy((int)(panelX + panelW - apW - 40), badgeY, apBadge);
        badgeY += lineH + 5;
    }

    if (fullCombo && !allPerfect) {
        settextcolor(RGB(255, 215, 0));
        settextstyle(24, 0, "Consolas");
        const char* fcBadge = "⭐ 全连击";
        int fcW = textwidth(fcBadge);
        outtextxy((int)(panelX + panelW - fcW - 40), badgeY, fcBadge);
    }

    // 排行榜显示
    int lbY = (int)(panelY + 430);
    settextcolor(RGB(0, 255, 255));
    settextstyle(16, 0, "Consolas");
    const char* lbTitle = "--- 排行榜 ---";
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
        const char* noData = "暂无记录";
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
    const char* hint1 = "W/S: 切换  |  回车: 确认";
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
    float panelH = 620.0f;
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
    const char* title = "设置";
    int titleW = textwidth(title);
    outtextxy((int)(panelX + (panelW - titleW) / 2), (int)(panelY + 30), title);

    // 设置选项
    const char* optionNames[] = {
        "音乐音量",
        "音效音量",
        "音效包",
        "按键1 (A - 底鼓)",
        "按键2 (S - 踩镲)",
        "按键3 (D - 吊镲)",
        "按键4 (F - 军鼓)",
        "按键5 (J - 通鼓)",
        "按键6 (K - 叮镲)",
        "恢复默认按键",
        "保存并返回"
    };

    COLORREF optionTextColors[] = {
        RGB(200, 200, 255),
        RGB(200, 255, 200),
        RGB(255, 200, 200),
        RGB(0, 255, 255),    // Key1 A=Kick 青色
        RGB(255, 100, 100),  // Key2 S=Hi-hat 红色
        RGB(100, 255, 100),  // Key3 D=Crash 绿色
        RGB(255, 255, 100),  // Key4 F=Snare 黄色
        RGB(255, 150, 50),
        RGB(200, 100, 255),
        RGB(255, 150, 150),
        RGB(255, 200, 100)
    };

    float optionY = panelY + 90.0f;
    float optionH = 45.0f;
    float barWidth = 200.0f;
    float barHeight = 15.0f;

    // 检出鼠标位置用于悬停高亮
    sf::Vector2i mousePos = sf::Mouse::getPosition(*g_window);
    float mx = (float)mousePos.x;
    float my = (float)mousePos.y;
    bool mouseOverPanel = (mx >= panelX && mx <= panelX + panelW && my >= panelY && my <= panelY + panelH);
    int hoveredItem = -1;

    for (int i = 0; i < 11; i++) {
        float currentY = optionY + i * optionH;

        // 鼠标悬停检测（在键盘选中前判断，避免被键盘选中覆盖）
        float hlX = panelX + (panelW - 400.0f) / 2.0f;
        float hlY = currentY - 5.0f;
        bool mouseHover = mouseOverPanel && isPointInRect(mx, my, hlX, hlY, 400.0f, 35.0f);
        if (mouseHover) hoveredItem = i;

        // 选中效果（键盘选中优先，鼠标悬停为辅）
        if (i == settingsMenuSelection || mouseHover) {
            sf::RectangleShape highlight({400.0f, 35.0f});
            highlight.setPosition({hlX, hlY});
            if (i == settingsMenuSelection) {
                highlight.setFillColor(sf::Color(80, 80, 120, 150));
            } else {
                highlight.setFillColor(sf::Color(60, 60, 80, 80));
            }
            if (i < 2) highlight.setOutlineColor(sf::Color(100, 150, 255, 200));
            else if (i == 2) highlight.setOutlineColor(sf::Color(255, 100, 100, 200));
            else if (i < 9) highlight.setOutlineColor(sf::Color(255, 200, 100, 200));
            else if (i == 9) highlight.setOutlineColor(sf::Color(255, 100, 100, 200));
            else highlight.setOutlineColor(sf::Color(100, 255, 100, 200));
            highlight.setOutlineThickness(i == settingsMenuSelection ? 1.5f : 0.8f);
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
        // 音效包选项
        else if (i == 2) {
            const char* packNames[] = {"叮咚", "打击乐"};
            int pack = audioManager.getSoundPack();
            settextcolor(pack == 0 ? RGB(200, 200, 255) : RGB(255, 100, 100));
            settextstyle(20, 0, "Consolas");
            outtextxy((int)(panelX + panelW - 180), (int)(currentY), packNames[pack]);
        }
        // 按键设置（3-8选项）
        else if (i < 9) {
            int keyIdx = i - 3;
            const char* keyName = getKeyName(customKeys[keyIdx]);
            settextcolor(currentKeySettingIndex == keyIdx ? RGB(255, 100, 100) : RGB(255, 255, 255));
            settextstyle(24, 0, "Consolas");
            const char* displayStr = keyName;
            if (currentKeySettingIndex == keyIdx) {
                displayStr = keySettingWaitingRelease ? "[请按键]" : "[请松开...]";
            }
            int displayW = textwidth(displayStr);
            outtextxy((int)(panelX + panelW - displayW - 50), (int)(currentY), displayStr);
        }
    }

    // 正在设置按键的提示
    if (currentKeySettingIndex >= 0) {
        settextcolor(RGB(255, 100, 100));
        settextstyle(22, 0, "Consolas");
        const char* hint = keySettingWaitingRelease ? "按任意键设置 (ESC取消)" : "请先松开所有按键...";
        int hintW = textwidth(hint);
        outtextxy((int)(panelX + (panelW - hintW) / 2), (int)(panelY + panelH - 60), hint);
    } else {
        // 普通提示
        settextcolor(RGB(150, 150, 170));
        settextstyle(16, 0, "Consolas");
        const char* hint = "W/S: 选择 | A/D: 调整 | 回车: 确认";
        int hintW = textwidth(hint);
        outtextxy((int)(panelX + (panelW - hintW) / 2), (int)(panelY + panelH - 30), hint);
    }
}

// ========== V3.2: 歌曲分析界面绘制 ==========
void GameWindow::drawAnalysisScreen() {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) { debugLog("drawAnalysisScreen: window closed, skip"); return; }
    debugLog("drawAnalysisScreen: start");

    // 渐变背景
    const int bands = 16;
    for (int i = 0; i < bands; i++) {
        float t = (float)i / bands;
        int r = (int)(5 + 10 * t);
        int g = (int)(5 + 5 * t);
        int b = (int)(15 + 12 * t);
        int y1 = (int)(height * t);
        int y2 = (int)(height * (t + 1.0f / bands));
        sf::RectangleShape band({(float)width, (float)(y2 - y1)});
        band.setPosition({0.0f, (float)y1});
        band.setFillColor(sf::Color(r, g, b));
        g_window->draw(band);
    }

    // 主面板
    float panelW = 700.0f;
    float panelH = 550.0f;
    float panelX = (width - panelW) / 2.0f;
    float panelY = (height - panelH) / 2.0f;

    sf::RectangleShape panel({panelW, panelH});
    panel.setPosition({panelX, panelY});
    panel.setFillColor(sf::Color(15, 15, 25, 240));
    panel.setOutlineColor(sf::Color(100, 200, 255, 150));
    panel.setOutlineThickness(2.0f);
    g_window->draw(panel);

    // 标题
    settextcolor(RGB(100, 200, 255));
    settextstyle(36, 0, "Consolas");
    const char* title = "歌曲分析"; // 歌曲分析
    int titleW = textwidth(title);
    outtextxy((int)(panelX + (panelW - titleW) / 2), (int)(panelY + 15), title);

    if (!analysisDone) {
        // 分析中提示
        settextcolor(RGB(200, 200, 200));
        settextstyle(24, 0, "Consolas");
        const char* loading = "正在分析，请稍候..."; // 正在分析，请稍候...
        int loadingW = textwidth(loading);
        outtextxy((int)(panelX + (panelW - loadingW) / 2), (int)(panelY + panelH / 2), loading);
        return;
    }

    float contentX = panelX + 30;
    float contentY = panelY + 65;

    // ===== 歌曲信息 =====
    settextcolor(RGB(0, 255, 200));
    settextstyle(14, 0, "Consolas");
    outtextxy((int)contentX, (int)contentY, "[歌曲信息]"); // [歌曲信息]
    contentY += 22;

    settextcolor(RGB(220, 220, 220));
    settextstyle(16, 0, "Consolas");
    char infoStr[256];

    snprintf(infoStr, sizeof(infoStr), "歌名: %s", analysisResult.metadata.title.c_str()); // 歌名
    outtextxy((int)contentX, (int)contentY, infoStr);
    contentY += 22;

    snprintf(infoStr, sizeof(infoStr), "歌手: %s", analysisResult.metadata.artist.empty() ? "未知" : analysisResult.metadata.artist.c_str()); // 歌手: 未知
    outtextxy((int)contentX, (int)contentY, infoStr);
    contentY += 22;

    int durMin = (int)(analysisResult.metadata.duration) / 60;
    int durSec = (int)(analysisResult.metadata.duration) % 60;
    snprintf(infoStr, sizeof(infoStr), "时长: %02d:%02d", durMin, durSec); // 时长
    outtextxy((int)contentX, (int)contentY, infoStr);
    contentY += 35;

    // ===== BPM分析 =====
    settextcolor(RGB(0, 255, 200));
    settextstyle(14, 0, "Consolas");
    outtextxy((int)contentX, (int)contentY, "[节奏分析]"); // [节奏分析]
    contentY += 22;

    // 置信度颜色
    COLORREF confColor;
    if (analysisResult.bpmConfidence >= 90) confColor = RGB(100, 255, 100);
    else if (analysisResult.bpmConfidence >= 70) confColor = RGB(255, 200, 100);
    else confColor = RGB(255, 100, 100);

    settextcolor(RGB(255, 255, 255));
    settextstyle(20, 0, "Consolas");
    char bpmStr[64];
    snprintf(bpmStr, sizeof(bpmStr), "BPM: %.1f  (置信度: %.1f%%)", manualBPM, analysisResult.bpmConfidence); // 置信度
    outtextxy((int)contentX, (int)contentY, bpmStr);

    // 置信度进度条
    float barX = contentX + 350;
    float barY = contentY + 3;
    float barW = 200.0f, barH = 16.0f;
    sf::RectangleShape confBg({barW, barH});
    confBg.setPosition({barX, barY});
    confBg.setFillColor(sf::Color(40, 40, 50));
    g_window->draw(confBg);
    sf::RectangleShape confFill({barW * (analysisResult.bpmConfidence / 100.0f), barH});
    confFill.setPosition({barX, barY});
    confFill.setFillColor(sf::Color(
        (confColor >> 16) & 0xFF, (confColor >> 8) & 0xFF, confColor & 0xFF, 200));
    g_window->draw(confFill);

    if (analysisResult.bpmConfidence < 90) {
        settextcolor(RGB(255, 150, 50));
        settextstyle(12, 0, "Consolas");
        outtextxy((int)contentX, (int)(contentY + 25), "[!] 置信度偏低，请用 A/D 调整或 T 键点拍校准"); // 置信度偏低，请用 A/D 调整或 T 键点拍校准
    }
    contentY += 50;

    // ===== 偏移 =====
    settextcolor(RGB(0, 255, 200));
    settextstyle(14, 0, "Consolas");
    outtextxy((int)contentX, (int)contentY, "[节拍偏移]"); // [节拍偏移]
    contentY += 22;

    settextcolor(RGB(255, 255, 255));
    settextstyle(18, 0, "Consolas");
    char offsetStr[64];
    snprintf(offsetStr, sizeof(offsetStr), "偏移: %.0f ms  (A/D: 左/右调整，每次5ms)", manualOffset); // 偏移: xx ms  (A/D: 左/右调整，每次5ms)
    outtextxy((int)contentX, (int)contentY, offsetStr);
    contentY += 35;

    // ===== 节拍统计 =====
    settextcolor(RGB(0, 255, 200));
    settextstyle(14, 0, "Consolas");
    outtextxy((int)contentX, (int)contentY, "[节拍统计]"); // [节拍统计]
    contentY += 22;

    int accentCount = 0;
    for (const auto& b : analysisResult.beats) {
        if (b.isAccent) accentCount++;
    }

    settextcolor(RGB(200, 200, 200));
    settextstyle(16, 0, "Consolas");
    char statStr[128];
    snprintf(statStr, sizeof(statStr), "总节拍: %zu  |  重音: %d  |  普通: %zu",
             analysisResult.beats.size(), accentCount, analysisResult.beats.size() - accentCount); // 总节拍 | 重音 | 普通
    outtextxy((int)contentX, (int)contentY, statStr);
    contentY += 22;
    snprintf(statStr, sizeof(statStr), "生成音符: %zu", analysisResult.chart.size()); // 生成音符
    outtextxy((int)contentX, (int)contentY, statStr);
    contentY += 35;

    // ===== 节拍时间线预览 =====
    settextcolor(RGB(0, 255, 200));
    settextstyle(14, 0, "Consolas");
    outtextxy((int)contentX, (int)contentY, "[音符分布预览]"); // [音符分布预览]
    contentY += 20;

    float tlX = contentX;
    float tlY = contentY;
    float tlW = panelW - 60;
    float tlH = 60.0f;

    // 时间线底板
    sf::RectangleShape tlBg({tlW, tlH});
    tlBg.setPosition({tlX, tlY});
    tlBg.setFillColor(sf::Color(10, 10, 20, 200));
    tlBg.setOutlineColor(sf::Color(60, 60, 80));
    tlBg.setOutlineThickness(1.0f);
    g_window->draw(tlBg);

    // 轨道分割线
    for (int i = 1; i < 4; i++) {
        float lx = tlX + (tlW / 4.0f) * i;
        sf::RectangleShape divider({1.0f, tlH});
        divider.setPosition({lx, tlY});
        divider.setFillColor(sf::Color(255, 255, 255, 20));
        g_window->draw(divider);
    }

    // 绘制音符点
    if (!analysisResult.chart.empty()) {
        long long maxTime = analysisResult.chart.back().first;
        if (maxTime > 0) {
            sf::Color trackColors[4] = {
                sf::Color(0, 200, 200), sf::Color(200, 80, 80),
                sf::Color(80, 200, 80), sf::Color(200, 200, 80)
            };
            for (const auto& [t, trk] : analysisResult.chart) {
                float ratio = (float)t / maxTime;
                float dotX = tlX + tlW * ratio;
                float dotY = tlY + (trk + 0.5f) * (tlH / 4.0f);
                float dotR = 3.0f;
                sf::CircleShape dot(dotR);
                dot.setOrigin({dotR, dotR});
                dot.setPosition({dotX, dotY});
                dot.setFillColor(trackColors[trk]);
                g_window->draw(dot);
            }
        }
    }
    contentY += tlH + 15;

    // ===== 操作按钮（含悬浮高亮） =====
    const char* btnTexts[] = {"BPM", "偏移", "开始游戏", "导出", "返回"};
    COLORREF btnColors[] = {
        RGB(255, 200, 80), RGB(255, 200, 80),
        RGB(100, 255, 100), RGB(100, 200, 255), RGB(255, 100, 100)
    };
    float btnW = 120.0f, btnH = 36.0f;
    float btnStartX = contentX;
    float btnY = contentY;
    float btnSpacing = 15.0f;

    for (int i = 0; i < 5; i++) {
        float bx = btnStartX + i * (btnW + btnSpacing);
        bool hovered = (mouseX >= bx && mouseX <= bx + btnW &&
                        mouseY >= btnY && mouseY <= btnY + btnH);
        if (hovered) analysisMenuSelection = i;

        sf::RectangleShape btn({btnW, btnH});
        btn.setPosition({bx, btnY});
        if (i == analysisMenuSelection) {
            btn.setFillColor(sf::Color(80, 80, 100, 255));
            btn.setOutlineColor(sf::Color(
                (btnColors[i] >> 16) & 0xFF, (btnColors[i] >> 8) & 0xFF, btnColors[i] & 0xFF, hovered ? 255 : 255));
            btn.setOutlineThickness(hovered ? 3.0f : 2.0f);
        } else if (hovered) {
            btn.setFillColor(sf::Color(50, 50, 70, 230));
            btn.setOutlineColor(sf::Color(
                (btnColors[i] >> 16) & 0xFF, (btnColors[i] >> 8) & 0xFF, btnColors[i] & 0xFF, 180));
            btn.setOutlineThickness(2.0f);
        } else {
            btn.setFillColor(sf::Color(40, 40, 55, 200));
            btn.setOutlineColor(sf::Color(80, 80, 100, 100));
            btn.setOutlineThickness(1.0f);
        }
        g_window->draw(btn);

        settextcolor(hovered ? btnColors[i] : btnColors[i]);
        settextstyle(14, 0, "Consolas");
        int tw = textwidth(btnTexts[i]);
        outtextxy((int)(bx + (btnW - tw) / 2), (int)(btnY + 10), btnTexts[i]);
    }

    // ===== 底部提示 =====
    settextcolor(RGB(120, 120, 140));
    settextstyle(12, 0, "Consolas");
    const char* hint1 = "W/S: 切换  |  A/D: 调整  |  T: 点拍校准  |  R: 重新分析"; // W/S: 切换 | A/D: 调整 | T: 点拍校准 | R: 重新分析
    const char* hint2 = "ENTER: 确认  |  ESC: 返回菜单"; // ENTER: 确认 | ESC: 返回菜单
    outtextxy((int)(panelX + (panelW - textwidth(hint1)) / 2), (int)(panelY + panelH - 40), hint1);
    outtextxy((int)(panelX + (panelW - textwidth(hint2)) / 2), (int)(panelY + panelH - 22), hint2);

    // 点拍模式指示
    if (isInTapping) {
        settextcolor(RGB(255, 100, 100));
        settextstyle(16, 0, "Consolas");
        char tapStr[64];
        snprintf(tapStr, sizeof(tapStr), "[点拍模式] 按 T 键跟着节奏点（至少3次）", songAnalyzer.getTapCount()); // [点拍模式] 按 T 键跟着节奏点（至少3次）
        int tapW = textwidth(tapStr);
        // 高亮底板
        sf::RectangleShape tapBg({(float)(tapW + 20), 28.0f});
        tapBg.setPosition({panelX + (panelW - tapW - 20) / 2, panelY + panelH - 70});
        tapBg.setFillColor(sf::Color(255, 50, 50, 60));
        tapBg.setOutlineColor(sf::Color(255, 100, 100, 150));
        tapBg.setOutlineThickness(1.0f);
        g_window->draw(tapBg);
        outtextxy((int)(panelX + (panelW - tapW) / 2), (int)(panelY + panelH - 65), tapStr);
    }
    debugLog("drawAnalysisScreen: end");
}

// ========== V3.3: 成就弹窗更新 ==========
void GameWindow::updateAchievementPopups(float dt) {
    for (int i = (int)achievementPopups.size() - 1; i >= 0; i--) {
        achievementPopups[i].elapsed += dt;
        if (achievementPopups[i].elapsed >= AchievementPopup::DURATION) {
            achievementPopups.erase(achievementPopups.begin() + i);
        }
    }
}

// ========== V3.3: 成就弹窗绘制 ==========
void GameWindow::drawAchievementPopups() {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) return;

    for (size_t i = 0; i < achievementPopups.size(); i++) {
        auto& pop = achievementPopups[i];
        float t = pop.elapsed / AchievementPopup::DURATION;

        // 从顶部滑入，最后0.5秒淡出
        float alpha = 1.0f;
        float slideIn = std::min(t * 4.0f, 1.0f); // 0.25秒滑入
        if (t > 0.8f) alpha = 1.0f - (t - 0.8f) / 0.2f; // 最后20%淡出
        if (alpha < 0) alpha = 0;

        float popupW = 300.0f, popupH = 60.0f;
        float popupX = (width - popupW) / 2.0f;
        float popupY = 20.0f + i * 70.0f - (1.0f - slideIn) * 80.0f; // 滑入动画

        uint8_t a = (uint8_t)(alpha * 220);

        // 弹窗底板
        sf::RectangleShape bg({popupW, popupH});
        bg.setPosition({popupX, popupY});
        bg.setFillColor(sf::Color(20, 30, 50, a));
        bg.setOutlineColor(sf::Color(255, 215, 0, (uint8_t)(alpha * 200)));
        bg.setOutlineThickness(2.0f);
        g_window->draw(bg);

        // 图标
        settextcolor(RGB(255, 215, 0));
        settextstyle(28, 0, "Consolas");
        outtextxy((int)(popupX + 15), (int)(popupY + 15), pop.icon.c_str());

        // 标题
        settextcolor(RGB(255, 255, 255));
        settextstyle(18, 0, "Consolas");
        outtextxy((int)(popupX + 55), (int)(popupY + 10), pop.title.c_str());

        // 描述
        settextcolor(RGB(180, 180, 180));
        settextstyle(13, 0, "Consolas");
        outtextxy((int)(popupX + 55), (int)(popupY + 35), pop.description.c_str());
    }
}

// ========== V3.3: 成就界面绘制 ==========
void GameWindow::drawAchievementsScreen() {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) return;

    // 背景
    sf::RectangleShape mask({(float)width, (float)height});
    mask.setPosition({0, 0});
    mask.setFillColor(sf::Color(0, 0, 0, 220));
    g_window->draw(mask);

    float panelW = 600.0f, panelH = 620.0f;
    float panelX = (width - panelW) / 2.0f;
    float panelY = (height - panelH) / 2.0f;

    sf::RectangleShape panel({panelW, panelH});
    panel.setPosition({panelX, panelY});
    panel.setFillColor(sf::Color(20, 20, 30, 240));
    panel.setOutlineColor(sf::Color(255, 215, 0, 150));
    panel.setOutlineThickness(2.0f);
    g_window->draw(panel);

    // 标题
    settextcolor(RGB(255, 215, 0));
    settextstyle(36, 0, "Consolas");
    const char* title = "成就";
    int titleW = textwidth(title);
    outtextxy((int)(panelX + (panelW - titleW) / 2), (int)(panelY + 20), title);

    // 进度
    int unlocked = achievementSystem.getUnlockedCount();
    int total = (int)achievementSystem.getAll().size();
    settextcolor(RGB(180, 180, 180));
    settextstyle(14, 0, "Consolas");
    char progStr[64];
    snprintf(progStr, sizeof(progStr), "%d / %d 已解锁", unlocked, total);
    int progW = textwidth(progStr);
    outtextxy((int)(panelX + (panelW - progW) / 2), (int)(panelY + 60), progStr);

    // 成就列表
    auto& all = achievementSystem.getAll();
    float itemY = panelY + 90;
    float itemH = 45.0f;

    for (size_t i = 0; i < all.size(); i++) {
        float y = itemY + i * itemH;
        if (y + itemH > panelY + panelH - 70) break;

        // 背景条（比行间距略窄以留出空隙）
        sf::RectangleShape row({panelW - 40, itemH - 3});
        row.setPosition({panelX + 20, y});
        if (all[i].unlocked) {
            row.setFillColor(sf::Color(40, 50, 30, 200));
            row.setOutlineColor(sf::Color(100, 255, 100, 80));
        } else {
            row.setFillColor(sf::Color(30, 30, 40, 150));
            row.setOutlineColor(sf::Color(60, 60, 80, 80));
        }
        row.setOutlineThickness(1.0f);
        g_window->draw(row);

        // 图标
        settextstyle(22, 0, "Consolas");
        if (all[i].unlocked) {
            settextcolor(RGB(255, 215, 0));
        } else {
            settextcolor(RGB(60, 60, 70));
        }
        outtextxy((int)(panelX + 30), (int)(y + 10), all[i].icon);

        // 名称
        if (all[i].unlocked) {
            settextcolor(RGB(255, 255, 255));
        } else {
            settextcolor(RGB(80, 80, 100));
        }
        settextstyle(16, 0, "Consolas");
        outtextxy((int)(panelX + 65), (int)(y + 4), all[i].name);

        // 描述
        if (all[i].unlocked) {
            settextcolor(RGB(150, 200, 150));
        } else {
            settextcolor(RGB(60, 60, 80));
        }
        settextstyle(12, 0, "Consolas");
        outtextxy((int)(panelX + 65), (int)(y + 24), all[i].description);

        // 解锁时间
        if (all[i].unlocked) {
            settextcolor(RGB(120, 150, 120));
            settextstyle(11, 0, "Consolas");
            int timeW = textwidth(all[i].unlockedTime.c_str());
            outtextxy((int)(panelX + panelW - timeW - 30), (int)(y + 14), all[i].unlockedTime.c_str());
        }
    }

    // 返回按钮
    float btnW = 200.0f, btnH = 40.0f;
    float btnX = panelX + (panelW - btnW) / 2;
    float btnY = panelY + panelH - 60;
    sf::RectangleShape btn({btnW, btnH});
    btn.setPosition({btnX, btnY});
    btn.setFillColor(sf::Color(80, 80, 100, 255));
    btn.setOutlineColor(sf::Color(255, 100, 100, 200));
    btn.setOutlineThickness(2.0f);
    g_window->draw(btn);

    settextcolor(RGB(255, 100, 100));
    settextstyle(20, 0, "Consolas");
    const char* backText = "返回";
    int backW = textwidth(backText);
    outtextxy((int)(btnX + (btnW - backW) / 2), (int)(btnY + 10), backText);

    // 提示
    settextcolor(RGB(100, 100, 120));
    settextstyle(12, 0, "Consolas");
    outtextxy((int)(panelX + (panelW - textwidth("ESC/回车: 返回")) / 2), (int)(panelY + panelH - 20), "ESC/回车: 返回");
}

// ========== V3.4: 歌曲列表界面 ==========
void GameWindow::drawSongListScreen() {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) return;

    // 背景
    sf::RectangleShape mask({(float)width, (float)height});
    mask.setPosition({0, 0});
    mask.setFillColor(sf::Color(0, 0, 0, 220));
    g_window->draw(mask);

    float panelW = 500.0f, panelH = 400.0f;
    float panelX = (width - panelW) / 2.0f;
    float panelY = (height - panelH) / 2.0f;

    sf::RectangleShape panel({panelW, panelH});
    panel.setPosition({panelX, panelY});
    panel.setFillColor(sf::Color(20, 20, 30, 240));
    panel.setOutlineColor(sf::Color(0, 200, 200, 150));
    panel.setOutlineThickness(2.0f);
    g_window->draw(panel);

    // 标题
    settextcolor(RGB(0, 255, 200));
    settextstyle(28, 0, "Consolas");
    const char* title = "歌曲列表";
    int titleW = textwidth(title);
    outtextxy((int)(panelX + (panelW - titleW) / 2), (int)(panelY + 15), title);

    // 数量提示
    settextcolor(RGB(120, 120, 150));
    settextstyle(12, 0, "Consolas");
    char countStr[64];
    snprintf(countStr, sizeof(countStr), "%zu 首歌曲", songList.size());
    int countW = textwidth(countStr);
    outtextxy((int)(panelX + (panelW - countW) / 2), (int)(panelY + 45), countStr);

    // 歌曲列表
    float itemH = 36.0f;
    float startY = panelY + 60;
    int maxVisible = (int)((panelH - 120) / itemH);
    int scrollOffset = 0;
    if (songListSelection >= maxVisible) {
        scrollOffset = songListSelection - maxVisible + 1;
    }

    for (int i = 0; i < maxVisible && (i + scrollOffset) < (int)songList.size(); i++) {
        int idx = i + scrollOffset;
        float y = startY + i * itemH;

        bool isSelected = (idx == songListSelection);
        
        // 悬浮检测
        bool hovered = (mouseX >= panelX + 20 && mouseX <= panelX + panelW - 20 &&
                        mouseY >= y && mouseY <= y + itemH);
        if (hovered) songListSelection = idx;

        // 背景条
        sf::RectangleShape row({panelW - 40, itemH - 2});
        row.setPosition({panelX + 20, y});
        if (isSelected) {
            row.setFillColor(sf::Color(0, 60, 60, 200));
            row.setOutlineColor(sf::Color(0, 220, 220, 150));
        } else if (hovered) {
            row.setFillColor(sf::Color(0, 80, 80, 150));
            row.setOutlineColor(sf::Color(0, 180, 180, 100));
        } else {
            row.setFillColor(sf::Color(30, 30, 40, 100));
            row.setOutlineColor(sf::Color(60, 60, 80, 50));
        }
        row.setOutlineThickness(1.0f);
        g_window->draw(row);

        // 歌曲名
        if (isSelected) {
            settextcolor(RGB(0, 255, 200));
            settextstyle(16, 0, "Consolas");
        } else if (hovered) {
            settextcolor(RGB(200, 220, 240));
            settextstyle(16, 0, "Consolas");
        } else {
            settextcolor(RGB(180, 180, 200));
            settextstyle(15, 0, "Consolas");
        }
        outtextxy((int)(panelX + 35), (int)(y + 8), songList[idx].name.c_str());

        // 谱面状态
        if (songList[idx].hasChart) {
            settextcolor(RGB(100, 200, 100));
        } else {
            settextcolor(RGB(80, 80, 100));
        }
        settextstyle(12, 0, "Consolas");
        const char* status = songList[idx].hasChart ? "[有谱]" : "[无谱]";
        outtextxy((int)(panelX + panelW - 90), (int)(y + 12), status);
    }

    // 滚动提示
    if ((int)songList.size() > maxVisible) {
        settextcolor(RGB(80, 80, 100));
        settextstyle(11, 0, "Consolas");
        outtextxy((int)(panelX + (panelW - textwidth("W/S: 滚动")) / 2), (int)(panelY + panelH - 35), "W/S: 滚动");
    }

    // 返回提示
    settextcolor(RGB(100, 100, 120));
    settextstyle(12, 0, "Consolas");
    outtextxy((int)(panelX + (panelW - textwidth("ESC: 返回  |  回车: 播放")) / 2), (int)(panelY + panelH - 18), "ESC: 返回  |  回车: 播放");

    // V4.0: 歌曲加载中提示（强制渲染一帧避免卡死错觉）
    if (songLoading) {
        sf::RectangleShape loadMask({panelW, panelH});
        loadMask.setPosition({panelX, panelY});
        loadMask.setFillColor(sf::Color(0, 0, 0, 130));
        g_window->draw(loadMask);

        settextcolor(RGB(0, 255, 200));
        settextstyle(18, 0, "Consolas");
        const char* loadText = "正在分析音频...";
        int lw = textwidth(loadText);
        outtextxy((int)(panelX + (panelW - lw) / 2), (int)(panelY + panelH / 2 - 20), loadText);

        settextcolor(RGB(140, 140, 160));
        settextstyle(12, 0, "Consolas");
        const char* subText = "首次分析可能需要数秒，请稍候";
        int sw = textwidth(subText);
        outtextxy((int)(panelX + (panelW - sw) / 2), (int)(panelY + panelH / 2 + 8), subText);
    }
}

// ========== 代管轨道选择界面 ==========
void GameWindow::drawTrackDelegateScreen() {
    using namespace _easyx_impl;
    
    // 半透明遮罩
    sf::RectangleShape overlay({(float)width, (float)height});
    overlay.setFillColor(sf::Color(10, 10, 25, 220));
    g_window->draw(overlay);
    
    float panelW = 450.0f, panelH = 420.0f;
    float panelX = (width - panelW) / 2.0f;
    float panelY = (height - panelH) / 2.0f;
    
    sf::RectangleShape panel({panelW, panelH});
    panel.setPosition({panelX, panelY});
    panel.setFillColor(sf::Color(20, 20, 40, 240));
    panel.setOutlineColor(sf::Color(0, 200, 200, 100));
    panel.setOutlineThickness(2.0f);
    g_window->draw(panel);
    
    // 标题
    settextcolor(RGB(0, 255, 200));
    settextstyle(22, 0, "Consolas");
    outtextxy((int)(panelX + (panelW - textwidth("代管轨道")) / 2), (int)(panelY + 20), "代管轨道");
    
    settextcolor(RGB(150, 150, 180));
    settextstyle(13, 0, "Consolas");
    outtextxy((int)(panelX + (panelW - textwidth("空格: 切换  |  回车: 开始  |  ESC: 返回")) / 2), (int)(panelY + 50), "空格: 切换  |  回车: 开始  |  ESC: 返回");
    
    // 轨道名称映射
    const char* trackNames[] = {"底鼓", "踩镲", "吊镲", "军鼓", "通鼓", "叮镲"};
    const char* trackKeys[] = {"A", "S", "D", "F", "J", "K"};
    sf::Color trackColors[] = {
        sf::Color(255, 100, 100), sf::Color(100, 255, 100), sf::Color(255, 255, 100),
        sf::Color(100, 100, 255), sf::Color(255, 180, 50), sf::Color(200, 100, 255)
    };
    
    float startY = panelY + 100.0f;
    float itemH = 32.0f;
    
    for (int i = 0; i < 6; i++) {
        float y = startY + i * itemH;
        bool sel = (i == trackDelegateSel);
        bool on = trackAutoPlay[i];
        
        // 悬浮检测
        bool hovered = (mouseX >= panelX + 30 && mouseX <= panelX + panelW - 30 &&
                        mouseY >= y && mouseY <= y + itemH);
        if (hovered) trackDelegateSel = i;
        
        // 行背景
        sf::RectangleShape row({panelW - 60, itemH - 2});
        row.setPosition({panelX + 30, y});
        if (sel) {
            row.setFillColor(sf::Color(0, 70, 70, 200));
            row.setOutlineColor(sf::Color(0, 220, 220, 150));
        } else if (hovered) {
            row.setFillColor(sf::Color(0, 80, 80, 150));
            row.setOutlineColor(sf::Color(0, 180, 180, 100));
        } else {
            row.setFillColor(sf::Color(30, 30, 45, 120));
            row.setOutlineColor(sf::Color(50, 50, 70, 50));
        }
        row.setOutlineThickness(1.0f);
        g_window->draw(row);
        
        // 轨道颜色标记
        sf::CircleShape dot(6);
        dot.setPosition({panelX + 45, y + 10});
        dot.setFillColor(sf::Color(trackColors[i].r, trackColors[i].g, trackColors[i].b, 220));
        g_window->draw(dot);
        
        // 轨道名 + 按键
        char label[64];
        snprintf(label, sizeof(label), "%s  [%s]", trackNames[i], trackKeys[i]);
        settextcolor(RGB(trackColors[i].r, trackColors[i].g, trackColors[i].b));
        settextstyle(16, 0, "Consolas");
        outtextxy((int)(panelX + 65), (int)(y + 6), label);
        
        // ON/OFF 状态
        if (on) {
            settextcolor(RGB(0, 255, 150));
            settextstyle(15, 0, "Consolas");
            outtextxy((int)(panelX + panelW - 120), (int)(y + 8), "[代管]");
        } else {
            settextcolor(RGB(120, 120, 140));
            settextstyle(15, 0, "Consolas");
            outtextxy((int)(panelX + panelW - 120), (int)(y + 8), "[演奏]");
        }
    }
    
    // 确认按钮（含悬浮高亮）
    float btnY = startY + 6 * itemH + 30;
    float btnBX = panelX + 100, btnBW = panelW - 200, btnBH = 36.0f;
    bool btnHover = (mouseX >= btnBX && mouseX <= btnBX + btnBW &&
                     mouseY >= btnY && mouseY <= btnY + btnBH);
    sf::RectangleShape btn({btnBW, btnBH});
    btn.setPosition({btnBX, btnY});
    btn.setFillColor(btnHover ? sf::Color(0, 160, 160, 240) : sf::Color(0, 120, 120, 200));
    btn.setOutlineColor(sf::Color(0, 220, 220, btnHover ? 220 : 150));
    btn.setOutlineThickness(btnHover ? 2.0f : 1.0f);
    g_window->draw(btn);
    
    settextcolor(btnHover ? RGB(0, 255, 220) : RGB(0, 255, 200));
    settextstyle(17, 0, "Consolas");
    outtextxy((int)(panelX + (panelW - textwidth("开始游戏")) / 2), (int)(btnY + 10), "开始游戏");
}

// ========== 游玩说明界面 ==========

void GameWindow::drawTutorialScreen() {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) return;

    // 背景
    sf::RectangleShape bg({(float)width, (float)height});
    bg.setFillColor(sf::Color(10, 10, 25));
    g_window->draw(bg);

    // 标题
    settextcolor(RGB(0, 255, 200));
    settextstyle(32, 0, "Consolas");
    const char* title = "游玩说明";
    outtextxy((width - textwidth(title)) / 2, 25, title);

    // 分割线
    sf::RectangleShape line({(float)(width - 100), 1.0f});
    line.setPosition({50.0f, 65.0f});
    line.setFillColor(sf::Color(0, 180, 180, 80));
    g_window->draw(line);

    int y = 80;
    int xLeft = 50;
    int xContent = 65;
    int xCol2 = 470;  // 右列起始
    int lineH = 17;

    auto drawSection = [&](int x, const char* title, int& y) {
        settextcolor(RGB(0, 220, 200));
        settextstyle(16, 0, "Consolas");
        outtextxy(x, y, title);
        y += 22;
    };

    auto drawText = [&](int x, const char* text, int& y) {
        settextcolor(RGB(170, 180, 200));
        settextstyle(13, 0, "Consolas");
        outtextxy(x, y, text);
        y += lineH;
    };

    auto drawKey = [&](int x, const char* label, const char* desc, int& y) {
        settextcolor(RGB(0, 255, 200));
        settextstyle(13, 0, "Consolas");
        outtextxy(x, y, label);
        settextcolor(RGB(140, 150, 170));
        outtextxy(x + textwidth(label) + 6, y, desc);
        y += lineH;
    };

    // ========== 左列 ==========
    int ly = y;

    drawSection(xLeft, "= 基本玩法 =", ly);
    drawText(xContent, "6键下落式节奏游戏", ly);
    drawText(xContent, "音符下落 → 到达判定线 → 按键得分", ly);
    drawText(xContent, "A/S/D/F/J/K 六键对应六条轨道", ly);
    ly += 6;

    drawSection(xLeft, "= 按键绑定 =", ly);
    drawKey(xContent, "1 底鼓:", "A", ly);
    drawKey(xContent, "2 踩镲:", "S", ly);
    drawKey(xContent, "3 吊镲:", "D", ly);
    drawKey(xContent, "4 军鼓:", "F", ly);
    drawKey(xContent, "5 通鼓:", "J", ly);
    drawKey(xContent, "6 叮镲:", "K", ly);
    drawText(xContent, "(可在设置中自定义)", ly);
    ly += 6;

    drawSection(xLeft, "= 判定系统 =", ly);
    drawText(xContent, "Perfect  <=50ms   100分+连击", ly);
    drawText(xContent, "Good     <=150ms  50分+连击", ly);
    drawText(xContent, "Miss     >150ms   0分+断连", ly);
    ly += 6;

    drawSection(xLeft, "= 导航说明 =", ly);
    drawKey(xContent, "W/S:", "上下选择", ly);
    drawKey(xContent, "Enter:", "确认/开始", ly);
    drawKey(xContent, "ESC:", "返回/暂停", ly);
    drawKey(xContent, "F1/F2:", "微调偏移 -/+10ms", ly);
    ly += 6;

    // ========== 右列 ==========
    int ry = y;

    drawSection(xCol2, "= 自定义歌曲 =", ry);
    drawText(xCol2 + 15, "MP3 放入 songs 文件夹", ry);
    drawText(xCol2 + 15, "在「歌曲列表」中选择播放", ry);
    drawText(xCol2 + 15, "首次播放自动分析生成谱面", ry);
    ry += 6;

    drawSection(xCol2, "= 点拍模式 =", ry);
    drawText(xCol2 + 15, "无谱面歌曲按 T 进入点拍", ry);
    drawText(xCol2 + 15, "跟着节奏按 T 标记节拍", ry);
    drawText(xCol2 + 15, "系统自动生成节拍谱面", ry);
    ry += 6;

    drawSection(xCol2, "= 代管轨道 =", ry);
    drawText(xCol2 + 15, "选歌后进入代管轨道界面", ry);
    drawText(xCol2 + 15, "空格切换轨道自动/手动", ry);
    drawText(xCol2 + 15, "不想玩的轨道设为自动代打", ry);
    drawText(xCol2 + 15, "回车确认后开始游戏", ry);
    ry += 6;

    drawSection(xCol2, "= P键自动演示 =", ry);
    drawText(xCol2 + 15, "游戏中按 P 切换自动模式", ry);
    drawText(xCol2 + 15, "自动模式仅演奏代管轨道", ry);
    drawText(xCol2 + 15, "手动轨道仍需自己按键", ry);
    ry += 6;

    drawSection(xCol2, "= 难度说明 =", ry);
    drawText(xCol2 + 15, "普通: 标准密度 新手入门", ry);
    drawText(xCol2 + 15, "困难: 密度提升 音符更多", ry);
    drawText(xCol2 + 15, "专家: 最高密度 节奏挑战", ry);
    ry += 6;

    drawSection(xCol2, "= 小技巧 =", ry);
    drawText(xCol2 + 15, "连击越高分数越多！", ry);
    drawText(xCol2 + 15, "看准判定线再出手", ry);
    drawText(xCol2 + 15, "从普通难度开始练", ry);

    // 底部返回提示
    settextcolor(RGB(80, 80, 100));
    settextstyle(14, 0, "Consolas");
    const char* hint = "按 ESC 返回主菜单";
    outtextxy((width - textwidth(hint)) / 2, height - 35, hint);
}

// ========== 主循环 ==========

void GameWindow::run() {
    const int FRAME_TIME = 1000 / fps;
    debugLog("run: entering main loop");

    while (isRunning) {
        long long frameStart = GetTickCount64();

        // 更新鼠标坐标
        {
            using namespace _easyx_impl;
            if (g_window && g_windowOpen) {
                sf::Vector2i mp = sf::Mouse::getPosition(*g_window);
                mouseX = (float)mp.x;
                mouseY = (float)mp.y;
            }
        }

        if (gameState == MENU) {
            if (isInSettings) {
                handleSettingsInput();
            } else if (showAchievements) {
                handleAchievementsInput();
            } else if (showTutorial) {
                handleTutorialInput();
            } else if (isShowingSongList) {
                handleSongListInput();
            } else {
                handleMenuInput();
            }
        } else if (gameState == ANALYZING) {
            handleAnalysisInput();
        } else if (gameState == TRACK_DELEGATE) {
            handleTrackDelegateInput();
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
