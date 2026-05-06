# 《像素节拍》V1.0 类结构说明

## 1. GameWindow 类
- **职责**：游戏窗口初始化、帧率控制（60FPS）、全局渲染调度、事件循环处理
- **成员变量**：
  - `width/height`：窗口尺寸
  - `fps`：目标帧率
  - `tracks`：4条NoteTrack轨道
  - `scoreSystem`：计分系统
  - `gameStartTime`：游戏开始时间戳
  - `currentTime`：当前游戏内时间
  - `lastJudgement`：最近判定结果（用于UI显示）
- **成员函数**：
  - `init()`：初始化窗口和游戏组件
  - `run()`：60FPS主循环
  - `handleInput()`：A/S/D/F按键检测
  - `update()`：更新所有轨道音符状态
  - `render()`：双缓冲渲染
  - `drawUI()`：分数、Combo、判定结果显示
  - `initTracks()`：初始化4条轨道
  - `loadDemoSong()`：加载演示谱面

## 2. NoteTrack 类
- **职责**：单个轨道管理，负责轨道绘制、该轨道的音符列表管理、判定区域绘制
- **成员变量**：
  - `trackId`：轨道编号 0-3
  - `x/width`：轨道位置和宽度
  - `judgeY`：判定线Y坐标
  - `noteSpeed`：音符下落速度
  - `notes`：音符智能指针列表
  - `key`：对应按键字符
- **成员函数**：
  - `addNote()`：添加音符到轨道
  - `update()`：更新所有音符位置，处理MISS判定
  - `draw()`：绘制轨道背景、判定线、按键提示、音符
  - `handlePress()`：按键判定触发，找到最近音符执行判定

## 3. Note 基类（多态）
- **职责**：音符通用属性和方法定义
- **成员变量**：`track`(轨道编号)、`y`(当前Y坐标)、`judgeY`(判定线Y)、`speed`(下落速度)、`isJudged`(是否已判定)、`judgeTime`(判定时间戳)
- **成员函数**：
  - `update()`：根据时间差更新Y坐标
  - `draw()`：纯虚函数，派生类实现绘制
  - `judge()`：三级判定逻辑（Perfect±50ms / Good±150ms / Miss）
  - `isPastJudgeLine()`：检测音符是否已过判定线
  - `isOffScreen()`：检测音符是否离开屏幕
- **派生类**：`NormalNote` 普通音符，实现矩形像素风格绘制

## 4. ScoreSystem 类
- **职责**：计分、判定等级、Combo计数管理
- **成员变量**：`totalScore`(总分)、`currentCombo`(当前Combo)、`maxCombo`(最高Combo)、`perfectCount/goodCount/missCount`(各级判定计数)
- **成员函数**：
  - `addJudgement()`：根据判定结果更新分数和Combo
  - `reset()`：重置所有计分数据
- **判定规则**：Perfect得100分，Good得50分，Miss得0分且重置Combo，每加1次Combo额外+1分

## 类关系图
```
GameWindow ──┬── NoteTrack[] ──── Note* (多态)
             │                        └── NormalNote
             └── ScoreSystem
```
- **组合关系**：GameWindow包含4条NoteTrack和1个ScoreSystem
- **聚合关系**：NoteTrack管理多个Note智能指针
- **多态实现**：Note基类通过纯虚函数draw()实现多态，NormalNote派生类实现具体绘制
