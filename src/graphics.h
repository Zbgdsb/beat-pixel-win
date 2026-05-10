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

#if defined(_WIN32) && !defined(BEATPIXEL_USE_SFML)
// Windows平台：使用EasyX原始头文件 + SFML兼容类型
#include <easyx.h>
#include <windows.h>
#include <cmath>
#include <cstdint>
#include <chrono>
#include <thread>
#include <string>

// ========== 路径辅助 ==========
namespace _easyx_impl {
    inline std::string getProjectRoot() {
        const char* candidates[] = {".", "..", "../..", "../../.."};
        for (auto& c : candidates) {
            std::string test = std::string(c) + "/assets/textures";
            DWORD attr = GetFileAttributesA(test.c_str());
            if (attr != INVALID_FILE_ATTRIBUTES) return std::string(c);
        }
        return ".";
    }
    inline std::string basePath(const char* rel) {
        static std::string root;
        if (root.empty()) root = getProjectRoot();
        return root + "/" + rel;
    }
    inline bool g_windowOpen = true;
}

// ========== SFML兼容类型（Windows EasyX版） ==========
namespace sf {
    struct Color {
        uint8_t r, g, b, a;
        Color() : r(255), g(255), b(255), a(255) {}
        Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}
        static const Color White, Black, Red, Green, Blue, Yellow, Cyan, Magenta, Transparent;
    };
    inline const Color Color::White(255,255,255), Color::Black(0,0,0),
        Color::Red(255,0,0), Color::Green(0,255,0), Color::Blue(0,0,255),
        Color::Yellow(255,255,0), Color::Cyan(0,255,255), Color::Magenta(255,0,255),
        Color::Transparent(0,0,0,0);

    template<typename T>
    struct Vector2 { T x, y; Vector2() : x(0), y(0) {} Vector2(T x, T y) : x(x), y(y) {} };
    using Vector2f = Vector2<float>;
    using Vector2i = Vector2<int>;
    using Vector2u = Vector2<unsigned int>;

    class RectangleShape {
        Vector2f m_pos, m_size;
        Color m_fill{0,0,0,0}, m_outline{0,0,0,0};
        float m_outlineThick = 0;
        float m_rotation = 0;
        Vector2f m_scale{1,1};
    public:
        RectangleShape(Vector2f size) : m_size(size) {}
        void setPosition(Vector2f p) { m_pos = p; }
        void setFillColor(Color c) { m_fill = c; }
        void setOutlineColor(Color c) { m_outline = c; }
        void setOutlineThickness(float t) { m_outlineThick = t; }
        void setRotation(float deg) { m_rotation = deg; }
        void setScale(Vector2f s) { m_scale = s; }
        void draw() const {
            if (m_rotation != 0) return;
            int x1 = (int)m_pos.x, y1 = (int)m_pos.y;
            int x2 = x1 + (int)(m_size.x * m_scale.x), y2 = y1 + (int)(m_size.y * m_scale.y);
            if (m_fill.a > 0) {
                COLORREF fc = RGB(m_fill.r, m_fill.g, m_fill.b);
                setfillcolor(fc);
                setlinecolor(fc);
                if (m_fill.a >= 255) solidrectangle(x1, y1, x2, y2);
                else fillrectangle(x1, y1, x2, y2);
            }
            if (m_outlineThick > 0 && m_outline.a > 0) {
                setlinecolor(RGB(m_outline.r, m_outline.g, m_outline.b));
                rectangle(x1, y1, x2, y2);
            }
        }
    };

    class CircleShape {
        Vector2f m_pos; float m_radius = 0; Color m_fill{0,0,0,0};
    public:
        CircleShape(float r) : m_radius(r) {}
        void setPosition(Vector2f p) { m_pos = p; }
        void setFillColor(Color c) { m_fill = c; }
        void setOutlineColor(Color) {}
        void setOutlineThickness(float) {}
        void draw() const {
            if (m_fill.a <= 0) return;
            setfillcolor(RGB(m_fill.r, m_fill.g, m_fill.b));
            solidcircle((int)(m_pos.x + m_radius), (int)(m_pos.y + m_radius), (int)m_radius);
        }
    };

    class Texture {
        int m_w = 0, m_h = 0;
    public:
        bool loadFromFile(const std::string&) { return false; }
        void setSmooth(bool) {}
        Vector2u getSize() const { return {(unsigned)m_w, (unsigned)m_h}; }
    };

    class Sprite {
        Texture* m_tex = nullptr;
        Vector2f m_pos, m_scale{1,1};
        Color m_color{255,255,255,255};
    public:
        Sprite() = default;
        Sprite(const Texture& t) : m_tex(const_cast<Texture*>(&t)) {}
        void setTexture(const Texture& t, bool = false) { m_tex = const_cast<Texture*>(&t); }
        void setPosition(Vector2f p) { m_pos = p; }
        void setScale(Vector2f s) { m_scale = s; }
        void setColor(Color c) { m_color = c; }
        void draw() const {}
    };

    class Mouse {
    public:
        enum Button { Left = 0 };
        static bool isButtonPressed(Button) { return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0; }
        static Vector2i getPosition(void*) {
            POINT p; GetCursorPos(&p);
            ScreenToClient(GetHWnd(), &p);
            return {p.x, p.y};
        }
    };

    inline float degrees(float deg) { return deg; }
}

