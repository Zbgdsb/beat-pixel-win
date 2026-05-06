# 《像素节拍》V1.0 类结构说明
## 1. GameWindow 类
- 职责：游戏窗口初始化、帧率控制（60FPS）、全局渲染调度、事件循环处理
- 成员变量：窗口宽度、窗口高度、帧率
- 成员函数：init(), run(), render(), handleInput()

## 2. NoteTrack 类
- 职责：单个轨道管理，负责轨道绘制、该轨道的音符列表管理、判定区域绘制
- 成员变量：轨道编号、轨道X坐标、判定线Y坐标、音符列表
- 成员函数：draw(), addNote(), update(), checkJudgement()

## 3. Note 基类
- 职责：音符通用属性和方法定义
- 成员变量：生成时间、判定时间、Y坐标、轨道编号、是否已判定
- 成员函数：update(), draw(), judge()
- 派生类：NormalNote 普通音符，实现普通下落和判定逻辑

## 4. ScoreSystem 类
- 职责：计分、判定等级、Combo计数管理
- 成员变量：总分、当前Combo、最高Combo、Perfect计数、Good计数、Miss计数
- 成员函数：addJudgement(), getScore(), getCombo(), reset()
- 判定规则：Perfect(±50ms) 得100分，Good(±150ms) 得50分，Miss 得0分，Combo每加1额外加1分