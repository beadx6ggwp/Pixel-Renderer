# Makefile for Pixel-Renderer

# 定義變數
CXX = C:/MinGW/mingw64/bin/g++.exe
CXXFLAGS = -fdiagnostics-color=always
SOURCES = *.cpp # 主目錄下的 cpp 檔案
# 如果未來新增 tool 資料夾或其他子資料夾，添加如：tool/*.cpp src/*.cpp
# 示例：SOURCES = *.cpp tool/*.cpp src/*.cpp
LIBS = -lgdi32 -luser32

DEBUG_OUTPUT_DIR = build/debug
DEBUG_OUTPUT = $(DEBUG_OUTPUT_DIR)/PixelRenderer_debug.exe

RELEASE_OUTPUT_DIR = build/release
RELEASE_OUTPUT = $(RELEASE_OUTPUT_DIR)/PixelRenderer_release.exe

# 預設目標：debug
all: debug

# Debug 模式：帶除錯符號，無優化
debug:
	mkdir -p $(DEBUG_OUTPUT_DIR)
	$(CXX) $(CXXFLAGS) -g $(SOURCES) -o $(DEBUG_OUTPUT) $(LIBS)

# Release 模式：帶優化，無除錯符號
release:
	mkdir -p $(RELEASE_OUTPUT_DIR)
	$(CXX) $(CXXFLAGS) -O2 $(SOURCES) -o $(RELEASE_OUTPUT) $(LIBS)

# 清除輸出檔案
clean:
	rm -rf build