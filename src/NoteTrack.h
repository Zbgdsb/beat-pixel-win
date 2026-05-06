#pragma once
#include <vector>
#include <memory>
#include "Note.h"
#include "ScoreSystem.h"
#include <climits>

class NoteTrack {
private:
    [[maybe_unused]] int trackId; // 轨道ID 0-3
    int x; // 轨道左上角X坐标
    int width; // 轨道宽度
    int judgeY; // 判定线Y坐标
    [[maybe_unused]] double noteSpeed; // 音符下落速度
    std::vector<std::unique_ptr<Note>> notes; // 该轨道的音符列表
    char key; // 对应按键 A/S/D/F

public:
    NoteTrack(int trackId, int x, int width, int judgeY, double noteSpeed, char key);
    ~NoteTrack() = default;

    void addNote(std::unique_ptr<Note> note);
    void update(long long currentTime, ScoreSystem& scoreSystem);
    void draw();
    Judgement handlePress(long long pressTime); // 处理该轨道按键按下
    bool isEmpty() const { return notes.empty(); } // 检测轨道是否无音符
};