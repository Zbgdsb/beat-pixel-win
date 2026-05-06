/**
 * @file graphics.h
 * @brief EasyX兼容层 - 将EasyX API映射到SFML
 * @details 在macOS/Linux上用SFML实现EasyX的常用绘图函数
 *          Windows上直接用EasyX原始头文件，本文件不生效
 *
 * 支持的EasyX函数：
 *   - initgraph(), closegraph()
 *   - setbkcolor(), cleardevice()
 *   - setfillcolor(), fillrectangle()
 *   - setlinecolor(), rectangle(), line()
 *   - setlinestyle()
 *   - settextcolor(), settextstyle(), outtextxy(), textwidth()
 *   - BeginBatchDraw(), FlushBatchDraw(), EndBatchDraw()
 *   - GetAsyncKeyState(), GetTickCount64(), Sleep()
 *   - getwidth(), getheight()
 */
#pragma once

#ifdef _WIN32
// Windows平台：直接使用EasyX原始头文件
#include <easyx.h>
#else
// macOS/Linux平台：通过SFML实现EasyX兼容接口

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <string>
#include <cstring>
#include <cmath>
#include <thread>
#include <chrono>
#include <map>
#include <cstdint>

// ========== EasyX常量定义 ==========
#define PS_SOLID 0
#define VK_ESCAPE 27
#define VK_RETURN 13

// ========== 全局SFML窗口和状态 =/
namespace _easyx_impl {
    // 全局窗口指针（initgraph创建）
    inline sf::RenderWindow* g_window = nullptr;

    // 全局绘图状态
    inline sf::Color g_fillColor = sf::Color::White;
    inline sf::Color g_lineColor = sf::Color::White;
    inline sf::Color g_textColor = sf::Color::White;
    inline sf::Color g_bgColor = sf::Color::Black;
    inline int g_lineThickness = 1;
    inline unsigned int g_textSize = 20;

    // 字体缓存
    inline sf::Font g_font;
    inline bool g_fontLoaded = false;

    // 窗口是否还开着
    inline bool g_windowOpen = false;

    // 加载字体（优先查找系统字体）
    inline bool loadFont() {
        if (g_fontLoaded) return true;
        // macOS常见中文字体路径
        const char* fontPaths[] = {
            "/System/Library/Fonts/STHeiti Medium.ttc",
            "/System/Library/Fonts/Hiragino Sans GB.ttc",
            "/System/Library/Fonts/Menlo.ttc",
            "/System/Library/Fonts/HelveticaNeue.ttc",
            "/Library/Fonts/Arial Unicode.ttf",
            "/Library/Fonts/Arial.ttf",
            "/System/Library/Fonts/Monaco.dfont",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", // Linux
            "C:/Windows/Fonts/consola.ttf", // Windows备用
        };
        for (const char* path : fontPaths) {
            if (g_font.openFromFile(path)) {
                g_fontLoaded = true;
                return true;
            }
        }
        return false;
    }

    // char* 转 sf::String（支持中文）
    inline sf::String toSfString(const char* str) {
        return sf::String::fromUtf8(str, str + strlen(str));
    }

    // 字符虚拟键码到SFML Key的映射
    inline sf::Keyboard::Key charToKey(char c) {
        switch (toupper(c)) {
            case 'A': return sf::Keyboard::Key::A;
            case 'S': return sf::Keyboard::Key::S;
            case 'D': return sf::Keyboard::Key::D;
            case 'F': return sf::Keyboard::Key::F;
            case 'E': return sf::Keyboard::Key::E;
            case 'Q': return sf::Keyboard::Key::Q;
            case 'W': return sf::Keyboard::Key::W;
            case 'R': return sf::Keyboard::Key::R;
            default: return sf::Keyboard::Key::Unknown;
        }
    }
}

// ========== EasyX类型定义 =/
typedef unsigned int COLORREF;
typedef unsigned long DWORD;
typedef short SHORT;
#ifndef _T
#define _T(x) x
#endif

