/**
 * @file GameWindow_input.cpp
 * @brief GameWindow输入处理 - V2.0
 * @details 包含：菜单输入、游戏输入、结算输入
 */

// 注意：此文件内容将合并到GameWindow.cpp中编译
// 由于Makefile只编译GameWindow.cpp，需要将此文件include到GameWindow.cpp末尾
// 或者在Makefile中添加此文件

#define VK_UP 0x26
#define VK_DOWN 0x28
#define VK_BACK 0x08
#define VK_TAB 0x09
#define VK_CLEAR 0x0C
#define VK_RETURN 0x0D
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_PAUSE 0x13
#define VK_CAPITAL 0x14
#define VK_ESCAPE 0x1B
#define VK_SPACE 0x20
#define VK_PRIOR 0x21
#define VK_NEXT 0x22
#define VK_END 0x23
#define VK_HOME 0x24
#define VK_LEFT 0x25
#define VK_RIGHT 0x27
#define VK_SELECT 0x29
#define VK_PRINT 0x2A
#define VK_EXECUTE 0x2B
#define VK_SNAPSHOT 0x2C
#define VK_INSERT 0x2D
#define VK_DELETE 0x2E
#define VK_HELP 0x2F
#define VK_LWIN 0x5B
#define VK_RWIN 0x5C
#define VK_APPS 0x5D
#define VK_SLEEP 0x5F
#define VK_NUMPAD0 0x60
#define VK_NUMPAD1 0x61
#define VK_NUMPAD2 0x62
#define VK_NUMPAD3 0x63
#define VK_NUMPAD4 0x64
#define VK_NUMPAD5 0x65
#define VK_NUMPAD6 0x66
#define VK_NUMPAD7 0x67
#define VK_NUMPAD8 0x68
#define VK_NUMPAD9 0x69
#define VK_MULTIPLY 0x6A
#define VK_ADD 0x6B
#define VK_SEPARATOR 0x6C
#define VK_SUBTRACT 0x6D
#define VK_DECIMAL 0x6E
#define VK_DIVIDE 0x6F
#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_F7 0x76
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F10 0x79
#define VK_F11 0x7A
#define VK_F12 0x7B
#define VK_NUMLOCK 0x90
#define VK_SCROLL 0x91
#define VK_LSHIFT 0xA0
#define VK_RSHIFT 0xA1
#define VK_LCONTROL 0xA2
#define VK_RCONTROL 0xA3
#define VK_LMENU 0xA4
#define VK_RMENU 0xA5

#include "GameWindow.h"
#include <cstdio>
#include <algorithm>
#include <filesystem>
#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#include <mach-o/dyld.h>
#endif

// MSVC兼容
#ifdef _MSC_VER
#define popen _popen
#define pclose _pclose
#endif

// ========== 鼠标点击辅助函数 ==========

bool GameWindow::isMouseClick() {
    bool pressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    bool clicked = pressed && !mouseWasPressed;
    mouseWasPressed = pressed;
    return clicked;
}

bool GameWindow::isPointInRect(float px, float py, float rx, float ry, float rw, float rh) {
    return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
}

// ========== 菜单输入处理（V2.0新增） ==========

void GameWindow::handleMenuInput() {
    if (!processWindowEvents()) {
        isRunning = false;
        return;
    }

    // 上下选择（每键独立 debounce）
    static bool prevW = false, prevS = false, prevEnter = false;

    bool upPressed = (GetAsyncKeyState('W') & 0x8000) != 0;
    bool downPressed = (GetAsyncKeyState('S') & 0x8000) != 0;
    bool enterPressed = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;

    // 从子页面返回菜单时同步按键状态，防止Enter穿透
    if (menuJustOpened) {
        menuJustOpened = false;
        prevW = upPressed; prevS = downPressed;
        prevEnter = enterPressed;
        mouseWasPressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
        return;
    }

    bool upJust = upPressed && !prevW;
    bool downJust = downPressed && !prevS;
    bool enterJust = enterPressed && !prevEnter;
    prevW = upPressed; prevS = downPressed; prevEnter = enterPressed;

    if (upJust) {
        menuSelection--;
        if (menuSelection < 0) menuSelection = 6;
    }
    if (downJust) {
        menuSelection++;
        if (menuSelection > 4) menuSelection = 0;
    }
    if (enterJust) {
        switch (menuSelection) {
            case 0: // 歌曲列表
                refreshSongList();
                isShowingSongList = true;
                songListSelection = 0;
                songListJustOpened = true;  // 防止 Enter 粘滞
                break;

            case 1: // 成就
                showAchievements = true;
                break;

            case 2: // 进入设置界面
                isInSettings = true;
                settingsMenuSelection = 0;
                settingsJustOpened = true;
                break;

            case 3: // 难度选择 - 循环切换
                difficultySelection = (difficultySelection + 1) % 3;
                currentDifficulty = (Difficulty)difficultySelection;
                break;

            case 4: // 退出
                isRunning = false;
                break;
        }
    }

    // 鼠标点击菜单项：单击直接选中并确认
    if (isMouseClick()) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(*_easyx_impl::g_window);
        float mx = (float)mousePos.x;
        float my = (float)mousePos.y;
        float menuX = (float)((width - 300) / 2);
        int menuY = 170;
        int itemH = 48;

        for (int i = 0; i < 5; i++) {
            float itemY = (float)(menuY + i * itemH);
            if (isPointInRect(mx, my, menuX, itemY - 5.0f, 300.0f, 40.0f)) {
                menuSelection = i;
                // 直接执行对应操作
                switch (i) {
                    case 0:
                        refreshSongList();
                        isShowingSongList = true;
                        songListSelection = 0;
                        songListJustOpened = true;
                        break;
                    case 1:
                        showAchievements = true;
                        break;
                    case 2:
                        isInSettings = true;
                        settingsMenuSelection = 0;
                        settingsJustOpened = true;
                        break;
                    case 3:
                        difficultySelection = (difficultySelection + 1) % 3;
                        currentDifficulty = (Difficulty)difficultySelection;
                        break;
                    case 4:
                        isRunning = false;
                        break;
                }
                break;
            }
        }
    }

}

// ========== V3.2: 开始歌曲分析 ==========
void GameWindow::startAnalysis() {
    printf("[GameWindow] 开始分析: %s\n", analysisFilePath.c_str());
    fflush(stdout);
    analysisResult = songAnalyzer.analyze(analysisFilePath);
    analysisDone = analysisResult.success;
    analysisMenuSelection = 0;
    manualBPM = analysisResult.bpm;
    manualOffset = analysisResult.beatOffset;
    isInTapping = false;
    songAnalyzer.resetTapTempo();
    gameState = ANALYZING;
}

// ========== V3.2: 加载歌曲进入游戏 ==========
void GameWindow::loadSongForPlaying() {
    tracks.clear();
    scoreSystem.reset();
    initTracks();

    noteTimeData = analysisResult.chart;
    if (!noteTimeData.empty()) {
        lastNoteTime = noteTimeData.back().first;
    }
    currentSongName = analysisResult.metadata.title;

    double speed = getDifficultySpeed();
    for (const auto& [timeMs, track] : noteTimeData) {
        auto note = std::make_unique<NormalNote>(
            track, timeMs, JUDGE_Y, speed, TRACK_COLORS[track]
        );
        tracks[track]->addNote(std::move(note));
    }

    currentBPM = analysisResult.bpm;
    bpmPulsePhase = 0.0f;

    // 优先播放原曲，失败则用合成BGM
    if (!audioManager.playOriginalSong(analysisFilePath)) {
        audioManager.generateSyncedBGM(noteTimeData);
    }
    // 不立即开始：等代管选轨确认后再启动
}

void GameWindow::startDelegatedGame() {
    gameStartTime = GetTickCount64();
    currentTime = 0;
    gameState = PLAYING;
    prevCombo = 0;
    hitAnims.clear();
    textAnims.clear();
    audioManager.playBGM();
}

