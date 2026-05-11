/**
 * @file SongAnalyzer.cpp
 * @brief 歌曲自动分析模块实现
 * @details 自相关BPM检测(精度±0.5)、重音标记、ID3读取、JSON导出
 */
#include "SongAnalyzer.h"
#ifdef _WIN32
#ifndef BEATPIXEL_USE_SFML
#include "graphics.h"
#else
#include "AudioManager.h"
#endif
#else
#include <SFML/Audio.hpp>
#include <mach-o/dyld.h>
#endif
#include <cmath>
#include <algorithm>
#include <set>
#include <map>
#include <numeric>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unistd.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// V3.3: 获取可执行文件目录
static std::string getExeDir() {
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        char* slash = strrchr(path, '/');
        if (slash) *slash = '\0';
        return std::string(path);
    }
    return ".";
}

// ========== 常量 ==========
static const size_t WINDOW_SIZE = 1024;
static const size_t HOP_SIZE = 512;
static const size_t LOCAL_ENERGY_FRAMES = 43;

SongAnalyzer::SongAnalyzer() : tapCount(0), sampleRate(0) {
    memset(tapTimes, 0, sizeof(tapTimes));
}

// ========== 主分析入口 ==========

SongAnalyzer::AnalysisResult SongAnalyzer::analyze(const std::string& filePath) {
    AnalysisResult result;
    result.success = false;
    currentFilePath = filePath;

    printf("[SongAnalyzer] 开始分析: %s\n", filePath.c_str());
    fflush(stdout);

    // 1. 读取ID3标签
    result.metadata = readID3Tags(filePath);
    if (result.metadata.title.empty()) {
        result.metadata.title = getFileNameWithoutExt(filePath);
    }
    printf("[SongAnalyzer] 标题: %s\n", result.metadata.title.c_str());
    fflush(stdout);

    // 2. 加载音频
    printf("[SongAnalyzer] 加载音频...\n");
    fflush(stdout);
    if (!loadAudio(filePath)) {
        result.errorMessage = "无法读取音频文件";
        return result;
    }
    if (samples.empty() || sampleRate == 0) {
        result.errorMessage = "音频数据为空";
        return result;
    }

    result.metadata.duration = (float)samples.size() / sampleRate;
    printf("[SongAnalyzer] 时长: %.1f秒, 采样率: %d, 采样点: %zu\n", result.metadata.duration, sampleRate, samples.size());
    fflush(stdout);

    // 3. 检测节拍点（带重音标记）
    printf("[SongAnalyzer] 检测节拍点...\n");
    fflush(stdout);
    result.beats = detectBeatsWithAccent();
    if (result.beats.empty()) {
        result.errorMessage = "未能检测到节拍";
        return result;
    }
    printf("[SongAnalyzer] %zu 个节拍点\n", result.beats.size());
    fflush(stdout);

    // 4. BPM检测（优先用Python结果，否则用C++自相关）
    printf("[SongAnalyzer] BPM分析...\n");
    fflush(stdout);
    if (resultBPM > 0) {
        result.bpm = resultBPM;
        result.bpmConfidence = 85.0f;
        printf("[SongAnalyzer] BPM(Python): %.1f\n", result.bpm);
    } else {
        auto [bpm, confidence] = detectBPMPrecise();
        result.bpm = bpm;
        result.bpmConfidence = confidence;
        printf("[SongAnalyzer] BPM(C++): %.1f, 置信度: %.1f%%\n", bpm, confidence);
    }
    fflush(stdout);

    float bpm = result.bpm;
    float confidence = result.bpmConfidence;

    // 5. 低置信度时尝试修正
    if (confidence < 90.0f) {
        printf("[SongAnalyzer] 置信度 %.1f%% < 90%%，尝试八度修正...\n", confidence);
        fflush(stdout);
        // 尝试半倍/双倍BPM
        float halfBPM = bpm / 2.0f;
        float doubleBPM = bpm * 2.0f;
        if (halfBPM >= 60.0f && halfBPM <= 200.0f) {
            result.bpm = halfBPM;
        } else if (doubleBPM >= 60.0f && doubleBPM <= 200.0f) {
            result.bpm = doubleBPM;
        }
    }

    // 6. 生成节拍网格并对齐
    printf("[SongAnalyzer] 对齐节拍网格...\n");
    fflush(stdout);
    auto grid = generateBeatGrid(result.bpm, 0, result.metadata.duration);
    result.beats = alignBeatsToGrid(result.beats, grid);
    result.beatOffset = 0;

    // 7. 生成谱面
    printf("[SongAnalyzer] 生成谱面...\n");
    fflush(stdout);
    result.chart = generateChart(result.beats, result.bpm);
    result.originalChart = result.chart;

    result.success = true;
    printf("[SongAnalyzer] 分析完成: BPM=%.1f, %zu个节拍, %zu个音符\n",
           result.bpm, result.beats.size(), result.chart.size());
    fflush(stdout);

    return result;
}