inline COLORREF RGB(int r, int g, int b) {
    return (0xFF << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
}

inline sf::Color toSfColor(COLORREF c) {
    return sf::Color((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
}

// ========== EasyX函数实现 ==========

/**
 * @brief 初始化图形窗口
 */
inline void initgraph(int width, int height) {
    using namespace _easyx_impl;
    if (g_window) { delete g_window; g_window = nullptr; }
    g_window = new sf::RenderWindow(sf::VideoMode({(unsigned)width, (unsigned)height}),
                                     "BeatPixel");
    g_window->setFramerateLimit(120); // 帧率由游戏主循环控制
    g_windowOpen = true;
    loadFont();
}

/**
 * @brief 关闭图形窗口
 */
inline void closegraph() {
    using namespace _easyx_impl;
    g_windowOpen = false;
    if (g_window) {
        g_window->close();
        delete g_window;
        g_window = nullptr;
    }
}

/**
 * @brief 设置背景色
 */
inline void setbkcolor(COLORREF color) {
    _easyx_impl::g_bgColor = toSfColor(color);
}

/**
 * @brief 清屏（用背景色填充）
 */
inline void cleardevice() {
    using namespace _easyx_impl;
    if (g_window && g_windowOpen) {
        g_window->clear(g_bgColor);
    }
}

/**
 * @brief 设置填充颜色
 */
inline void setfillcolor(COLORREF color) {
    _easyx_impl::g_fillColor = toSfColor(color);
}

/**
 * @brief 设置线条颜色
 */
inline void setlinecolor(COLORREF color) {
    _easyx_impl::g_lineColor = toSfColor(color);
}

/**
 * @brief 设置线条样式
 */
inline void setlinestyle(int style, int thickness = 1) {
    _easyx_impl::g_lineThickness = thickness;
}

/**
 * @brief 绘制填充矩形
 */
inline void fillrectangle(int left, int top, int right, int bottom) {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) return;

    // 填充部分
    sf::RectangleShape fill({(float)(right - left), (float)(bottom - top)});
    fill.setPosition({(float)left, (float)top});
    fill.setFillColor(g_fillColor);
    g_window->draw(fill);

    // 边框部分
    sf::RectangleShape outline({(float)(right - left), (float)(bottom - top)});
    outline.setPosition({(float)left, (float)top});
    outline.setFillColor(sf::Color::Transparent);
    outline.setOutlineColor(g_lineColor);
    outline.setOutlineThickness((float)g_lineThickness);
    g_window->draw(outline);
}

/**
 * @brief 绘制矩形边框
 */
inline void rectangle(int left, int top, int right, int bottom) {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) return;

    sf::RectangleShape rect({(float)(right - left), (float)(bottom - top)});
    rect.setPosition({(float)left, (float)top});
    rect.setFillColor(sf::Color::Transparent);
    rect.setOutlineColor(g_lineColor);
    rect.setOutlineThickness((float)g_lineThickness);
    g_window->draw(rect);
}

/**
 * @brief 绘制直线
 */
inline void line(int x1, int y1, int x2, int y2) {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) return;

    // SFML没有原生line，用细矩形模拟
    float dx = (float)(x2 - x1);
    float dy = (float)(y2 - y1);
    float length = std::sqrt(dx * dx + dy * dy);
    if (length < 0.1f) return;

    sf::RectangleShape lineShape({length, (float)g_lineThickness});
    lineShape.setPosition({(float)x1, (float)y1});
    lineShape.setFillColor(g_lineColor);

    float angle = std::atan2(dy, dx) * 180.0f / 3.14159265f;
    lineShape.setRotation(sf::degrees(angle));
    g_window->draw(lineShape);
}

/**
 * @brief 设置文字颜色
 */
inline void settextcolor(COLORREF color) {
    _easyx_impl::g_textColor = toSfColor(color);
}

/**
 * @brief 设置文字样式
 * @param height 字体高度
 * @param width  字体宽度（0为自适应）
 * @param face   字体名称
 */
inline void settextstyle(int height, int width, const char* face) {
    _easyx_impl::g_textSize = (unsigned int)height;
    (void)width;
    (void)face;
}

/**
 * @brief 在指定位置输出文字
 */
inline void outtextxy(int x, int y, const char* str) {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen || !g_fontLoaded) return;

    sf::Text text(g_font, toSfString(str), g_textSize);
    text.setPosition({(float)x, (float)y});
    text.setFillColor(g_textColor);
    g_window->draw(text);
}

/**
 * @brief 获取文字宽度
 */
inline int textwidth(const char* str) {
    using namespace _easyx_impl;
    if (!g_fontLoaded) return (int)strlen(str) * 10;

    sf::Text text(g_font, toSfString(str), g_textSize);
    return (int)text.getLocalBounds().size.x;
}

/**
 * @brief 获取窗口宽度
 */
inline int getwidth() {
    if (_easyx_impl::g_window)
        return (int)_easyx_impl::g_window->getSize().x;
    return 0;
}

/**
 * @brief 获取窗口高度
 */
inline int getheight() {
    if (_easyx_impl::g_window)
        return (int)_easyx_impl::g_window->getSize().y;
    return 0;
}

// ========== 双缓冲 ==========
inline void BeginBatchDraw() {
    // SFML默认就是双缓冲模式，无需特殊处理
}

inline void FlushBatchDraw() {
    using namespace _easyx_impl;
    if (g_window && g_windowOpen) {
        g_window->display();
    }
}

/**
 * @brief 处理SFML窗口事件（关闭等）
 * @return true窗口仍打开，false窗口已关闭
 */
inline bool processWindowEvents() {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) return false;
    while (const auto event = g_window->pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            g_windowOpen = false;
            g_window->close();
            return false;
        }
    }
    return true;
}

inline void EndBatchDraw() {
    FlushBatchDraw();
}

// ========== 输入处理 ==========

/**
 * @brief GetAsyncKeyState兼容实现
 * @details 通过SFML实时查询键盘状态
 *          最高位(0x8000)表示当前是否按下
 */
inline SHORT GetAsyncKeyState(int vKey) {
    using namespace _easyx_impl;
    if (!g_window || !g_windowOpen) return 0;

    sf::Keyboard::Key key = sf::Keyboard::Key::Unknown;

    // 字母键
    if (vKey >= 'A' && vKey <= 'Z') {
        key = (sf::Keyboard::Key)(vKey - 'A' + (int)sf::Keyboard::Key::A);
    }
    // 特殊键
    else if (vKey == 27) {
        key = sf::Keyboard::Key::Escape;
    }
    else if (vKey == 13) {
        key = sf::Keyboard::Key::Enter;
    }

    if (key != sf::Keyboard::Key::Unknown) {
        if (sf::Keyboard::isKeyPressed(key)) {
            return (SHORT)0x8001; // 按下状态
        }
    }
    return 0;
}

// ========== 时间和Sleep ==========

/**
 * @brief GetTickCount64兼容实现（返回毫秒时间戳）
 */
inline uint64_t GetTickCount64() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(
        steady_clock::now().time_since_epoch()
    ).count();
}

/**
 * @brief Sleep兼容实现（毫秒）
 */
inline void Sleep(DWORD ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

#endif // _WIN32
