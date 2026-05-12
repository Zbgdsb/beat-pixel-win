/**
 * @file ChartPackage.cpp
 * @brief 谱面导入导出实现
 * @details 使用系统zip/unzip命令打包，.beatpixel = zip改后缀
 */
#include "ChartPackage.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <algorithm>

#ifndef _WIN32
#include <unistd.h>
#endif

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define popen _popen
#define pclose _pclose
// Windows没有mkdtemp，用_mktemp_s+_mkdir替代
inline char* mkdtemp(char* tpl) {
    if (_mktemp_s(tpl, strlen(tpl) + 1) != 0) return nullptr;
    if (_mkdir(tpl) != 0) return nullptr;
    return tpl;
}
#endif

/**
 * @brief 导出歌曲为.beatpixel文件
 * @details 打包结构：
 *   metadata.json   - 歌曲元信息
 *   song.mp3        - 音频文件
 *   chart.json      - 谱面数据
 */
bool ChartPackage::exportSong(const std::string& mp3Path,
                               const std::string& chartJsonPath,
                               const std::string& outputPath) {
    // 1. 读取chart.json
    std::ifstream chartFile(chartJsonPath);
    if (!chartFile.is_open()) {
        fprintf(stderr, "[ChartPackage] 无法读取谱面: %s\n", chartJsonPath.c_str());
        return false;
    }
    std::stringstream ss;
    ss << chartFile.rdbuf();
    std::string chartData = ss.str();
    chartFile.close();

    // 2. 生成metadata.json
    // 从mp3Path提取文件名
    std::string mp3Name = mp3Path;
    auto pos = mp3Name.find_last_of("/\\");
    if (pos != std::string::npos) mp3Name = mp3Name.substr(pos + 1);

    // 从chartJsonPath推断歌曲名
    std::string songName = chartJsonPath;
    pos = songName.find_last_of("/\\");
    if (pos != std::string::npos) songName = songName.substr(pos + 1);
    // 去掉.chart.json后缀
    auto dotPos = songName.find(".chart.json");
    if (dotPos != std::string::npos) songName = songName.substr(0, dotPos);
    // 去掉.mp3后缀
    dotPos = songName.rfind(".mp3");
    if (dotPos != std::string::npos) songName = songName.substr(0, dotPos);

    // 创建临时目录
    char tmpDir[] = "/tmp/beatpixel_export_XXXXXX";
    if (!mkdtemp(tmpDir)) {
        fprintf(stderr, "[ChartPackage] 无法创建临时目录\n");
        return false;
    }

    // 写metadata.json
    std::string metaPath = std::string(tmpDir) + "/metadata.json";
    FILE* metaFp = fopen(metaPath.c_str(), "w");
    if (!metaFp) return false;
    fprintf(metaFp, "{\n");
    fprintf(metaFp, "  \"format\": \"beatpixel-v1\",\n");
    fprintf(metaFp, "  \"songName\": \"%s\",\n", songName.c_str());
    fprintf(metaFp, "  \"mp3File\": \"%s\"\n", mp3Name.c_str());
    fprintf(metaFp, "}\n");
    fclose(metaFp);

    // 复制MP3到临时目录
    std::string tmpMp3 = std::string(tmpDir) + "/" + mp3Name;
    {
        std::ifstream src(mp3Path, std::ios::binary);
        std::ofstream dst(tmpMp3, std::ios::binary);
        if (!src.is_open()) {
            fprintf(stderr, "[ChartPackage] 无法读取MP3: %s\n", mp3Path.c_str());
            return false;
        }
        dst << src.rdbuf();
    }

    // 写chart.json
    std::string tmpChart = std::string(tmpDir) + "/chart.json";
    {
        std::ofstream dst(tmpChart);
        dst << chartData;
    }

    // 3. 打包为zip
    std::string cmd = "cd '" + std::string(tmpDir) + "' && zip -j '" + outputPath + "' metadata.json '" + mp3Name + "' chart.json 2>&1";
    int ret = system(cmd.c_str());

    // 清理临时文件
    cmd = "rm -rf '" + std::string(tmpDir) + "'";
    system(cmd.c_str());

    if (ret != 0) {
        fprintf(stderr, "[ChartPackage] 打包失败\n");
        return false;
    }

    printf("[ChartPackage] 导出成功: %s\n", outputPath.c_str());
    fflush(stdout);
    return true;
}

