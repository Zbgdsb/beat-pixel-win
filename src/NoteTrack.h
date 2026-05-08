/**
 * @file NoteTrack.h
 * @brief NoteTrack类定义 - 单个轨道管理
 * @details 负责轨道绘制、音符列表管理、按键判定触发逻辑
 *          v2.0: 使用像素素材纹理渲染，支持按键按下状态切换和轨道发光效果
 */
#pragma once
#include <vector>
#include <memory>
#include "Note.h"
#include "ScoreSystem.h"
#include "TextureManager.h"
#include <climits>

/**
 * @brief 自动Miss事件信息
 * @details 音符越过判定线未被按下时自动触发Miss，记录位置信息用于播放动画
 */
struct AutoMissInfo {
    int trackId;
    float y;       // 音符Y坐标（屏幕坐标）
    COLORREF color; // 音符颜色
};

class NoteTrack {
private:
    int trackId;        // 轨道ID 0-3
    int x;              // 轨道左上角X坐标
    int width;          // 轨道宽度
    int judgeY;         // 判定线Y坐标
    [[maybe_unused]] double noteSpeed;   // 音符下落速度（传递给Note）
    std::vector<std::unique_ptr<Note>> notes; // 该轨道的音符列表
    [[maybe_unused]] char key;           // 对应按键 A/S/D/F（纹理通过trackId索引）

public:
    NoteTrack(int trackId, int x, int width, int judgeY, double noteSpeed, char key);
    ~NoteTrack() = default;

    void addNote(std::unique_ptr<Note> note);

    /**
     * @brief 更新轨道上所有音符状态
     * @param currentTime 当前时间戳(ms)
     * @param scoreSystem 计分系统引用
     * @return 自动Miss的音符信息列表（用于触发Miss动画）
     */
    std::vector<AutoMissInfo> update(long long currentTime, ScoreSystem& scoreSystem);

    /**
     * @brief 绘制轨道（纹理版本）
     * @details 绘制顺序：轨道背景 → 判定线 → 音符
     * @param tex 纹理管理器引用
     * @param glowAlpha 轨道发光透明度（0-255，按键按下时为255，松开后渐变到0）
     */
    void draw(const TextureManager& tex, int glowAlpha);

    /**
     * @brief 绘制按键按钮
     * @details 根据按键状态显示normal或pressed纹理
     * @param tex 纹理管理器引用
     * @param isPressed 按键是否按下
     */
    void drawKeyButton(const TextureManager& tex, bool isPressed);

    /**
     * @brief 获取判定线Y坐标
     */
    int getJudgeY() const { return judgeY; }

    /**
     * @brief 获取轨道左上角X坐标
     */
    int getX() const { return x; }

    /**
     * @brief 判定结果（包含判定等级和音符位置信息，用于触发动画）
     */
    struct JudgeResult {
        Judgement judgement = NONE;
        float noteY = 0.0f;    // 音符Y坐标（用于定位打击动画）
        COLORREF color = RGB(255, 255, 255); // 音符颜色
    };

    /**
     * @brief 处理该轨道按键按下
     * @param pressTime 按键时间戳(ms)
     * @return 判定结果（含位置信息用于动画）
     */
    JudgeResult handlePress(long long pressTime);

    bool isEmpty() const { return notes.empty(); }
};
