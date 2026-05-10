# Windows运行方案

## 方案一：GitHub Actions自动编译（推荐，无需装任何东西）

1. 注册GitHub账号：https://github.com
2. 创建一个新仓库，把BeatPixel代码推上去
3. 推送后自动开始编译，等2-3分钟
4. 点击仓库页面的 `Actions` → 最新的构建 → `BeatPixel-Windows` → 下载
5. 解压后双击 `BeatPixel.exe` 即可运行

### 操作步骤：
```bash
# 在BeatPixel目录下打开终端，执行：
git init
git add .
git commit -m "init"
git branch -M main
git remote add origin https://github.com/你的用户名/BeatPixel.git
git push -u origin main
```

## 方案二：MSYS2本地编译（需要Windows电脑）

### 安装MSYS2
1. 下载安装：https://www.msys2.org
2. 打开 MSYS2 MinGW 64-bit 终端
3. 执行：
```bash
pacman -S mingw-w64-x86_64-gcc
```

### 安装EasyX
1. 下载：https://easyx.cn
2. 运行安装程序，选择MinGW版本

### 编译运行
```bash
cd /path/to/BeatPixel
chmod +x build_windows.sh
./build_windows.sh
cd dist
./BeatPixel.exe
```

## 注意事项
- 运行时确保 `assets/` 和 `songs/` 文件夹与exe在同一目录
- 配置文件和排行榜自动生成在exe所在目录
