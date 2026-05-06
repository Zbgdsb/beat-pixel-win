#include "Note.h"
#include <cmath>

Note::Note(int track, long long judgeTime, int judgeY, double speed) 
    : track(track), judgeTime(judgeTime), judgeY(judgeY), speed(speed), isJudged(false) {
    // 计算初始Y坐标：根据判定时间和速度，初始位置在屏幕上方
    y = judgeY - speed * (judgeTime - GetTickCount64()) / 1000 * 60; // 60帧每秒
}

bool Note::update(long long currentTime) {
    if (isJudged) return false;
    // 计算当前Y坐标
    y = judgeY - speed * (judgeTime - currentTime) / 1000 * 60;
    // 如果Y超过屏幕底部+100，返回true需要删除
    return y > judgeY + 200;
}

Judgement Note::judge(long long pressTime) {
    if (isJudged) return NONE;
    isJudged = true;
    long long diff = abs(pressTime - judgeTime);
    if (diff <= 50) {
        return PERFECT;
    } else if (diff <= 150) {
        return GOOD;
    } else {
        return MISS;
    }
}

void NormalNote::draw(int trackX) {
    if (isJudged) return;
    // 绘制普通音符：矩形，像素风格
    setfillcolor(RGB(0, 255, 255));
    fillrectangle(trackX + 10, y - 20, trackX + 90, y + 20);
    setlinecolor(RGB(255, 255, 255));
    rectangle(trackX + 10, y - 20, trackX + 90, y + 20);
}