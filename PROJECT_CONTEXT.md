# BeatPixel 项目上下文文档
> 生成时间：2026-05-09 19:57
> 项目路径：/Users/Admin/Desktop/c++期末大作业/BeatPixel/

---

## 一、项目概述

**BeatPixel** 是一个基于 C++17 + SFML 的音游（节奏游戏）项目，使用 EasyX 兼容层实现图形渲染。

### 技术栈
- **语言**：C++17
- **图形库**：SFML 2.6.x（Graphics/Window/System/Audio）
- **编译器**：g++ (macOS Homebrew)
- **构建**：Makefile
- **平台**：macOS (Apple Silicon)

### 编译命令
```bash
cd /Users/Admin/Desktop/c++期末大作业/BeatPixel
make clean && make
./BeatPixel  # 运行
```

---

## 二、文件结构

```
BeatPixel/
├── src/
│   ├── main.cpp              # 程序入口
│   ├── GameWindow.h          # 游戏窗口类声明（最重要）
│   ├── GameWindow.cpp        # 游戏窗口主实现（包含其他.cpp）
│   ├── GameWindow_render.cpp # 渲染系统（菜单/游戏/结算/设置）
│   ├── GameWindow_input.cpp  # 输入处理（菜单/游戏/结算/暂停/设置）
│   ├── Note.h / Note.cpp     # 音符类
│   ├── NoteTrack.h / NoteTrack.cpp  # 音轨类
│   ├── ScoreSystem.h / ScoreSystem.cpp  # 计分系统
│   ├── AudioManager.h / AudioManager.cpp  # 音频管理
│   ├── BeatParser.h / BeatParser.cpp  # 谱面解析
│   ├── DataManager.h / DataManager.cpp  # 数据管理（排行榜）
│   ├── Animations.h          # 动画结构体定义
│   └── graphics.h            # EasyX兼容层
├── assets/textures/          # 纹理资源
├── songs/                    # 歌曲目录
├── config.ini                # 配置文件（音量+按键）
├── leaderboard.dat           # 排行榜数据
├── Makefile                  # 构建脚本
└── PROJECT_CONTEXT.md        # 本文档
```

---

## 三、已实现功能（V3.1）

### 3.1 核心游戏功能
- ✅ 4轨道音符下落判定（Perfect/Good/Miss）
- ✅ 连击系统（Combo）+ 断连特效
- ✅ 计分系统 + 最大连击统计
- ✅ 音符背景透明修复
- ✅ 判定线发光效果
- ✅ 按键反馈动画（缩放+发光）
- ✅ 粒子特效系统
- ✅ 动态背景

### 3.2 游戏暂停功能
- ✅ ESC键暂停/继续
- ✅ 暂停菜单（继续/重新开始/返回主菜单）
- ✅ 暂停时音乐停止，恢复时继续
- ✅ 暂停时间补偿（不影响音符判定）

### 3.3 结算界面（V3.1新版）
- ✅ 准确率计算和显示
- ✅ 评级系统（S/A/B/C/D）
- ✅ Full Combo / All Perfect 标记显示
- ✅ 排行榜显示
- ✅ 重新开始/返回菜单选项

### 3.4 双音量独立调节
- ✅ 背景音乐音量（0%~100%）
- ✅ 音效音量（0%~100%）
- ✅ 调节时实时预览（音效自动播放测试）
- ✅ 配置持久化（保存到config.ini）

### 3.5 自定义按键设置
- ✅ 4个轨道独立设置按键
- ✅ 支持任意键（W/S/A/D/ESC除外）
- ✅ 自动检测按键重复
- ✅ 恢复默认按键功能
- ✅ 配置持久化（保存到config.ini）

---

## 四、游戏状态枚举

```cpp
enum GameState {
    MENU,     // 主菜单
    PLAYING,  // 游戏中
    PAUSED,   // 暂停
    RESULT    // 结算
};
```

---

## 五、按键配置

### 主菜单
| 按键 | 功能 |
|------|------|
| W | 上移选择 |
| S | 下移选择 |
| ENTER | 确认 |

### 游戏中
| 按键 | 功能 |
|------|------|
| A/S/D/F | 轨道1/2/3/4（可自定义） |
| ESC | 暂停 |

### 暂停界面
| 按键 | 功能 |
|------|------|
| ↑ | 上移选择 |
| ↓ | 下移选择 |
| ENTER | 确认 |
| ESC | 继续游戏 |

### 结算界面
| 按键 | 功能 |
|------|------|
| ↑ | 上移选择 |
| ↓ | 下移选择 |
| ENTER | 确认 |
| ESC | 返回菜单 |

### 设置界面
| 按键 | 功能 |
|------|------|
| W/S | 上下切换选项 |
| A/D | 左右调整音量 |
| ENTER | 确认/进入按键设置 |
| ESC | 返回菜单 |

---

## 六、配置文件格式（config.ini）

