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
        "Difficulty: Normal",
        "Exit"
    };
    char diffStr[64];
    snprintf(diffStr, sizeof(diffStr), "Difficulty: %s", getDifficultyName());
    menuItems[2] = diffStr;

    int menuY = 190;
    int itemH = 55;

    for (int i = 0; i < 4; i++) {
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
        drawMenuScreen();
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
            tracks[i]->draw(textures, keyGlowAlpha[i]);
        }
        for (int i = 0; i < TRACK_COUNT; i++) {
            tracks[i]->drawKeyButton(textures, keyPressed[i]);
        }
        drawAnimations();
        drawUI();
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
        float displayW = 80.0f;  // 音符显示宽度
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
        float displayW = 180.0f * scale;  // 文字显示宽度
        float displayH = displayW * texH / texW;  // 按比例计算高度

        sf::Color bgColor;
        if (anim.texture == &textures.textPerfect)
            bgColor = sf::Color(255, 200, 0);
        else if (anim.texture == &textures.textGood)
            bgColor = sf::Color(80, 220, 80);
        else
            bgColor = sf::Color(255, 60, 60);

        sf::RectangleShape bg({displayW + 12, displayH + 8});
        bg.setPosition({anim.x - displayW/2 - 6, anim.y - displayH/2 - 4});
        bg.setFillColor(sf::Color(bgColor.r, bgColor.g, bgColor.b, (uint8_t)(alpha * 0.85f)));
        bg.setOutlineColor(sf::Color(255, 255, 255, alpha));
        bg.setOutlineThickness(3.0f);
        g_window->draw(bg);

        sf::Sprite sprite(*anim.texture);
        sprite.setOrigin({texW / 2.0f, texH / 2.0f});
        sprite.setScale({displayW / texW, displayH / texH});
        sprite.setPosition({anim.x, anim.y});
        sprite.setColor(sf::Color(255, 255, 255, alpha));
        g_window->draw(sprite);
    }
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
    if (scoreSystem.getCurrentCombo() > 0) {
        int baseComboSize = 28 + std::min(scoreSystem.getCurrentCombo() / 10, 5) * 4;

        float comboScale = 1.0f;
        if (comboAnim.elapsed < comboAnim.duration) {
            float t = comboAnim.elapsed / comboAnim.duration;
            comboScale = 1.0f + 0.3f * std::cos(t * 3.14159f * 2.0f) * (1.0f - t);
        }
        int comboSize = (int)(baseComboSize * comboScale);

        float colorT = 1.0f;
        if (comboAnim.elapsed < comboAnim.duration) {
            colorT = comboAnim.elapsed / comboAnim.duration;
        }
        int r = 255;
        int g = (int)(165 + (255 - 165) * colorT);
        int b = (int)(0 + 255 * colorT);

        if (scoreSystem.getCurrentCombo() >= 100) { r = 255; g = 50; b = 50; }
        else if (scoreSystem.getCurrentCombo() >= 50) { r = 255; g = 215; b = 0; }

        settextstyle(comboSize, 0, "Consolas");
        settextcolor(RGB(r, g, b));
        char comboStr[32];
        snprintf(comboStr, sizeof(comboStr), "%d COMBO", scoreSystem.getCurrentCombo());
        int comboW = textwidth(comboStr);

        // Combo底板
        sf::RectangleShape comboPanel({(float)(comboW + 20), (float)(comboSize + 10)});
        comboPanel.setPosition({(float)(width - comboW - 30), 10.0f});
        comboPanel.setFillColor(sf::Color(0, 0, 0, 100));
        g_window->draw(comboPanel);

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

// ========== 结算界面（V2.0增强：显示排行榜） ==========

void GameWindow::drawResultScreen() {
    setfillcolor(RGB(0, 0, 0));
    fillrectangle(0, 0, width, height);

    // 标题
    settextcolor(RGB(255, 255, 255));
    settextstyle(36, 0, "Consolas");
    const char* title = "SONG COMPLETE!";
    int titleW = textwidth(title);
    outtextxy((width - titleW) / 2, 40, title);

    // 歌曲名和难度
    settextcolor(RGB(150, 150, 180));
    settextstyle(16, 0, "Consolas");
    char songStr[128];
    snprintf(songStr, sizeof(songStr), "%s [%s]",
             currentSongName.c_str(), getDifficultyName());
    int songW = textwidth(songStr);
    outtextxy((width - songW) / 2, 85, songStr);

    // 评级
    const char* rating = getRating();
    COLORREF ratingColor = getRatingColor();
    settextstyle(80, 0, "Consolas");
    settextcolor(ratingColor);
    int ratingW = textwidth(rating);
    outtextxy((width - ratingW) / 2, 110, rating);

    // 最终分数
    settextstyle(28, 0, "Consolas");
    settextcolor(RGB(255, 255, 255));
    char finalScoreStr[64];
    snprintf(finalScoreStr, sizeof(finalScoreStr), "Final Score: %d", scoreSystem.getTotalScore());
    int scoreW = textwidth(finalScoreStr);
    outtextxy((width - scoreW) / 2, 210, finalScoreStr);

    // 最高Combo
    settextstyle(22, 0, "Consolas");
    settextcolor(RGB(255, 215, 0));
    char maxComboStr[32];
    snprintf(maxComboStr, sizeof(maxComboStr), "Max Combo: %d", scoreSystem.getMaxCombo());
    int maxComboW = textwidth(maxComboStr);
    outtextxy((width - maxComboW) / 2, 250, maxComboStr);

    // 判定统计
    settextstyle(18, 0, "Consolas");
    int statsY = 290;
    int lineH = 26;

    settextcolor(RGB(255, 215, 0));
    char perfectStr[32];
    snprintf(perfectStr, sizeof(perfectStr), "Perfect:  %d", scoreSystem.getPerfectCount());
    outtextxy((width - textwidth(perfectStr)) / 2, statsY, perfectStr);

    settextcolor(RGB(100, 255, 100));
    char goodStr[32];
    snprintf(goodStr, sizeof(goodStr), "Good:     %d", scoreSystem.getGoodCount());
    outtextxy((width - textwidth(goodStr)) / 2, statsY + lineH, goodStr);

    settextcolor(RGB(255, 80, 80));
    char missStr[32];
    snprintf(missStr, sizeof(missStr), "Miss:     %d", scoreSystem.getMissCount());
    outtextxy((width - textwidth(missStr)) / 2, statsY + lineH * 2, missStr);

    // 排行榜显示（V2.0新增）
    int lbY = statsY + lineH * 3 + 15;
    settextcolor(RGB(0, 255, 255));
    settextstyle(16, 0, "Consolas");
    const char* lbTitle = "--- Leaderboard ---";
    outtextxy((width - textwidth(lbTitle)) / 2, lbY, lbTitle);

    auto leaderboard = dataManager.getLeaderboard(currentSongName, (int)currentDifficulty, 3);
    int entryY = lbY + 25;
    int rank = 1;
    for (const auto& entry : leaderboard) {
        settextcolor(rank == 1 ? RGB(255, 215, 0) : RGB(180, 180, 180));
        settextstyle(14, 0, "Consolas");
        char entryStr[128];
        snprintf(entryStr, sizeof(entryStr), "#%d  Score: %d  Combo: %d  Plays: %d",
                 rank, entry.highScore, entry.maxCombo, entry.playCount);
        outtextxy((width - textwidth(entryStr)) / 2, entryY, entryStr);
        entryY += 20;
        rank++;
    }

    if (leaderboard.empty()) {
        settextcolor(RGB(100, 100, 100));
        settextstyle(14, 0, "Consolas");
        const char* noData = "No records yet";
        outtextxy((width - textwidth(noData)) / 2, entryY, noData);
    }

    // 提示
    settextcolor(RGB(120, 120, 120));
    settextstyle(16, 0, "Consolas");
    const char* hint1 = "ENTER: Retry  |  ESC: Menu";
    outtextxy((width - textwidth(hint1)) / 2, height - 45, hint1);
}

// ========== 主循环 ==========

void GameWindow::run() {
    const int FRAME_TIME = 1000 / fps;

    while (isRunning) {
        long long frameStart = GetTickCount64();

        if (gameState == MENU) {
            handleMenuInput();
        } else if (gameState == PLAYING) {
            handleInput();
            if (!isRunning) break;
            if (gameState == PLAYING) update();
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