// ========== V3.2: 分析界面输入处理 ==========
void GameWindow::handleAnalysisInput() {
    // 只处理窗口关闭事件，不消费键盘事件
    {
        using namespace _easyx_impl;
        if (g_window && g_windowOpen) {
            while (const auto event = g_window->pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    g_windowOpen = false;
                    isRunning = false;
                    return;
                }
            }
        }
    }

    // 帧间差分检测按键（不依赖静态变量，避免状态丢失）
    bool upPressed = (GetAsyncKeyState('W') & 0x8000) != 0;
    bool downPressed = (GetAsyncKeyState('S') & 0x8000) != 0;
    bool leftPressed = (GetAsyncKeyState('A') & 0x8000) != 0;
    bool rightPressed = (GetAsyncKeyState('D') & 0x8000) != 0;
    bool enterPressed = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
    bool escPressed = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
    bool rPressed = (GetAsyncKeyState('R') & 0x8000) != 0;
    bool tPressed = (GetAsyncKeyState('T') & 0x8000) != 0;

    static bool prevUp = false, prevDown = false, prevLeft = false, prevRight = false;
    static bool prevEnter = false, prevEsc = false, prevR = false, prevT = false;

    // 刚进入分析界面时同步所有按键状态，防止上一屏的按键穿透
    if (analysisJustOpened) {
        analysisJustOpened = false;
        prevUp = upPressed; prevDown = downPressed;
        prevLeft = leftPressed; prevRight = rightPressed;
        prevEnter = enterPressed; prevEsc = escPressed;
        prevR = rPressed; prevT = tPressed;
        mouseWasPressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
        return;
    }

    bool upJustPressed = upPressed && !prevUp;
    bool downJustPressed = downPressed && !prevDown;
    bool leftJustPressed = leftPressed && !prevLeft;
    bool rightJustPressed = rightPressed && !prevRight;
    bool enterJustPressed = enterPressed && !prevEnter;
    bool escJustPressed = escPressed && !prevEsc;
    bool rJustPressed = rPressed && !prevR;
    bool tJustPressed = tPressed && !prevT;

    prevUp = upPressed; prevDown = downPressed; prevLeft = leftPressed; prevRight = rightPressed;
    prevEnter = enterPressed; prevEsc = escPressed; prevR = rPressed; prevT = tPressed;

    // 调试
    static bool dbgW = false, dbgS = false;
    if (upPressed != dbgW || downPressed != dbgS) {
        printf("[Analysis] W=%d S=%d upJ=%d dnJ=%d sel=%d\n", upPressed, downPressed, upJustPressed, downJustPressed, analysisMenuSelection);
        fflush(stdout);
        dbgW = upPressed; dbgS = downPressed;
    }

    if (!analysisDone) return;

    // T键
    if (tJustPressed) {
        if (!isInTapping) {
            // 第一次按T：进入点拍模式并开始第一次点拍
            isInTapping = true;
            songAnalyzer.resetTapTempo();
            songAnalyzer.tapTempo(GetTickCount64());
            printf("[点拍] 第1次，继续按T\n");
            fflush(stdout);
        } else {
            // 点拍模式中按T：继续点拍
            float tapBPM = songAnalyzer.tapTempo(GetTickCount64());
            int cnt = songAnalyzer.getTapCount();
            printf("[点拍] 第%d次", cnt);
            if (tapBPM > 0) {
                manualBPM = tapBPM;
                songAnalyzer.regenerateChart(analysisResult, manualBPM, manualOffset);
                printf(" -> BPM: %.1f", tapBPM);
            }
            printf("\n");
            fflush(stdout);
        }
    }

    // ESC在点拍模式下退出点拍，否则返回菜单
    if (escJustPressed) {
        if (isInTapping) {
            isInTapping = false;
            printf("[点拍模式] 已退出\n");
            fflush(stdout);
        } else {
            gameState = MENU;
            return;
        }
    }

    // R键：重置分析
    if (rJustPressed) {
        startAnalysis();
        return;
    }

    // 上下切换选项
    if (upJustPressed) {
        analysisMenuSelection--;
        if (analysisMenuSelection < 0) analysisMenuSelection = 4;
    }

    if (downJustPressed) {
        analysisMenuSelection++;
        if (analysisMenuSelection > 4) analysisMenuSelection = 0;
    }

    // 左右调整BPM/偏移
    if (analysisMenuSelection == 0) {
        // BPM调整
        if (leftJustPressed) {
            manualBPM = std::max(60.0f, manualBPM - 0.5f);
            songAnalyzer.regenerateChart(analysisResult, manualBPM, manualOffset);
        }
        if (rightJustPressed) {
            manualBPM = std::min(200.0f, manualBPM + 0.5f);
            songAnalyzer.regenerateChart(analysisResult, manualBPM, manualOffset);
        }
    } else if (analysisMenuSelection == 1) {
        // 偏移调整
        if (leftJustPressed) {
            manualOffset -= 5.0f;
            songAnalyzer.regenerateChart(analysisResult, manualBPM, manualOffset);
        }
        if (rightJustPressed) {
            manualOffset += 5.0f;
            songAnalyzer.regenerateChart(analysisResult, manualBPM, manualOffset);
        }
    }

    // ENTER确认
    if (enterJustPressed) {
        switch (analysisMenuSelection) {
            case 2: // 开始游戏 → 先选代管轨道
                loadSongForPlaying();
                gameState = TRACK_DELEGATE;
                trackDelegateJustOpened = true;
                for (int i = 0; i < 6; i++) trackAutoPlay[i] = false;
                trackDelegateSel = 0;
                break;
            case 3: { // 导出JSON
                std::string outPath = analysisFilePath + ".chart.json";
                SongAnalyzer::exportChart(outPath, analysisResult);
                printf("谱面已导出: %s\n", outPath.c_str());
                fflush(stdout);
                break;
            }
            case 4: // 返回菜单
                gameState = MENU;
                menuJustOpened = true;
                break;
        }
    }

    // 鼠标点击分析界面按钮：第一次选中，第二次确认
    {
        bool mpressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
        static bool mlast = false;
        if (mpressed && !mlast) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(*_easyx_impl::g_window);
            printf("[Analysis] Mouse click at (%d, %d)\n", mousePos.x, mousePos.y);
            fflush(stdout);
        }
        mlast = mpressed;
    }
    if (isMouseClick()) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(*_easyx_impl::g_window);
        float mx = (float)mousePos.x;
        float my = (float)mousePos.y;

        // 和渲染代码完全一致的布局计算
        float panelW = 700.0f, panelH = 550.0f;
        float panelX = (width - panelW) / 2.0f;
        float panelY = (height - panelH) / 2.0f;
        float contentX = panelX + 30;
        float contentY = panelY + 65;
        // 歌曲信息：header + title + artist + duration
        contentY += 22 + 22 + 22 + 35;
        // BPM分析：header + (bpm+bar+warning 整块)
        contentY += 22 + 50;
        // 偏移：header + value
        contentY += 22 + 35;
        // 节拍统计：header + stats + noteCount
        contentY += 22 + 22 + 35;
        // 时间线：header + timeline + spacing
        contentY += 20 + 60.0f + 15.0f;

        float btnW = 120.0f, btnH = 36.0f;
        float btnSpacing = 15.0f;
        float btnBaseY = contentY;
        float btnStartX = contentX;

        for (int i = 0; i < 5; i++) {
            float bx = btnStartX + i * (btnW + btnSpacing);
            if (isPointInRect(mx, my, bx, btnBaseY, btnW, btnH)) {
                analysisMenuSelection = i;
                // hover已选中，单击直接执行
                switch (i) {
                    case 0: case 1: // BPM/偏移 - 用A/D调整，仅选中
                        break;
                    case 2:
                        loadSongForPlaying();
                        gameState = TRACK_DELEGATE;
                        trackDelegateJustOpened = true;
                        return;
                    case 3: {
                        std::string outPath = analysisFilePath + ".chart.json";
                        SongAnalyzer::exportChart(outPath, analysisResult);
                        printf("谱面已导出: %s\n", outPath.c_str());
                        fflush(stdout);
                        break;
                    }
                    case 4:
                        gameState = MENU;
                        return;
                }
                break;
            }
        }
    }
}

// ========== 游戏输入处理 ==========

void GameWindow::handleInput() {
    if (!processWindowEvents()) {
        isRunning = false;
        return;
    }

    // 读取按键状态并更新发光Alpha
    for (int i = 0; i < TRACK_COUNT; i++) {
        SHORT keyState = GetAsyncKeyState(customKeys[i]);
        keyPressed[i] = (keyState & 0x8000) != 0;

        if (keyPressed[i]) {
            keyGlowAlpha[i] = 255;
            // V3.0: 按键按下反馈 - 触发发光计时器
            keyFeedback[i].glowTimer = KeyFeedback::GLOW_DURATION;

            if (!keyWasPressed[i]) {
                // V3.5: 打击乐模式按键即时发声
                if (audioManager.getSoundPack() == 1) {
                    audioManager.playHit(true, i);
                }

                int comboBefore = scoreSystem.getCurrentCombo();

                NoteTrack::JudgeResult result = tracks[i]->handlePress(currentTime);

                if (result.judgement != NONE) {
                    scoreSystem.addJudgement(result.judgement);
                    lastJudgement = result.judgement;
                    judgementDisplayTimer = 30;

                    spawnHitAnim(i, result.noteY, result.judgement);
                    spawnTextPopup(i, result.judgement);

                    // V3.0: 判定粒子特效
                    spawnParticles(getTrackX(i) + TRACK_WIDTH / 2.0f, result.noteY, result.judgement);

                    if (result.judgement == PERFECT) audioManager.playHit(true, i);
                    else if (result.judgement == GOOD) audioManager.playHit(false, i);
                    else if (result.judgement == MISS) audioManager.playMiss();
                }

                // V3.0: Combo断连检测
                if (comboBefore > 0 && scoreSystem.getCurrentCombo() == 0) {
                    comboBreakFlash = COMBO_BREAK_DURATION;
                }

                if (scoreSystem.getCurrentCombo() > comboBefore && comboBefore > 0) {
                    comboAnim.elapsed = 0.0f;
                }
            }
        }
        keyWasPressed[i] = keyPressed[i];
    }

    // ESC键处理
    bool escPressed = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
    if (escPressed && escKeyReleased) {
        escKeyReleased = false;
        if (gameState == PLAYING) {
            // 切换暂停/继续
            gameState = PAUSED;
            pauseStartTime = GetTickCount64();
            pauseJustOpened = true;  // 防止同一ESC按键在下一帧触发恢复
            audioManager.pauseBGM();
        } else if (gameState == PAUSED) {
            // 继续游戏
            long long pauseDuration = GetTickCount64() - pauseStartTime;
            gameStartTime += pauseDuration;
            audioManager.resumeBGM();
            gameState = PLAYING;
        } else {
            // 其他状态返回主菜单
            audioManager.stopBGM();
            gameState = MENU;
        }
    } else if (!escPressed) {
        escKeyReleased = true;
    }

    // V3.3: F1/F2 游戏内实时偏移调整 (±10ms)
    if (gameState == PLAYING) {
        static bool f1WasPressed = false, f2WasPressed = false;
        bool f1Pressed = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;
        bool f2Pressed = (GetAsyncKeyState(VK_F2) & 0x8000) != 0;

        if (f1Pressed && !f1WasPressed) {
            gameOffsetMs -= 10.0f;
            // 实时移动所有音符
            for (auto& track : tracks) {
                track->applyTimeOffset(-10.0);
            }
            printf("[GameWindow] Offset: %.0fms\n", gameOffsetMs);
            fflush(stdout);
        }
        if (f2Pressed && !f2WasPressed) {
            gameOffsetMs += 10.0f;
            for (auto& track : tracks) {
                track->applyTimeOffset(10.0);
            }
            printf("[GameWindow] Offset: %.0fms\n", gameOffsetMs);
            fflush(stdout);
        }
        f1WasPressed = f1Pressed;
        f2WasPressed = f2Pressed;

        // V3.7: P键切换自动演示模式
        static bool pWasPressed = false;
        bool pPressed = (GetAsyncKeyState('P') & 0x8000) != 0;
        if (pPressed && !pWasPressed) {
            autoPlay = !autoPlay;
            printf("[GameWindow] AutoPlay: %s\n", autoPlay ? "ON" : "OFF");
            fflush(stdout);
        }
        pWasPressed = pPressed;
    }
}

