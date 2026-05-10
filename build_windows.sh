#!/bin/bash
# BeatPixel 一键编译脚本 (MSYS2/MinGW环境)
# 用法: 在MSYS2 MinGW64终端中执行 ./build_windows.sh

echo "=== BeatPixel 编译器 ==="

# 检查g++
if ! command -v g++ &> /dev/null; then
    echo "[错误] 未找到g++，请先安装: pacman -S mingw-w64-x86_64-gcc"
    exit 1
fi

# 检查EasyX
if ! echo "#include <easyx.h>" | g++ -x c++ -c - -o /dev/null 2>/dev/null; then
    echo "[错误] 未找到EasyX，请先安装: https://easyx.cn"
    exit 1
fi

echo "[1/3] 编译源文件..."
mkdir -p build
OK=1
for f in src/*.cpp; do
    obj="build/$(basename "$f" .cpp).o"
    echo "  -> $f"
    g++ -std=c++17 -O2 -Wall -c -o "$obj" "$f" || OK=0
done

if [ $OK -eq 0 ]; then
    echo "[错误] 编译失败"
    exit 1
fi

echo "[2/3] 链接..."
g++ -std=c++17 -O2 -o BeatPixel.exe build/*.o \
    -leasyx -lmsimg32 -lwinmm -lgdi32 -luser32 || {
    echo "[错误] 链接失败"
    exit 1
}

echo "[3/3] 打包..."
mkdir -p dist
cp BeatPixel.exe dist/
cp -r assets dist/ 2>/dev/null
cp -r songs dist/ 2>/dev/null

echo ""
echo "=== 编译成功! ==="
echo "运行: cd dist && ./BeatPixel.exe"
