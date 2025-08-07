# 通用變數
CXX = C:/MinGW/mingw64/bin/g++.exe
# include 路徑 + UNICODE；-I. 包含當前，若有子資料夾如 src/ 可加 -Isrc -Wextra
CXXFLAGS_COMMON = -fdiagnostics-color=always -Wall -I. 
SOURCES = $(shell find . -name "*.cpp")  # 動態抓所有 cpp (包含子資料夾)
LIBS = -lgdi32 -luser32

# Debug 模式
DEBUG_DIR = build/debug
DEBUG_OBJS = $(addprefix $(DEBUG_DIR)/, $(notdir $(SOURCES:.cpp=.o)))
debug: CXXFLAGS = $(CXXFLAGS_COMMON) -g -O0
debug: $(DEBUG_DIR)/PixelRenderer_debug.exe

$(DEBUG_DIR)/PixelRenderer_debug.exe: $(DEBUG_OBJS)
	$(CXX) $^ -o $@ $(LIBS)

$(DEBUG_DIR)/%.o: %.cpp | $(DEBUG_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(DEBUG_DIR):
	mkdir -p $(DEBUG_DIR)

# Release 模式
RELEASE_DIR = build/release
RELEASE_OBJS = $(addprefix $(RELEASE_DIR)/, $(notdir $(SOURCES:.cpp=.o)))
release: CXXFLAGS = $(CXXFLAGS_COMMON) -O2
release: $(RELEASE_DIR)/PixelRenderer_release.exe

$(RELEASE_DIR)/PixelRenderer_release.exe: $(RELEASE_OBJS)
	$(CXX) $^ -o $@ $(LIBS)

$(RELEASE_DIR)/%.o: %.cpp | $(RELEASE_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(RELEASE_DIR):
	mkdir -p $(RELEASE_DIR)

# 預設 + clean
all: debug
clean:
	rm -rf build
	rm -f PixelRenderer_debug.exe PixelRenderer_release.exe