# Pixel-Renderer

![](docs/img1.jpg)

A software rasterizer built from scratch in pure C++ with zero graphics-API dependencies.
Starting from a single SetPixel(), the project derives and implements every stage of the GPU pipeline by hand — scan-line rasterization, barycentric coordinates, Z-buffer, MVP transforms, perspective-correct interpolation, and a pluggable IShader system mirroring GLSL vertex/fragment shaders.
The long-term goal is a layered engine architecture: a Command Queue separating rendering frontend from backend, and a self-built immediate-mode UI system (inspired by microui and jserv/libiui) running entirely on the same software framebuffer.
No OpenGL. No DirectX.

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

瀏覽器發現沒有 GPU 時，它會被迫啟動軟體渲染 (Software Rendering)
Google SwiftShader: https://github.com/google/swiftshader
Linux OSMesa: https://mesa3d.org/

## TODO

**TODO List(從 Ch1 完整列出)**

**Part 1 — 像素世界的基礎**
- [x] Ch01：Framebuffer、座標系、SetPixel 原子操作
- [ ] Ch02：DDA 直線演算法
- [ ] I-01：Framebuffer 與 SetPixel 實作

**Part 2 — Bresenham**
- [ ] Ch04：Bresenham 最簡版
- [ ] Ch05：d1/d2 推導
- [ ] Ch06：八個方向
- [ ] Ch07：理論總結
- [ ] I-02：DrawLine 實作

**Part 3 — 三角形光柵化**
- [x] Ch08：2D 叉積
- [ ] Ch09：Edge Function
- [x] Ch10：重心座標
- [ ] Ch11：Bounding Box
- [ ] Ch12：Z-Buffer 理論
- [ ] I-03：DrawTriangle 實作

**Part 4 — 3D 空間數學**
- [ ] Ch13：點積
- [ ] Ch14：3D 叉積
- [ ] Ch15：矩陣與線性變換
- [ ] Ch16：齊次座標
- [ ] Ch17：Model 矩陣

**Part 5 — 投影與相機**
- [ ] Ch18：透視投影推導
- [ ] Ch19：Look At / View 矩陣
- [ ] Ch20：視口變換
- [ ] Ch21：MVP 管線串場

**Part 7 — 可程式化管線(理論先行)**
- [ ] Ch24：OpenGL 管線剖析
- [ ] Ch25：IShader 介面設計
- [ ] Ch26：DrawTriangle 重構 + 透視正確插值

**基礎設施補完(接下來第一步)**
- [ ] 實作 `math_utils.h`(Vec4f、Mat4x4、矩陣運算)
- [ ] `IShader` 抽象類別實作(`vertex()` / `fragment()`、uniforms / varyings 儲存)
- [ ] `RenderDevice` 加入 Z-Buffer(`ClearDepth()`、SetPixel 加深度測試)
- [ ] **Reverse-Z Buffer**(深度範圍改為 [1→0]，near plane 精度更好；矩陣推導 + Clear 改為 0.0f)
- [ ] `Rasterizer::DrawTriangle` 重構為接受 `IShader&`
- [ ] **透視正確插值實作**(`1/w trick`：插值時以 `attr/w` 線性插值再除回來，修正螢幕空間線性插值的錯誤)

**Part 6 — 資源載入**
- [ ] Ch22：TGA 格式解析(自製 `tga_image.h/cpp`)
- [ ] Ch23：OBJ 格式解析(自製 `obj_model.h/cpp`)

**Part 8 — Shader 實作集**
- [ ] Ch27：Flat Shader(驗證整條管線)
- [ ] Ch28：Gouraud Shader(N·L 逐頂點)
- [ ] Ch29：Texture Shader(diffuse 採樣 + 雙線性過濾)
- [ ] Ch30：Phong Shader(逐像素光照)
- [ ] Ch31：Normal Map Shader(TBN 矩陣)

**Part 9 — 進階技術**
- [ ] Ch32：裁剪(Cohen-Sutherland + Sutherland-Hodgman)
- [ ] Ch33：Shadow Mapping(two-pass + PCF)
- [ ] Ch34：Anti-Aliasing(MSAA)

---

**Command Queue — 渲染前後端分離**

*資料結構設計*
- [ ] 定義 `RenderCommand` variant 型別：
  - `ClearCmd { color }`
  - `DrawLineCmd { x1, y1, x2, y2, color }`
  - `DrawTriangleCmd { verts[3], shader_id }`
  - `DrawRectCmd { x, y, w, h, color, alpha }`
  - `SetClipCmd { x, y, w, h }`
  - `SetShaderCmd { shader_id }`
  - `UpdateUniformCmd { key, value }`(傳 MVP 矩陣等全域參數)
- [ ] 每筆 Command 儲存**數據快照**(矩陣、頂點值直接拷入)，不存指標(避免前後端資料競爭)

*CommandQueue 實作*
- [ ] `CommandQueue`：固定大小環形 buffer(避免動態 alloc)
- [ ] `push(cmd)`：前端錄製期間呼叫
- [ ] `execute_all()`：後端消費，依型別 dispatch 到 `Rasterizer` / `RenderDevice`
- [ ] `clear()`：每幀執行完後重置

*雙緩衝(Double Buffered Commands)*
- [ ] 兩個 `CommandQueue`：Queue A(錄製中)、Queue B(執行中)
- [ ] 每幀結束後 swap(前端繼續往 A 寫，後端同時消費 B)
- [ ] swap 時機：`Application::OnRender()` 結束後、`BitBlt` 之前

