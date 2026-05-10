/**
 * @file SongAnalyzer.h
 * @brief 歌曲自动分析模块 - V3.2新增
 * @details 独立模块，不修改现有核心游戏逻辑
 *          功能：BPM自动检测(精度±0.5)、节拍点提取、重音标记、谱面生成、ID3读取
 */
#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "BeatParser.h"

class SongAnalyzer {
public:
    // ========== 数据结构 ==========

    /**
     * @brief 节拍点信息（带重音标记）
     */
    struct BeatInfo {
        long long timeMs;       // 时间戳(ms)
        float energy;           // 能量值
        bool isAccent;          // 是否重音拍（鼓点重的拍子）
    };

    /**
     * @brief 歌曲元信息（ID3标签）
     */
    struct SongMetadata {
        std::string title;      // 歌曲名
        std::string artist;     // 歌手
        std::string album;      // 专辑
        float duration;         // 总时长(秒)
        SongMetadata() : duration(0.0f) {}
    };

    /**
     * @brief 分析结果
     */
    struct AnalysisResult {
        bool success;                                       // 是否成功
        std::string errorMessage;                           // 错误信息
        SongMetadata metadata;                              // 歌曲元信息
        std::vector<BeatInfo> beats;                        // 节拍点列表（带重音标记）
        float bpm;                                          // 检测到的BPM
        float bpmConfidence;                                // BPM置信度(0~100)
        float beatOffset;                                   // 节拍偏移(ms)
        std::vector<std::pair<long long, int>> chart;       // 生成的谱面 (时间戳ms, 轨道编号)
        std::vector<std::pair<long long, int>> originalChart; // 原始谱面（重置用）
        AnalysisResult() : success(false), bpm(0), bpmConfidence(0), beatOffset(0) {}
    };

    SongAnalyzer();
    ~SongAnalyzer() = default;

    /**
     * @brief 完整分析一首歌曲
     * @param filePath 音频文件路径
     * @return 分析结果
     */
    AnalysisResult analyze(const std::string& filePath);

    /**
     * @brief 重新生成谱面（使用新的BPM和偏移）
     * @param result 原始分析结果（会被修改）
     * @param newBPM 新的BPM
     * @param newOffset 新的偏移(ms)
     */
    void regenerateChart(AnalysisResult& result, float newBPM, float newOffset);

    /**
     * @brief 点3拍校准
     * @param tapTimeMs 按下时间戳(ms)
     * @return 计算出的BPM（0表示还需要更多拍）
     */
    float tapTempo(long long tapTimeMs);

    /**
     * @brief 重置点拍状态
     */
    void resetTapTempo();

    /**
     * @brief 获取当前点拍状态
     * @return 已点拍数 (0, 1, 2, 3)
     */
    int getTapCount() const { return tapCount; }

    /**
     * @brief 导出谱面为JSON文件
     * @param filePath 输出文件路径
     * @param result 分析结果
     * @return 是否成功
     */
    static bool exportChart(const std::string& filePath, const AnalysisResult& result);

    /**
     * @brief 读取MP3 ID3标签
     * @param filePath 文件路径
     * @return 元信息
     */
    static SongMetadata readID3Tags(const std::string& filePath);

private:
    // 点拍校准状态
    long long tapTimes[10];     // 最近10次点拍时间
    int tapCount;               // 已点拍数

    // 音频数据（从BeatParser共享）
    std::vector<float> samples;
    int sampleRate;
    std::string currentFilePath;   // V3.3: 当前分析的文件路径

    /**
     * @brief 从音频文件加载采样数据
     */
    bool loadAudio(const std::string& filePath);

    /**
     * @brief 计算短时能量
     */
    float calculateEnergy(size_t start, size_t count);

    /**
     * @brief 基于自相关的BPM检测（精度±0.5）
     * @return (bpm, confidence)
     */
    std::pair<float, float> detectBPMPrecise();

    /**
     * @brief 自相关计算
     */
    std::vector<float> autocorrelation(const std::vector<float>& signal, int maxLag);

    /**
     * @brief 检测节拍点（带重音标记）
     */
    std::vector<BeatInfo> detectBeatsWithAccent();
    std::vector<BeatInfo> detectBeatsViaPython();    // V3.3: Python aubio检测
    std::vector<BeatInfo> detectBeatsFallback();      // V3.3: C++内置降级方案
    float resultBPM = 0;                              // V3.3: Python检测的BPM

    /**
     * @brief 基于BPM的节拍网格生成
     */
    std::vector<long long> generateBeatGrid(float bpm, float offset, float duration);

    /**
     * @brief 将实际节拍对齐到网格
     */
    std::vector<BeatInfo> alignBeatsToGrid(const std::vector<BeatInfo>& detected,
                                            const std::vector<long long>& grid);

    /**
     * @brief 生成4轨道谱面（重音拍1-2个音符，普通拍0-1个）
     */
    std::vector<std::pair<long long, int>> generateChart(
        const std::vector<BeatInfo>& beats, float bpm);

    /**
     * @brief 从文件名提取歌曲名
     */
    static std::string getFileNameWithoutExt(const std::string& filePath);
};
