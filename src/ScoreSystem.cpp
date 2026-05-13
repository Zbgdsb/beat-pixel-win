/**
 * @file ScoreSystem.cpp
 * @brief 计分系统实现 - V2.0新增Combo加成
 * @details V2.0变更：
 *          1. Combo加成倍率：10连×1.1 / 50连×1.3 / 100连×1.5
 *          2. 得分公式：基础分 × Combo倍率 + Combo数
 */
#include "ScoreSystem.h"

ScoreSystem::ScoreSystem() {
    reset();
}

void ScoreSystem::reset() {
    totalScore = 0;
    currentCombo = 0;
    maxCombo = 0;
    perfectCount = 0;
    goodCount = 0;
    missCount = 0;
    lastJudgement = NONE;
}

/**
 * @brief 计算Combo加成倍率
 * @details 阶梯式加成，Combo越高倍率越大
 *          - < 10 combo: 1.0倍（无加成）
 *          - >= 10 combo: 1.1倍
 *          - >= 50 combo: 1.3倍
 *          - >= 100 combo: 1.5倍
 */
float ScoreSystem::getComboMultiplier(int combo) const {
    if (combo >= 100) return 1.5f;
    if (combo >= 50)  return 1.3f;
    if (combo >= 10)  return 1.1f;
    return 1.0f;
}

/**
 * @brief 获取当前Combo等级描述
 */
const char* ScoreSystem::getComboLevel() const {
    if (currentCombo >= 100) return "×1.5";
    if (currentCombo >= 50)  return "×1.3";
    if (currentCombo >= 10)  return "×1.1";
    return "";
}

/**
 * @brief 添加判定结果并计算得分
 * @details V2.0得分公式：
 *          Perfect: (100 + currentCombo) × comboMultiplier
 *          Good:    (50 + currentCombo) × comboMultiplier
 *          Miss:    Combo重置
 */
void ScoreSystem::addJudgement(Judgement judgement) {
    lastJudgement = judgement;
    switch (judgement) {
        case PERFECT: {
            perfectCount++;
            currentCombo++;
            float multiplier = getComboMultiplier(currentCombo);
            int baseScore = 100 + currentCombo;
            totalScore += (int)(baseScore * multiplier);
            break;
        }
        case GOOD: {
            goodCount++;
            currentCombo++;
            float multiplier = getComboMultiplier(currentCombo);
            int baseScore = 50 + currentCombo;
            totalScore += (int)(baseScore * multiplier);
            break;
        }
        case MISS:
            missCount++;
            currentCombo = 0; // Miss重置Combo
            break;
        default:
            break;
    }
    if (currentCombo > maxCombo) {
        maxCombo = currentCombo;
    }
}
