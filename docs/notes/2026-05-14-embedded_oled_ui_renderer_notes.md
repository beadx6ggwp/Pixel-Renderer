# Embedded OLED UI 與 Pixel-Renderer 的整合筆記

> 主題：單晶片 / MCU 上的小型 OLED UI，如何與 software renderer、framebuffer、immediate-mode UI、retained-mode UI、dirty update 等概念連接。

当UI遇上旋钮那才能叫丝滑UI
https://www.bilibili.com/video/BV1P35b6bEuX/
---

## 0. 這類專案的本質

圖中的 UI 屬於 **Embedded UI / MCU UI / OLED menu UI**。它不是一般桌面 GUI，也不是完整 3D graphics pipeline，而是一個極小型的 2D rendering system：

```text
旋鈕 / 按鍵
  ↓
Input scan / debounce
  ↓
Input event system
  ↓
Menu state machine
  ↓
UI layout / widget rendering
  ↓
1-bit framebuffer
  ↓
OLED driver
  ↓
I2C / SPI 傳到螢幕
```

它的核心不是「用什麼 library 畫 UI」，而是理解：

```text
state → draw commands → framebuffer → display flush
```

對 Pixel-Renderer 來說，這是一個很好的 **low-level display backend / UI backend 支線**。

---

## 1. 相關知識關鍵字

### 1.1 顯示器與硬體

常見關鍵字：

```text
SSD1306 OLED
SH1106 OLED
128x64 OLED
Monochrome OLED
I2C OLED
SPI OLED
OLED framebuffer
1-bit display
```

常見顯示晶片：

```text
SSD1306
SH1106
ST7565
ST7735
ILI9341
```

如果是黑白小 OLED，最常見是 **SSD1306 / SH1106**。

---

### 1.2 圖形繪製層

這層負責畫線、矩形、文字、bitmap、icon、反白選單。

```text
Framebuffer
Double buffering
Dirty rectangle
Dirty page
Bitmap font
Glyph rendering
1bpp rendering
Raster graphics
Pixel drawing
Line drawing
Rectangle fill
Icon bitmap
```

128×64 monochrome OLED 的 framebuffer 大小：

```text
128 * 64 / 8 = 1024 bytes
```

也就是整個畫面只要 1 KB，因為每個 pixel 只佔 1 bit。

---

### 1.3 Embedded GUI library

可以搜尋：

```text
U8g2
Adafruit GFX
LVGL
TFT_eSPI
uGUI
microUI
Nuklear embedded
emWin
TouchGFX
Qt for MCU
```

常見定位：

| Library | 用途 |
|---|---|
| U8g2 | Monochrome OLED 常用，適合 SSD1306 / SH1106 |
| Adafruit GFX | Arduino 生態常見 2D graphics API |
| LVGL | 完整 embedded GUI framework，較適合彩色 LCD，也可跑 OLED |
| TFT_eSPI | ESP32 + TFT LCD 常見 |
| TouchGFX | STM32 官方 GUI stack |
| emWin | 工業 / 商業 embedded GUI |

對小 OLED menu 來說，最接近的搜尋詞：

```text
U8g2 menu UI
SSD1306 menu system
Arduino OLED menu
Embedded menu system
```

---

### 1.4 UI 架構

真正關鍵是這些：

```text
Menu system
State machine
Finite State Machine, FSM
Event loop
Input event
Focus navigation
Selection cursor
Widget tree
Immediate mode UI
Retained mode UI
Scene stack
Page stack
Screen manager
Modal dialog
```

典型 menu 結構：

```text
主選單
  ├─ 顯示幀率
  ├─ 保存數據
  ├─ 此設備
  └─ 關於 OLED UI
```

可能的資料模型：

```cpp
struct MenuItem {
    const char* label;
    void (*onClick)();
    MenuItem* children;
};
```

或用 screen/page 來建模：

```text
Screen
  ├─ draw()
  ├─ onEncoder()
  ├─ onButton()
  └─ update()
```

---

### 1.5 輸入裝置

常見是 rotary encoder、button、joystick。

```text
Rotary encoder
EC11 encoder
Quadrature encoder
Encoder debounce
Button debounce
Long press
Short press
Click event
Input polling
Interrupt input
GPIO input
```