```ini
[Settings]
music_volume = 1.00
effect_volume = 1.00
key1 = 65        # A键
key2 = 83        # S键
key3 = 68        # D键
key4 = 70        # F键
```

### 常用虚拟键码
| 键名 | 键码 |
|------|------|
| A-Z | 65-90 |
| 0-9 | 48-57 |
| 空格 | 32 |
| ENTER | 13 |
| ESC | 27 |
| ↑ | 38 |
| ↓ | 40 |
| ← | 37 |
| → | 39 |

---

## 七、关键代码位置

### 7.1 游戏主循环
- **文件**：`src/GameWindow_render.cpp`
- **函数**：`void GameWindow::run()`
- **位置**：约第1170行

### 7.2 菜单输入处理
- **文件**：`src/GameWindow_input.cpp`
- **函数**：`void GameWindow::handleMenuInput()`
- **位置**：约第85行

### 7.3 游戏输入处理
- **文件**：`src/GameWindow_input.cpp`
- **函数**：`void GameWindow::handleInput()`
- **位置**：约第200行

### 7.4 结算界面输入
- **文件**：`src/GameWindow_input.cpp`
- **函数**：`void GameWindow::handleResultInput()`
- **位置**：约第280行

### 7.5 暂停界面输入
- **文件**：`src/GameWindow_input.cpp`
- **函数**：`void GameWindow::handlePauseInput()`
- **位置**：约第568行

### 7.6 设置界面输入
- **文件**：`src/GameWindow_input.cpp`
- **函数**：`void GameWindow::handleSettingsInput()`
- **位置**：约第770行

### 7.7 按键名称转换
- **文件**：`src/GameWindow_input.cpp`
- **函数**：`const char* GameWindow::getKeyName(int vkCode)`
- **位置**：约第657行

### 7.8 配置保存/加载
- **文件**：`src/GameWindow_input.cpp`
- **函数**：`saveConfig()` / `loadConfig()`
- **位置**：约第910行

---

## 八、重要修复记录

### 8.1 音符背景透明修复
- 将音符渲染从纯色背景改为透明背景
- 使用SFML的Alpha混合

### 8.2 暂停功能实现
- 添加PAUSED状态
- 暂停时记录暂停开始时间
- 恢复时补偿暂停时间

### 8.3 上下键切换修复
- **问题**：菜单界面`upPressed`定义错误，把ESC也当作上键
- **修复**：`bool upPressed = (GetAsyncKeyState('W') & 0x8000) != 0;`

### 8.4 结算界面选项切换修复
- **问题**：`resultMenuSelection`在两个函数中分别定义为static局部变量
- **修复**：改为类成员变量，两个函数共享

### 8.5 按键自定义设置修复
- **问题**：按ENTER进入设置模式时，ENTER被立刻设置为自定义按键
- **修复**：添加`keySettingWaitingRelease`标志，先等待所有按键松开

---

## 九、GameWindow.h 成员变量速查

```cpp
// 窗口相关
int width, height;
int fps;
bool isRunning;
GameState gameState;

// 输入状态
bool keyPressed[4];
bool keyWasPressed[4];
int keyGlowAlpha[4];
KeyFeedback keyFeedback[4];

// 游戏状态
int menuSelection;
int pauseMenuSelection;
int resultMenuSelection;        // 结算菜单选择
int settingsMenuSelection;
bool isInSettings;

// 暂停相关
long long pauseStartTime;
bool escKeyReleased;

// 音量设置
float musicVolume;              // 0.0~1.0
float effectVolume;             // 0.0~1.0

// 按键自定义
int customKeys[4];              // 默认 A/S/D/F
int currentKeySettingIndex;     // -1表示未在设置
bool keySettingWaitingRelease;  // 等待按键松开标志

// 动画/特效
std::vector<HitAnim> hitAnims;
std::vector<TextAnim> textAnims;
std::vector<Particle> particles;
ComboAnim comboAnim;
float comboBreakFlash;

// 音符/轨道
std::vector<std::unique_ptr<NoteTrack>> tracks;
ScoreSystem scoreSystem;
AudioManager audioManager;
DataManager dataManager;
BeatParser::Result parseResult;
```

---

## 十、后续可扩展功能

- [ ] 更多歌曲支持
- [ ] 谱面编辑器
- [ ] 多难度切换（Easy/Normal/Hard）
- [ ] 在线排行榜
- [ ] 音符皮肤自定义
- [ ] 背景图片自定义
- [ ] 键位显示在游戏界面
- [ ] 回放系统

---

## 十一、注意事项

1. **编译警告**：有一些未使用变量的警告，不影响运行
2. **文件路径**：配置和排行榜使用绝对路径，移植时需修改
3. **资源加载**：纹理从`assets/textures/`加载，支持多路径查找
4. **音频生成**：所有音频通过正弦波程序化生成，无需外部音频文件

---

*文档结束*