// ========== 结算输入处理 ==========

void GameWindow::handleResultInput() {
    if (!processWindowEvents()) {
        isRunning = false;
        return;
    }

    static bool keyUpWasPressed = false;
    static bool keyDownWasPressed = false;
    static bool enterWasPressed = false;

    bool upPressed = (GetAsyncKeyState('W') & 0x8000) != 0;
    bool downPressed = (GetAsyncKeyState('S') & 0x8000) != 0;
    bool enterPressed = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
    bool escPressed = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;

    // 上键切换
    if (upPressed && !keyUpWasPressed) {
        keyUpWasPressed = true;
        resultMenuSelection = (resultMenuSelection - 1 + 2) % 2;
    } else if (!upPressed) {
        keyUpWasPressed = false;
    }

    // 下键切换
    if (downPressed && !keyDownWasPressed) {
        keyDownWasPressed = true;
        resultMenuSelection = (resultMenuSelection + 1) % 2;
    } else if (!downPressed) {
        keyDownWasPressed = false;
    }

    // ESC返回菜单
    if (escPressed) {
        // 保存排行榜后返回菜单
        dataManager.saveResult(currentSongName, (int)currentDifficulty,
                               scoreSystem.getTotalScore(), scoreSystem.getMaxCombo());
        gameState = MENU;
        return;
    }

    // 回车键确认
    if (enterPressed && !enterWasPressed) {
        // 保存排行榜
        dataManager.saveResult(currentSongName, (int)currentDifficulty,
                               scoreSystem.getTotalScore(), scoreSystem.getMaxCombo());

        if (resultMenuSelection == 0) {
            // 重新开始
            scoreSystem.reset();
            tracks.clear();
            initTracks();

            if (parseResult.success && !parseResult.noteTimeData.empty()) {
                // 重新加载MP3歌曲
                double speed = getDifficultySpeed();
                for (const auto& [timeMs, track] : parseResult.noteTimeData) {
                    auto note = std::make_unique<NormalNote>(
                        track, timeMs, JUDGE_Y, speed, TRACK_COLORS[track]
                    );
                    tracks[track]->addNote(std::move(note));
                }
                noteTimeData = parseResult.noteTimeData;
                lastNoteTime = parseResult.lastNoteTime;
            } else {
                loadDemoSong();
            }

            audioManager.generateSyncedBGM(noteTimeData);
            gameStartTime = GetTickCount64();
            currentTime = 0;
            lastJudgement = NONE;
            judgementDisplayTimer = 0;
            prevCombo = 0;
            hitAnims.clear();
            textAnims.clear();
            comboAnim.elapsed = comboAnim.duration;
            for (int i = 0; i < 4; i++) { keyPressed[i] = false; keyWasPressed[i] = false; keyGlowAlpha[i] = 0; }
            gameState = PLAYING;
            audioManager.playBGM();
        } else {
            // 返回主菜单
            gameState = MENU;
        }
    }

    // 鼠标点击结算菜单按钮：单击直接执行
    if (isMouseClick()) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(*_easyx_impl::g_window);
        float mx = (float)mousePos.x;
        float my = (float)mousePos.y;

        float panelW = 600.0f, panelH = 580.0f;
        float panelX = (width - panelW) / 2.0f;
        float panelY = (height - panelH) / 2.0f;
        float buttonW = 200.0f, buttonH = 40.0f;
        float buttonY = panelY + panelH - 70.0f;
        float buttonSpacing = 50.0f;
        float startX = panelX + (panelW - buttonW * 2 - buttonSpacing) / 2;

        for (int i = 0; i < 2; i++) {
            float x = startX + i * (buttonW + buttonSpacing);
            if (isPointInRect(mx, my, x, buttonY, buttonW, buttonH)) {
                // 单击直接执行
                dataManager.saveResult(currentSongName, (int)currentDifficulty,
                                       scoreSystem.getTotalScore(), scoreSystem.getMaxCombo());
                if (i == 0) {
                    // 重新开始
                    scoreSystem.reset();
                    tracks.clear();
                    initTracks();
                    if (parseResult.success && !parseResult.noteTimeData.empty()) {
                        double speed = getDifficultySpeed();
                        for (const auto& [timeMs, track] : parseResult.noteTimeData) {
                            auto note = std::make_unique<NormalNote>(track, timeMs, JUDGE_Y, speed, TRACK_COLORS[track]);
                            tracks[track]->addNote(std::move(note));
                        }
                        noteTimeData = parseResult.noteTimeData;
                        lastNoteTime = parseResult.lastNoteTime;
                    } else {
                        loadDemoSong();
                    }
                    audioManager.generateSyncedBGM(noteTimeData);
                    gameStartTime = GetTickCount64();
                    currentTime = 0;
                    lastJudgement = NONE;
                    judgementDisplayTimer = 0;
                    prevCombo = 0;
                    hitAnims.clear();
                    textAnims.clear();
                    comboAnim.elapsed = comboAnim.duration;
                    for (int j = 0; j < 4; j++) { keyPressed[j] = false; keyWasPressed[j] = false; keyGlowAlpha[j] = 0; }
                    gameState = PLAYING;
                    audioManager.playBGM();
                } else {
                    // 返回主菜单
                    gameState = MENU;
                }
                break;
            }
        }
    }
}

// ========== 动画触发 ==========

void GameWindow::spawnHitAnim(int trackId, float noteY, Judgement j) {
    HitAnim anim;
    anim.x = getTrackX(trackId) + 35.0f;  // 轨道中心
    anim.y = noteY;
    anim.elapsed = 0.0f;
    switch (j) {
        case PERFECT: anim.texture = &textures.notePerfect; break;
        case GOOD:    anim.texture = &textures.noteGood;    break;
        default:      anim.texture = &textures.noteMiss;    break;
    }
    hitAnims.push_back(anim);
}

void GameWindow::spawnTextPopup(int trackId, Judgement j) {
    TextPopupAnim anim;
    anim.x = getTrackX(trackId) + 35.0f;  // 轨道中心
    anim.y = JUDGE_Y - 60.0f;
    anim.elapsed = 0.0f;
    switch (j) {
        case PERFECT: anim.texture = &textures.textPerfect; break;
        case GOOD:    anim.texture = &textures.textGood;    break;
        default:      anim.texture = &textures.textMiss;    break;
    }
    textAnims.push_back(anim);
}

// ========== 更新 ==========

