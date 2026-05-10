/**
 * @file BeatParser.cpp
 * @brief 节拍解析类实现 - V2.0新增
 * @details 使用SFML的InputSoundFile读取音频文件
 *          基于能量峰值的节拍检测算法，自动分析音频生成谱面
 */
#include "BeatParser.h"
#ifdef _WIN32
#ifndef BEATPIXEL_USE_SFML
#include "graphics.h"
#else
#include "AudioManager.h"
#endif
#else
#include <SFML/Audio.hpp>
#endif
#include <cmath>
#include <algorithm>
#include <numeric>
#include <cstdio>
#include <cctype>

// 常量定义
static const size_t WINDOW_SIZE = 1024;       // 分析窗口大小（采样数）
static const size_t HOP_SIZE = 512;           // 窗口步进（50%重叠）
static const float ENERGY_THRESHOLD_RATIO = 1.5f;  // 能量阈值倍数（相对于局部平均）
static const size_t LOCAL_ENERGY_FRAMES = 43;      // 局部平均能量的帧数（约1秒）
static const float MIN_BEAT_INTERVAL_MS = 150.0f;  // 最小节拍间隔(ms)
static const int MIN_NOTES = 20;                    // 最少音符数
static const int MAX_NOTES = 500;                   // 最多音符数

BeatParser::BeatParser() : sampleRate(0) {
}

/**
 * @brief 检查文件格式是否支持
 * @details 通过文件扩展名判断，支持mp3/wav/ogg/flac格式
 */
bool BeatParser::isSupportedFormat(const std::string& filePath) {
    // 找到最后一个点
    size_t dotPos = filePath.rfind('.');
    if (dotPos == std::string::npos) return false;

    // 提取扩展名并转小写
    std::string ext = filePath.substr(dotPos + 1);
    for (auto& c : ext) c = std::tolower(c);

    return (ext == "mp3" || ext == "wav" || ext == "ogg" ||
            ext == "flac" || ext == "aiff" || ext == "wma");
}

/**
 * @brief 解析音频文件生成谱面
 * @details 主流程：加载音频 → 节拍检测 → 轨道分配 → 结果输出
 */
BeatParser::ParseResult BeatParser::parse(const std::string& filePath, float targetBPM) {
    ParseResult result;
    result.success = false;
    result.lastNoteTime = 0;
    result.detectedBPM = 0.0f;
    result.duration = 0.0f;

    // 1. 检查文件格式
    if (!isSupportedFormat(filePath)) {
        result.errorMessage = "不支持的文件格式，请使用MP3/WAV/OGG格式";
        return result;
    }

    // 2. 加载音频文件
    printf("[BeatParser] 正在解析: %s\n", filePath.c_str());
    fflush(stdout);

    if (!loadAudioFile(filePath)) {
        result.errorMessage = "无法读取音频文件，请检查文件是否损坏";
        return result;
    }

    if (samples.empty() || sampleRate == 0) {
        result.errorMessage = "音频数据为空";
        return result;
    }

    // 计算音频时长
    result.duration = (float)samples.size() / sampleRate;
    printf("[BeatParser] 音频时长: %.1f秒, 采样率: %d Hz\n", result.duration, sampleRate);
    fflush(stdout);

    // 3. 节拍检测
    std::vector<long long> beatTimes = detectBeats(targetBPM);

    if (beatTimes.empty()) {
        result.errorMessage = "未能检测到节拍，请尝试其他音频文件";
        return result;
    }

    // 4. 估算BPM
    result.detectedBPM = estimateBPM(beatTimes);
    printf("[BeatParser] 检测到 %zu 个节拍, 估算BPM: %.1f\n",
           beatTimes.size(), result.detectedBPM);
    fflush(stdout);

    // 5. 如果指定了目标BPM，调整节拍密度
    if (targetBPM > 0 && result.detectedBPM > 0) {
        float ratio = targetBPM / result.detectedBPM;
        // 如果比例差异较大，按比例调整节拍间隔
        if (ratio < 0.7f || ratio > 1.4f) {
            std::vector<long long> adjusted;
            for (auto t : beatTimes) {
                adjusted.push_back((long long)(t / ratio));
            }
            beatTimes = adjusted;
            result.detectedBPM = targetBPM;
            printf("[BeatParser] 已调整BPM到 %.1f\n", targetBPM);
            fflush(stdout);
        }
    }

    // 6. 分配到轨道
    result.noteTimeData = distributeToTracks(beatTimes);
    if (!result.noteTimeData.empty()) {
        result.lastNoteTime = result.noteTimeData.back().first;
    }

    result.success = true;
    printf("[BeatParser] 解析完成，生成 %zu 个音符\n", result.noteTimeData.size());
    fflush(stdout);

    return result;
}