// ========== 自相关BPM检测 ==========

std::pair<float, float> SongAnalyzer::detectBPMPrecise() {
    // 计算能量包络
    size_t numWindows = (samples.size() - WINDOW_SIZE) / HOP_SIZE + 1;
    std::vector<float> energies;
    energies.reserve(numWindows);
    for (size_t i = 0; i < numWindows; i++) {
        energies.push_back(calculateEnergy(i * HOP_SIZE, WINDOW_SIZE));
    }

    // 计算自相关
    int maxLag = (int)(energies.size() * 0.8f);
    auto acf = autocorrelation(energies, maxLag);

    // 在BPM 60~200范围内找峰值
    float minBPM = 60.0f, maxBPM = 200.0f;
    int minLag = std::max(1, (int)(60.0f * sampleRate / (HOP_SIZE * maxBPM)));
    int maxLagBPM = std::min((int)acf.size() - 1, (int)(60.0f * sampleRate / (HOP_SIZE * minBPM)));

    // 找自相关峰值
    float bestACF = 0;
    int bestLag = 0;
    for (int lag = minLag; lag <= maxLagBPM; lag++) {
        if (acf[lag] > bestACF) {
            bestACF = acf[lag];
            bestLag = lag;
        }
    }

    if (bestLag == 0) return {120.0f, 0.0f};

    // 抛物线插值，精确到±0.5
    float refinedLag = (float)bestLag;
    if (bestLag > 0 && bestLag < (int)acf.size() - 1) {
        float y1 = acf[bestLag - 1];
        float y2 = acf[bestLag];
        float y3 = acf[bestLag + 1];
        float denom = 2.0f * (2.0f * y2 - y1 - y3);
        if (std::abs(denom) > 1e-6f) {
            refinedLag = bestLag + (y1 - y3) / denom;
        }
    }

    float bpm = 60.0f * sampleRate / (HOP_SIZE * refinedLag);

    // 计算置信度
    float confidence = 0.0f;
    if (bestACF > 0) {
        // 归一化自相关值
        float normACF = acf[0] > 0 ? bestACF / acf[0] : 0;
        confidence = std::min(100.0f, normACF * 120.0f);
    }

    return {bpm, confidence};
}

std::vector<float> SongAnalyzer::autocorrelation(const std::vector<float>& signal, int maxLag) {
    int N = (int)signal.size();
    maxLag = std::min(maxLag, N - 1);
    std::vector<float> acf(maxLag + 1, 0.0f);

    // 计算均值
    float mean = 0;
    for (float v : signal) mean += v;
    mean /= N;

    // 自相关
    for (int lag = 0; lag <= maxLag; lag++) {
        float sum = 0;
        int count = N - lag;
        for (int i = 0; i < count; i++) {
            sum += (signal[i] - mean) * (signal[i + lag] - mean);
        }
        acf[lag] = sum / count;
    }

    return acf;
}

// ========== 节拍检测（带重音标记） ==========

/**
 * @brief 频谱onset检测算法
 * @details 优先调用Python脚本(频谱分析)，失败则用C++内置算法
 */
std::vector<SongAnalyzer::BeatInfo> SongAnalyzer::detectBeatsWithAccent() {
    // 方案1: 调用Python脚本做频谱onset检测
    std::vector<BeatInfo> beats = detectBeatsViaPython();
    if (!beats.empty()) {
        printf("[SongAnalyzer] Python频谱onset检测: %zu个节拍点\n", beats.size());
        fflush(stdout);
        return beats;
    }

    // 方案2: 降级到C++内置算法
    printf("[SongAnalyzer] Python不可用，使用C++内置算法\n");
    fflush(stdout);
    return detectBeatsFallback();
}

