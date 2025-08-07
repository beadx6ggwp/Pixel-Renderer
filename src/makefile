# Makefile for Pixel-Renderer

# 定義變數
CXX = g++.exe
CXXFLAGS = -fdiagnostics-color=always  # 添加更多旗標如 -Wall -Wextra -std=c++11 如果需要

# 新增資料夾修改這邊
INCLUDES = -I. -Itool -Isrc  # 包含根目錄及子目錄的頭檔路徑（根據專案調整）
SRCDIRS = . tool src  # 來源檔目錄：根目錄及子目錄（如 tool/ src/），wildcard 會自動處理不存在的

VPATH = $(SRCDIRS)  # VPATH 告訴 Make 在這些目錄中尋找來源檔
SOURCES = $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.cpp))
LIBS = -lgdi32 -luser32

# Debug 設定
DEBUG_BUILDDIR = build/debug
DEBUG_OBJECTS = $(patsubst %.cpp,$(DEBUG_BUILDDIR)/%.o,$(notdir $(SOURCES)))
DEBUG_TARGET = $(DEBUG_BUILDDIR)/PixelRenderer_debug.exe

# Release 設定
RELEASE_BUILDDIR = build/release
RELEASE_OBJECTS = $(patsubst %.cpp,$(RELEASE_BUILDDIR)/%.o,$(notdir $(SOURCES)))
RELEASE_TARGET = $(RELEASE_BUILDDIR)/PixelRenderer_release.exe

# 預設目標：debug
all: debug

# Debug 模式：建置可執行檔
debug: $(DEBUG_TARGET)

$(DEBUG_TARGET): $(DEBUG_OBJECTS) | $(DEBUG_BUILDDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

$(DEBUG_BUILDDIR)/%.o: %.cpp | $(DEBUG_BUILDDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -g -c $< -o $@

# [輸出]:[來源]
# $< 代表列舉來源，$@ 代表列舉輸出檔案

# Release 模式：建置可執行檔
release: $(RELEASE_TARGET)

$(RELEASE_TARGET): $(RELEASE_OBJECTS) | $(RELEASE_BUILDDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

$(RELEASE_BUILDDIR)/%.o: %.cpp | $(RELEASE_BUILDDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -O2 -c $< -o $@

# 建立輸出目錄
$(DEBUG_BUILDDIR):
	mkdir -p $(DEBUG_BUILDDIR)

$(RELEASE_BUILDDIR):
	mkdir -p $(RELEASE_BUILDDIR)

# 清除輸出檔案
clean:
	rm -rf build

# Phony 目標
.PHONY: all debug release clean