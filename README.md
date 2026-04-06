# Pixel-Renderer

![](docs/img1.jpg)

## Overview
Pixel-Renderer is a basic template for software rendering on Windows using the Win32 API. It provides a modular framework for drawing pixels, lines, and triangles efficiently, ideal for learning the fundamentals of computer graphics pipelines. The project uses Device-Independent Bitmaps (DIB) for high-performance pixel manipulation and supports a simple game loop with input handling.

## Features
- Efficient pixel buffer operations using DIB for software rendering.
- Modular structure: ScreenManager for window and input, RenderDevice for drawing, Application for custom logic.
- Supports keyboard and mouse input.
- Retains console for debugging (e.g., printf output).
- Easy to extend for full graphics pipelines (rasterization, shading, etc.).
- No external dependencies beyond Win32 API (links to gdi32 and user32).

## Getting Started
### Prerequisites
- Windows OS.
- MinGW or MSVC compiler.
- Visual Studio Code (optional, for tasks.json setup).

### Building
1. Clone the repository:
   ```
   git clone https://github.com/yourusername/Pixel-Renderer.git
   ```
2. Manual Build (in Git Bash terminal)
    - Debug: make debug (outputs to build/debug/PixelRenderer_debug.exe)
    - Release: make release (outputs to build/release/PixelRenderer_release.exe)
    - Clean: make clean

3. Open bulid dir to run `PixelRenderer_xxx.exe` to see the demo.

### Usage
Inherit from `Application` and override methods:
```cpp
class MyApp : public Application {
public:
    MyApp() : Application(800, 600, L"My App") {}
    void OnRender() override {
        device->Clear(0x000000);
        device->DrawLine(100, 100, 700, 500, 0x00FF00);
    }
};

int main() {
    MyApp app;
    app.Run();
    return 0;
}
```


## Directory Tree

```
Pixel-Renderer
│   makefile                 # [Stable] 編譯自動化 (UCRT64/MSYS2)
│   types.h                  # [Stable] 通用協議層: 頂點、顏色與畫布配置定義
│   main.cpp                 # [Stable] 引擎組裝點: 繼承 Application 並實作自定義渲染邏輯
│
├─core/                      # [Stable] 基礎設施層 (Infrastructure)
│   ├── screen_manager.h/cpp # Win32 視窗封裝、輸入事件、GDI 畫布初始化
│   ├── render_device.h/cpp  # [Refactored] 純粹畫布管理: 像素寫入與清除
│   └── application.h/cpp    # 高精度時鐘管理、生命週期 (OnInit/Update/Render)
│
├─render/                    # [Active] 渲染邏輯層 (Graphics Logic)
│   ├── rasterizer.h/cpp     # [In Progress] 幾何處理核心: 線段與重心坐標三角形填充
│   ├── shader.h/cpp         # ⏳ 規劃中: 軟體著色器介面 (Vertex/Fragment Shader)
│   ├── texture.h/cpp        # ⏳ 規劃中: 紋理加載與雙線性過濾 (Bilinear Filtering)
│   └── math_utils.h         # ⏳ 規劃中: 3D 數學庫 (Matrix4x4, Quaternions, Projection)
│
├─ui/                        # [Planned] 工具與調試層
│   └── ui_context.h         # ⏳ 預計: 整合 microui/libiui/imgui
│
└─docs/                      # [Planned] 教學文件與筆記
    └── index.html           # 導引地圖: 連結「程式碼」與「現代引擎原理」
```

## TODO
* [ ] Follow the tutorial
* [ ] UI library like [libiui](https://github.com/sysprog21/libiui) / [microui](https://github.com/rxi/microui)