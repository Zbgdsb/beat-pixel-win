/**
 * @file Note.cpp
 * @brief Note基类与NormalNote派生类实现
 */
#include "Note.h"
#include <cmath>

/**
 * @brief Note基类构造函数
 * @param track     轨道编号 0-3
 * @param judgeTime 音符应该被判定的时间戳(ms)
 * @param judgeY    判定线Y坐标
 * @param speed     下落速度 px/帧
 */
Note::Note(int track, long long judgeTime, int judgeY, double speed)
    : track(track), judgeY(judgeY), speed(speed),
      isJudged(false), judgeTime(judgeTime) {
    // 初始Y坐标：根据判定时间差计算音符初始位置
    // 音符从屏幕上方开始，随时间推移向判定线移动
    y = judgeY - (int)(speed * 60.0 * 3.0); // 预留3秒的下落距离
}

/**
 * @brief 更新音符位置
 * @details 根据当前时间和判定时间的差值计算Y坐标
 *          公式：y = judgeY - speed * (judgeTime - currentTime) / 1000 * 60
 * @param currentTime 当前时间戳(ms)
 * @return true表示音符超出屏幕需要删除
 */
bool Note::update(long long currentTime) {
    if (isJudged) return false;
    // 基于时间差计算当前位置：越接近判定时间，y越接近judgeY
    double timeDiff = (judgeTime - currentTime) / 1000.0; // 转换为秒
    y = judgeY - (int)(speed * timeDiff * 60.0); // speed是px/帧，60帧/秒
    return y > judgeY + 300; // 超出屏幕底部
}

/**
 * @brief 执行判定逻辑
 * @details 根据按键时间与预定判定时间的差值判断等级
 *          Perfect: |pressTime - judgeTime| <= 50ms
 *          Good:    |pressTime - judgeTime| <= 150ms
 *          Miss:    |pressTime - judgeTime| > 150ms
 * @param pressTime 按键按下的时间戳(ms)
 * @return 判定结果
 */
Judgement Note::judge(long long pressTime) {
    if (isJudged) return NONE;
    isJudged = true;
    long long diff = std::abs(pressTime - judgeTime);
    if (diff <= 50) {
        return PERFECT;
    } else if (diff <= 150) {
        return GOOD;
    } else {
        return MISS;
    }
}

// ========== NormalNote 派生类实现 ==========

/**
 * @brief NormalNote构造函数
 * @param color 音符显示颜色（不同轨道不同颜色便于区分）
 */
NormalNote::NormalNote(int track, long long judgeTime, int judgeY, double speed, COLORREF color)
    : Note(track, judgeTime, judgeY, speed), color(color) {
}

/**
 * @brief 绘制普通音符
 * @details 像素风格：带边框的实心矩形，已判定的音符不绘制
 * @param trackX 所属轨道的左上角X坐标
 */
void NormalNote::draw(int trackX) {
    if (isJudged) return;
    // 音符矩形区域：在轨道内居中，宽80px高30px
    int left   = trackX + 10;
    int top    = y - 15;
    int right  = trackX + 90;
    int bottom = y + 15;

    // 绘制填充矩形（音符主体）
    setfillcolor(color);
    fillrectangle(left, top, right, bottom);

    // 绘制白色边框（像素风格描边）
    setlinecolor(RGB(255, 255, 255));
    rectangle(left, top, right, bottom);

    // 绘制内部高光线（像素风格细节）
    setlinecolor(RGB(255, 255, 255));
    line(left + 3, top + 3, right - 3, top + 3);
}