/**
 * @brief 从音频文件读取采样数据
 * @details 使用SFML的InputSoundFile读取音频，转换为单声道浮点数据
 */
bool BeatParser::loadAudioFile(const std::string& filePath) {
    sf::InputSoundFile file;
    if (!file.openFromFile(filePath)) {
        fprintf(stderr, "[BeatParser] 无法打开文件: %s\n", filePath.c_str());
        return false;
    }

    sampleRate = file.getSampleRate();
    unsigned int channelCount = file.getChannelCount();
    std::uint64_t totalFrames = file.getSampleCount() / channelCount;

    if (sampleRate == 0 || channelCount == 0 || totalFrames == 0) {
        fprintf(stderr, "[BeatParser] 音频参数异常\n");
        return false;
    }

    printf("[BeatParser] 声道数: %u, 总帧数: %llu\n", channelCount, totalFrames);
    fflush(stdout);

    // 读取所有采样数据
    const size_t BUFFER_FRAMES = 4096;
    std::vector<int16_t> rawBuffer(BUFFER_FRAMES * channelCount);
    samples.clear();
    samples.reserve((size_t)totalFrames);

    std::uint64_t framesRead = 0;
    while (framesRead < totalFrames) {
        std::uint64_t toRead = std::min((std::uint64_t)BUFFER_FRAMES, totalFrames - framesRead);
        std::uint64_t actuallyRead = file.read(rawBuffer.data(), toRead);
        if (actuallyRead == 0) break;

        // 转换为单声道浮点
        for (std::uint64_t i = 0; i < actuallyRead; i++) {
            float sum = 0.0f;
            for (unsigned int ch = 0; ch < channelCount; ch++) {
                sum += rawBuffer[i * channelCount + ch];
            }
            // 归一化到 [-1.0, 1.0]
            samples.push_back(sum / (channelCount * 32768.0f));
        }
        framesRead += actuallyRead;
    }

    printf("[BeatParser] 读取 %zu 个采样点\n", samples.size());
    fflush(stdout);

    return !samples.empty();
}

/**
 * @brief 计算音频能量
 * @details 能量 = 采样值的平方和的平均值
 */
float BeatParser::calculateEnergy(size_t start, size_t count) {
    float energy = 0.0f;
    size_t end = std::min(start + count, samples.size());
    for (size_t i = start; i < end; i++) {
        energy += samples[i] * samples[i];
    }
    return energy / (end - start);
}

/**
 * @brief 基于能量峰值的节拍检测
 * @details 算法步骤：
 *          1. 将音频分成小窗口，计算每个窗口的能量
 *          2. 计算局部平均能量（滑动窗口）
 *          3. 当当前能量 > 局部平均 * 阈值倍数时，判定为节拍
 *          4. 去除间隔过近的节拍
 */
std::vector<long long> BeatParser::detectBeats(float targetBPM) {
    // 计算每个窗口的能量
    size_t numWindows = (samples.size() - WINDOW_SIZE) / HOP_SIZE + 1;
    std::vector<float> energies;
    energies.reserve(numWindows);

    for (size_t i = 0; i < numWindows; i++) {
        size_t start = i * HOP_SIZE;
        energies.push_back(calculateEnergy(start, WINDOW_SIZE));
    }

    // 计算局部平均能量（滑动窗口平均值）
    std::vector<float> localAvg(energies.size(), 0.0f);
    for (size_t i = 0; i < energies.size(); i++) {
        size_t start = (i >= LOCAL_ENERGY_FRAMES) ? i - LOCAL_ENERGY_FRAMES : 0;
        float sum = 0.0f;
        for (size_t j = start; j <= i; j++) {
            sum += energies[j];
        }
        localAvg[i] = sum / (i - start + 1);
    }

    // 检测能量峰值
    std::vector<long long> beats;

    for (size_t i = 1; i < energies.size() - 1; i++) {
        // 条件1：能量超过局部平均的阈值倍数
        float threshold = localAvg[i] * ENERGY_THRESHOLD_RATIO;
        // 添加最小绝对阈值，避免静音区域误触发
        float absThreshold = 0.001f;

        if (energies[i] > threshold && energies[i] > absThreshold &&
            energies[i] > energies[i - 1] && energies[i] >= energies[i + 1]) {
            // 条件2：与上一个节拍保持最小间隔
            long long timeMs = (long long)(i * HOP_SIZE * 1000.0f / sampleRate);
            if (beats.empty() || (timeMs - beats.back()) >= MIN_BEAT_INTERVAL_MS) {
                beats.push_back(timeMs);
            }
        }
    }

    // 如果节拍太少，降低阈值重新检测
    if ((int)beats.size() < MIN_NOTES && energies.size() > 10) {
        beats.clear();
        for (size_t i = 1; i < energies.size() - 1; i++) {
            float threshold = localAvg[i] * 1.2f; // 降低阈值
            if (energies[i] > threshold && energies[i] > 0.0005f &&
                energies[i] > energies[i - 1] && energies[i] >= energies[i + 1]) {
                long long timeMs = (long long)(i * HOP_SIZE * 1000.0f / sampleRate);
                if (beats.empty() || (timeMs - beats.back()) >= MIN_BEAT_INTERVAL_MS) {
                    beats.push_back(timeMs);
                }
            }
        }
    }

    // 如果节拍太多，提高阈值重新检测
    if ((int)beats.size() > MAX_NOTES) {
        beats.clear();
        for (size_t i = 1; i < energies.size() - 1; i++) {
            float threshold = localAvg[i] * 2.0f; // 提高阈值
            if (energies[i] > threshold && energies[i] > 0.002f &&
                energies[i] > energies[i - 1] && energies[i] >= energies[i + 1]) {
                long long timeMs = (long long)(i * HOP_SIZE * 1000.0f / sampleRate);
                if (beats.empty() || (timeMs - beats.back()) >= MIN_BEAT_INTERVAL_MS * 1.5f) {
                    beats.push_back(timeMs);
                }
            }
        }
    }

    // 去掉开头2秒的节拍（给玩家准备时间）
    auto it = std::find_if(beats.begin(), beats.end(), [](long long t) { return t >= 2000; });
    if (it != beats.begin()) {
        beats.erase(beats.begin(), it);
    }

    return beats;
}