void GameWindow::update() {
    // 暂停状态不更新游戏逻辑
    if (gameState == PAUSED) {
        return;
    }

    currentTime = GetTickCount64() - gameStartTime;

    // V3.7: 自动演示/代管模式 - 仅代管选中轨道
    if (gameState == PLAYING) {
        for (int i = 0; i < TRACK_COUNT; i++) {
            if (!autoPlay && !trackAutoPlay[i]) continue;
            long long nextNoteTime = tracks[i]->getNextNoteJudgeTime();
            if (nextNoteTime == LLONG_MAX) continue;
            long long diff = currentTime - nextNoteTime;
            // 在音符到达判定线时自动触发（误差<30ms）
            if (diff >= 0 && diff < 30) {
                NoteTrack::JudgeResult result = tracks[i]->handlePress(currentTime);
                if (result.judgement != NONE) {
                    scoreSystem.addJudgement(result.judgement);
                    lastJudgement = result.judgement;
                    judgementDisplayTimer = 30;
                    spawnHitAnim(i, result.noteY, result.judgement);
                    spawnTextPopup(i, result.judgement);
                    spawnParticles(getTrackX(i) + TRACK_WIDTH / 2.0f, result.noteY, result.judgement);
                    if (result.judgement == PERFECT) audioManager.playHit(true, i);
                    else if (result.judgement == GOOD) audioManager.playHit(false, i);
                    keyGlowAlpha[i] = 255;
                    keyFeedback[i].glowTimer = KeyFeedback::GLOW_DURATION;
                }
            }
        }
    }

    for (auto& track : tracks) {
        auto autoMisses = track->update(currentTime, scoreSystem);
        for (auto& miss : autoMisses) {
            spawnHitAnim(miss.trackId, miss.y, MISS);
            spawnTextPopup(miss.trackId, MISS);
            // V3.0: AutoMiss也生成粒子（红色X）
            spawnParticles(getTrackX(miss.trackId) + TRACK_WIDTH / 2.0f, miss.y, MISS);
        }
    }

    if (judgementDisplayTimer > 0) {
        judgementDisplayTimer--;
        if (judgementDisplayTimer == 0) lastJudgement = NONE;
    }

    for (int i = 0; i < TRACK_COUNT; i++) {
        if (!keyPressed[i] && keyGlowAlpha[i] > 0) {
            keyGlowAlpha[i] -= 15;
            if (keyGlowAlpha[i] < 0) keyGlowAlpha[i] = 0;
        }
    }

    updateAnimations();

    // V3.0: 更新粒子和按键反馈
    const float dt = 1.0f / 60.0f;
    updateParticles(dt);
    updateKeyFeedback(dt);

    // V3.0: 更新Combo断连闪烁
    if (comboBreakFlash > 0.0f) {
        comboBreakFlash -= dt;
        if (comboBreakFlash < 0.0f) comboBreakFlash = 0.0f;
    }

    int currentCombo = scoreSystem.getCurrentCombo();
    if (currentCombo > prevCombo && prevCombo > 0) {
        comboAnim.elapsed = 0.0f;
    }
    prevCombo = currentCombo;

    if (isGameFinished()) {
        audioManager.stopBGM();
        // 保存排行榜
        dataManager.saveResult(currentSongName, (int)currentDifficulty,
                               scoreSystem.getTotalScore(), scoreSystem.getMaxCombo());

        // V3.3: 检查成就
        totalSongsPlayed++;
        int customCount = ChartPackage::countCustomSongs(exeDir + "/songs/");
        auto newAch = achievementSystem.checkAchievements(
            currentSongName, (int)currentDifficulty,
            scoreSystem.getTotalScore(), scoreSystem.getMaxCombo(),
            scoreSystem.getPerfectCount(), scoreSystem.getGoodCount(), scoreSystem.getMissCount(),
            totalSongsPlayed, customCount, 1.0f
        );
        // 添加成就弹窗
        for (int achId : newAch) {
            auto& all = achievementSystem.getAll();
            for (auto& a : all) {
                if (a.id == achId) {
                    AchievementPopup pop;
                    pop.title = a.name;
                    pop.description = a.description;
                    pop.icon = a.icon;
                    pop.elapsed = 0;
                    achievementPopups.push_back(pop);
                    break;
                }
            }
        }

        gameState = RESULT;
        resultStartTime = GetTickCount64();
    }
}

void GameWindow::updateAnimations() {
    float dt = 1.0f / 60.0f;

    for (int i = (int)hitAnims.size() - 1; i >= 0; i--) {
        hitAnims[i].elapsed += dt;
        if (hitAnims[i].elapsed >= hitAnims[i].duration) {
            hitAnims.erase(hitAnims.begin() + i);
        }
    }

    for (int i = (int)textAnims.size() - 1; i >= 0; i--) {
        textAnims[i].elapsed += dt;
        if (textAnims[i].elapsed >= textAnims[i].duration) {
            textAnims.erase(textAnims.begin() + i);
        }
    }

    if (comboAnim.elapsed < comboAnim.duration) {
        comboAnim.elapsed += dt;
    }
}

// ========== V3.0: 粒子特效 ==========

void GameWindow::spawnParticles(float x, float y, Judgement j) {
    int count = 0;
    sf::Color color;
    float lifetime = 0.5f;

    switch (j) {
        case PERFECT:
            count = 8 + rand() % 3;  // 8-10个
            color = sf::Color(100, 200, 255, 255);  // 淡蓝色
            lifetime = 0.5f;
            break;
        case GOOD:
            count = 4 + rand() % 3;  // 4-6个
            color = sf::Color(100, 255, 100, 255);  // 淡绿色
            lifetime = 0.4f;
            break;
        case MISS: {
            // Miss显示红色小叉动画
            MissCrossAnim cross;
            cross.x = x;
            cross.y = y;
            cross.duration = 0.3f;
            cross.elapsed = 0.0f;
            missCrossAnims.push_back(cross);
            return;
        }
        default:
            return;
    }

    for (int i = 0; i < count; i++) {
        Particle p;
        p.x = x + (rand() % 20 - 10);
        p.y = y + (rand() % 20 - 10);
        // 随机方向向外扩散
        float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
        float speed = 80.0f + (float)(rand() % 120);  // 80~200 px/s
        p.vx = cosf(angle) * speed;
        p.vy = sinf(angle) * speed - 50.0f;  // 略微向上偏移
        p.lifetime = lifetime;
        p.elapsed = 0.0f;
        p.size = 2.0f + (float)(rand() % 3);  // 2~4 px
        p.color = color;
        particles.push_back(p);
    }
}

void GameWindow::updateParticles(float dt) {
    // 更新粒子
    for (int i = (int)particles.size() - 1; i >= 0; i--) {
        particles[i].elapsed += dt;
        if (particles[i].elapsed >= particles[i].lifetime) {
            particles.erase(particles.begin() + i);
            continue;
        }
        particles[i].x += particles[i].vx * dt;
        particles[i].y += particles[i].vy * dt;
        particles[i].vy += 150.0f * dt;  // 轻微重力
    }

    // 更新Miss叉号动画
    for (int i = (int)missCrossAnims.size() - 1; i >= 0; i--) {
        missCrossAnims[i].elapsed += dt;
        if (missCrossAnims[i].elapsed >= missCrossAnims[i].duration) {
            missCrossAnims.erase(missCrossAnims.begin() + i);
        }
    }
}

void GameWindow::updateKeyFeedback(float dt) {
    for (int i = 0; i < TRACK_COUNT; i++) {
        // 更新发光计时器
        if (keyFeedback[i].glowTimer > 0.0f) {
            keyFeedback[i].glowTimer -= dt;
            if (keyFeedback[i].glowTimer < 0.0f) keyFeedback[i].glowTimer = 0.0f;
        }

        // 更新按键缩放
        float targetScale = keyPressed[i] ? KeyFeedback::PRESS_SCALE_MAX : 1.0f;
        float diff = targetScale - keyFeedback[i].pressScale;
        keyFeedback[i].pressScale += diff * KeyFeedback::SCALE_SPEED * dt;
    }
}

