@echo off
REM ============================================
REM  BeatPixel Windows编译脚本
REM  需要：MinGW-w64 (g++) + EasyX + SFML (可选)
REM ============================================

echo [BeatPixel] 正在编译...

REM 创建build目录
if not exist build mkdir build

REM 编译所有源文件
g++ -std=c++17 -O2 -Wall -I./src ^
    -c -o build/main.o src/main.cpp ^
    -c -o build/GameWindow.o src/GameWindow.cpp ^
    -c -o build/NoteTrack.o src/NoteTrack.cpp ^
    -c -o build/Note.o src/Note.cpp ^
    -c -o build/ScoreSystem.o src/ScoreSystem.cpp ^
    -c -o build/AudioManager.o src/AudioManager.cpp ^
    -c -o build/BeatParser.o src/BeatParser.cpp ^
    -c -o build/DataManager.o src/DataManager.cpp

if %ERRORLEVEL% NEQ 0 (
    echo [错误] 编译失败！
    pause
    exit /b 1
)

REM 链接（Windows: -leasyx -lmsimg32 -lwinmm）
g++ -std=c++17 -O2 -o BeatPixel.exe ^
    build/main.o build/GameWindow.o build/NoteTrack.o ^
    build/Note.o build/ScoreSystem.o build/AudioManager.o ^
    build/BeatParser.o build/DataManager.o ^
    -leasyx -lmsimg32 -lwinmm -lgdi32 -luser32

if %ERRORLEVEL% NEQ 0 (
    echo [错误] 链接失败！请确认已安装EasyX
    echo 下载地址: https://easyx.cn
    pause
    exit /b 1
)

echo [BeatPixel] 编译成功！运行 BeatPixel.exe
pause