/**
 * @brief 估算BPM
 * @details 通过分析相邻节拍的时间间隔，取中位数估算BPM
 */
float BeatParser::estimateBPM(const std::vector<long long>& beatTimes) {
    if (beatTimes.size() < 4) return 120.0f; // 默认BPM

    // 计算相邻节拍间隔
    std::vector<float> intervals;
    for (size_t i = 1; i < beatTimes.size(); i++) {
        float interval = (float)(beatTimes[i] - beatTimes[i - 1]);
        if (interval > 100.0f && interval < 2000.0f) { // 合理的间隔范围
            intervals.push_back(interval);
        }
    }

    if (intervals.empty()) return 120.0f;

    // 取中位数
    std::sort(intervals.begin(), intervals.end());
    float medianInterval = intervals[intervals.size() / 2];

    // BPM = 60000 / 间隔(ms)
    return 60000.0f / medianInterval;
}

/**
 * @brief 将节拍分配到4个轨道
 * @details 使用伪随机策略确保：
 *          1. 不连续出现同一轨道
 *          2. 各轨道使用均衡
 *          3. 有概率生成双押（同时两个轨道）
 */
std::vector<std::pair<long long, int>> BeatParser::distributeToTracks(
    const std::vector<long long>& beatTimes) {

    std::vector<std::pair<long long, int>> result;
    if (beatTimes.empty()) return result;

    srand(42); // 固定种子，保证可重现

    int lastTrack = -1;
    int trackUsage[4] = {0, 0, 0, 0};

    for (size_t i = 0; i < beatTimes.size(); i++) {
        // 选择轨道：避免连续相同，优先使用较少使用的轨道
        int track;
        int attempts = 0;
        do {
            // 加权随机：使用次数少的轨道概率更高
            int totalUsed = trackUsage[0] + trackUsage[1] + trackUsage[2] + trackUsage[3];
            if (totalUsed == 0) {
                track = rand() % 4;
            } else {
                // 反向加权
                float weights[4];
                float totalWeight = 0;
                for (int t = 0; t < 4; t++) {
                    weights[t] = (float)(totalUsed + 4) - trackUsage[t];
                    totalWeight += weights[t];
                }
                float r = (rand() / (float)RAND_MAX) * totalWeight;
                float cumulative = 0;
                track = 3;
                for (int t = 0; t < 4; t++) {
                    cumulative += weights[t];
                    if (r <= cumulative) { track = t; break; }
                }
            }
            attempts++;
        } while (track == lastTrack && attempts < 10);

        lastTrack = track;
        trackUsage[track]++;
        result.push_back({beatTimes[i], track});

        // 15%概率生成双押（在较快的段落提高概率）
        float doubleChance = 0.15f;
        if (i > 0) {
            float interval = (float)(beatTimes[i] - beatTimes[i - 1]);
            if (interval < 400.0f) doubleChance = 0.25f; // 快节奏更多双押
        }

        if ((rand() / (float)RAND_MAX) < doubleChance && i + 1 < beatTimes.size()) {
            int track2;
            do { track2 = rand() % 4; } while (track2 == track);
            result.push_back({beatTimes[i], track2});
            trackUsage[track2]++;
        }
    }

    // 按时间排序
    std::sort(result.begin(), result.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    return result;
}