std::vector<SongAnalyzer::BeatInfo> SongAnalyzer::detectBeatsViaPython() {
    // 查找Python脚本路径
    std::string scriptPath = getExeDir() + "/tools/beat_detect.py";
    FILE* f = fopen(scriptPath.c_str(), "r");
    if (!f) {
        scriptPath = getExeDir() + "/../tools/beat_detect.py";
        f = fopen(scriptPath.c_str(), "r");
    }
    if (!f) return {};
    fclose(f);

    std::string cmd = "python3 '" + scriptPath + "' '" + currentFilePath + "' 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return {};

    // 读取JSON输出
    std::string json;
    char buffer[65536];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        json += buffer;
    }
    int ret = pclose(pipe);
    if (ret != 0 || json.empty()) return {};

    // 检查success字段
    size_t successPos = json.find("\"success\"");
    if (successPos != std::string::npos) {
        size_t colon = json.find(":", successPos);
        if (colon != std::string::npos && json.substr(colon + 1, 5).find("false") != std::string::npos) {
            return {};
        }
    }

    std::vector<BeatInfo> beats;

    // 提取bpm
    size_t bpmPos = json.find("\"bpm\"");
    if (bpmPos != std::string::npos) {
        size_t colon = json.find(":", bpmPos);
        size_t end = json.find(",", colon);
        if (end == std::string::npos) end = json.find("}", colon);
        std::string val = json.substr(colon + 1, end - colon - 1);
        val.erase(0, val.find_first_not_of(" \t\n\r"));
        resultBPM = std::stof(val);
    }

    // 新格式: 提取beats数组（带drum类型）
    size_t beatsArrStart = json.find("[", json.find("\"beats\""));
    if (beatsArrStart != std::string::npos) {
        size_t pos = beatsArrStart + 1;
        while (pos < json.size() && pos < json.find("]", beatsArrStart)) {
            // 找 time 字段
            size_t timePos = json.find("\"time\":", pos);
            if (timePos == std::string::npos) break;
            size_t timeEnd = json.find(",", timePos);
            if (timeEnd == std::string::npos) timeEnd = json.find("}", timePos);
            std::string timeStr = json.substr(timePos + 7, timeEnd - timePos - 7);
            timeStr.erase(0, timeStr.find_first_not_of(" \t\n\r"));
            long long timeMs = std::stoll(timeStr);
            
            // 找 drum 字段
            int drumType = 0;
            size_t drumPos = json.find("\"drum\":", timePos);
            if (drumPos != std::string::npos) {
                size_t quoteStart = json.find("\"", drumPos + 7);
                size_t quoteEnd = json.find("\"", quoteStart + 1);
                if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                    std::string drum = json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
                    if (drum == "kick") drumType = 1;
                    else if (drum == "snare") drumType = 2;
                    else if (drum == "hihat") drumType = 3;
                    else if (drum == "tom_low") drumType = 4;
                    else if (drum == "tom_mid") drumType = 5;
                    else if (drum == "tom_high") drumType = 6;
                    else if (drum == "ride") drumType = 7;
                    else if (drum == "crash") drumType = 8;
                }
            }

            BeatInfo b;
            b.timeMs = timeMs;
            b.energy = 1.0f;
            b.isAccent = (drumType == 1 || drumType == 2 || drumType == 8);
            b.drumType = drumType;
            beats.push_back(b);
            pos = json.find("}", timePos) + 1;
        }
    }

    // 兼容旧格式: 提取beat_times_ms数组（无鼓类型）
    if (beats.empty()) {
        size_t btmsPos = json.find("\"beat_times_ms\"");
        if (btmsPos != std::string::npos) {
            size_t arrStart = json.find("[", btmsPos);
            if (arrStart != std::string::npos) {
                size_t arrEnd = json.find("]", arrStart);
                std::string arr = json.substr(arrStart + 1, arrEnd - arrStart - 1);
                size_t pos = 0;
                while (pos < arr.size()) {
                    size_t next = arr.find(",", pos);
                    if (next == std::string::npos) next = arr.size();
                    std::string num = arr.substr(pos, next - pos);
                    num.erase(0, num.find_first_not_of(" \t\n\r"));
                    if (!num.empty()) {
                        long long timeMs = std::stoll(num);
                        BeatInfo b; b.timeMs = timeMs; b.energy = 1.0f; b.isAccent = false; b.drumType = 0; beats.push_back(b);
                    }
                    pos = next + 1;
                }
            }
        }
    }

    // 旧格式兼容: 提取beats数组（含timeMs和isAccent）
    if (beats.empty()) {
        size_t beatsStart = json.find("[", json.find("\"beats\""));
        if (beatsStart != std::string::npos) {
            size_t pos = beatsStart + 1;
            while (pos < json.size()) {
                size_t timePos = json.find("\"timeMs\":", pos);
                if (timePos == std::string::npos || timePos > json.find("]", beatsStart)) break;
                size_t valStart = json.find(":", timePos) + 1;
                size_t valEnd = json.find(",", valStart);
                if (valEnd == std::string::npos) valEnd = json.find("}", valStart);
                std::string timeStr = json.substr(valStart, valEnd - valStart);
                timeStr.erase(0, timeStr.find_first_not_of(" \t\n\r"));
                long long timeMs = std::stoll(timeStr);
                BeatInfo b; b.timeMs = timeMs; b.energy = 1.0f; b.isAccent = false; b.drumType = 0; beats.push_back(b);
                pos = json.find("}", timePos) + 1;
            }
        }
    }

    return beats;
}
std::vector<SongAnalyzer::BeatInfo> SongAnalyzer::detectBeatsFallback() {
    // Step 1: 用ffmpeg转PCM (16-bit, mono, 22050Hz)
    const int SR = 22050;
    char tmpPcm[] = "/tmp/beatpixel_pcm_XXXXXX.pcm";
    int tmpFd = mkstemps(tmpPcm, 4);
    if (tmpFd < 0) return {};
    close(tmpFd);

    std::string cmd = "ffmpeg -y -i '" + currentFilePath + "' -f s16le -acodec pcm_s16le -ac 1 -ar " + std::to_string(SR) + " '" + tmpPcm + "' 2>/dev/null";
    int ret = system(cmd.c_str());
    if (ret != 0) {
        remove(tmpPcm);
        return {};
    }

    // Step 2: 读取PCM样本
    FILE* fp = fopen(tmpPcm, "rb");
    if (!fp) { remove(tmpPcm); return {}; }

    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    size_t numSamples = fileSize / 2; // 16-bit = 2 bytes per sample
    std::vector<float> pcm(numSamples);
    std::vector<int16_t> buf(numSamples);
    fread(buf.data(), 2, numSamples, fp);
    fclose(fp);
    remove(tmpPcm);

    // 归一化到[-1, 1]
    for (size_t i = 0; i < numSamples; i++) {
        pcm[i] = buf[i] / 32768.0f;
    }

    // Step 3: 计算短时能量 (STE)
    const int WIN_SIZE = 1024;   // 窗口大小 (~46ms at 22050Hz)
    const int HOP = 512;         // 帧移 (~23ms)
    size_t numFrames = (numSamples - WIN_SIZE) / HOP + 1;
    std::vector<float> ste(numFrames, 0.0f);

    for (size_t f = 0; f < numFrames; f++) {
        size_t offset = f * HOP;
        float energy = 0;
        for (int i = 0; i < WIN_SIZE; i++) {
            float s = pcm[offset + i];
            energy += s * s;
        }
        ste[f] = energy / WIN_SIZE;
    }

    // Step 4: 计算能量通量 (Spectral Flux) - 半波整流
    std::vector<float> flux(numFrames, 0.0f);
    for (size_t f = 1; f < numFrames; f++) {
        float diff = ste[f] - ste[f - 1];
        flux[f] = diff > 0 ? diff : 0; // 半波整流：只保留能量增加的部分
    }

    // Step 5: 自适应阈值检测onset
    const int LOCAL_WIN = 16; // 局部窗口 (~370ms)
    std::vector<float> localMean(numFrames, 0.0f);
    std::vector<float> localStd(numFrames, 0.0f);

    for (size_t f = 0; f < numFrames; f++) {
        size_t start = (f >= LOCAL_WIN) ? f - LOCAL_WIN : 0;
        float sum = 0, sumSq = 0;
        size_t count = f - start + 1;
        for (size_t j = start; j <= f; j++) {
            sum += flux[j];
            sumSq += flux[j] * flux[j];
        }
        localMean[f] = sum / count;
        float variance = sumSq / count - localMean[f] * localMean[f];
        localStd[f] = std::sqrt(std::max(0.0f, variance));
    }

    // 全局能量统计
    float globalFluxMean = 0, globalFluxStd = 0;
    for (float v : flux) globalFluxMean += v;
    globalFluxMean /= numFrames;
    for (float v : flux) globalFluxStd += (v - globalFluxMean) * (v - globalFluxMean);
    globalFluxStd = std::sqrt(globalFluxStd / numFrames);

    // 检测onset峰值
    std::vector<BeatInfo> beats;
    float minIntervalMs = 150.0f; // 最小间隔150ms
    int minIntervalFrames = (int)(minIntervalMs * SR / (1000.0f * HOP));

    for (size_t f = 2; f < numFrames - 1; f++) {
        // 自适应阈值 = 局部均值 + 0.5 * 局部标准差
        float threshold = localMean[f] + 0.25f * localStd[f];  // 降低阈值增加音符密度
        // 绝对阈值防止噪声触发
        float absThreshold = globalFluxMean + 0.2f * globalFluxStd;
        float thresh = std::max(threshold, absThreshold);

        if (flux[f] > thresh && flux[f] > flux[f - 1] && flux[f] >= flux[f + 1]) {
            long long timeMs = (long long)(f * HOP * 1000.0f / SR);

            // 最小间隔检查
            if (!beats.empty() && (timeMs - beats.back().timeMs) < minIntervalMs) {
                // 保留能量更大的那个
                if (flux[f] > flux[f - 1]) {
                    beats.back().timeMs = timeMs;
                    beats.back().energy = flux[f];
                }
                continue;
            }

            // 跳过开头1秒
            if (timeMs < 1000) continue;

            // 重音判定：能量通量超过全局均值+1.5倍标准差
            bool isAccent = flux[f] > (globalFluxMean + 1.5f * globalFluxStd);

            BeatInfo beat;
            beat.timeMs = timeMs;
            beat.energy = flux[f];
            beat.isAccent = isAccent;
            beats.push_back(beat);
        }
    }

    // 节拍太少则降低阈值重试
    if (beats.size() < 15 && numFrames > 100) {
        beats.clear();
        for (size_t f = 2; f < numFrames - 1; f++) {
            float thresh = std::max(localMean[f] + 0.3f * localStd[f], globalFluxMean + 0.1f * globalFluxStd);
            if (flux[f] > thresh && flux[f] > flux[f - 1] && flux[f] >= flux[f + 1]) {
                long long timeMs = (long long)(f * HOP * 1000.0f / SR);
                if (!beats.empty() && (timeMs - beats.back().timeMs) < minIntervalMs) continue;
                if (timeMs < 1000) continue;
                BeatInfo b; b.timeMs = timeMs; b.energy = flux[f]; b.isAccent = (flux[f] > (globalFluxMean + globalFluxStd)); b.drumType = 0; beats.push_back(b);
            }
        }
    }

    printf("[SongAnalyzer] 频谱onset检测: %zu个节拍点\n", beats.size());
    fflush(stdout);
    return beats;
}

