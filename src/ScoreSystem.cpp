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

void ScoreSystem::addJudgement(Judgement judgement) {
    lastJudgement = judgement;
    switch (judgement) {
        case PERFECT:
            perfectCount++;
            currentCombo++;
            totalScore += 100 + currentCombo; // 基础100分 + Combo加成
            break;
        case GOOD:
            goodCount++;
            currentCombo++;
            totalScore += 50 + currentCombo; // 基础50分 + Combo加成
            break;
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