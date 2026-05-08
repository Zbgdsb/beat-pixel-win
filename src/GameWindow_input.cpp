/**
 * @file GameWindow_input.cpp
 * @brief GameWindow输入处理 - V2.0
 * @details 包含：菜单输入、游戏输入、结算输入
 */

// 注意：此文件内容将合并到GameWindow.cpp中编译
// 由于Makefile只编译GameWindow.cpp，需要将此文件include到GameWindow.cpp末尾
// 或者在Makefile中添加此文件

#include "GameWindow.h"
#include <cstdio>

// ========== 菜单输入处理（V2.0新增） ==========

void GameWindow::handleMenuInput() {
    if (!processWindowEvents()) {
        isRunning = false;
        return;
    }

    // 上下选择
    bool upPressed = (GetAsyncKeyState('W') & 0x8000) || (GetAsyncKeyState(VK_ESCAPE) & 0x8000);
    bool downPressed = (GetAsyncKeyState('S') & 0x8000);
    bool enterPressed = (GetAsyncKeyState(VK_RETURN) & 0x8000);

    // 防重复：只在按下瞬间响应
    bool anyKey = upPressed || downPressed || enterPressed;
    if (anyKey && !keyMenuWasPressed) {
        if (upPressed) {
            menuSelection--;
            if (menuSelection < 0) menuSelection = 3;
        }
        if (downPressed) {
            menuSelection++;
            if (menuSelection > 3) menuSelection = 0;
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

                case 2: // 难度选择 - 循环切换
                    difficultySelection = (difficultySelection + 1) % 3;
                    currentDifficulty = (Difficulty)difficultySelection;
                    break;

                case 3: // 退出
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
        SHORT keyState = GetAsyncKeyState(TRACK_KEYS[i]);
        keyPressed[i] = (keyState & 0x8000) != 0;

        if (keyPressed[i]) {
            keyGlowAlpha[i] = 255;

            if (!keyWasPressed[i]) {
                int comboBefore = scoreSystem.getCurrentCombo();

                NoteTrack::JudgeResult result = tracks[i]->handlePress(currentTime);

                if (result.judgement != NONE) {
                    scoreSystem.addJudgement(result.judgement);
                    lastJudgement = result.judgement;
                    judgementDisplayTimer = 30;

                    spawnHitAnim(i, result.noteY, result.judgement);
                    spawnTextPopup(i, result.judgement);

                    if (result.judgement == PERFECT) audioManager.playHit(true);
                    else if (result.judgement == GOOD) audioManager.playHit(false);
                    else if (result.judgement == MISS) audioManager.playMiss();
                }

                if (scoreSystem.getCurrentCombo() > comboBefore && comboBefore > 0) {
                    comboAnim.elapsed = 0.0f;
                }
            }
        }
        keyWasPressed[i] = keyPressed[i];
    }

    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        audioManager.stopBGM();
        gameState = MENU;
    }
}

// ========== 结算输入处理 ==========

void GameWindow::handleResultInput() {
    if (!processWindowEvents()) {
        isRunning = false;
        return;
    }
    if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
        // 保存排行榜
        dataManager.saveResult(currentSongName, (int)currentDifficulty,
                               scoreSystem.getTotalScore(), scoreSystem.getMaxCombo());

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
    }
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        // 保存排行榜后返回菜单
        dataManager.saveResult(currentSongName, (int)currentDifficulty,
                               scoreSystem.getTotalScore(), scoreSystem.getMaxCombo());
        gameState = MENU;
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
    currentTime = GetTickCount64() - gameStartTime;

    for (auto& track : tracks) {
        auto autoMisses = track->update(currentTime, scoreSystem);
        for (auto& miss : autoMisses) {
            spawnHitAnim(miss.trackId, miss.y, MISS);
            spawnTextPopup(miss.trackId, MISS);
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