/**
 * @brief 导入.beatpixel文件到songs目录
 * @details 解压到临时目录，读取metadata，复制MP3到songs/
 */
std::string ChartPackage::importSong(const std::string& packagePath,
                                      const std::string& songsDir) {
    // 1. 创建临时解压目录
    char tmpDir[] = "/tmp/beatpixel_import_XXXXXX";
    if (!mkdtemp(tmpDir)) {
        fprintf(stderr, "[ChartPackage] 无法创建临时目录\n");
        return "";
    }

    // 2. 解压
    std::string cmd = "unzip -o '" + packagePath + "' -d '" + tmpDir + "' 2>&1";
    int ret = system(cmd.c_str());
    if (ret != 0) {
        fprintf(stderr, "[ChartPackage] 解压失败: %s\n", packagePath.c_str());
        cmd = "rm -rf '" + std::string(tmpDir) + "'";
        system(cmd.c_str());
        return "";
    }

    // 3. 读取metadata.json（如果存在）
    std::string songName;
    std::string mp3FileName;
    std::string metaPath = std::string(tmpDir) + "/metadata.json";
    FILE* metaFp = fopen(metaPath.c_str(), "r");
    if (metaFp) {
        char line[512];
        while (fgets(line, sizeof(line), metaFp)) {
            if (strstr(line, "songName")) {
                char* start = strchr(line, '"');
                if (start) {
                    start++;
                    char* end = strchr(start, '"');
                    if (end) songName = std::string(start, end);
                }
            }
            if (strstr(line, "mp3File")) {
                char* start = strchr(line, '"');
                if (start) {
                    start++;
                    char* end = strchr(start, '"');
                    if (end) mp3FileName = std::string(start, end);
                }
            }
        }
        fclose(metaFp);
    }

    // 4. 查找MP3文件
    std::string mp3Path;
    if (!mp3FileName.empty()) {
        mp3Path = std::string(tmpDir) + "/" + mp3FileName;
    } else {
        // 自动查找第一个mp3
        cmd = "find '" + std::string(tmpDir) + "' -name '*.mp3' | head -1";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (pipe) {
            char buf[512];
            if (fgets(buf, sizeof(buf), pipe)) {
                mp3Path = buf;
                if (!mp3Path.empty() && mp3Path.back() == '\n')
                    mp3Path.pop_back();
            }
            pclose(pipe);
        }
    }

    if (mp3Path.empty()) {
        fprintf(stderr, "[ChartPackage] 包内未找到MP3文件\n");
        cmd = "rm -rf '" + std::string(tmpDir) + "'";
        system(cmd.c_str());
        return "";
    }

    // 5. 复制MP3到songs目录
    if (songName.empty()) {
        // 从文件名推断
        songName = mp3FileName;
        auto dotPos = songName.rfind(".mp3");
        if (dotPos != std::string::npos) songName = songName.substr(0, dotPos);
    }

    std::string destPath = songsDir + "/" + songName + ".mp3";
    cmd = "cp '" + mp3Path + "' '" + destPath + "'";
    ret = system(cmd.c_str());

    // 复制chart.json（如果存在）
    std::string chartPath = std::string(tmpDir) + "/chart.json";
    FILE* cf = fopen(chartPath.c_str(), "r");
    if (cf) {
        fclose(cf);
        std::string destChart = songsDir + "/" + songName + ".mp3.chart.json";
        cmd = "cp '" + chartPath + "' '" + destChart + "'";
        system(cmd.c_str());
    }

    // 清理
    cmd = "rm -rf '" + std::string(tmpDir) + "'";
    system(cmd.c_str());

    if (ret != 0) {
        fprintf(stderr, "[ChartPackage] 复制失败\n");
        return "";
    }

    printf("[ChartPackage] 导入成功: %s -> %s\n", songName.c_str(), destPath.c_str());
    fflush(stdout);
    return songName;
}

/**
 * @brief 统计songs目录中的MP3文件数
 */
int ChartPackage::countCustomSongs(const std::string& songsDir) {
    std::string cmd = "find '" + songsDir + "' -name '*.mp3' 2>/dev/null | wc -l";
    FILE* pipe = popen(cmd.c_str(), "r");
    int count = 0;
    if (pipe) {
        char buf[16];
        if (fgets(buf, sizeof(buf), pipe)) {
            count = atoi(buf);
        }
        pclose(pipe);
    }
    return count;
}
