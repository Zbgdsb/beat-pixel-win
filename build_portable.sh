#!/bin/bash
# ==========================================
#  BeatPixel 一键编译脚本 (便携版)
#  不需要安装任何软件！
#
#  使用方法：
#  1. 下载便携版MSYS2（见下方链接）
#  2. 把本文件夹放到MSYS2的home/你的用户名/目录下
#  3. 双击运行 msys2_shell.cmd
#  4. 执行: cd ~/BeatPixel && bash build_portable.sh
# ==========================================

set -e
echo "=============================="
echo "  BeatPixel Windows 编译器"
echo "=============================="

# 检查g++
if ! command -v g++ &> /dev/null; then
    echo ""
    echo "[提示] 首次运行需要安装编译器，请执行："
    echo "  pacman -S --noconfirm mingw-w64-x86_64-gcc"
    echo ""
    echo "安装完成后重新运行本脚本"
    exit 1
fi

# 检查EasyX
if ! echo '#include <easyx.h>' | g++ -x c++ -c - -o /dev/null 2>/dev/null; then
    echo ""
    echo "[提示] 需要安装EasyX图形库"
    echo "  下载地址: https://easyx.cn"
    echo "  下载后运行安装程序，选择MinGW版本即可"
    echo ""
    echo "安装完成后重新运行本脚本"
    exit 1
fi

echo "[1/4] 清理旧文件..."
rm -rf build dist 2>/dev/null
mkdir -p build

echo "[2/4] 编译源文件..."
for f in src/*.cpp; do
    name=$(basename "$f" .cpp)
    echo "  -> $name"
    g++ -std=c++17 -O2 -Wall -c -o "build/$name.o" "$f"
done

echo "[3/4] 链接..."
g++ -std=c++17 -O2 -o BeatPixel.exe build/*.o \
    -leasyx -lmsimg32 -lwinmm -lgdi32 -luser32

echo "[4/4] 打包..."
mkdir -p dist
cp BeatPixel.exe dist/
cp -r assets dist/ 2>/dev/null || true
cp -r songs dist/ 2>/dev/null || true

echo ""
echo "=============================="
echo "  编译成功!"
echo "  输出: dist/BeatPixel.exe"
echo "=============================="
echo ""
echo "把dist文件夹发给老师，双击BeatPixel.exe即可运行"