常見互動模式：

```text
旋轉：上下移動選單
短按：進入 / 確認
長按：返回 / 退出
```

---

### 1.6 中文字型與字庫

OLED UI 顯示中文時會碰到字庫大小問題。

```text
Bitmap font
Chinese bitmap font
GB2312 font
Unicode glyph
UTF-8 decoding
Font subset
Glyph cache
BDF font
u8g2 chinese font
HZK16
12x12 font
16x16 font
```

英文 ASCII 字元數量少；中文字數量大很多，因此常見做法是：

```text
只內嵌會用到的中文字
使用 font subset
使用外部 Flash 儲存字庫
直接把固定文字轉成 bitmap image
```

---

### 1.7 動畫與效能

```text
OLED animation
Menu transition
Easing function
Frame rate
FPS counter
Partial update
Dirty region rendering
Display refresh rate
I2C bandwidth
SPI bandwidth
```

小 OLED 的效能瓶頸常常不是 CPU，而是 **display bus bandwidth**。

128×64 1bpp 全畫面是 1024 bytes。如果每秒 30 FPS 全畫面更新：

```text
1024 bytes * 30 FPS = 30 KB/s 以上
```

這還沒計入 I2C / SPI protocol overhead 與 command bytes。

---

## 2. 和 Pixel-Renderer 的關係

### 2.1 不要把它當成 3D renderer 主線

Pixel-Renderer 主線：

```text
3D model
  ↓
vertex processing
  ↓
rasterization
  ↓
fragment / shading
  ↓
framebuffer
```

Embedded OLED UI：

```text
input
  ↓
UI state
  ↓
2D drawing
  ↓
1-bit framebuffer
  ↓
OLED flush
```

兩者交集在：

```text
draw command → rasterize pixels → write framebuffer → flush/display
```

所以它很適合作為 Pixel-Renderer 的 **display backend / UI backend 支線**，但不是 3D pipeline 的替代品。

---

### 2.2 最合理的整合方式：Backend

可以把 Pixel-Renderer 設計成多 backend：

```text
Pixel-Renderer
  ├─ DesktopBackend
  │    └─ SDL / Win32 / image output
  │
  ├─ ImageBackend
  │    └─ PNG / PPM / BMP
  │
  └─ EmbeddedOLEDBackend
       └─ SSD1306 / SH1106 / 1-bit framebuffer
```

這樣 Embedded OLED 不會變成獨立無關的小玩具，而是你的 renderer abstraction 的一個具體輸出端。

---

### 2.3 它能訓練的 renderer 能力

這類專案很適合練：

```text
pixel format
stride / pitch
1bpp framebuffer
bit packing
clipping
dirty rectangle
partial update
bitmap font
font atlas / glyph blit
icon rendering
UI event loop
```

尤其是 1bpp framebuffer，會逼你理解：

```text
abstract pixel coordinate
不等於
actual memory layout
```

例如一個 pixel 不是一個 byte，而是一個 bit。

---

### 2.4 它不太能直接訓練的部分

它不太會直接訓練：

```text
perspective projection
clip space
homogeneous coordinates
triangle setup
barycentric interpolation
depth buffer
texture sampling
shading model
```

除非刻意做：

```text
wireframe cube on SSD1306
1-bit triangle rasterizer
tiny depth buffer
tiny software renderer on MCU
```

但這會多出許多 embedded 成本，例如 GPIO、I2C/SPI driver、toolchain、timing bug、memory limit。建議不要一開始就把主線搞太大。

---

## 3. Immediate-mode UI 的適合性

OLED menu 很適合先用 immediate-mode UI 實作。

範例：

```cpp
ui_begin_frame();

if (ui_menu_item("顯示幀率")) {
    show_fps = !show_fps;
}

if (ui_menu_item("保存數據")) {
    save_config();
}

ui_end_frame();
```

底層流程：

```text
UI command
  ↓
draw rect / text / icon
  ↓
rasterize into framebuffer
  ↓
flush to OLED
```

這和 `microui`、`libiui`、Dear ImGui 的核心問題相近：

```text
每一 frame 根據目前 state 重新描述 UI
```

優點：