// ========== 兼容g_window->draw(shape)用法 ==========
namespace _easyx_impl {
    struct DummyWindow {
        template<typename T>
        void draw(const T& shape) const { shape.draw(); }
        void close() { g_windowOpen = false; }
        void clear(sf::Color = {}) { cleardevice(); }
        void display() {}
        bool isOpen() const { return g_windowOpen; }
    };
    inline DummyWindow g_windowObj;
    inline DummyWindow* g_window = &g_windowObj;
}

// ========== processWindowEvents ==========
inline bool processWindowEvents() {
    ExMessage msg;
    while (peekmessage(&msg, EM_KEY, false)) {
        if (msg.message == WM_KEYDOWN && msg.vkcode == VK_ESCAPE) {}
    }
    return _easyx_impl::g_windowOpen;
}

// ========== SFML音频兼容类型 ==========
namespace sf {
    enum class SoundChannel { Mono };
    namespace SoundSource { enum Status { Stopped, Paused, Playing }; }

    class SoundBuffer {
        int m_samples = 0, m_rate = 0, m_channels = 0;
    public:
        bool loadFromSamples(const int16_t*, size_t count, int ch, int rate) {
            m_samples = (int)count; m_channels = ch; m_rate = rate; return true;
        }
        int getSampleCount() const { return m_samples; }
        unsigned int getSampleRate() const { return (unsigned)m_rate; }
        unsigned int getChannelCount() const { return (unsigned)m_channels; }
        std::vector<SoundChannel> getChannelMap() const { return {SoundChannel::Mono}; }
    };

    class Sound {
        SoundSource::Status m_st = SoundSource::Stopped;
    public:
        Sound() = default;
        Sound(const SoundBuffer&) {}
        void play() { m_st = SoundSource::Playing; }
        void pause() { m_st = SoundSource::Paused; }
        void stop() { m_st = SoundSource::Stopped; }
        void setPlayingOffset(int64_t) {}
        void setVolume(float) {}
        void setLooping(bool) {}
        SoundSource::Status getStatus() const { return m_st; }
    };

    class InputSoundFile {
    public:
        bool openFromFile(const std::string&) { return true; }
        size_t read(int16_t* d, size_t max) { if (d) memset(d, 0, max * sizeof(int16_t)); return max; }
        int64_t getSampleCount() const { return 0; }
        unsigned int getSampleRate() const { return 44100; }
        unsigned int getChannelCount() const { return 1; }
    };
}

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
            "C:/Windows/Fonts/msyh.ttc",     // Windows 微软雅黑（中文）
            "C:/Windows/Fonts/simhei.ttf",    // Windows 黑体
            "C:/Windows/Fonts/simsun.ttc",    // Windows 宋体
            "C:/Windows/Fonts/consola.ttf",   // Windows Consolas
            "/System/Library/Fonts/STHeiti Medium.ttc",
            "/System/Library/Fonts/Hiragino Sans GB.ttc",
            "/System/Library/Fonts/Menlo.ttc",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        };
        for (const char* path : fontPaths) {
            if (g_font.loadFromFile(path)) {
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
    lineShape.setRotation(angle);
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

    sf::Text text(toSfString(str), g_font, g_textSize);
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

    sf::Text text(toSfString(str), g_font, g_textSize);
    return (int)text.getLocalBounds().width;
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
    sf::Event event;
    while (g_window->pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
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