*渲染前端(Frontend / Producer)*
- [ ] main.cpp 改為純錄製模式：只呼叫 `queue.push()`，不直接碰 `Rasterizer`
- [ ] Frustum Culling(AABB vs Frustum 平面測試，剔除後不產生 DrawTriangleCmd)
- [ ] 排序：Opaque 物件由近到遠(減少 Overdraw)；Transparent 由遠到近(正確 Alpha Blend)
- [ ] Batching：相同 Shader + 相同 Texture 的三角形合併成一筆 `DrawBatchCmd`(減少 SetShaderCmd 切換次數)
- [ ] State Deduplication：連續相同的 `SetShaderCmd` / `SetClipCmd` 自動去除

*渲染後端(Backend / Consumer)*
- [ ] Execute 迴圈：線性掃描 CommandQueue，switch dispatch 各型別
- [ ] 執行期不做任何邏輯決策(只執行，不判斷)
- [ ] 後端統計：每幀 DrawCall 數量、Triangle 數量(debug overlay 用)

*Near Plane Clipping 與三角形拆分(前端負責，後端只執行完整三角形)*
- [ ] 偵測跨越 near plane 的三角形(頂點 w < near 的情況)
- [ ] Sutherland-Hodgman 對 near plane 裁切：一個三角形裁後可能產生 0、1 或 2 個輸出三角形
- [ ] 裁切後重新計算插值屬性(UV、法向量、顏色按比例重算)
- [ ] 裁切結果打包成獨立的 `DrawTriangleCmd` 再進 Queue

*UI Clip Rect 與三角形的交叉問題*
- [ ] UI 視窗有 `ClipRect`；3D DrawTriangleCmd 與 UI DrawRectCmd 共用 Queue 時需處理 `SetClipCmd` 的影響範圍
- [ ] 方案 A(軟體裁切)：前端偵測三角形與 ClipRect 交集，超出部分 Sutherland-Hodgman 裁成子三角形
- [ ] 方案 B(後端邊界測試)：`SetPixel` 時直接判斷是否在 ClipRect 內(UI 三角形少，開銷可接受)
- [ ] 推薦：3D Queue 不走 ClipRect；UI Queue 統一用方案 B

*Queue 分層設計(解決 3D 與 UI 合流問題)*
- [ ] 分為兩條獨立 Queue：`scene_queue`(3D)和 `ui_queue`(UI)
- [ ] 執行順序：先 flush `scene_queue`，再 flush `ui_queue`(UI 永遠疊在最上層)
- [ ] `ui_queue` 執行前插入 `ClearDepthCmd`(UI 不參與 Z-Test，永遠繪製)
- [ ] 未來可擴充第三條 `overlay_queue`(fps counter、debug info)

*未來多執行緒擴充路徑(設計時預留，不急著實作)*
- [ ] 前端跑 Main Thread，後端跑獨立 Render Thread，Queue 作為橋樑
- [ ] Tile-based 並行：把螢幕切成 N×N 的 Tile，多個 Worker Thread 各自光柵化不同區塊
- [ ] Job System 雛形：`DrawBatchCmd` 拆分成多個 `TileJob` 並行執行

---

**自製 UI 系統(參考 microui + libiui 設計)**

*RenderDevice 擴充(UI 原子操作)*
- [ ] `DrawRect(x, y, w, h, color)`
- [ ] `DrawRectAlpha(x, y, w, h, color, alpha)`(Alpha Blending)
- [ ] `SetClipRect(x, y, w, h)` / `ClearClip()`(裁切區域)
- [ ] `DrawSpan(x, y, w, color)`(水平線段快速填充，比逐像素快)

*核心架構*
- [ ] `UiContext`：固定大小 buffer 管理(參考 libiui zero heap allocation)
- [ ] `UiCommandList`：frame 期間收集 `UiDrawRectCmd` / `UiDrawTextCmd` / `UiClipCmd`(參考 microui command list)
- [ ] `iui_begin_frame()` / `iui_end_frame()` 驅動一幀
- [ ] 輸入狀態機(mouse pos / button pressed / released、key events)

*Immediate Mode Widget 實作(參考 microui API 風格)*
- [ ] `iui_button(ctx, label)` → 回傳 `bool`(點擊即回傳，無狀態物件)
- [ ] `iui_checkbox(ctx, label, &value)`
- [ ] `iui_slider(ctx, label, min, max, &value)`
- [ ] `iui_text_input(ctx, &buf, buf_size)`
- [ ] `iui_window_begin(ctx, title, x, y, w, h)` / `iui_window_end(ctx)`

*Layout 系統(參考 libiui flex/grid)*
- [ ] `iui_flex(ctx, cols, ratios[])` — 負值＝比例、正值＝固定 px
- [ ] `iui_flex_next(ctx)` 切換欄
- [ ] `iui_grid_begin(ctx, cols, cell_w, cell_h, gap)` / `iui_grid_end(ctx)`

*字體渲染(參考 libiui built-in vector font，無外部依賴)*
- [ ] 自製 bitmap font(8×8 或 6×8 點陣，燒進 `font_data.h`)
- [ ] `draw_text(ctx, x, y, str, color)` 逐字元查表 → `DrawRect` 逐像素寫入
- [ ] 或升級：參考 libiui 的 built-in vector font(Bézier 輪廓，無需字體檔)

*渲染後端橋接*
- [ ] UI 後端 Execute：讀取 `UiCommandList` → 轉為 `RenderDevice` 呼叫
- [ ] UI CommandList 最終進 `ui_queue`(統一由後端執行)

**Application 層整合**
- [ ] `OnUpdate`：先跑 3D 邏輯，再呼叫 `iui_begin_frame()` 宣告 UI
- [ ] `OnRender`：先 flush `scene_queue`，再 flush `ui_queue`(UI 永遠疊在最上層)
- [ ] 輸入消費優先順序：UI hover/active 時攔截，不傳給 3D 場景