```text
架構簡單
容易 debug
不需要維護複雜 widget tree
適合小螢幕與小型 menu
```

缺點：

```text
naive 寫法容易每 frame 重畫整張畫面
若 display bus 慢，會浪費傳輸頻寬
動畫時容易卡在 full redraw / full flush
```

---

## 4. Retained-mode UI 與「動靜分離」

### 4.1 Retained-mode 真正提供的東西

Retained-mode UI 的價值不是名稱本身，而是它保留了 UI 結構：

```text
UI tree / scene graph 長期存在
  ↓
可以知道哪些 node 沒變
  ↓
可以 cache static layer
  ↓
只重畫 dynamic node
  ↓
只 flush 變動區域到 OLED / LCD
```

這能優化兩件事：

```text
CPU rendering cost
Display bus transfer cost
```

在 embedded OLED 上，第二個通常更關鍵。

---

### 4.2 Naive immediate-mode 的問題

最直覺的寫法：

```cpp
void draw() {
    clear_screen();

    draw_icon();
    draw_title("設定");
    draw_menu_items();
    draw_scrollbar();
    draw_cursor_animation();

    flush_full_screen();
}
```

問題：

```text
即使只有 cursor 閃爍
也重畫整張畫面
也可能把整個 framebuffer 傳給 OLED
```

---

### 4.3 動靜分離

把畫面分成 static layer 與 dynamic layer。

Static layer：

```text
背景
標題
固定 icon
固定 menu text
固定邊框
```

Dynamic layer：

```text
選取反白框
scroll bar 位置
FPS 數字
loading dots
transition animation
cursor blink
```

結構：

```text
              UI retained tree
                    │
        ┌───────────┴───────────┐
        │                       │
   static nodes             dynamic nodes
        │                       │
        v                       v
 static framebuffer       per-frame drawing
        │                       │
        └───────────┬───────────┘
                    v
              final framebuffer
                    │
                    v
             dirty rect flush
```

---

## 5. Embedded OLED 上的最佳化策略

### 5.1 策略 A：Full static framebuffer cache

保留一份 static framebuffer：

```text
static_fb = 固定內容
final_fb  = 每 frame 實際輸出
```

每一 frame：

```cpp
memcpy(final_fb, static_fb, 1024);
draw_dynamic(final_fb);
flush_dirty_rect(final_fb);
```

優點：

```text
簡單
不需要重畫文字
動畫容易
```

缺點：

```text
多佔一份 framebuffer
每 frame 還是要 memcpy
```

但 1024 bytes 的 memcpy 通常不貴，常常比重新 render font 便宜。

適合：

```text
ESP32
RP2040
STM32
PC simulator
```

---

### 5.2 策略 B：Dirty rectangle

不存完整 static layer，只記錄哪裡髒了。

例如選單從第 1 項移到第 2 項：

```text
old selected item dirty
new selected item dirty
```

流程：

```text
event changes state
  ↓
compute dirty rects
  ↓
redraw only affected widgets
  ↓
flush only affected pages / rects
```

優點：

```text
省 RAM
省傳輸
```

缺點：

```text
程式複雜
每個 widget 要知道自己怎麼局部重畫
容易有殘影 bug
```

---

### 5.3 策略 C：Page-based dirty update

SSD1306 類 OLED 常見 memory layout 是 page-based。

128×64 會分成 8 pages：

```text
page 0: y = 0..7
page 1: y = 8..15
page 2: y = 16..23
page 3: y = 24..31
page 4: y = 32..39
page 5: y = 40..47
page 6: y = 48..55
page 7: y = 56..63
```

所以 dirty rect：

```text
x = 10..40
y = 13..18
```

實際上會跨：

```text
page 1: y = 8..15
page 2: y = 16..23
```

因此 embedded OLED 常用：

```text
dirty pages
dirty page ranges
```

而不是任意 pixel-level rectangle。

範例：

```cpp
uint8_t dirty_pages_mask = 0b00000110; // page 1 and page 2 dirty
```

這點很重要，因為 display backend 要根據硬體 layout 決定真正的 flush granularity。

---

## 6. 反白選單為什麼會讓 dirty update 複雜

反白選單通常不是只畫一個白框，而是：

