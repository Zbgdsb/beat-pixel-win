/**
 * @file DataManager.h
 * @brief 数据存储类定义 - V2.0新增
 * @details 本地排行榜功能：每首歌每个难度保存最高得分、最高Combo记录
 *          数据以文本文件形式持久化存储，重启程序不丢失
 */
#pragma once
#include <string>
#include <vector>
#include <map>

/**
 * @brief 单条排行榜记录
 */
struct LeaderboardEntry {
    std::string songName;       // 歌曲名称
    int difficulty;             // 难度等级 0=Easy 1=Normal 2=Hard
    int highScore;              // 最高得分
    int maxCombo;               // 最高Combo
    int playCount;              // 游玩次数
    std::string lastPlayed;     // 最后游玩时间

    LeaderboardEntry()
        : difficulty(0), highScore(0), maxCombo(0), playCount(0) {}
};

/**
 * @brief 数据管理器
 * @details 管理排行榜数据的读取、写入、查询
 *          数据文件格式：每行一条记录，字段用 | 分隔
 */
class DataManager {
public:
    DataManager();
    ~DataManager() = default;

    /**
     * @brief 初始化数据管理器
     * @param dataPath 数据文件路径（默认在程序目录下）
     * @return 是否初始化成功
     */
    bool init(const std::string& dataPath = "leaderboard.dat");

    /**
     * @brief 保存游戏结果
     * @param songName 歌曲名称
     * @param difficulty 难度等级
     * @param score 本次得分
     * @param combo 本次最大Combo
     * @return 是否保存成功（是否刷新了记录）
     */
    bool saveResult(const std::string& songName, int difficulty,
                    int score, int combo);

    /**
     * @brief 获取指定歌曲指定难度的最高分
     */
    int getHighScore(const std::string& songName, int difficulty) const;

    /**
     * @brief 获取指定歌曲指定难度的最高Combo
     */
    int getMaxCombo(const std::string& songName, int difficulty) const;

    /**
     * @brief 获取排行榜（按分数降序）
     * @param songName 歌曲名称（空字符串表示全部）
     * @param difficulty 难度等级（-1表示全部）
     * @param limit 最大返回数量
     * @return 排行榜条目列表
     */
    std::vector<LeaderboardEntry> getLeaderboard(
        const std::string& songName = "",
        int difficulty = -1,
        int limit = 10) const;

    /**
     * @brief 获取总游玩次数
     */
    int getTotalPlayCount() const;

    /**
     * @brief 是否存在该歌曲的记录
     */
    bool hasRecord(const std::string& songName, int difficulty) const;

private:
    std::string filePath;                              // 数据文件路径
    std::vector<LeaderboardEntry> entries;             // 所有排行榜记录

    /**
     * @brief 从文件加载数据
     */
    bool loadFromFile();

    /**
     * @brief 将数据保存到文件
     */
    bool saveToFile();

    /**
     * @brief 生成记录的唯一键
     */
    std::string makeKey(const std::string& songName, int difficulty) const;
};