// ========== V3.1: 暂停界面输入处理 ==========
void GameWindow::handlePauseInput() {
    // 每帧重置防重复标志，确保按键能被检测到
    static bool upWasPressed = false;
    static bool downWasPressed = false;
    static bool enterWasPressed = false;
    static bool escWasPressed = false;

    // 先检测按键再处理事件（避免processWindowEvents消费按键）
    bool upPressed = (GetAsyncKeyState('W') & 0x8000) != 0;
    bool downPressed = (GetAsyncKeyState('S') & 0x8000) != 0;
    bool enterPressed = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
    bool escPressed = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;

    {
        using namespace _easyx_impl;
        if (g_window && g_windowOpen) {
            while (const auto event = g_window->pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    g_windowOpen = false;
                    isRunning = false;
                    return;
                }
            }
        }
    }

    // 刚进入暂停时同步ESC状态，防止同一按键触发立即恢复
    if (pauseJustOpened) {
        pauseJustOpened = false;
        escWasPressed = escPressed;
        upWasPressed = upPressed; downWasPressed = downPressed;
        enterWasPressed = enterPressed;
        return;
    }

    // ESC直接继续游戏
    if (escPressed && !escWasPressed) {
        escWasPressed = true;
        enterWasPressed = enterPressed; // 同步ENTER状态避免误触
        long long pauseDuration = GetTickCount64() - pauseStartTime;
        gameStartTime += pauseDuration;
        audioManager.resumeBGM();
        gameState = PLAYING;
        return;
    } else if (!escPressed) {
        escWasPressed = false;
    }

    // 正常防重复逻辑：仅按下瞬间触发一次
    if (upPressed && !upWasPressed) {
        upWasPressed = true;
        pauseMenuSelection--;
        if (pauseMenuSelection < 0) pauseMenuSelection = 2;
    } else if (!upPressed) {
        upWasPressed = false;
    }

    if (downPressed && !downWasPressed) {
        downWasPressed = true;
        pauseMenuSelection++;
        if (pauseMenuSelection > 2) pauseMenuSelection = 0;
    } else if (!downPressed) {
        downWasPressed = false;
    }

    if (enterPressed && !enterWasPressed) {
        enterWasPressed = true;
        switch (pauseMenuSelection) {
            case 0: // 继续游戏
            {
                long long pauseDuration = GetTickCount64() - pauseStartTime;
                gameStartTime += pauseDuration;
                audioManager.resumeBGM();
                gameState = PLAYING;
                break;
            }
            case 1: // 重新开始
            {
                audioManager.stopBGM();
                scoreSystem.reset();
                tracks.clear();
                initTracks();

                if (parseResult.success && !parseResult.noteTimeData.empty()) {
                    double speed = getDifficultySpeed();
                    for (const auto& [timeMs, track] : parseResult.noteTimeData) {
                        auto note = std::make_unique<NormalNote>(
                            track, timeMs, JUDGE_Y, speed, TRACK_COLORS[track]
                        );
                        tracks[track]->addNote(std::move(note));
                    }
                    noteTimeData = parseResult.noteTimeData;
                    lastNoteTime = parseResult.lastNoteTime;
                } else {
                    loadDemoSong();
                }

                audioManager.generateSyncedBGM(noteTimeData);
                gameStartTime = GetTickCount64();
                hitAnims.clear();
                textAnims.clear();
                particles.clear();
                missCrossAnims.clear();
                comboAnim.elapsed = 0.0f;
                pauseMenuSelection = 0;
                gameState = PLAYING;
                audioManager.playBGM();
                break;
            }
            case 2: // 返回主菜单
            {
                audioManager.stopBGM();
                pauseMenuSelection = 0;
                gameState = MENU;
                menuJustOpened = true;
                break;
            }
        }
    }

    // 鼠标点击暂停菜单按钮：第一次选中，第二次确认
    if (isMouseClick()) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(*_easyx_impl::g_window);
        float mx = (float)mousePos.x;
        float my = (float)mousePos.y;

        float panelW = 450.0f, panelH = 380.0f;
        float panelX = (width - panelW) / 2.0f;
        float panelY = (height - panelH) / 2.0f;
        float buttonW = 300.0f, buttonH = 60.0f;
        float buttonX = panelX + (panelW - buttonW) / 2.0f;
        float buttonY = panelY + 100.0f;

        for (int i = 0; i < 3; i++) {
            float currentY = buttonY + i * (buttonH + 20.0f);
            if (isPointInRect(mx, my, buttonX, currentY, buttonW, buttonH)) {
                if (pauseMenuSelection == i) {
                    // 已选中，再次点击才执行
                    switch (i) {
                        case 0: {
                            long long pauseDuration = GetTickCount64() - pauseStartTime;
                            gameStartTime += pauseDuration;
                            audioManager.resumeBGM();
                            gameState = PLAYING;
                            break;
                        }
                        case 1: {
                            audioManager.stopBGM();
                            scoreSystem.reset();
                            tracks.clear();
                            initTracks();
                            if (parseResult.success && !parseResult.noteTimeData.empty()) {
                                double speed = getDifficultySpeed();
                                for (const auto& [timeMs, track] : parseResult.noteTimeData) {
                                    auto note = std::make_unique<NormalNote>(track, timeMs, JUDGE_Y, speed, TRACK_COLORS[track]);
                                    tracks[track]->addNote(std::move(note));
                                }
                                noteTimeData = parseResult.noteTimeData;
                                lastNoteTime = parseResult.lastNoteTime;
                            } else {
                                loadDemoSong();
                            }
                            audioManager.generateSyncedBGM(noteTimeData);
                            gameStartTime = GetTickCount64();
                            hitAnims.clear();
                            textAnims.clear();
                            particles.clear();
                            missCrossAnims.clear();
                            comboAnim.elapsed = 0.0f;
                            pauseMenuSelection = 0;
                            gameState = PLAYING;
                            audioManager.playBGM();
                            break;
                        }
                        case 2: {
                            audioManager.stopBGM();
                            pauseMenuSelection = 0;
                            gameState = MENU;
                            break;
                        }
                    }
                } else {
                    pauseMenuSelection = i;
                }
                break;
            }
        }
    }
}

// ========== V3.1: 虚拟键码转显示名称 ==========
const char* GameWindow::getKeyName(int vkCode) {
    switch(vkCode) {
        case VK_BACK:       return "BACK";
        case VK_TAB:        return "TAB";
        case VK_CLEAR:      return "CLEAR";
        case VK_RETURN:     return "ENTER";
        case VK_SHIFT:      return "SHIFT";
        case VK_CONTROL:    return "CTRL";
        case VK_MENU:       return "ALT";
        case VK_PAUSE:      return "PAUSE";
        case VK_CAPITAL:    return "CAPS";
        case VK_ESCAPE:     return "ESC";
        case VK_SPACE:      return "SPACE";
        case VK_PRIOR:      return "PAGE UP";
        case VK_NEXT:       return "PAGE DOWN";
        case VK_END:        return "END";
        case VK_HOME:       return "HOME";
        case VK_LEFT:       return "LEFT";
        case VK_UP:         return "UP";
        case VK_RIGHT:      return "RIGHT";
        case VK_DOWN:       return "DOWN";
        case VK_SELECT:     return "SELECT";
        case VK_PRINT:      return "PRINT";
        case VK_EXECUTE:    return "EXECUTE";
        case VK_SNAPSHOT:   return "PRINT SCREEN";
        case VK_INSERT:     return "INSERT";
        case VK_DELETE:     return "DELETE";
        case VK_HELP:       return "HELP";
        case '0':           return "0";
        case '1':           return "1";
        case '2':           return "2";
        case '3':           return "3";
        case '4':           return "4";
        case '5':           return "5";
        case '6':           return "6";
        case '7':           return "7";
        case '8':           return "8";
        case '9':           return "9";
        case 'A':           return "A";
        case 'B':           return "B";
        case 'C':           return "C";
        case 'D':           return "D";
        case 'E':           return "E";
        case 'F':           return "F";
        case 'G':           return "G";
        case 'H':           return "H";
        case 'I':           return "I";
        case 'J':           return "J";
        case 'K':           return "K";
        case 'L':           return "L";
        case 'M':           return "M";
        case 'N':           return "N";
        case 'O':           return "O";
        case 'P':           return "P";
        case 'Q':           return "Q";
        case 'R':           return "R";
        case 'S':           return "S";
        case 'T':           return "T";
        case 'U':           return "U";
        case 'V':           return "V";
        case 'W':           return "W";
        case 'X':           return "X";
        case 'Y':           return "Y";
        case 'Z':           return "Z";
        case VK_LWIN:       return "LEFT WIN";
        case VK_RWIN:       return "RIGHT WIN";
        case VK_APPS:       return "APPS";
        case VK_SLEEP:      return "SLEEP";
        case VK_NUMPAD0:    return "NUMPAD 0";
        case VK_NUMPAD1:    return "NUMPAD 1";
        case VK_NUMPAD2:    return "NUMPAD 2";
        case VK_NUMPAD3:    return "NUMPAD 3";
        case VK_NUMPAD4:    return "NUMPAD 4";
        case VK_NUMPAD5:    return "NUMPAD 5";
        case VK_NUMPAD6:    return "NUMPAD 6";
        case VK_NUMPAD7:    return "NUMPAD 7";
        case VK_NUMPAD8:    return "NUMPAD 8";
        case VK_NUMPAD9:    return "NUMPAD 9";
        case VK_MULTIPLY:   return "NUMPAD *";
        case VK_ADD:        return "NUMPAD +";
        case VK_SEPARATOR:  return "SEPARATOR";
        case VK_SUBTRACT:   return "NUMPAD -";
        case VK_DECIMAL:    return "NUMPAD .";
        case VK_DIVIDE:     return "NUMPAD /";
        case VK_F1:         return "F1";
        case VK_F2:         return "F2";
        case VK_F3:         return "F3";
        case VK_F4:         return "F4";
        case VK_F5:         return "F5";
        case VK_F6:         return "F6";
        case VK_F7:         return "F7";
        case VK_F8:         return "F8";
        case VK_F9:         return "F9";
        case VK_F10:        return "F10";
        case VK_F11:        return "F11";
        case VK_F12:        return "F12";
        case VK_NUMLOCK:    return "NUM LOCK";
        case VK_SCROLL:     return "SCROLL LOCK";
        case VK_LSHIFT:     return "LEFT SHIFT";
        case VK_RSHIFT:     return "RIGHT SHIFT";
        case VK_LCONTROL:   return "LEFT CTRL";
        case VK_RCONTROL:   return "RIGHT CTRL";
        case VK_LMENU:      return "LEFT ALT";
        case VK_RMENU:      return "RIGHT ALT";
        default: {
            static char buf[16];
            snprintf(buf, sizeof(buf), "KEY 0x%X", vkCode);
            return buf;
        }
    }
}

// ========== V3.1: 设置界面输入处理 ==========