```text
未選取：黑底 + 白字
選取：白底 + 黑字
```

所以當 highlight 從 item 0 移到 item 1：

```text
item 0:
  白底黑字 → 黑底白字

item 1:
  黑底白字 → 白底黑字
```

必須重畫 old selected item 與 new selected item。

這和遊戲 sprite 移動類似：

```text
清掉舊位置
畫出新位置
```

否則容易產生殘影或錯誤反白。

---

## 7. Immediate-mode 和 Retained-mode 不是絕對二分

Immediate-mode naive 版本：

```cpp
ui_menu_item("顯示幀率");
ui_menu_item("保存數據");
ui_menu_item("此設備");
```

每一 frame 重新跑 UI code。

但 immediate-mode 也可以加入 cache / dirty tracking：

```cpp
if (ui_state_changed()) {
    mark_dirty(item_rect);
}
```

或：

```cpp
UIHash id = hash(label, position);

if (previous_style[id] != current_style[id]) {
    mark_dirty(rect);
}
```

因此更精準的說法是：

```text
Immediate-mode 和 retained-mode 不是絕對二分。
很多高效 immediate UI 內部仍然有 ID、state cache、layout cache、dirty tracking。
```

Dear ImGui 也是表面 immediate API，但內部有大量 state / cache。

---

## 8. UI rendering 可以拆成四層

```text
1. UI semantic state
   selected_index, page, checked, value

2. Layout state
   widget rect, text position, scroll offset

3. Visual cache
   static layer, glyph bitmap, icon bitmap, previous style

4. Display backend
   framebuffer, dirty pages, I2C/SPI flush
```

動靜分離主要發生在第 3 層與第 4 層：

```text
Visual cache:
  什麼可以不用重畫？

Display backend:
  什麼可以不用重傳？
```

兩者要分開看。

有時候可以：

```text
重畫整個 framebuffer，但只 flush dirty pages
```

也可以：

```text
只重畫 dirty widgets，但 flush 整張畫面
```

最佳化目標不同，不要混在一起。

---

## 9. 建議實作路線

### 9.1 階段 1：PC 上做 128×64 1bpp framebuffer simulator

先不要碰 MCU。

目標：

```text
建立 128x64 1bpp framebuffer
支援 setPixel(x, y)
支援 drawLine
支援 drawRect
支援 fillRect
支援 drawBitmap
支援 drawText
輸出成 PNG 或 SDL window
```

這可以先把 graphics 問題解掉，不被硬體 toolchain 分散注意力。

---

### 9.2 階段 2：OLED UI simulator

在 PC 上模擬圖中那種 UI。

```text
128x64 monochrome screen
左側 icon
右側 menu list
目前選項反白
旋鈕上下移動
按下進入子頁
```

輸入可以先用鍵盤：

```text
Keyboard ↑↓ Enter Back
  ↓
InputEvent
  ↓
MenuState
  ↓
Immediate UI draw
  ↓
1bpp framebuffer
  ↓
Desktop preview
```

---

### 9.3 階段 3：Static cache + dirty update

做三種 path 比較：

```text
Path 1: Full redraw
每 frame clear + draw all + full flush

Path 2: Static cache
static framebuffer cache + dynamic redraw + partial flush

Path 3: Retained dirty tree
UI node dependency + dirty propagation + partial redraw
```

可以量：

```text
CPU draw calls
bytes flushed
frame time
FPS
RAM usage
code complexity
```

這會形成一篇很有價值的工程報告，因為不是只說「retained-mode 比較快」，而是實際量化 tradeoff。

---

### 9.4 階段 4：接 SSD1306 / SH1106 backend

最後才接硬體：

```text
same framebuffer
  ↓
SSD1306 flush()
  ↓
I2C / SPI
  ↓
OLED
```

這可以驗證 abstraction 是否乾淨：

```text
同一套 UI code
PC 可以跑
MCU 也可以跑
```

---

## 10. 建議架構

```text
Pixel-Renderer
  ├─ Core
  │    ├─ Vec2 / Vec3 / Mat4
  │    ├─ Color
  │    ├─ Framebuffer<TPixel>
  │    ├─ Rasterizer2D
  │    ├─ Rasterizer3D
  │    └─ BitmapFont
  │
  ├─ UI
  │    ├─ ImmediateModeUI
  │    ├─ Menu
  │    ├─ Layout
  │    └─ InputEvent
  │
  └─ Backends
       ├─ Desktop
       ├─ PNG
       └─ SSD1306
```