// ========== 节拍网格对齐 ==========

std::vector<long long> SongAnalyzer::generateBeatGrid(float bpm, float offset, float duration) {
    std::vector<long long> grid;
    if (bpm <= 0) return grid;
    float interval = 60000.0f / bpm;
    long long t = (long long)offset;
    while (t < (long long)(duration * 1000)) {
        if (t >= 2000) grid.push_back(t); // 跳过开头2秒
        t += (long long)interval;
    }
    return grid;
}

std::vector<SongAnalyzer::BeatInfo> SongAnalyzer::alignBeatsToGrid(
    const std::vector<BeatInfo>& detected, const std::vector<long long>& grid) {
    if (grid.empty() || detected.empty()) return detected;

    std::vector<BeatInfo> aligned;

    for (size_t gi = 0; gi < grid.size(); gi++) {
        long long gridTime = grid[gi];
        float minDist = 1e9f;
        size_t bestIdx = 0;
        for (size_t di = 0; di < detected.size(); di++) {
            float dist = std::abs((float)(detected[di].timeMs - gridTime));
            if (dist < minDist) {
                minDist = dist;
                bestIdx = di;
            }
        }

        // 鼓组分档容差：核心鼓±10ms，装饰鼓±20ms，Hihat±30ms
        float tol = 30.0f;
        int dt = detected[bestIdx].drumType;
        if (dt == 1 || dt == 2) tol = 10.0f;       // kick/snare
        else if (dt >= 4 && dt <= 7) tol = 20.0f;  // tom/ride

        BeatInfo beat;
        beat.timeMs = gridTime;
        if (minDist < tol) {
            beat.energy = detected[bestIdx].energy;
            beat.isAccent = detected[bestIdx].isAccent;
            beat.drumType = detected[bestIdx].drumType;
        } else if (dt == 1 || dt == 2) {
            // 核心鼓超出容差，判定为误检，跳过
            continue;
        } else {
            beat.energy = 0.001f;
            beat.isAccent = false;
            beat.drumType = 0;
        }
        aligned.push_back(beat);
    }

    return aligned;
}