void GameWindow::handleSettingsInput() {
    if (!processWindowEvents()) {
        isRunning = false;
        return;
    }

    static bool keyUpWasPressed = false;
    static bool keyDownWasPressed = false;
    static bool keyLeftWasPressed = false;
    static bool keyRightWasPressed = false;
    static bool enterWasPressed = false;
    static bool escWasPressed = false;

    // 防粘滞：刚打开时同步static prev为当前按键状态
    if (settingsJustOpened) {
        settingsJustOpened = false;
        keyUpWasPressed = (GetAsyncKeyState('W') & 0x8000) != 0;
        keyDownWasPressed = (GetAsyncKeyState('S') & 0x8000) != 0;
        keyLeftWasPressed = (GetAsyncKeyState('A') & 0x8000) != 0;
        keyRightWasPressed = (GetAsyncKeyState('D') & 0x8000) != 0;
        enterWasPressed = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
        escWasPressed = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
        mouseWasPressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
        return;
    }

    bool upPressed = (GetAsyncKeyState('W') & 0x8000) != 0;
    bool downPressed = (GetAsyncKeyState('S') & 0x8000) != 0;
    bool leftPressed = (GetAsyncKeyState('A') & 0x8000) != 0;
    bool rightPressed = (GetAsyncKeyState('D') & 0x8000) != 0;
    bool enterPressed = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
    bool escPressed = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;

    // 正在设置按键模式
    if (currentKeySettingIndex >= 0) {
        // ESC取消设置
        if (escPressed && !escWasPressed) {
            escWasPressed = true;
            currentKeySettingIndex = -1;
            keySettingWaitingRelease = false;
            return;
        } else if (!escPressed) {
            escWasPressed = false;
        }

        // 先等待所有按键松开（避免ENTER立刻被设置）
        if (!keySettingWaitingRelease) {
            bool anyKeyDown = false;
            for (int vk = 0x08; vk <= 0xFF; vk++) {
                if (GetAsyncKeyState(vk) & 0x8000) {
                    anyKeyDown = true;
                    break;
                }
            }
            if (!anyKeyDown) {
                keySettingWaitingRelease = true; // 所有按键已松开，开始等待新按键
            }
            return;
        }

        // 检测新按键按下，排除W、S、A、D这些菜单控制键
        for (int vk = 0x08; vk <= 0xFF; vk++) {
            if ((GetAsyncKeyState(vk) & 0x8000) && vk != 'W' && vk != 'S' && vk != 'A' && vk != 'D' && vk != VK_ESCAPE) {
                // 检查是否和其他按键重复
                bool duplicate = false;
                for (int i = 0; i < 6; i++) {
                    if (i != currentKeySettingIndex && customKeys[i] == vk) {
                        duplicate = true;
                        break;
                    }
                }
                if (!duplicate) {
                    customKeys[currentKeySettingIndex] = vk;
                    currentKeySettingIndex = -1;
                    keySettingWaitingRelease = false;
                    return;
                }
            }
        }
        return;
    }

    // 正常设置界面模式
    // 上下切换选项
    if (upPressed && !keyUpWasPressed) {
        keyUpWasPressed = true;
        settingsMenuSelection = (settingsMenuSelection - 1 + 11) % 11;
    } else if (!upPressed) {
        keyUpWasPressed = false;
    }

    if (downPressed && !keyDownWasPressed) {
        keyDownWasPressed = true;
        settingsMenuSelection = (settingsMenuSelection + 1) % 11;
    } else if (!downPressed) {
        keyDownWasPressed = false;
    }

    // 左右调整（音量和音效包）
    if (settingsMenuSelection < 3) {
        if (leftPressed && !keyLeftWasPressed) {
            keyLeftWasPressed = true;
            if (settingsMenuSelection == 0) {
                musicVolume = std::max(0.0f, musicVolume - 0.1f);
                audioManager.setMusicVolume(musicVolume);
            } else if (settingsMenuSelection == 1) {
                effectVolume = std::max(0.0f, effectVolume - 0.1f);
                audioManager.setEffectVolume(effectVolume);
                audioManager.playHit(false);
            } else {
                // 音效包切换
                int pack = audioManager.getSoundPack();
                pack = (pack - 1 + 2) % 2;
                audioManager.setSoundPack(pack);
                audioManager.playHit(false);
            }
        } else if (!leftPressed) {
            keyLeftWasPressed = false;
        }

        if (rightPressed && !keyRightWasPressed) {
            keyRightWasPressed = true;
            if (settingsMenuSelection == 0) {
                musicVolume = std::min(1.0f, musicVolume + 0.1f);
                audioManager.setMusicVolume(musicVolume);
            } else if (settingsMenuSelection == 1) {
                effectVolume = std::min(1.0f, effectVolume + 0.1f);
                audioManager.setEffectVolume(effectVolume);
                audioManager.playHit(false);
            } else {
                int pack = audioManager.getSoundPack();
                pack = (pack + 1) % 2;
                audioManager.setSoundPack(pack);
                audioManager.playHit(false);
            }
        } else if (!rightPressed) {
            keyRightWasPressed = false;
        }
    }

    // ENTER确认
    if (enterPressed && !enterWasPressed) {
        enterWasPressed = true;

        // 按键设置选项（6个按键）
        if (settingsMenuSelection >= 3 && settingsMenuSelection < 9) {
            currentKeySettingIndex = settingsMenuSelection - 3;
            keySettingWaitingRelease = false;
        }
        // 恢复默认按键
        else if (settingsMenuSelection == 9) {
            customKeys[0] = 'A'; customKeys[1] = 'S'; customKeys[2] = 'D';
            customKeys[3] = 'F'; customKeys[4] = 'J'; customKeys[5] = 'K';
        }
        // 保存返回
        else if (settingsMenuSelection == 10) {
            saveConfig();
            isInSettings = false;
            settingsMenuSelection = 0;
        }
    }

    // ESC返回
    if (escPressed && !escWasPressed) {
        escWasPressed = true;
        saveConfig();
        isInSettings = false;
        settingsMenuSelection = 0;
    } else if (!escPressed) {
        escWasPressed = false;
    }

    // 鼠标点击设置选项：单击直接执行（音量条直接调整，其他选中并执行）
    if (currentKeySettingIndex < 0 && isMouseClick()) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(*_easyx_impl::g_window);
        float mx = (float)mousePos.x;
        float my = (float)mousePos.y;

        float panelW = 500.0f, panelH = 620.0f;
        float panelX = (width - panelW) / 2.0f;
        float panelY = (height - panelH) / 2.0f;
        float optionY = panelY + 90.0f;
        float optionH = 45.0f;
        float highlightW = 400.0f, highlightH = 35.0f;
        float highlightX = panelX + (panelW - highlightW) / 2.0f;

        for (int i = 0; i < 11; i++) {
            float currentY = optionY + i * optionH - 5.0f;
            if (isPointInRect(mx, my, highlightX, currentY, highlightW, highlightH)) {
                settingsMenuSelection = i;
                if (i < 2) {
                    // 音量选项：点击位置决定加减
                    float midX = highlightX + highlightW / 2.0f;
                    if (mx < midX) {
                        if (i == 0) { musicVolume = std::max(0.0f, musicVolume - 0.1f); audioManager.setMusicVolume(musicVolume); }
                        else { effectVolume = std::max(0.0f, effectVolume - 0.1f); audioManager.setEffectVolume(effectVolume); audioManager.playHit(false); }
                    } else {
                        if (i == 0) { musicVolume = std::min(1.0f, musicVolume + 0.1f); audioManager.setMusicVolume(musicVolume); }
                        else { effectVolume = std::min(1.0f, effectVolume + 0.1f); audioManager.setEffectVolume(effectVolume); audioManager.playHit(false); }
                    }
                } else if (i == 2) {
                    // 音效包：点击切换
                    int pack = audioManager.getSoundPack();
                    audioManager.setSoundPack((pack + 1) % 2);
                    audioManager.playHit(false);
                } else if (i >= 3 && i < 9) {
                    // 按键设置（菜单3-8 → keyIdx 0-5）
                    currentKeySettingIndex = i - 3;
                    keySettingWaitingRelease = false;
                } else if (i == 9) {
                    // 重置为默认
                    customKeys[0] = 'A'; customKeys[1] = 'S'; customKeys[2] = 'D';
                    customKeys[3] = 'F'; customKeys[4] = 'J'; customKeys[5] = 'K';
                } else if (i == 10) {
                    // 保存并退出
                    saveConfig(); isInSettings = false; settingsMenuSelection = 0;
                }
                break;
            }
        }
    }
}

// ========== V3.1: 配置保存和加载 ==========

void GameWindow::saveConfig() {
    FILE* fp = fopen(configFilePath.c_str(), "w");
    if (fp) {
        fprintf(fp, "[Settings]\n");
        fprintf(fp, "music_volume = %.2f\n", musicVolume);
        fprintf(fp, "effect_volume = %.2f\n", effectVolume);
        fprintf(fp, "key1 = %d\n", customKeys[0]);
        fprintf(fp, "key2 = %d\n", customKeys[1]);
        fprintf(fp, "key3 = %d\n", customKeys[2]);
        fprintf(fp, "key4 = %d\n", customKeys[3]);
        fprintf(fp, "key5 = %d\n", customKeys[4]);
        fprintf(fp, "key6 = %d\n", customKeys[5]);
        fprintf(fp, "sound_pack = %d\n", audioManager.getSoundPack());
        fclose(fp);
        printf("[Config] Saved config\n");
    }
}

