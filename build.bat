@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"

echo ========================================
echo   BeatPixel build script
echo ========================================
echo.

where cl >nul 2>nul
if errorlevel 1 (
  echo MSVC compiler was not found in PATH.
  echo Trying to load Visual Studio 2022 build environment...

  set "VSDEVCMD="
  if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" set "VSDEVCMD=%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
  if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" set "VSDEVCMD=%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
  if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat" set "VSDEVCMD=%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"
  if exist "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" set "VSDEVCMD=%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"

  if not defined VSDEVCMD (
    echo.
    echo Visual Studio 2022 build tools were not found.
    echo Install Visual Studio 2022 with "Desktop development with C++",
    echo or run this script from "x64 Native Tools Command Prompt for VS 2022".
    echo.
    pause
    exit /b 1
  )

  call "!VSDEVCMD!" -arch=x64 -host_arch=x64
)

where cl >nul 2>nul
if errorlevel 1 (
  echo.
  echo cl.exe is still unavailable. Build failed.
  echo.
  pause
  exit /b 1
)

echo.
echo Building BeatPixel.exe...
cl /utf-8 /MT /std:c++17 /O2 /EHsc /Isrc /Isfml/include /DBEATPIXEL_USE_SFML /FeBeatPixel.exe ^
  src/main.cpp ^
  src/GameWindow.cpp ^
  src/NoteTrack.cpp ^
  src/Note.cpp ^
  src/ScoreSystem.cpp ^
  src/AudioManager.cpp ^
  src/BeatParser.cpp ^
  src/DataManager.cpp ^
  src/SongAnalyzer.cpp ^
  src/ChartPackage.cpp ^
  src/AchievementSystem.cpp ^
  /link /LIBPATH:sfml/lib ^
  sfml-graphics.lib sfml-window.lib sfml-system.lib sfml-audio.lib ^
  winmm.lib opengl32.lib gdi32.lib freetype.lib user32.lib

if errorlevel 1 (
  echo.
  echo Build failed. Check the compiler output above.
  echo.
  pause
  exit /b 1
)

echo.
echo ========================================
echo   Build succeeded. Run BeatPixel.exe.
echo ========================================
echo.
pause
