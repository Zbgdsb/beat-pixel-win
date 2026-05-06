/**
 * @file NoteTrack.cpp
 * @brief NoteTrack类实现 - 单个轨道管理
 * @details 负责轨道绘制、音符列表管理、按键判定触发逻辑
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
 * @param currentTime 当前时间戳(ms)
 * @param scoreSystem 计分系统引用，用于记录MISS判定
 */
void NoteTrack::update(long long currentTime, ScoreSystem& scoreSystem) {
    // 从后往前遍历，安全删除
    for (int i = (int)notes.size() - 1; i >= 0; i--) {
        notes[i]->update(currentTime);

        // 检测：音符已过判定线且未被判定 -> 自动MISS
        if (!notes[i]->getIsJudged() && notes[i]->isPastJudgeLine()) {
            scoreSystem.addJudgement(MISS);
            notes[i]->markJudged(); // 标记为已判定，避免重复计分
        }

        // 移除已判定且完全离开屏幕的音符
        if (notes[i]->getIsJudged() && notes[i]->isOffScreen()) {
            notes.erase(notes.begin() + i);
        }
    }
}

/**
 * @brief 绘制轨道背景和判定线
 * @details 绘制半透明轨道底色 + 判定线 + 按键提示文字
 */
void NoteTrack::draw() {
    // 1. 绘制轨道背景（深灰色半透明效果）
    setfillcolor(RGB(30, 30, 30));
    fillrectangle(x, 0, x + width, getheight());

    // 2. 绘制轨道分隔线（左右边界）
    setlinecolor(RGB(80, 80, 80));
    line(x, 0, x, getheight());
    line(x + width, 0, x + width, getheight());

    // 3. 绘制判定线（亮白色横线）
    setlinecolor(RGB(255, 255, 255));
    setlinestyle(PS_SOLID, 3);
    line(x + 5, judgeY, x + width - 5, judgeY);
    setlinestyle(PS_SOLID, 1); // 恢复默认线型

    // 4. 绘制判定区域提示（判定线上下方的半透明区域）
    setfillcolor(RGB(50, 50, 50));
    fillrectangle(x + 2, judgeY - 30, x + width - 2, judgeY + 30);

    // 5. 绘制按键提示文字（判定线下方）
    settextcolor(RGB(200, 200, 200));
    settextstyle(20, 0, _T("Consolas"));
    char keyStr[2] = {key, '\0'};
    // 居中绘制按键字母
    int textW = textwidth(keyStr);
    outtextxy(x + (width - textW) / 2, judgeY + 40, keyStr);

    // 6. 绘制该轨道的所有音符
    for (auto& note : notes) {
        note->draw(x); // 传入轨道X坐标，音符在轨道内绘制
    }
}

/**
 * @brief 处理该轨道的按键按下事件
 * @details 遍历轨道内未判定的音符，找到时间上最接近的音符进行判定
 *          优先判定距离判定线最近的音符（即judgeTime最接近pressTime的）
 * @param pressTime 按键按下的时间戳(ms)
 * @return 判定结果（PERFECT/GOOD/MISS/NONE）
 */
Judgement NoteTrack::handlePress(long long pressTime) {
    Note* closestNote = nullptr;
    long long minDiff = LLONG_MAX;

    // 找到时间上最接近按键时刻的未判定音符
    for (auto& note : notes) {
        if (note->getIsJudged()) continue;

        long long diff = std::abs(pressTime - note->getJudgeTime());
        // 只考虑在判定窗口内（±150ms）的音符
        if (diff <= 150 && diff < minDiff) {
            minDiff = diff;
            closestNote = note.get();
        }
    }

    // 找到可判定的音符，执行判定
    if (closestNote) {
        return closestNote->judge(pressTime);
    }

    // 按键时该轨道没有可判定的音符，返回NONE（不扣分）
    return NONE;
}
