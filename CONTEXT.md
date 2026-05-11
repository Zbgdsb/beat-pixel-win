# BeatPixel 项目上下文文档
> 生成时间: 2026-05-11 09:26 GMT+8
> 用途: 新对话启动时读取此文件恢复上下文

## 项目概述
C++ 4键音游项目，SFML 3.x (macOS) + EasyX (Windows) 双平台。
Gitee仓库: https://gitee.com/wangzheliangjian/beat-pixel
项目路径: `/Users/Admin/Desktop/c++期末大作业/BeatPixel/`

## 当前版本: V3.5
最近5次提交:
- a9a6d79 fix: 打击乐模式改进（按键即时发声+GOOD判定用鼓声）
- 200acac fix: 鼓声模式soundPack被init重置
- 711e132 fix: Song List鼠标点击+暂停ENTER
- d11d3b1 feat: 架子鼓音效-4轨道4种鼓声
- 7f05535 feat: 打击乐音效包+设置界面切换

## 核心架构

### 源文件结构
```
src/
├── GameWindow.h           # 主窗口类定义
├── GameWindow.cpp         # 主窗口实现+init()
├── GameWindow_input.cpp   # 所有输入处理（菜单/游戏/暂停/设置/分析/歌曲列表/成就）
├── GameWindow_render.cpp  # 所有渲染（菜单/游戏/暂停/设置/分析/歌曲列表/成就）
├── AudioManager.h/cpp     # 音频管理（BGM+音效+原曲播放）
├── SongAnalyzer.h/cpp     # 歌曲分析（BPM检测+onset检测+谱面生成）
├── NoteTrack.h/cpp        # 轨道管理
├── Note.h                 # 音符类（含shiftTime偏移方法）
├── ScoreSystem.h/cpp      # 计分系统
├── BeatParser.h/cpp       # 旧版谱面解析（.txt格式）
├── DataManager.h/cpp      # 排行榜持久化
├── TextureManager.h       # 纹理管理
├── Animations.h           # 动画系统
├── ChartPackage.h/cpp     # 谱面导入导出（.beatpixel格式）
├── AchievementSystem.h/cpp # 成就系统（10个成就）
├── graphics.h             # SFML 3.x vs SFML 2.6兼容层
tools/
└── beat_detect.py         # Python节拍检测脚本（numpy+scipy）
```

### 游戏状态机
```
MENU → PLAYING ↔ PAUSED → RESULT → MENU
  ↓
ANALYZING → PLAYING (从分析界面开始游戏)
  ↓
(子界面: Settings / Achievements / SongList 都在MENU状态下叠加显示)
```

### 关键成员变量（GameWindow.h）
```cpp
// 状态
GameState gameState;           // MENU/PLAYING/PAUSED/RESULT/ANALYZING
bool isInSettings;             // 设置界面
bool showAchievements;         // 成就界面
bool isShowingSongList;        // 歌曲列表界面
int menuSelection;             // 主菜单选中项 (0=Song List)
int settingsMenuSelection;     // 设置选项 (0-8)
int songListSelection;         // 歌曲列表选中项

// 歌曲数据
std::string currentSongName;
std::string analysisFilePath;  // 当前分析的文件路径
SongAnalyzer::AnalysisResult analysisResult;
BeatParser::ParseResult parseResult;
std::vector<std::pair<long long, int>> noteTimeData;
float gameOffsetMs;            // 游戏内实时偏移补偿(ms)

// 音效
int soundPack;                 // 0=叮咚 1=打击乐（保存在AudioManager）
```

## 已实现的功能

### 1. 歌曲分析界面 (V3.2)
- BPM自动检测（Python脚本 → aubio/numpy+scipy）
- onset检测 → 谱面生成
- A/D调整BPM, 左右调整偏移
- T键点拍校准
- 导出chart.json

### 2. 谱面导入导出 (V3.3)
- .beatpixel格式（zip改后缀，含MP3+chart.json）
- 导入自动去重

### 3. 成就系统 (V3.3)
- 10个成就，解锁弹窗动画，数据持久化(achievements.dat)

### 4. 歌曲列表 (V3.4)
- 自动扫描songs目录下所有.mp3和.beatpixel
- W/S选择，ENTER播放，ESC返回
- Demo歌曲作为第一项

### 5. 打击乐音效 (V3.5)
- Settings → Sound Pack 切换 (Ding-Dong / Percussion)
- 打击乐模式: A=kick, S=snare, D=hihat, F=tom
- 按键即时发声（不依赖音符判定）
- 保存到config.ini

### 6. 游戏内偏移校准
- F1: -10ms, F2: +10ms
- 左上角实时显示偏移量

### 7. 原曲播放
- MP3导入后播放原曲（sf::Music流式播放）
- 不支持的格式自动ffmpeg转码

### 8. Python节拍检测 (tools/beat_detect.py)
- 三后端自动切换: librosa > aubio > numpy+scipy
- 当前实际用: numpy+scipy（spectral flux + DP beat tracking）
- 输出JSON: success, beat_times_ms, bpm, total_notes, error
- BPM网格对齐后处理（误差<30ms）
- 最小间隔80ms过滤

## 待解决的问题

### 已知Bug
1. **音效文件**: 用户想找4个真实的打击乐WAV采样替换合成音效
   - 需要: kick.wav, snare.wav, hihat.wav, tom.wav
   - 格式: WAV, 44100Hz, 16bit, mono, 0.1-0.2秒
   - 放到 assets/sounds/ 目录

### 待优化
1. **节拍对齐**: 用户反馈导入的MP3节奏和原曲对不上
   - 原因: 音频播放延迟(系统缓冲区20-100ms)
   - 已有F1/F2校准，但需要用户手动调
   - 曾尝试装librosa(需numba→需编译LLVM)，Python 3.14没有预编译wheel，太慢被系统杀掉
2. **谱面质量**: 轨道分配是随机的，不能根据旋律分配
3. **Windows编译**: graphics.h有SFML 2.6兼容层，GitHub Actions CI/CD自动编译

## 编译命令
```bash
cd /Users/Admin/Desktop/c++期末大作业/BeatPixel
make clean && make
./BeatPixel
```

## 配置文件
- `config.ini`: 音量、按键绑定、soundPack
- `leaderboard.dat`: 排行榜
- `achievements.dat`: 成就数据

## 用户偏好
- 称呼: 老大
- 所有代码修改直接推Gitee
- 中文界面
- 不要再装numba/librosa了（太慢）
- 打击乐音效要真实采样，不要合成的
