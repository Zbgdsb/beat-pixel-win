/**
 * @file BeatParser.h
 * @brief 节拍解析类定义 - V2.0新增
 * @details 支持导入本地MP3音频文件，自动解析音频波形生成音符时间点
 *          使用基于能量峰值的节拍检测算法，支持自定义BPM调整
 */
#pragma once
#include <string>
#include <vector>
#include <utility>
#include <cstdint>

/**
 * @brief 节拍解析器
 * @details 通过分析音频波形的能量变化来检测节拍位置
 *          将检测到的节拍自动分配到4个轨道上，生成游戏谱面
 */
class BeatParser {
public:
    /**
     * @brief 解析结果结构体
     */
    struct ParseResult {
        bool success;                                           // 是否解析成功
        std::string errorMessage;                               // 错误信息
        std::vector<std::pair<long long, int>> noteTimeData;   // (时间戳ms, 轨道编号)
        long long lastNoteTime;                                 // 最后一个音符时间
        float detectedBPM;                                      // 检测到的BPM
        float duration;                                         // 音频总时长(秒)
    };

    BeatParser();
    ~BeatParser() = default;

    /**
     * @brief 解析音频文件生成谱面
     * @param filePath 音频文件路径（支持MP3/WAV/OGG等SFML支持的格式）
     * @param targetBPM 目标BPM（0表示自动检测）
     * @return 解析结果
     */
    ParseResult parse(const std::string& filePath, float targetBPM = 0.0f);

    /**
     * @brief 检查文件格式是否支持
     * @param filePath 文件路径
     * @return 是否支持
     */
    static bool isSupportedFormat(const std::string& filePath);

private:
    // 音频数据
    std::vector<float> samples;     // 归一化后的音频采样数据（单声道）
    int sampleRate;                 // 采样率

    /**
     * @brief 从音频文件读取采样数据
     * @param filePath 文件路径
     * @return 是否读取成功
     */
    bool loadAudioFile(const std::string& filePath);

    /**
     * @brief 基于能量峰值的节拍检测
     * @param targetBPM 目标BPM（用于调整灵敏度）
     * @return 检测到的节拍时间点列表(ms)
     */
    std::vector<long long> detectBeats(float targetBPM);

    /**
     * @brief 计算音频能量
     * @param start 起始采样索引
     * @param count 采样数量
     * @return 能量值
     */
    float calculateEnergy(size_t start, size_t count);

    /**
     * @brief 估算BPM
     * @param beatTimes 节拍时间点列表
     * @return 估算的BPM值
     */
    float estimateBPM(const std::vector<long long>& beatTimes);

    /**
     * @brief 将节拍分配到4个轨道
     * @param beatTimes 节拍时间点列表(ms)
     * @return (时间戳ms, 轨道编号)列表
     */
    std::vector<std::pair<long long, int>> distributeToTracks(
        const std::vector<long long>& beatTimes);
};
