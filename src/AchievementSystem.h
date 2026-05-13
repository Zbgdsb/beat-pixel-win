/**
 * @file AchievementSystem.h
 * @brief 成就系统
 * @details 10个成就，覆盖游戏各方面，数据持久化到本地文件
 */
#pragma once
#include <string>
#include <vector>
#include <functional>

struct Achievement {
    int id;
    const char* name;           // 成就名称
    const char* description;    // 成就描述
    const char* icon;           // 图标emoji
    bool unlocked;              // 是否已解锁
    std::string unlockedTime;   // 解锁时间

    Achievement() : id(0), name(""), description(""), icon(""), unlocked(false) {}
    Achievement(int id, const char* name, const char* desc, const char* icon)
        : id(id), name(name), description(desc), icon(icon), unlocked(false) {}
};

class AchievementSystem {
public:
    AchievementSystem();
    ~AchievementSystem() = default;

    /**
     * @brief 初始化成就列表并加载存档
     * @param savePath 存档文件路径
     */
    void init(const std::string& savePath);

    /**
     * @brief 检查并解锁成就（每次游戏结算时调用）
     * @param songName 歌曲名
     * @param difficulty 难度 0=Easy 1=Normal 2=Hard
     * @param score 得分
     * @param maxCombo 最大连击
     * @param perfectCount Perfect数
     * @param goodCount Good数
     * @param missCount Miss数
     * @param totalSongsPlayed 累计游玩歌曲数
     * @param customSongCount 自定义谱面数
     * @param playbackSpeed 播放速度倍率
     * @return 新解锁的成就ID列表（用于弹窗提示）
     */
    std::vector<int> checkAchievements(
        const std::string& songName,
        int difficulty,
        int score,
        int maxCombo,
        int perfectCount,
        int goodCount,
        int missCount,
        int totalSongsPlayed,
        int customSongCount,
        float playbackSpeed
    );

    /**
     * @brief 获取所有成就列表
     */
    const std::vector<Achievement>& getAll() const { return achievements; }

    /**
     * @brief 获取已解锁数量
     */
    int getUnlockedCount() const;

    /**
     * @brief 保存到文件
     */
    void save();

private:
    std::vector<Achievement> achievements;
    std::string saveFilePath;

    void loadSave();
    void unlock(int id);
};
