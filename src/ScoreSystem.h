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
};