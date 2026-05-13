cmake_minimum_required(VERSION 3.20)
project(BeatPixel VERSION 3.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 静态 CRT（/MT），和 CI 编译一致
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# 捆绑的 SFML 路径
set(SFML_ROOT "${CMAKE_SOURCE_DIR}/sfml")

add_executable(BeatPixel
    src/main.cpp
    src/GameWindow.cpp
    src/NoteTrack.cpp
    src/Note.cpp
    src/ScoreSystem.cpp
    src/AudioManager.cpp
    src/BeatParser.cpp
    src/DataManager.cpp
    src/SongAnalyzer.cpp
    src/ChartPackage.cpp
    src/AchievementSystem.cpp
)

target_include_directories(BeatPixel PRIVATE src "${SFML_ROOT}/include")
target_compile_definitions(BeatPixel PRIVATE BEATPIXEL_USE_SFML NOMINMAX)
target_link_directories(BeatPixel PRIVATE "${SFML_ROOT}/lib")
target_link_libraries(BeatPixel PRIVATE
    sfml-graphics sfml-window sfml-system sfml-audio
    winmm opengl32 gdi32 freetype user32
)

# 从项目根目录运行，这样能找到 DLL、songs、assets
set_target_properties(BeatPixel PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