設計原則：

```text
Embedded OLED 只是 backend
不要讓硬體限制反過來污染 renderer core
```

---

## 11. 半 retained 的最小實作模型

不用一開始就做完整 retained-mode tree。可以先做「半 retained」：

```cpp
struct ScreenCache {
    Framebuffer1bpp static_fb;
    Framebuffer1bpp final_fb;

    int prev_selected = -1;
    int selected = 0;

    DirtyRegion dirty;
};
```

流程：

```cpp
void enter_settings_screen() {
    static_fb.clear();
    draw_static_settings_ui(static_fb);
    cache.prev_selected = -1;
    cache.dirty.mark_full();
}

void update_settings_screen(Input input) {
    if (input.encoder_delta != 0) {
        cache.prev_selected = cache.selected;
        cache.selected += input.encoder_delta;

        cache.dirty.mark(menu_item_rect(cache.prev_selected));
        cache.dirty.mark(menu_item_rect(cache.selected));
        cache.dirty.mark(scrollbar_rect());
    }
}

void render_settings_screen() {
    copy_dirty_from_static(final_fb, static_fb, cache.dirty);

    draw_menu_item_dynamic(final_fb, cache.prev_selected);
    draw_menu_item_dynamic(final_fb, cache.selected);
    draw_scrollbar(final_fb);

    oled.flush(cache.dirty);
    cache.dirty.clear();
}
```

這不是純 retained-mode，也不是純 immediate-mode。它比較符合 embedded UI 的現實：

```text
小範圍 state tracking
小範圍 dirty update
足夠快
足夠可控
```

---

## 12. 未來可以和 FPGA / GPU golden model 連接的點

這個 OLED UI 支線未來可以變成 renderer 或 FPGA GPU prototype 的 debug panel：

```text
FPS: 32
Mode: Wireframe
Cull: Backface
Depth: On
Shader: Flat
Scene: Cube
```

旋鈕可以切換 rendering mode：

```text
Wireframe / Solid / Depth View / Overdraw View
```

這會讓它不只是 OLED menu demo，而是 renderer tooling 的一部分。

---

## 13. 最精準的搜尋關鍵字清單

可以直接搜尋：

```text
embedded OLED menu UI
microcontroller OLED menu system
SSD1306 rotary encoder menu
U8g2 menu system
Arduino SSD1306 Chinese menu
embedded immediate mode GUI
1-bit framebuffer UI
monochrome OLED GUI
FSM menu system embedded C
SSD1306 framebuffer tutorial
monochrome GUI framebuffer
Chinese font OLED SSD1306
LVGL input device encoder
LVGL monochrome display
```

對 Pixel-Renderer 最有價值的核心關鍵字：

```text
Immediate Mode UI
Retained Mode UI
Framebuffer
1bpp framebuffer
Bitmap Font Rendering
Input Event System
Menu State Machine
Dirty Rectangle
Dirty Page
Partial Update
SSD1306 Driver
```

---

## 14. 總結

這類單晶片 OLED UI 很適合與 Pixel-Renderer 搭配，但定位要正確：

```text
它不是 3D renderer 主線
它是 framebuffer / UI / display backend 支線
```

最有價值的學習點是：

```text
rendering 不只是怎麼畫 pixel
還包括哪些 pixel 不該重畫
哪些 bytes 不該重傳
以及 UI state 如何映射到 framebuffer update
```

這個問題在很多系統裡都會反覆出現：

```text
Embedded OLED UI
Game UI
Browser compositor
Unreal / Unity UI
Tile-based GPU rendering
Display controller partial update
```

建議路線：

```text
PC 版 128x64 1bpp framebuffer simulator
  ↓
OLED menu UI simulator
  ↓
Immediate-mode UI
  ↓
static cache + dirty update
  ↓
SSD1306 / SH1106 hardware backend
```

這樣它會服務 Pixel-Renderer，而不是變成另一條分散注意力的電子小專案。