void GameWindow::loadConfig(const std::string& path) {
    configFilePath = path;
    FILE* fp = fopen(configFilePath.c_str(), "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "music_volume")) {
                sscanf(line, "music_volume = %f", &musicVolume);
            } else if (strstr(line, "effect_volume")) {
                sscanf(line, "effect_volume = %f", &effectVolume);
            } else if (strstr(line, "key1")) {
                int k; sscanf(line, "key1 = %d", &k); customKeys[0] = k;
            } else if (strstr(line, "key2")) {
                int k; sscanf(line, "key2 = %d", &k); customKeys[1] = k;
            } else if (strstr(line, "key3")) {
                int k; sscanf(line, "key3 = %d", &k); customKeys[2] = k;
            } else if (strstr(line, "key4")) {
                int k; sscanf(line, "key4 = %d", &k); customKeys[3] = k;
            } else if (strstr(line, "key5")) {
                int k; sscanf(line, "key5 = %d", &k); customKeys[4] = k;
            } else if (strstr(line, "key6")) {
                int k; sscanf(line, "key6 = %d", &k); customKeys[5] = k;
            } else if (strstr(line, "sound_pack")) {
                int sp; sscanf(line, "sound_pack = %d", &sp); audioManager.setSoundPack(sp);
            }
        }
        fclose(fp);
        musicVolume = std::max(0.0f, std::min(1.0f, musicVolume));
        effectVolume = std::max(0.0f, std::min(1.0f, effectVolume));
        printf("[Config] Loaded config: music=%.2f effect=%.2f keys=[%s, %s, %s, %s, %s, %s]\n", musicVolume, effectVolume,
               getKeyName(customKeys[0]), getKeyName(customKeys[1]), getKeyName(customKeys[2]),
               getKeyName(customKeys[3]), getKeyName(customKeys[4]), getKeyName(customKeys[5]));
    } else {
        musicVolume = 1.0f;
        effectVolume = 1.0f;
        customKeys[0] = 'A'; customKeys[1] = 'S'; customKeys[2] = 'D';
        customKeys[3] = 'F'; customKeys[4] = 'J'; customKeys[5] = 'K';
        saveConfig();
        printf("[Config] Created new default config\n");
    }
}

// ========== V3.3: 导入谱面 ==========
void GameWindow::importChartPackage() {
    // 扫描songs目录下的.beatpixel文件（跨平台文件系统扫描）
    std::string songsDir = exeDir + "/songs/";
    std::string foundFile;
    
    for (const auto& entry : std::filesystem::directory_iterator(songsDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".beatpixel") {
            foundFile = entry.path().string();
            break;
        }
    }
    
    if (!foundFile.empty()) {
        std::string imported = ChartPackage::importSong(foundFile, songsDir);
        if (!imported.empty()) {
            printf("[GameWindow] 导入成功: %s\n", imported.c_str());
        } else {
            printf("[GameWindow] 导入失败\n");
        }
    } else {
        printf("[GameWindow] 未找到.beatpixel文件，请放入songs目录\n");
    }
    fflush(stdout);
}

// ========== V3.3: 导出谱面 ==========
void GameWindow::exportCurrentSong() {
    if (!analysisResult.success) return;

    // 查找MP3文件（跨平台）
    std::string songsDir = exeDir + "/songs/";
    std::string mp3Path = songsDir + analysisResult.metadata.title + ".mp3";
    FILE* f = fopen(mp3Path.c_str(), "rb");
    if (!f) {
        // 尝试查找任意mp3文件
        for (const auto& entry : std::filesystem::directory_iterator(songsDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".mp3") {
                mp3Path = entry.path().string();
                break;
            }
        }
    } else {
        fclose(f);
    }

    // 导出chart.json到临时位置
    std::string chartPath = mp3Path + ".chart.json";
    SongAnalyzer::exportChart(chartPath, analysisResult);

    // 打包为.beatpixel
    std::string outPath = exeDir + "/songs/" + analysisResult.metadata.title + ".beatpixel";
    bool ok = ChartPackage::exportSong(mp3Path, chartPath, outPath);
    if (ok) {
        printf("[GameWindow] 导出成功: %s\n", outPath.c_str());
    }
    fflush(stdout);
}

// ========== V3.3: 成就界面输入 ==========
void GameWindow::handleAchievementsInput() {
    // 只处理窗口关闭
    {
        using namespace _easyx_impl;
        if (g_window && g_windowOpen) {
            while (const auto event = g_window->pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    g_windowOpen = false;
                    isRunning = false;
                    return;
                }
            }
        }
    }

    bool escPressed = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
    bool enterPressed = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;

    static bool prevEsc = false, prevEnter = false;
    bool escJust = escPressed && !prevEsc;
    bool enterJust = enterPressed && !prevEnter;
    prevEsc = escPressed; prevEnter = enterPressed;

    if (escJust || enterJust) {
        showAchievements = false;
        gameState = MENU;
    }

    // 鼠标点击返回
    if (isMouseClick()) {
        sf::Vector2i mp = sf::Mouse::getPosition(*_easyx_impl::g_window);
        float mx = (float)mp.x, my = (float)mp.y;
        float panelW = 600.0f, panelH = 620.0f;
        float panelX = (width - panelW) / 2.0f;
        float panelY = (height - panelH) / 2.0f;
        float btnX = panelX + (panelW - 200) / 2;
        float btnY = panelY + panelH - 60;
        if (isPointInRect(mx, my, btnX, btnY, 200.0f, 40.0f)) {
            showAchievements = false;
            gameState = MENU;
        }
    }
}

// ========== V3.4: 歌曲列表 ==========

// 从 .chart.json 文件直接加载谱面（跳过音频分析，用于有预置谱面的歌曲）
bool GameWindow::loadChartFromFile(const std::string& chartPath, const std::string& mp3Path) {
    FILE* fp = fopen(chartPath.c_str(), "r");
    if (!fp) {
        printf("[GameWindow] 无法打开谱面: %s\n", chartPath.c_str());
        return false;
    }
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    std::string json(sz, '\0');
    fread(&json[0], 1, sz, fp);
    fclose(fp);

    // 解析标题
    std::string title;
    size_t titlePos = json.find("\"title\"");
    if (titlePos != std::string::npos) {
        size_t colon = json.find(":", titlePos);
        size_t start = json.find("\"", colon + 1);
        size_t end = json.find("\"", start + 1);
        if (start != std::string::npos && end != std::string::npos) {
            title = json.substr(start + 1, end - start - 1);
        }
    }

    // 解析BPM
    float bpm = 120.0f;
    size_t bpmPos = json.find("\"bpm\"");
    if (bpmPos != std::string::npos) {
        size_t colon = json.find(":", bpmPos);
        size_t end = json.find(",", colon);
        if (end == std::string::npos) end = json.find("\n", colon);
        if (end == std::string::npos) end = json.size();
        std::string val = json.substr(colon + 1, end - colon - 1);
        val.erase(0, val.find_first_not_of(" \t\n\r"));
        val.erase(val.find_last_not_of(" \t\n\r") + 1);
        try { bpm = std::stof(val); } catch (...) {}
    }

    // 解析音符
    noteTimeData.clear();
    size_t notesPos = json.find("\"notes\"");
    if (notesPos != std::string::npos) {
        size_t arrStart = json.find("[", notesPos);
        size_t arrEnd = json.find("]", arrStart);
        if (arrStart != std::string::npos && arrEnd != std::string::npos) {
            size_t pos = arrStart + 1;
            while (pos < arrEnd) {
                size_t timePos = json.find("\"time\"", pos);
                if (timePos == std::string::npos || timePos >= arrEnd) break;
                size_t timeColon = json.find(":", timePos);
                size_t timeEnd = json.find_first_of(",}", timeColon);
                if (timeEnd == std::string::npos) break;
                std::string timeVal = json.substr(timeColon + 1, timeEnd - timeColon - 1);
                timeVal.erase(0, timeVal.find_first_not_of(" \t\n\r"));
                timeVal.erase(timeVal.find_last_not_of(" \t\n\r") + 1);
                long long timeMs = 0;
                try { timeMs = std::stoll(timeVal); } catch (...) { break; }

                size_t trackPos = json.find("\"track\"", timeEnd);
                if (trackPos == std::string::npos || trackPos >= arrEnd) break;
                size_t trackColon = json.find(":", trackPos);
                size_t trackEnd = json.find_first_of(",}", trackColon);
                if (trackEnd == std::string::npos) break;
                std::string trackVal = json.substr(trackColon + 1, trackEnd - trackColon - 1);
                trackVal.erase(0, trackVal.find_first_not_of(" \t\n\r"));
                trackVal.erase(trackVal.find_last_not_of(" \t\n\r") + 1);
                int track = 0;
                try { track = std::stoi(trackVal); } catch (...) { break; }

                if (track >= 0 && track < 6) {
                    noteTimeData.push_back({timeMs, track});
                }
                pos = trackEnd + 1;
            }
        }
    }

    if (noteTimeData.empty()) {
        printf("[GameWindow] 谱面为空\n");
        return false;
    }

    // 填充 analysisResult（供 loadSongForPlaying 使用）
    analysisResult.success = true;
    analysisResult.bpm = bpm;
    analysisResult.chart = noteTimeData;
    analysisResult.metadata.title = title;
    analysisFilePath = mp3Path;
    currentSongName = title.empty() ? "Unknown" : title;
    currentBPM = bpm;

    printf("[GameWindow] 谱面加载成功: %s, BPM=%.1f, %zu个音符\n",
           currentSongName.c_str(), bpm, noteTimeData.size());
    fflush(stdout);
    return true;
}

