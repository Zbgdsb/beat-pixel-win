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

// ========== 菜单输入处理（V2.0新增） ==========

void GameWindow::handleMenuInput() {
    if (!processWindowEvents()) {
        isRunning = false;
        return;
    }

    // 上下选择
    bool upPressed = (GetAsyncKeyState('W') & 0x8000) != 0;
    bool downPressed = (GetAsyncKeyState('S') & 0x8000) != 0;
    bool enterPressed = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;

    // 防重复：只在按下瞬间响应
    bool anyKey = upPressed || downPressed || enterPressed;
    if (anyKey && !keyMenuWasPressed) {
        if (upPressed) {
            menuSelection--;
            if (menuSelection < 0) menuSelection = 4;
        }
        if (downPressed) {
            menuSelection++;
            if (menuSelection > 4) menuSelection = 0;
        }
        if (enterPressed) {
            switch (menuSelection) {
                case 0: // 开始游戏 - Demo歌曲
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
                    break;

                case 1: // 导入MP3
                    importRequested = true;
                    break;

                case 2: // 进入设置界面
                    isInSettings = true;
                    settingsMenuSelection = 0;
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
    }
    keyMenuWasPressed = anyKey;

    // 处理MP3导入请求
    if (importRequested) {
        importRequested = false;

        // 使用系统文件对话框（macOS）
        // 由于SFML没有文件对话框API，使用简单的命令行提示
        // 在实际运行时，用户需要将MP3文件放在指定路径
        printf("\n========================================\n");
        printf("  请将MP3文件放到以下路径：\n");
        printf("  /Users/Admin/Desktop/c++期末大作业/BeatPixel/songs/\n");
        printf("  然后输入文件名（不含路径）：\n");
        printf("========================================\n");
        fflush(stdout);

        // 简化方案：自动扫描songs目录下的第一个MP3文件
        std::string songsDir = "/Users/Admin/Desktop/c++期末大作业/BeatPixel/songs/";
        // 尝试使用system命令列出文件
        std::string cmd = "ls " + songsDir + "*.mp3 2>/dev/null | head -1";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (pipe) {
            char buffer[512];
            std::string filePath;
            if (fgets(buffer, sizeof(buffer), pipe)) {
                filePath = buffer;
                // 去掉换行符
                if (!filePath.empty() && filePath.back() == '\n') {
                    filePath.pop_back();
                }
            }
            pclose(pipe);

            if (!filePath.empty()) {
                tracks.clear();
                scoreSystem.reset();
                initTracks();

                if (loadSongFromMP3(filePath)) {
                    audioManager.generateSyncedBGM(noteTimeData);
                    gameStartTime = GetTickCount64();
                    currentTime = 0;
                    gameState = PLAYING;
                    prevCombo = 0;
                    hitAnims.clear();
                    textAnims.clear();
                    audioManager.playBGM();
                } else {
                    printf("导入失败，请检查文件格式\n");
                    fflush(stdout);
                }
            } else {
                printf("未找到MP3文件，请先放入songs目录\n");
                fflush(stdout);
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

                    if (result.judgement == PERFECT) audioManager.playHit(true);
                    else if (result.judgement == GOOD) audioManager.playHit(false);
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
        enterWasPressed = true;
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
    } else if (!enterPressed) {
        enterWasPressed = false;
    }
}

// ========== 动画触发 ==========

void GameWindow::spawnHitAnim(int trackId, float noteY, Judgement j) {
    HitAnim anim;
    anim.x = getTrackX(trackId) + 50.0f;
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
    anim.x = getTrackX(trackId) + 50.0f;
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
    // 先检测按键再处理事件
    bool upPressed = (GetAsyncKeyState('W') & 0x8000) != 0;
    bool downPressed = (GetAsyncKeyState('S') & 0x8000) != 0;
    bool enterPressed = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
    bool escPressed = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;

    if (!processWindowEvents()) {
        isRunning = false;
        return;
    }

    // 防重复触发
    static bool upWasPressed = false;
    static bool downWasPressed = false;
    static bool enterWasPressed = false;
    static bool escWasPressed = false;

    // ESC直接继续游戏
    if (escPressed && !escWasPressed) {
        escWasPressed = true;
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
                break;
            }
        }
    } else if (!enterPressed) {
        enterWasPressed = false;
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
                for (int i = 0; i < 4; i++) {
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
        settingsMenuSelection = (settingsMenuSelection - 1 + 8) % 8;
    } else if (!upPressed) {
        keyUpWasPressed = false;
    }

    if (downPressed && !keyDownWasPressed) {
        keyDownWasPressed = true;
        settingsMenuSelection = (settingsMenuSelection + 1) % 8;
    } else if (!downPressed) {
        keyDownWasPressed = false;
    }

    // 左右调整音量（只有前2个选项）
    if (settingsMenuSelection < 2) {
        if (leftPressed && !keyLeftWasPressed) {
            keyLeftWasPressed = true;
            if (settingsMenuSelection == 0) {
                musicVolume = std::max(0.0f, musicVolume - 0.1f);
                audioManager.setMusicVolume(musicVolume);
            } else {
                effectVolume = std::max(0.0f, effectVolume - 0.1f);
                audioManager.setEffectVolume(effectVolume);
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
            } else {
                effectVolume = std::min(1.0f, effectVolume + 0.1f);
                audioManager.setEffectVolume(effectVolume);
                audioManager.playHit(false);
            }
        } else if (!rightPressed) {
            keyRightWasPressed = false;
        }
    }

    // ENTER确认
    if (enterPressed && !enterWasPressed) {
        enterWasPressed = true;

        // 按键设置选项
        if (settingsMenuSelection >= 2 && settingsMenuSelection < 6) {
            currentKeySettingIndex = settingsMenuSelection - 2;
            keySettingWaitingRelease = false; // 进入设置模式，先等待按键松开
        }
        // 恢复默认按键
        else if (settingsMenuSelection == 6) {
            customKeys[0] = 'A';
            customKeys[1] = 'S';
            customKeys[2] = 'D';
            customKeys[3] = 'F';
        }
        // 保存返回
        else if (settingsMenuSelection == 7) {
            saveConfig();
            isInSettings = false;
            settingsMenuSelection = 0;
        }
    } else if (!enterPressed) {
        enterWasPressed = false;
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
}

// ========== V3.1: 配置保存和加载 ==========

void GameWindow::saveConfig() {
    FILE* fp = fopen("/Users/Admin/Desktop/c++期末大作业/BeatPixel/config.ini", "w");
    if (fp) {
        fprintf(fp, "[Settings]\n");
        fprintf(fp, "music_volume = %.2f\n", musicVolume);
        fprintf(fp, "effect_volume = %.2f\n", effectVolume);
        fprintf(fp, "key1 = %d\n", customKeys[0]);
        fprintf(fp, "key2 = %d\n", customKeys[1]);
        fprintf(fp, "key3 = %d\n", customKeys[2]);
        fprintf(fp, "key4 = %d\n", customKeys[3]);
        fclose(fp);
        printf("[Config] Saved config\n");
    }
}

void GameWindow::loadConfig() {
    FILE* fp = fopen("/Users/Admin/Desktop/c++期末大作业/BeatPixel/config.ini", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "music_volume")) {
                sscanf(line, "music_volume = %f", &musicVolume);
            } else if (strstr(line, "effect_volume")) {
                sscanf(line, "effect_volume = %f", &effectVolume);
            } else if (strstr(line, "key1")) {
                int k;
                sscanf(line, "key1 = %d", &k);
                customKeys[0] = k;
            } else if (strstr(line, "key2")) {
                int k;
                sscanf(line, "key2 = %d", &k);
                customKeys[1] = k;
            } else if (strstr(line, "key3")) {
                int k;
                sscanf(line, "key3 = %d", &k);
                customKeys[2] = k;
            } else if (strstr(line, "key4")) {
                int k;
                sscanf(line, "key4 = %d", &k);
                customKeys[3] = k;
            }
        }
        fclose(fp);
        // 音量范围限制
        musicVolume = std::max(0.0f, std::min(1.0f, musicVolume));
        effectVolume = std::max(0.0f, std::min(1.0f, effectVolume));
        printf("[Config] Loaded config: music=%.2f effect=%.2f keys=[%s, %s, %s, %s]\n", musicVolume, effectVolume,
               getKeyName(customKeys[0]), getKeyName(customKeys[1]), getKeyName(customKeys[2]), getKeyName(customKeys[3]));
    } else {
        // 默认值
        musicVolume = 1.0f;
        effectVolume = 1.0f;
        customKeys[0] = 'A';
        customKeys[1] = 'S';
        customKeys[2] = 'D';
        customKeys[3] = 'F';
        saveConfig();
        printf("[Config] Created new default config\n");
    }
}
