/**
 * @file Note.h
 * @brief 音符基类与派生类定义
 * @details 定义音符通用属性和方法，派生类实现具体绘制逻辑
 *          判定规则：Perfect(±50ms) / Good(±150ms) / Miss(>150ms)
 */
#pragma once
#include "graphics.h"
#include <cstdlib>

// 判定结果枚举
enum Judgement {
    PERFECT,  // 完美判定 ±50ms
    GOOD,     // 较好判定 ±150ms
    MISS,     // 未命中
    NONE      // 无判定（用于按键时无对应音符的情况）
};

class Note {
protected:
    int track;          // 轨道编号 0-3
    int y;              // 当前Y坐标（像素）
    int judgeY;         // 判定线Y坐标
    double speed;       // 下落速度 px/帧
    bool isJudged;      // 是否已完成判定
    long long judgeTime;// 应该被判定的时间戳(ms)

public:
    Note(int track, long long judgeTime, int judgeY, double speed);
    virtual ~Note() = default;

    /**
     * @brief 更新音符位置
     * @param currentTime 当前时间戳(ms)
     * @return true表示音符需要被删除（已离开屏幕）
     */
    virtual bool update(long long currentTime);

    /**
     * @brief 绘制音符（纯虚函数，由派生类实现）
     * @param trackX 所属轨道的左上角X坐标
     */
    virtual void draw(int trackX) = 0;

    /**
     * @brief 执行判定
     * @param pressTime 按键按下的时间戳(ms)
     * @return 判定结果
     */
    virtual Judgement judge(long long pressTime);

    // 音符状态查询
    bool isPastJudgeLine() const { return y > judgeY + 30; }
    bool isOffScreen() const { return y > judgeY + 300; }
    void markJudged() { isJudged = true; }

    // Getter方法
    int getTrack() const { return track; }
    bool getIsJudged() const { return isJudged; }
    long long getJudgeTime() const { return judgeTime; }
    int getY() const { return y; }
};

/**
 * @brief 普通音符派生类
 * @details 实现基础的矩形像素风格音符绘制
 */
class NormalNote : public Note {
private:
    COLORREF color; // 音符颜色，不同轨道不同颜色便于区分

public:
    NormalNote(int track, long long judgeTime, int judgeY, double speed, COLORREF color);
    void draw(int trackX) override;
};
