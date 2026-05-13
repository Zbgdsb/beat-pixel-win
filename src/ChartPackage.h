/**
 * @file ChartPackage.h
 * @brief 谱面导入导出模块
 * @details .beatpixel格式 = zip改后缀，内含MP3 + chart.json + metadata.json
 */
#pragma once
#include <string>
#include <vector>

struct SongPackage {
    std::string songName;
    std::string artist;
    float bpm;
    float duration;
    std::string mp3FileName;
    std::string chartJsonData;
};

class ChartPackage {
public:
    /**
     * @brief 导出歌曲为.beatpixel文件
     * @param mp3Path MP3文件路径
     * @param chartJsonPath chart.json路径
     * @param outputPath 输出.beatpixel路径
     * @return 是否成功
     */
    static bool exportSong(const std::string& mp3Path,
                           const std::string& chartJsonPath,
                           const std::string& outputPath);

    /**
     * @brief 导入.beatpixel文件到songs目录
     * @param packagePath .beatpixel文件路径
     * @param songsDir 目标songs目录
     * @return 导入的歌曲名（空表示失败）
     */
    static std::string importSong(const std::string& packagePath,
                                   const std::string& songsDir);

    /**
     * @brief 检查songs目录中的歌曲数量（用于收藏家成就）
     */
    static int countCustomSongs(const std::string& songsDir);
};