// ========== 谱面生成 ==========

/**
 * @brief V3.6: 鼓组类型 → 固定轨道映射（贴合真实架子鼓逻辑）
 * Kick→0(A), Snare→3(F), Hihat→1/2(S/D轮换), Tom→4(J), Ride→5(K)
 * Tom fill: 检测连续通鼓，按高→中→低轮换音效
 */
static int drumTypeToTrack(int drumType, int& hihatAlt) {
    switch (drumType) {
        case 1: return 0;  // kick → A
        case 2: return 3;  // snare → F
        case 3: return 1;  // hihat → S
        case 4: case 5: case 6: return 4;  // tom → J
        case 7: return 5;  // ride → K
        case 8: return 2;  // crash → D
        default: return -1;
    }
}

// Tom fill 轮换索引：高→中→低→高...
static int tomFillCycle = 0;
static long long lastTomTime = 0;

/**
 * @brief 获取tom音效索引（0=高, 1=中, 2=低）
 * Tom fill逻辑：连续tom间隔<500ms时自动轮换，>500ms重置为高tom
 */
static int getTomSoundIndex(long long currentTime) {
    if (currentTime - lastTomTime > 500) {
        tomFillCycle = 0;  // 间隔太长，重置为高tom
    }
    int idx = tomFillCycle % 3;
    tomFillCycle++;
    lastTomTime = currentTime;
    return idx;
}

/**
 * @brief 优先级过滤：同一时间点最多2个音符
 * 铁则：Kick(0) > Snare(3) > Crash > Tom(4) > Ride(5) > Hihat(1/2)
 */
static int getPriority(int track) {
    switch (track) {
        case 0: return 0;   // Kick 最高
        case 3: return 1;   // Snare
        case 2: return 2;   // Crash (D键，高于Tom/Ride/Hihat)
        case 4: return 3;   // Tom
        case 5: return 4;   // Ride
        case 1: return 5;   // Hihat 最低
        default: return 99;
    }
}