// 统一的歌曲选择+启动（键盘Enter/鼠标点击共用）
void GameWindow::startSelectedSong(SongListItem& selected) {
    if (selected.name == "[Demo] TESO") {
        tracks.clear();
        scoreSystem.reset();
        initTracks();
        loadDemoSong();
        audioManager.generateSyncedBGM(noteTimeData);
        gameStartTime = GetTickCount64();
        currentTime = 0;
        gameState = PLAYING;
        prevCombo = 0;
        hitAnims.clear();
        textAnims.clear();
        audioManager.playBGM();
        return;
    }

    // 有预置谱面 → 加载谱面，进入分析确认界面（ANALYZING → DELEGATE → PLAYING）
    if (selected.hasChart) {
        std::string chartPath = selected.filePath + ".chart.json";
        if (loadChartFromFile(chartPath, selected.filePath)) {
            manualBPM = analysisResult.bpm;
            manualOffset = 0;
            analysisDone = true;
            analysisJustOpened = true;
            analysisFilePath = selected.filePath;
            gameState = ANALYZING;
            return;
        }
        // 谱面加载失败 → 退回歌曲列表
        printf("[GameWindow] 谱面加载失败: %s\n", chartPath.c_str());
        fflush(stdout);
        isShowingSongList = true;
        return;
    }

    // 无预置谱面 → 尝试分析MP3（需要ffmpeg在PATH中）
    analysisFilePath = selected.filePath;
    currentSongName = selected.name;
    SongAnalyzer analyzer;
    analysisResult = analyzer.analyze(selected.filePath);
    if (analysisResult.success) {
        manualBPM = analysisResult.bpm;
        manualOffset = 0;
        analysisDone = true;
        analysisJustOpened = true;
        gameState = ANALYZING;
    } else {
        // 分析失败（无ffmpeg等）→ 退回歌曲列表
        printf("[GameWindow] 音频分析失败: %s\n", selected.filePath.c_str());
        printf("[GameWindow] 需要安装ffmpeg: https://ffmpeg.org/download.html\n");
        printf("[GameWindow] 或放入预置谱面: %s\n", (selected.filePath + ".chart.json").c_str());
        fflush(stdout);
        isShowingSongList = true;
    }
}

void GameWindow::refreshSongList() {
    songList.clear();
    std::string songsDir = exeDir + "/songs/";

    // 跨平台文件系统扫描
    try {
        for (const auto& entry : std::filesystem::directory_iterator(songsDir)) {
            if (!entry.is_regular_file()) continue;
            std::string filename = entry.path().filename().string();
            std::string extension = entry.path().extension().string();

            if (extension == ".mp3" || extension == ".wav") {
                // 用u8string()获取UTF-8歌名，SFML渲染不乱码
                std::string name = entry.path().stem().u8string();
                std::string path = entry.path().string();
                bool hasChart = false;
                std::string chartPath = path + ".chart.json";
                FILE* f = fopen(chartPath.c_str(), "r");
                if (f) { fclose(f); hasChart = true; }
                songList.push_back({name, path, hasChart});
            } else if (extension == ".beatpixel") {
                std::string name = entry.path().stem().u8string();
                songList.push_back({name, entry.path().string(), true});
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        printf("[GameWindow] 无法访问songs目录: %s\n", songsDir.c_str());
    }

    // 加一个Demo歌曲
    songList.insert(songList.begin(), {"[Demo] TESO", "", false});

    printf("[GameWindow] 歌曲列表: %zu首\n", songList.size());
    fflush(stdout);
}

void GameWindow::handleSongListInput() {
    // 只处理窗口关闭
    {
        using namespace _easyx_impl;
        if (g_window && g_windowOpen) {
            while (const auto event = g_window->pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    g_windowOpen = false;
                    isRunning = false;
                    return;
                }
            }
        }
    }

    bool escPressed = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
    bool upPressed = (GetAsyncKeyState('W') & 0x8000) != 0;
    bool downPressed = (GetAsyncKeyState('S') & 0x8000) != 0;
    bool enterPressed = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;

    static bool prevEsc = false, prevUp = false, prevDown = false, prevEnter = false;

    // 防粘滞：刚打开时同步static prev为当前按键状态
    if (songListJustOpened) {
        songListJustOpened = false;
        mouseWasPressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);  // 同步鼠标状态，防止同一点击穿透
        prevEsc = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
        prevUp = (GetAsyncKeyState('W') & 0x8000) != 0;
        prevDown = (GetAsyncKeyState('S') & 0x8000) != 0;
        prevEnter = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
        return;
    }
    bool escJust = escPressed && !prevEsc;
    bool upJust = upPressed && !prevUp;
    bool downJust = downPressed && !prevDown;
    bool enterJust = enterPressed && !prevEnter;
    prevEsc = escPressed; prevUp = upPressed; prevDown = downPressed; prevEnter = enterPressed;

    if (escJust) {
        isShowingSongList = false;
        gameState = MENU;
        return;
    }

    int maxIdx = (int)songList.size() - 1;
    if (maxIdx < 0) maxIdx = 0;

    if (upJust) {
        songListSelection--;
        if (songListSelection < 0) songListSelection = maxIdx;
    }
    if (downJust) {
        songListSelection++;
        if (songListSelection > maxIdx) songListSelection = 0;
    }

    if (enterJust && !songList.empty()) {
        auto& selected = songList[songListSelection];
        isShowingSongList = false;
        startSelectedSong(selected);
    }

    // 鼠标点击歌曲列表：单击直接开始
    if (isMouseClick()) {
        sf::Vector2i mp = sf::Mouse::getPosition(*_easyx_impl::g_window);
        float mx = (float)mp.x, my = (float)mp.y;
        float panelW = 500.0f, panelH = 400.0f;
        float panelX = (width - panelW) / 2.0f;
        float panelY = (height - panelH) / 2.0f;
        float itemH = 36.0f;
        float startY = panelY + 60;

        for (int i = 0; i < (int)songList.size(); i++) {
            float itemY = startY + i * itemH;
            if (isPointInRect(mx, my, panelX + 20, itemY, panelW - 40, itemH)) {
                songListSelection = i;
                if (!songList.empty()) {
                    auto& selected = songList[songListSelection];
                    isShowingSongList = false;
                    startSelectedSong(selected);
                }
                break;
            }
        }
    }
}

// ========== 代管轨道选择界面输入处理 ==========
void GameWindow::handleTrackDelegateInput() {
    // 窗口事件
    {
        using namespace _easyx_impl;
        if (g_window && g_windowOpen) {
            while (const auto event = g_window->pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    g_windowOpen = false;
                    isRunning = false;
                    return;
                }
            }
        }
    }

    bool upPressed = (GetAsyncKeyState('W') & 0x8000) != 0;
    bool downPressed = (GetAsyncKeyState('S') & 0x8000) != 0;
    bool enterPressed = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
    bool escPressed = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
    bool spacePressed = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

    static bool prevUp = false, prevDown = false, prevEnter = false, prevEsc = false, prevSpace = false;

    // 刚进入代管界面时同步所有按键状态，防止分析界面的Enter穿透
    if (trackDelegateJustOpened) {
        trackDelegateJustOpened = false;
        prevUp = upPressed; prevDown = downPressed;
        prevEnter = enterPressed; prevEsc = escPressed;
        prevSpace = spacePressed;
        mouseWasPressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
        return;
    }

    bool upJ = upPressed && !prevUp;
    bool dnJ = downPressed && !prevDown;
    bool enJ = enterPressed && !prevEnter;
    bool esJ = escPressed && !prevEsc;
    bool spJ = spacePressed && !prevSpace;

    prevUp = upPressed; prevDown = downPressed;
    prevEnter = enterPressed; prevEsc = escPressed; prevSpace = spacePressed;

    if (upJ) { trackDelegateSel = (trackDelegateSel + 5) % 6; }
    if (dnJ) { trackDelegateSel = (trackDelegateSel + 1) % 6; }
    if (spJ) { trackAutoPlay[trackDelegateSel] = !trackAutoPlay[trackDelegateSel]; }
    if (esJ) { gameState = MENU; }
    if (enJ) { startDelegatedGame(); }

    // 鼠标点击
    if (isMouseClick()) {
        sf::Vector2i mp = sf::Mouse::getPosition(*_easyx_impl::g_window);
        float mx = (float)mp.x, my = (float)mp.y;
        float panelW = 450.0f, panelH = 420.0f;
        float panelX = (width - panelW) / 2.0f;
        float panelY = (height - panelH) / 2.0f;
        float startY = panelY + 100.0f;
        float itemH = 32.0f;
        for (int i = 0; i < 6; i++) {
            float iy = startY + i * itemH;
            if (mx >= panelX + 50 && mx <= panelX + panelW - 50 &&
                my >= iy && my <= iy + itemH) {
                trackDelegateSel = i;
                trackAutoPlay[i] = !trackAutoPlay[i];
                break;
            }
        }
        // 确认按钮
        float btnY = startY + 6 * itemH + 30;
        if (mx >= panelX + 100 && mx <= panelX + panelW - 100 &&
            my >= btnY && my <= btnY + 36) {
            startDelegatedGame();
        }
    }
}
