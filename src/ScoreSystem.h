/**
 * @file ScoreSystem.h
 * @brief 计分系统定义 - V2.0新增Combo加成
 * @details 判定规则：Perfect(±120ms)得100分 / Good(±300ms)得50分 / Miss重置Combo
 *          Combo加成：10连击×1.1 / 50连击×1.3 / 100连击×1.5
 */
#pragma once
#include "Note.h"

class ScoreSystem {
private:
    int totalScore;
    int currentCombo;
    int maxCombo;
    int perfectCount;
    int goodCount;
    int missCount;
    Judgement lastJudgement;

    /**
     * @brief 计算Combo加成倍率
     * @details 10 combo → 1.1倍
     *          50 combo → 1.3倍
     *          100 combo → 1.5倍
     * @param combo 当前Combo数
     * @return 加成倍率
     */
    float getComboMultiplier(int combo) const;

public:
    ScoreSystem();
    void reset();
    void addJudgement(Judgement judgement);

    // Getter方法
    int getTotalScore() const { return totalScore; }
    int getCurrentCombo() const { return currentCombo; }
    int getMaxCombo() const { return maxCombo; }
    int getPerfectCount() const { return perfectCount; }
    int getGoodCount() const { return goodCount; }
    int getMissCount() const { return missCount; }
    Judgement getLastJudgement() const { return lastJudgement; }

    /**
     * @brief 获取当前Combo等级描述
     * @return 等级字符串（如"×1.1"）
     */
    const char* getComboLevel() const;
};
