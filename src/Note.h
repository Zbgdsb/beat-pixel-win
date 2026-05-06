#pragma once
#include <graphics.h>

// 判定结果枚举
enum Judgement {
    PERFECT,
    GOOD,
    MISS,
    NONE
};

class Note {
protected:
    int track; // 轨道编号 0-3
    int y; // 当前Y坐标
    int judgeY; // 判定线Y坐标
    double speed; // 下落速度 px/帧
    bool isJudged; // 是否已判定
    long long judgeTime; // 应该被判定的时间戳 ms

public:
    Note(int track, long long judgeTime, int judgeY, double speed);
    virtual ~Note() = default;

    // 更新位置，返回true表示音符已经超出屏幕需要删除
    virtual bool update(long long currentTime);
    virtual void draw(int trackX) = 0; // 纯虚函数，派生类实现
    virtual Judgement judge(long long pressTime); // 判定方法

    int getTrack() const { return track; }
    bool getIsJudged() const { return isJudged; }
    long long getJudgeTime() const { return judgeTime; }
};

// 普通音符派生类
class NormalNote : public Note {
public:
    using Note::Note;
    void draw(int trackX) override;
};