# 《像素节拍》Makefile - macOS/Linux (SFML)
CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -I/opt/homebrew/include

# 检测操作系统，适配不同的链接选项
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Darwin)
    # macOS需要链接OpenAL框架（SFML Audio后端）
    SFML_FLAGS = -L/opt/homebrew/lib -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -framework OpenAL
else
    # Linux
    SFML_FLAGS = -L/opt/homebrew/lib -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
endif

SRC_DIR = src
BUILD_DIR = build
TARGET = BeatPixel

SRCS = $(SRC_DIR)/main.cpp \
       $(SRC_DIR)/GameWindow.cpp \
       $(SRC_DIR)/NoteTrack.cpp \
       $(SRC_DIR)/Note.cpp \
       $(SRC_DIR)/ScoreSystem.cpp \
       $(SRC_DIR)/AudioManager.cpp

OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(SFML_FLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