std::vector<std::pair<long long, int>> SongAnalyzer::generateChart(
    const std::vector<BeatInfo>& beats, float bpm) {
    std::vector<std::pair<long long, int>> chart;
    if (beats.empty()) return chart;

    int hihatAlt = 0;
    int lastTrack = -1;
    float beatInterval = 60000.0f / bpm;
    float halfBeat = beatInterval / 2.0f;

    // 第一步：按鼓组类型分配轨道
    srand(42);
    for (size_t i = 0; i < beats.size(); i++) {
        int track = drumTypeToTrack(beats[i].drumType, hihatAlt);
        if (track < 0) {
            do { track = rand() % 6; } while (track == lastTrack);
        }
        lastTrack = track;
        chart.push_back({beats[i].timeMs, track});
    }

    // 第二步：节奏子填充（off-beat hihat, crash, tom fill）
    std::sort(chart.begin(), chart.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
    
    std::set<long long> occupied;
    for (auto& c : chart) occupied.insert(c.first);
    
    long long lastTime = chart.back().first;
    std::vector<std::pair<long long, int>> extras;
    
    long long nBeats = (long long)(lastTime / beatInterval) + 1;
    for (long long bi = 0; bi < nBeats; bi++) {
        long long t = (long long)(bi * beatInterval);
        
        // Off-beat Hihat (.5位置，跳过前8拍让音乐先展开)
        if (bi >= 8) {
            long long offBeat = t + (long long)halfBeat;
            if (offBeat <= lastTime && occupied.count(offBeat) == 0) {
                extras.push_back({offBeat, 1});
                occupied.insert(offBeat);
            }
        }
        
        // Crash every 32 beats（跳过前32拍，降低密度）
        if (bi >= 32 && bi % 32 == 0 && occupied.count(t) == 0) {
            extras.push_back({t, 2});
            occupied.insert(t);
        }
        
    }
    
    // 建立每拍能量分布
    std::map<long long, float> beatEnergy;
    for (auto& b : beats) {
        long long bt = (b.timeMs / (long long)beatInterval) * (long long)beatInterval;
        beatEnergy[bt] = std::max(beatEnergy[bt], b.energy);
    }

    // === Tom fill: 分段均匀分布（每段1个，避开Crash，能量选点） ===
    {
        long long fillSpacing = (long long)(beatInterval / 4.0f);
        int numSegments = 6;  // 6段 → 6个tom fill
        long long segLength = lastTime / numSegments;
        long long minFillGap = (long long)(24.0f * beatInterval);  // 段内最小间隔
        
        for (int seg = 1; seg < numSegments; seg++) {  // 跳过分段0（开头段）
            long long segStart = seg * segLength;
            long long segEnd = segStart + segLength;
            
            // 段内找最佳位置：有能量峰值的拍，避开Crash和开头
            long long bestT = 0;
            float bestEnergy = -1.0f;
            
            for (long long bi = 0; bi < nBeats; bi++) {
                long long t = (long long)(bi * beatInterval);
                if (t < segStart || t >= segEnd) continue;
                // 避开Crash及其附近（±4拍内）
                int mod32 = (int)(bi % 32);
                if (mod32 == 0 || mod32 == 1 || mod32 == 31 || mod32 == 30) continue;
                if (t - segStart < (long long)(4.0f * beatInterval)) continue;  // 不过早
                
                // 看这个拍子附近的平均能量
                float sum = 0; int cnt = 0;
                for (int d = -2; d <= 2; d++) {
                    long long bt = (long long)((bi + d) * beatInterval);
                    auto it = beatEnergy.find(bt);
                    if (it != beatEnergy.end()) { sum += it->second; cnt++; }
                }
                if (cnt > 0 && sum / cnt > bestEnergy) {
                    bestEnergy = sum / cnt;
                    bestT = t;
                }
            }
            
            if (bestT > 0) {
                for (int ti = 0; ti < 3; ti++) {
                    long long fillTime = bestT + ti * fillSpacing;
                    if (fillTime <= lastTime) {
                        extras.push_back({fillTime, 4});
                    }
                }
            }
        }
        
        // 兜底：如果某段没放fill，在中点补
        {
            int fillsPlaced = 0;
            for (int seg = 1; seg < numSegments && fillsPlaced < 4; seg++) {
                long long segStart = seg * segLength;
                long long segEnd = segStart + segLength;
                // 检查此段是否已有tom fill
                bool hasFill = false;
                for (auto& e : extras) {
                    if (e.second == 4 && e.first >= segStart && e.first < segEnd) {
                        hasFill = true; break;
                    }
                }
                if (hasFill) continue;
                long long t = segStart + segLength / 2;
                t = (t / (long long)beatInterval) * (long long)beatInterval;
                if (t + 2 * fillSpacing > lastTime) continue;
                for (int ti = 0; ti < 3; ti++) {
                    extras.push_back({t + ti * fillSpacing, 4});
                }
                fillsPlaced++;
            }
        }
    }
    
    chart.insert(chart.end(), extras.begin(), extras.end());
    std::sort(chart.begin(), chart.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    // 第三步：优先级过滤（同一时间最多3个）
    std::vector<std::pair<long long, int>> filtered;
    size_t i = 0;
    while (i < chart.size()) {
        long long t = chart[i].first;
        std::vector<int> tracks;
        while (i < chart.size() && chart[i].first == t) {
            tracks.push_back(chart[i].second);
            i++;
        }
        std::sort(tracks.begin(), tracks.end(),
                  [](int a, int b) { return getPriority(a) < getPriority(b); });
        int count = std::min((int)tracks.size(), 3);
        for (int j = 0; j < count; j++) {
            filtered.push_back({t, tracks[j]});
        }
    }
    return filtered;
}

// ========== 点3拍校准
// ========== 点3拍校准 ==========

float SongAnalyzer::tapTempo(long long tapTimeMs) {
    if (tapCount >= 10) {
        // 已满，重置
        tapCount = 0;
    }

    tapTimes[tapCount] = tapTimeMs;
    tapCount++;

    if (tapCount < 3) return 0.0f; // 还需要更多拍

    // 计算最近几次间隔的平均值
    float totalInterval = 0;
    int count = 0;
    for (int i = 1; i < tapCount; i++) {
        float interval = (float)(tapTimes[i] - tapTimes[i - 1]);
        if (interval > 200.0f && interval < 3000.0f) { // 合理范围
            totalInterval += interval;
            count++;
        }
    }

    if (count == 0) return 0.0f;
    float avgInterval = totalInterval / count;
    return 60000.0f / avgInterval;
}

void SongAnalyzer::resetTapTempo() {
    tapCount = 0;
    memset(tapTimes, 0, sizeof(tapTimes));
}

// ========== 重新生成谱面 ==========

void SongAnalyzer::regenerateChart(AnalysisResult& result, float newBPM, float newOffset) {
    result.bpm = newBPM;
    result.beatOffset = newOffset;

    // 重新生成网格并对齐
    auto grid = generateBeatGrid(newBPM, newOffset, result.metadata.duration);
    auto reAligned = alignBeatsToGrid(result.beats, grid);
    result.beats = reAligned;

    // 重新生成谱面
    result.chart = generateChart(result.beats, newBPM);
}

// ========== ID3标签读取 ==========

SongAnalyzer::SongMetadata SongAnalyzer::readID3Tags(const std::string& filePath) {
    SongMetadata meta;

    FILE* fp = fopen(filePath.c_str(), "rb");
    if (!fp) return meta;

    // 跳到文件末尾-128字节（ID3v1）
    fseek(fp, -128, SEEK_END);
    char tag[128];
    if (fread(tag, 1, 128, fp) != 128) {
        fclose(fp);
        return meta;
    }

    // 检查TAG标识
    if (tag[0] != 'T' || tag[1] != 'A' || tag[2] != 'G') {
        fclose(fp);
        // 没有ID3v1标签，用文件名
        meta.title = getFileNameWithoutExt(filePath);
        return meta;
    }

    // 解析字段（ID3v1格式：30+30+30+4+30+1字节）
    char title[31] = {0}, artist[31] = {0}, album[31] = {0};
    memcpy(title, tag + 3, 30);
    memcpy(artist, tag + 33, 30);
    memcpy(album, tag + 63, 30);

    // 去掉尾部空格
    auto trim = [](char* s) {
        for (int i = 29; i >= 0; i--) {
            if (s[i] == ' ' || s[i] == '\0') s[i] = '\0';
            else break;
        }
    };
    trim(title);
    trim(artist);
    trim(album);

    meta.title = title;
    meta.artist = artist;
    meta.album = album;

    // 如果标题为空，用文件名
    if (meta.title.empty()) {
        meta.title = getFileNameWithoutExt(filePath);
    }

    fclose(fp);
    return meta;
}

// ========== JSON导出 ==========

bool SongAnalyzer::exportChart(const std::string& filePath, const AnalysisResult& result) {
    FILE* fp = fopen(filePath.c_str(), "w");
    if (!fp) {
        fprintf(stderr, "[SongAnalyzer] 无法写入: %s\n", filePath.c_str());
        return false;
    }

    fprintf(fp, "{\n");
    fprintf(fp, "  \"metadata\": {\n");
    fprintf(fp, "    \"title\": \"%s\",\n", result.metadata.title.c_str());
    fprintf(fp, "    \"artist\": \"%s\",\n", result.metadata.artist.c_str());
    fprintf(fp, "    \"album\": \"%s\",\n", result.metadata.album.c_str());
    fprintf(fp, "    \"duration\": %.2f\n", result.metadata.duration);
    fprintf(fp, "  },\n");
    fprintf(fp, "  \"bpm\": %.1f,\n", result.bpm);
    fprintf(fp, "  \"bpmConfidence\": %.1f,\n", result.bpmConfidence);
    fprintf(fp, "  \"beatOffset\": %.1f,\n", result.beatOffset);
    fprintf(fp, "  \"beatCount\": %zu,\n", result.beats.size());
    fprintf(fp, "  \"accentCount\": %zu,\n",
            std::count_if(result.beats.begin(), result.beats.end(),
                          [](const BeatInfo& b) { return b.isAccent; }));
    fprintf(fp, "  \"noteCount\": %zu,\n", result.chart.size());
    fprintf(fp, "  \"notes\": [\n");
    for (size_t i = 0; i < result.chart.size(); i++) {
        fprintf(fp, "    { \"time\": %lld, \"track\": %d }%s\n",
                result.chart[i].first, result.chart[i].second,
                i < result.chart.size() - 1 ? "," : "");
    }
    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");

    fclose(fp);
    printf("[SongAnalyzer] 谱面已导出: %s\n", filePath.c_str());
    fflush(stdout);
    return true;
}

// ========== 工具函数 ==========

bool SongAnalyzer::loadAudio(const std::string& filePath) {
    // 先尝试SFML直接加载
    sf::InputSoundFile file;
    if (file.openFromFile(filePath)) {
        sampleRate = file.getSampleRate();
        unsigned int channelCount = file.getChannelCount();
        std::uint64_t totalFrames = file.getSampleCount() / channelCount;
        if (sampleRate > 0 && channelCount > 0 && totalFrames > 0) {
            const size_t BUFFER_FRAMES = 4096;
            std::vector<int16_t> rawBuffer(BUFFER_FRAMES * channelCount);
            samples.clear();
            samples.reserve((size_t)totalFrames);
            std::uint64_t framesRead = 0;
            while (framesRead < totalFrames) {
                std::uint64_t toRead = std::min((std::uint64_t)BUFFER_FRAMES, totalFrames - framesRead);
                std::uint64_t actuallyRead = file.read(rawBuffer.data(), toRead);
                if (actuallyRead == 0) break;
                for (std::uint64_t i = 0; i < actuallyRead; i++) {
                    float sum = 0;
                    for (unsigned int ch = 0; ch < channelCount; ch++)
                        sum += rawBuffer[i * channelCount + ch];
                    samples.push_back(sum / (channelCount * 32768.0f));
                }
                framesRead += actuallyRead;
            }
            if (!samples.empty()) return true;
        }
    }

    // SFML不支持该格式，用ffmpeg转换
    printf("[SongAnalyzer] SFML不支持该格式，使用ffmpeg转换...\n");
    fflush(stdout);

    char tmpPath[] = "/tmp/beatpixel_convert_XXXXXX.wav";
    int tmpFd = mkstemps(tmpPath, 4);
    if (tmpFd < 0) {
        fprintf(stderr, "[SongAnalyzer] 无法创建临时文件\n");
        return false;
    }
    close(tmpFd);

    std::string cmd = "ffmpeg -y -i '" + filePath + "' -ar 44100 -ac 1 -f wav '" + tmpPath + "' 2>/dev/null";
    int ret = system(cmd.c_str());
    if (ret != 0) {
        fprintf(stderr, "[SongAnalyzer] ffmpeg转换失败\n");
        remove(tmpPath);
        return false;
    }

    // 加载转换后的WAV
    bool ok = false;
    sf::InputSoundFile wavFile;
    if (wavFile.openFromFile(tmpPath)) {
        sampleRate = wavFile.getSampleRate();
        unsigned int channelCount = wavFile.getChannelCount();
        std::uint64_t totalFrames = wavFile.getSampleCount() / channelCount;
        if (sampleRate > 0 && totalFrames > 0) {
            const size_t BUFFER_FRAMES = 4096;
            std::vector<int16_t> rawBuffer(BUFFER_FRAMES * channelCount);
            samples.clear();
            samples.reserve((size_t)totalFrames);
            std::uint64_t framesRead = 0;
            while (framesRead < totalFrames) {
                std::uint64_t toRead = std::min((std::uint64_t)BUFFER_FRAMES, totalFrames - framesRead);
                std::uint64_t actuallyRead = wavFile.read(rawBuffer.data(), toRead);
                if (actuallyRead == 0) break;
                for (std::uint64_t i = 0; i < actuallyRead; i++) {
                    float sum = 0;
                    for (unsigned int ch = 0; ch < channelCount; ch++)
                        sum += rawBuffer[i * channelCount + ch];
                    samples.push_back(sum / (channelCount * 32768.0f));
                }
                framesRead += actuallyRead;
            }
            ok = !samples.empty();
        }
    }

    remove(tmpPath);
    if (ok) printf("[SongAnalyzer] ffmpeg转换成功\n");
    fflush(stdout);
    return ok;
}

float SongAnalyzer::calculateEnergy(size_t start, size_t count) {
    float energy = 0;
    size_t end = std::min(start + count, samples.size());
    for (size_t i = start; i < end; i++) energy += samples[i] * samples[i];
    return energy / (end - start);
}

std::string SongAnalyzer::getFileNameWithoutExt(const std::string& filePath) {
    std::string name = filePath;
    size_t lastSlash = name.find_last_of("/\\");
    if (lastSlash != std::string::npos) name = name.substr(lastSlash + 1);
    size_t lastDot = name.rfind('.');
    if (lastDot != std::string::npos) name = name.substr(0, lastDot);
    return name;
}

