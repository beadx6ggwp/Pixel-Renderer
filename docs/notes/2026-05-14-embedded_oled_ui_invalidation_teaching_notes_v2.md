# Embedded OLED UI × Pixel-Renderer 教學紀錄

> 主題：從單晶片 OLED UI 出發，連結 Pixel-Renderer、Immediate/Retained UI、Dirty Rectangle、Cocos Scene Graph、React、Browser Compositor，以及更廣義的 **Invalidation-based incremental rendering**。

---

## 0. 這份筆記的主線

本次對話從一個單晶片 OLED UI 畫面開始，逐步延伸到一個更大的 rendering system 概念：

```text
狀態改變後，不要全部重算。
只讓「被影響的部分」失效，然後重算 / 重畫 / 重傳必要的最小集合。
```

這個大概念可以稱為：

```text
Invalidation-based incremental rendering
基於失效標記的增量渲染
```

更廣義則是：

```text
Incremental computation with dependency tracking
帶依賴追蹤的增量計算
```

它不只存在於 OLED UI，也存在於：

```text
Cocos sprite transform
React reconciliation
Browser layout / paint / compositor
Game engine scene graph
Render graph / frame graph
Build system
Compiler incremental compilation
Database materialized view
```

---

# 1. 單晶片 OLED UI 的知識關鍵字

## 1.1 顯示硬體

圖中的 UI 類型很像小尺寸 OLED 顯示器，常見關鍵字：

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

常見 display controller：

```text
SSD1306
SH1106
ST7565
ST7735
ILI9341
```

黑白小 OLED 最常見是：

```text
SSD1306 / SH1106
```

---

## 1.2 圖形繪製層

這層負責畫 pixel、線、矩形、文字、icon、選單反白。

```text
Framebuffer
Double buffering
Dirty rectangle
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

這代表整個畫面只需要 1 KB，因為一個 pixel 只佔 1 bit。

---

## 1.3 Embedded GUI / OLED UI Library

常見 library：

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

大致定位：

| Library | 主要用途 |
|---|---|
| U8g2 | monochrome OLED / LCD，常用於 SSD1306/SH1106 |
| Adafruit GFX | Arduino 生態常見的 2D graphics API |
| LVGL | 較完整 embedded GUI framework |
| TFT_eSPI | ESP32 + TFT LCD 常見 |
| TouchGFX | STM32 官方 GUI stack |
| emWin | 商業 / 工業 embedded GUI |

圖中這種 UI 最接近：

```text
U8g2 menu UI
SSD1306 menu system
Arduino OLED menu
Embedded menu system
```

---

## 1.4 UI 架構關鍵字

這是更值得深入的部分：

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

典型資料流：

```text
旋鈕 / 按鍵
  ↓
Input scan / debounce
  ↓
Event system
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

---

## 1.5 輸入裝置

圖中的操作通常來自 rotary encoder / joystick / button：

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

常見互動：

```text
旋轉：上下移動選單
短按：進入 / 確認
長按：返回 / 退出
```

---

## 1.6 中文字型與文字顯示

圖中有中文，因此中文字型是重要問題。

關鍵字：

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

中文字庫麻煩的原因：

```text
ASCII:
  約 96 個可見字元

中文:
  常用字可能數千個 glyph
```

常見策略：

```text
只內嵌會用到的中文字
使用 font subset
使用外部 Flash 儲存字庫
把固定文字轉成 bitmap image
```

---

# 2. 這種單晶片 UI 適不適合搭配 Pixel-Renderer？

結論：

```text
適合，但它不是 3D renderer 主線。
它適合作為 Pixel-Renderer 的 embedded display / UI backend 支線。
```

你的 Pixel-Renderer 主線：

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

OLED UI 主線：

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

兩者交集：

```text
draw command
  ↓
rasterize pixels
  ↓
write framebuffer
  ↓
flush / display
```

---

## 2.1 最合理的定位：Backend

可以把 OLED 當成一個 backend：

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

這會讓你練：

```text
同一套 rendering abstraction
如何輸出到不同 display target
```

這和 graphics API / engine backend 的設計思路接近。

---

## 2.2 很適合拿來練 Immediate-mode UI

OLED UI 很適合作為 immediate-mode UI 練習場：

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

底層變成：

```text
UI command
  ↓
draw rect / text / icon
  ↓
rasterize into framebuffer
  ↓
flush to OLED
```

這和 microui、Nuklear、Dear ImGui 的底層思路有關。

---

## 2.3 它會逼你理解 memory layout

OLED UI 很適合練：

```text
pixel format
stride
pitch
1bpp framebuffer
bit packing
clipping
dirty rectangle
partial update
font atlas / bitmap font
glyph blit
```

重點：

```text
abstract pixel coordinate
不等於
actual memory layout
```

例如 1bpp framebuffer 中，一個 pixel 不是一個 byte，而是一個 bit。

---

## 2.4 不適合直接訓練 3D pipeline 的部分

OLED UI 不太直接訓練：

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

但不建議一開始就做太大，因為 MCU 會引入很多和 graphics 無關的成本：

```text
I2C / SPI driver
MCU toolchain
GPIO
debounce
flash storage
cross compilation
memory limit
timing bug
```

---

## 2.5 建議專案切入方式

合理順序：

```text
先在 PC 寫 128x64 1bpp OLED simulator
→ 做 immediate-mode menu UI
→ 做 bitmap font rendering
→ 抽象 framebuffer backend
→ 最後 port 到 SSD1306 / MCU
```

推薦架構：

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

---

# 3. 動靜分離：Retained-mode UI 的價值

你提出的想法：

> 如果把維持式 UI 的動靜分離概念用上，是不是能優化動畫？

結論：

```text
是。
但真正有價值的不是 retained-mode 這個名詞本身，
而是它讓系統知道哪些東西沒變、哪些東西變了。
```

---

## 3.1 Naive immediate-mode 問題

假設每一 frame 都這樣：

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
即使只有 cursor 閃爍，
你還是重畫整張畫面，
也可能把整個 framebuffer 傳給 OLED。
```

128×64 1bpp framebuffer：

```text
128 * 64 / 8 = 1024 bytes
```

如果 30 FPS full flush：

```text
1024 bytes * 30 FPS = 30 KB/s 以上
```

I2C 會有 protocol overhead，因此實際成本更高。

---

## 3.2 動靜分離的核心

把 UI 分成：

```text
Static layer:
  - 背景
  - 標題
  - 固定 icon
  - 固定 menu text
  - 固定邊框

Dynamic layer:
  - 選取反白框
  - scrollbar 位置
  - FPS 數字
  - loading dots
  - transition animation
  - cursor blink
```

流程：

```text
第一次進入畫面：
  render static layer → cache

每一 frame：
  copy static layer
  render dynamic layer
  flush dirty region
```

ASCII：

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

## 3.3 MCU 上的限制：RAM

如果做多個 layer：

```text
static layer: 1024 bytes
dynamic layer: 1024 bytes
final buffer: 1024 bytes
```

總共 3 KB。

對 ESP32 / RP2040 / STM32F4 以上通常還好；  
對 AVR / Arduino UNO 可能很痛。

所以要根據平台選策略。

---

## 3.4 策略 A：Full static framebuffer cache

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

---

## 3.5 策略 B：Dirty rectangle

不存完整 static layer，只記錄哪裡髒了。

例如選取從第 1 項移到第 2 項：

```text
Before:
> 顯示幀率
  保存數據

After:
  顯示幀率
> 保存數據
```

dirty region：

```text
old selected item rect
new selected item rect
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

## 3.6 策略 C：Page-based dirty update

SSD1306 常見 memory layout 是 page-based：

```text
128 x 64
分成 8 pages
每 page 高 8 pixels
每 page 128 bytes
```

示意：

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

因此 dirty rect 可能要轉成 dirty pages：

```cpp
uint8_t dirty_pages_mask = 0b00000110; // page 1 and page 2 dirty
```

這很重要，因為它直接說明：

```text
abstract 2D coordinate
不等於
hardware framebuffer layout
```

---

## 3.7 反白文字讓 dirty rect 更複雜

選單反白通常是：

```text
未選取：
黑底 + 白字

選取：
白底 + 黑字
```

所以 highlight 移動時，髒的不是只有背景矩形，還有文字顏色。

需要重畫：

```text
old selected item:
  白底黑字 → 黑底白字

new selected item:
  黑底白字 → 白底黑字
```

這就是為什麼 dirty rect 通常要包含 old item 和 new item。

---

# 4. Immediate-mode 和 Retained-mode 不是絕對二分

Immediate-mode naive：

```cpp
ui_menu_item("顯示幀率");
ui_menu_item("保存數據");
ui_menu_item("此設備");
```

每 frame 都重新跑一次 UI code。

但 immediate-mode 也可以加入 cache：

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

這其實是在 immediate-mode 裡引入 retained/cache 概念。

重點：

```text
Immediate-mode 和 retained-mode 不是絕對二分。
很多高效 immediate UI 內部仍然有 cache、ID、dirty tracking。
```

Dear ImGui 表面是 immediate API，但內部仍然有：

```text
ID
state storage
layout cache
draw list
clipping
navigation state
```

所以真正要問的是：

```text
誰持有 state？
誰追蹤 dependency？
誰決定 dirty region？
誰負責 cache？
```

---

# 5. Dirty rectangle 和 Cocos Sprite 座標計算的關係

你提出的想法：

> Dirty rectangle 髒標記是不是跟 Cocos 在處理 sprite 座標的計算也有類似概念？

結論：

```text
有類似，但不是同一層問題。
```

可以分成三類：

```text
A. Dirty transform / dirty flag
   代表：這個物件的狀態變了，需要重新計算某些資料

B. Dirty rectangle / region invalidation
   代表：畫面上的某個區域變了，需要重新繪製或重新傳輸

C. Renderer batching dirty
   代表：vertex data / command buffer / batch 內容變了，需要重新提交
```

Cocos sprite 座標計算比較接近：

```text
A. dirty transform
```

OLED dirty rectangle 比較接近：

```text
B. dirty region
```

---

## 5.1 Cocos / Scene graph 的 transform dirty

Scene graph：

```text
Root
 └─ Player
     └─ Weapon
         └─ Effect
```

每個 node 有 local transform：

```text
position
rotation
scale
anchor
```

render 時需要 world transform：

```text
world_transform = parent_world_transform * local_transform
```

Naive 做法：每 frame 對所有 node 重算 world transform。  
Dirty flag 做法：只有 local transform 變動的 node 及其 children 需要更新。

範例：

```cpp
node->setPosition(x, y);
node->transformDirty = true;
```

render / visit 時：

```cpp
if (node->transformDirty || parentTransformDirty) {
    node->worldTransform = parent.worldTransform * node.localTransform;
    node->transformDirty = false;
}
```

這就是 dirty flag。

---

## 5.2 OLED dirty rectangle 是畫面區域失效

選單從第 1 項移到第 2 項：

```text
Before:
> 顯示幀率
  保存數據

After:
  顯示幀率
> 保存數據
```

髒掉的是：

```text
old selected item rect
new selected item rect
```

程式上：

```cpp
dirty.mark(menu_item_rect(old_index));
dirty.mark(menu_item_rect(new_index));
```

---

## 5.3 共同結構

共同點：

```text
狀態變化
  ↓
標記受影響的東西
  ↓
延後到 render/update 階段才處理
  ↓
只處理必要部分
```

抽象成：

```text
Change
  ↓
Invalidate
  ↓
Recompute / Redraw / Reupload
```

但 dirty 對象不同：

| 系統 | dirty 的對象 | 目標 |
|---|---|---|
| Cocos transform dirty | node 的 transform / world matrix | 避免重算座標 |
| Cocos vertex/batch dirty | sprite quad / vertex buffer | 避免重建 render data |
| OLED dirty rectangle | framebuffer 的局部區域 | 避免重畫、避免重傳 |
| Browser paint dirty | DOM/CSS 對應的 paint region | 避免 repaint 整頁 |
| GPU resource dirty | buffer / texture / descriptor | 避免不必要 upload |

精準說法：

```text
Dirty rectangle 和 Cocos sprite 座標計算都屬於 invalidation-based optimization，
但 dirty 的對象不同。
Cocos 常見的是 transform invalidation，
OLED UI 是 pixel region invalidation。
```

---

# 6. 動靜分離和 React / Browser 的關係

你提出的想法：

> 這個動靜分離是不是就是現代瀏覽器跟 React 等框架在解決的問題？

結論：

```text
是，但 React 和 Browser 解決的是不同層。
```

拆成兩個系統：

```text
React:
  解決 UI state → UI tree / DOM mutation 的問題

Browser:
  解決 DOM/CSS → layout → paint → composite → pixels 的問題
```

React 不直接處理 pixel-level dirty rectangle。  
真正處理 repaint、compositing、layer cache 的是 browser engine。

---

## 6.1 React 解的是語義樹變動

React 的問題：

```text
state 變了
  ↓
哪些 component output 變了？
  ↓
哪些 DOM node 需要更新？
```

例如：

```jsx
function Menu({ selected }) {
  return (
    <ul>
      <li className={selected === 0 ? "active" : ""}>顯示幀率</li>
      <li className={selected === 1 ? "active" : ""}>保存數據</li>
    </ul>
  );
}
```

當 `selected` 從 0 變 1：

```text
Item[0] class: active → normal
Item[1] class: normal → active
```

React 的輸出是：

```text
DOM mutation
```

不是：

```text
pixel update
```

因此 React 解的是比較上層的 invalidation。

關鍵字：

```text
Reconciliation
Virtual DOM
Fiber
Render phase
Commit phase
Memoization
useMemo
useCallback
React.memo
Keyed diff
State update batching
```

---

## 6.2 Browser 解的是畫面輸出的動靜分離

Browser pipeline：

```text
DOM
  ↓
Style calculation
  ↓
Layout
  ↓
Paint
  ↓
Raster
  ↓
Composite
  ↓
Screen
```

更接近 renderer / OLED UI 的是：

```text
Paint invalidation
Layerization
Raster cache
Compositor
```

如果頁面有：

```text
背景
文字
圖片
按鈕
一個正在滑動的 panel
```

Naive 做法：

```text
每一 frame 重新 layout
每一 frame 重新 paint
每一 frame 重新 raster 整頁
```

現代 browser 會盡量：

```text
靜態內容保留在已 rasterized 的 layer
動態物件放在 compositor layer
動畫時只改 transform / opacity
GPU compositor 重新合成
```

ASCII：

```text
Static page content  ── raster once ──┐
                                     ├─ compositor ── screen
Moving panel layer  ── transform  ───┘
```

---

## 6.3 為什麼 CSS transform / opacity 動畫通常比較順？

例如：

```css
.panel {
  transform: translateX(100px);
}
```

很多情況下可以只走 compositor：

```text
不用重新 layout
不用重新 paint
只改 layer transform
```

但如果動畫的是：

```css
width
height
top
left
font-size
```

就可能牽涉：

```text
layout change
  ↓
paint invalidation
  ↓
raster
  ↓
composite
```

這和 OLED UI 類似：

```text
只移動 highlight layer
  → 可以局部 redraw / partial flush

改變整個 menu layout
  → 可能整頁重畫
```

---

## 6.4 React、Browser、OLED UI 對照

Web 世界：

```text
React component tree
        │
        v
Virtual DOM diff / reconciliation
        │
        v
DOM mutation
        │
        v
Browser style/layout/paint/composite
        │
        v
Pixels on screen
```

OLED UI：

```text
UI state
  │
  v
Menu tree / widget tree
  │
  v
dirty widget / dirty rect
  │
  v
draw into framebuffer
  │
  v
flush to OLED
```

對照表：

| Web 世界 | Embedded OLED 世界 |
|---|---|
| React state | UI state |
| Component tree | Menu / Widget tree |
| Virtual DOM diff | Widget state diff |
| DOM mutation | Update UI node / style |
| Browser paint invalidation | Dirty rectangle |
| Compositor layer | Static/dynamic framebuffer layer |
| GPU composite | framebuffer merge / OLED flush |

---

# 7. 大概念：Invalidation-based Incremental Rendering

這是本次對話最重要的核心。

## 7.1 基本模型

```text
State change
  ↓
Invalidate affected data
  ↓
Propagate dirty flags
  ↓
Recompute only dirty parts
  ↓
Update cache / output
```

工程化一點：

```text
資料 A 改變
  ↓
依賴 A 的 B 失效
  ↓
依賴 B 的 C 失效
  ↓
重新計算 B、C
  ↓
未受影響的 D、E 保持 cache
```

同一個大概念會出現在：

```text
Cocos sprite transform dirty
React reconciliation
Browser paint invalidation
OLED dirty rectangle
Game engine render graph
Makefile incremental build
Compiler incremental compilation
Database materialized view update
```

---

## 7.2 關鍵字總表

核心關鍵字：

```text
Dirty flag
Dirty rectangle
Damage region
Invalidation
Cache invalidation
Incremental rendering
Incremental computation
Dependency tracking
Change propagation
Partial redraw
Partial update
Retained mode UI
Scene graph
Render graph
Frame graph
Compositor
Layer caching
Memoization
```

UI / graphics 關鍵字：

```text
Layout invalidation
Paint invalidation
Raster invalidation
Transform dirty
Bounding box dirty
Region tracking
Display list
Retained display list
Compositor layer
Layer promotion
Partial present
Tile-based rendering
Occlusion culling
```

Game engine 關鍵字：

```text
Scene graph dirty flag
World transform cache
Batch invalidation
Vertex buffer dirty
Material dirty
Render state dirty
ECS change detection
Transform hierarchy update
Render queue rebuild
```

Embedded / MCU UI 關鍵字：

```text
Dirty rect
Dirty page
Dirty page mask
Framebuffer cache
Static layer
Dynamic layer
Partial flush
SSD1306 page addressing
1bpp framebuffer
Damage tracking
```

---

# 8. Dirty flag：資料失效

Dirty flag 解決的是：

```text
我是否需要重新計算這份 cached data？
```

Scene graph 例子：

```text
local transform 改了
  ↓
world transform 失效
  ↓
children world transform 也失效
```

範例資料結構：

```cpp
struct Node {
    Mat3 local;
    Mat3 world;

    bool local_dirty;
    bool world_dirty;

    Node* parent;
    std::vector<Node*> children;
};
```

改 position 時：

```cpp
void setPosition(Node& n, Vec2 p) {
    n.local = makeTranslate(p);
    markWorldDirty(n);
}
```

dirty propagation：

```cpp
void markWorldDirty(Node& n) {
    if (n.world_dirty) return;

    n.world_dirty = true;

    for (Node* child : n.children) {
        markWorldDirty(*child);
    }
}
```

重新計算：

```cpp
Mat3 getWorldTransform(Node& n) {
    if (n.world_dirty) {
        if (n.parent) {
            n.world = getWorldTransform(*n.parent) * n.local;
        } else {
            n.world = n.local;
        }

        n.world_dirty = false;
    }

    return n.world;
}
```

---

# 9. Dirty rectangle / Damage region：畫面區域失效

Dirty rectangle 處理的是畫面區域。

例如 OLED menu：

```text
Before:
> 顯示幀率
  保存數據

After:
  顯示幀率
> 保存數據
```

髒掉：

```text
old selected item rect
new selected item rect
```

可以記成：

```text
dirty flag:
  標記資料失效

dirty rectangle:
  標記畫面區域失效
```

其他名稱：

```text
damage region
invalid region
repaint region
update region
```

Browser 常用 `damage`。  
Embedded GUI 常用 `dirty rect`。  
Window system 常用 `invalid region`。

---

# 10. Dependency tracking：誰依賴誰？

系統變大後，不能只靠手動 dirty flag。必須知道：

```text
A 變了，誰依賴 A？
B 變了，誰要跟著失效？
哪些東西可以不動？
```

這變成 dependency graph 問題。

UI 例子：

```text
selected_index
  ↓
MenuItem[0].style
MenuItem[1].style
Scrollbar.thumb_position
  ↓
Dirty rectangles
  ↓
Framebuffer
  ↓
OLED flush
```

圖：

```text
selected_index
   ├── item0 visual state
   │       └── item0 dirty rect
   │
   ├── item1 visual state
   │       └── item1 dirty rect
   │
   └── scrollbar position
           └── scrollbar dirty rect
```

Build system 例子：

```text
foo.cpp
  ↓
foo.o
  ↓
app.exe
```

`foo.cpp` 改了，只重新編譯 `foo.o`，再重新 link，不需要整個專案全部重做。

---

# 11. Browser pipeline 中的 invalidation

Browser pipeline：

```text
DOM
  ↓
Style calculation
  ↓
Layout
  ↓
Paint
  ↓
Raster
  ↓
Composite
  ↓
Display
```

每一層都有 invalidation 問題。

例如改 `color`：

```text
style invalidation
  ↓
paint invalidation
  ↓
raster
  ↓
composite
```

改 `width`：

```text
style invalidation
  ↓
layout invalidation
  ↓
paint invalidation
  ↓
raster
  ↓
composite
```

改 `transform`：

```text
可能只需要 compositor update
不用 layout
不用 repaint
```

常見理解表：

| 改動 | 常見成本 |
|---|---|
| `transform` | composite |
| `opacity` | composite |
| `color` | paint + composite |
| `background` | paint + composite |
| `width` / `height` | layout + paint + composite |
| `top` / `left` | layout 或 paint，視情況 |
| `font-size` | layout + paint + composite |

這解釋了為什麼網頁動畫常建議用：

```css
transform: translate(...);
opacity: ...;
```

而不是一直改：

```css
top: ...;
left: ...;
width: ...;
height: ...;
```

---

# 12. Compositor layer：動靜分離的高階版本

Browser / game engine / UI framework 都會想把畫面拆 layer：

```text
Static background layer
Moving panel layer
Cursor layer
Popup layer
```

如果只有 panel 在動：

```text
background layer 不重畫
panel layer 不重畫內容，只改 transform
compositor 重新合成
```

示意：

```text
Background layer  ── cached texture ──┐
Text layer        ── cached texture ──┤
Moving panel      ── transform only ──┤── compositor ── screen
Cursor layer      ── small update  ───┘
```

OLED 對應：

```text
static framebuffer
dynamic drawing
dirty page flush
```

Browser 對應：

```text
rasterized layer texture
GPU compositor
damage region
swapchain present
```

Game engine 對應：

```text
static geometry batching
dynamic object transform
render queue
GPU command buffer
```

---

# 13. Game engine 裡的多種 dirty

## 13.1 Transform dirty

```text
local transform dirty
  ↓
world transform dirty
  ↓
bounding box dirty
  ↓
render proxy dirty
```

---

## 13.2 Bounding volume dirty

物件移動後，AABB / bounding sphere 也要重算：

```text
transform changed
  ↓
world-space bounding box invalid
  ↓
culling structure needs update
```

影響：

```text
frustum culling
occlusion culling
spatial partition
BVH / quadtree / octree
```

---

## 13.3 Render batch dirty

Sprite 的 texture、material、z-order、blend mode 變了，可能導致 batch 重建：

```text
sprite material changed
  ↓
batch key changed
  ↓
render queue dirty
  ↓
vertex buffer / command list update
```

---

## 13.4 GPU resource dirty

CPU 上資料變了，不代表 GPU 已經知道。

```text
CPU-side uniform changed
  ↓
UBO dirty
  ↓
upload to GPU
  ↓
bind descriptor
```

關鍵字：

```text
Uniform buffer dirty
Descriptor set dirty
Material parameter dirty
Constant buffer update
Resource state tracking
Command buffer rebuild
```

---

# 14. Render graph / Frame graph

現代引擎常用 render graph / frame graph 管一幀裡的 render pass 依賴。

例子：

```text
GBuffer Pass
  ↓
Lighting Pass
  ↓
Postprocess Pass
  ↓
UI Pass
  ↓
Present
```

每個 pass 讀寫不同 texture：

```text
GBuffer Pass:
  write normal texture
  write albedo texture
  write depth texture

Lighting Pass:
  read normal texture
  read albedo texture
  read depth texture
  write lighting texture

Postprocess Pass:
  read lighting texture
  write final texture
```

Render graph 解的問題：

```text
哪些 pass 依賴哪些 resource？
resource lifetime 怎麼安排？
哪些 texture 可以重用？
哪些 barrier 要插？
哪些 pass 可以省略？
```

它和 dirty rectangle 不同，但都在處理：

```text
dependency + invalidation + minimal work
```

---

# 15. Embedded UI 的 dirty granularity

## 15.1 Full screen dirty

```text
任何改變 → 重畫整張 → flush 整張
```

優點：

```text
簡單
不容易殘影
debug 容易
```

缺點：

```text
浪費 CPU
浪費 I2C/SPI bandwidth
動畫可能卡
```

---

## 15.2 Rect dirty

標記矩形區域：

```text
dirty rect = x, y, w, h
```

適合一般 framebuffer。

---

## 15.3 Region dirty

多個 rect 合併：

```text
dirty region = rect0 + rect1 + rect2
```

但 rect 合併也有成本。

例如兩個小 rect 很遠：

```text
[rect A]                [rect B]
```

策略選擇：

```text
分別 flush 兩塊：
  command overhead 多，但傳輸少

合併成大 rect：
  command overhead 少，但傳輸多
```

---

## 15.4 Page dirty

SSD1306 這種 OLED 常用 page memory layout：

```text
page 0: y = 0..7
page 1: y = 8..15
page 2: y = 16..23
...
page 7: y = 56..63
```

dirty rect 最後可能要轉成：

```text
dirty pages
dirty page ranges
```

這比較符合硬體。

---

# 16. Dirty tracking 的 tradeoff

Dirty tracking 不是越細越好，因為 tracking 本身也有成本：

```text
要記錄 dirty 狀態
要傳播 dirty
要合併 dirty region
要避免漏標
要避免重複標
要處理 cache consistency
```

工程上要問：

```text
重算成本大，還是追蹤成本大？
```

策略表：

| 情境 | 適合策略 |
|---|---|
| 128x64 OLED，簡單選單 | full redraw 或 page dirty |
| 128x64 OLED，有動畫 | static cache + dirty page |
| 320x240 TFT，SPI 慢 | dirty rect / partial flush |
| PC UI | retained tree + paint invalidation + compositor |
| Game engine | transform dirty + culling + batching |
| Browser | style/layout/paint/composite invalidation |
| React app | component diff + memoization |

---

# 17. 最容易出 bug 的地方：漏標 dirty

最常見 bug：

```text
資料變了，但是忘記標 dirty
```

結果：

```text
畫面沒更新
座標錯
cache stale
殘影
點擊區域錯
hit test 錯
```

例如：

```cpp
node.position = new_pos;
// 忘了 markWorldDirty(node)
```

或：

```cpp
selected_index++;
// 忘了 mark old item dirty
```

解法：不要讓外部直接改會影響 cache 的欄位，而是集中到 setter / command / transaction。

```cpp
void Node::setPosition(Vec2 p) {
    position = p;
    markTransformDirty();
}
```

原則：

```text
會影響 cache 的資料，不應該被隨便直接改。
要透過 setter / command / transaction 來集中做 invalidation。
```

---

# 18. 和 ECS Change Detection 的關係

ECS 裡也常見 change detection：

```text
Position component changed
  ↓
TransformSystem updates WorldTransform
  ↓
RenderSystem updates render proxy
```

Entity：

```text
Entity
  ├── LocalTransform
  ├── WorldTransform
  ├── Sprite
  └── RenderBounds
```

當 `LocalTransform` 改變：

```text
LocalTransform dirty
  ↓
WorldTransform dirty
  ↓
RenderBounds dirty
  ↓
Culling dirty
```

關鍵字：

```text
ECS change detection
Archetype version
Component dirty bit
Sparse set
Generational index
System dependency
```

---

# 19. 和 Compiler / Build System 的類比

這個大概念不只 graphics 有。

## 19.1 Build system

```text
main.cpp 改了
  ↓
main.o 失效
  ↓
app 失效
```

Make / Ninja / Bazel 都在處理：

```text
input file change
dependency graph
incremental rebuild
cache invalidation
```

---

## 19.2 Compiler

Incremental compiler 會追蹤：

```text
source file
AST
symbol table
type checking result
IR
object file
```

當某個 function 改了，不希望整個 project 全部重編。

---

## 19.3 Database

Materialized view：

```text
base table changed
  ↓
view dirty
  ↓
incremental refresh
```

真正抽象是：

```text
cached derived data
depends on source data
source data changes
derived data becomes invalid
recompute minimal necessary part
```

---

# 20. 抽象統一：Source data / Derived data / Cache

所有例子都可以統一成：

```text
source data
  ↓ compute
derived data
  ↓ cache
output
```

當 source data 改變：

```text
source data changed
  ↓
derived data cache invalid
  ↓
recompute
```

例子：

| 系統 | source data | derived data / cache |
|---|---|---|
| Cocos | local position | world transform |
| OLED UI | selected_index | visual appearance / framebuffer pixels |
| React | component state | virtual tree / DOM |
| Browser | DOM + CSS | layout tree / paint records / layers |
| Build system | `.cpp` file | `.o` file |
| Renderer | scene data | command buffer / framebuffer |

這就是整個大概念的核心。

---

# 21. 對 Pixel-Renderer 的吸收方式

原本 Pixel-Renderer 主線可能是：

```text
clear framebuffer
draw all triangles
present
```

下一階段可以加入：

```text
scene graph
transform dirty
bounding box dirty
render object dirty
framebuffer dirty region
UI dirty region
resource dirty
```

學習路線：

```text
Stage 1:
Full redraw renderer

Stage 2:
Scene graph + transform dirty

Stage 3:
2D UI dirty rectangle

Stage 4:
Static/dynamic layer cache

Stage 5:
Render command cache / display list

Stage 6:
Render graph / pass dependency
```

這會讓 Pixel-Renderer 從：

```text
rasterizer demo
```

變成：

```text
small rendering system
```

---

# 22. 建議實驗

## 實驗 A：Transform dirty

做一個簡單 scene graph：

```text
Root
 └─ Arm
     └─ Hand
         └─ Finger
```

改 `Arm` 的 rotation，觀察只有 Arm subtree 的 world transform 需要更新。

紀錄：

```text
total nodes
updated nodes
skipped nodes
```

---

## 實驗 B：OLED dirty rect

做 128x64 menu simulator：

```text
full redraw
vs
dirty rect redraw
vs
dirty page flush
```

紀錄：

```text
draw calls
dirty pixels
flushed bytes
frame time
```

---

## 實驗 C：Static/dynamic layer

把畫面分成：

```text
static_fb
dynamic_fb
final_fb
```

比較：

```text
每 frame render text
vs
文字 cache，只畫 highlight
```

---

## 實驗 D：Browser-like mini pipeline

做一個小 UI pipeline：

```text
Widget tree
  ↓
Layout
  ↓
Paint command list
  ↓
Raster
  ↓
Composite
```

測試這些 change：

```text
改文字內容
改位置
改 transform
改顏色
```

觀察每種 change 會讓哪些階段 invalid。

---

# 23. 最小可實作版本：半 Retained OLED UI

不需要一開始就做完整 retained-mode tree。可以先做半 retained：

```cpp
struct ScreenCache {
    Framebuffer1bpp static_fb;
    Framebuffer1bpp final_fb;

    int prev_selected = -1;
    int selected = 0;

    DirtyRegion dirty;
};
```

進入畫面：

```cpp
void enter_settings_screen() {
    static_fb.clear();
    draw_static_settings_ui(static_fb);
    cache.prev_selected = -1;
    cache.dirty.mark_full();
}
```

更新狀態：

```cpp
void update_settings_screen(Input input) {
    if (input.encoder_delta != 0) {
        cache.prev_selected = cache.selected;
        cache.selected += input.encoder_delta;

        cache.dirty.mark(menu_item_rect(cache.prev_selected));
        cache.dirty.mark(menu_item_rect(cache.selected));
        cache.dirty.mark(scrollbar_rect());
    }
}
```

render：

```cpp
void render_settings_screen() {
    copy_dirty_from_static(final_fb, static_fb, cache.dirty);

    draw_menu_item_dynamic(final_fb, cache.prev_selected);
    draw_menu_item_dynamic(final_fb, cache.selected);
    draw_scrollbar(final_fb);

    oled.flush(cache.dirty);
    cache.dirty.clear();
}
```

這不是純 retained-mode，也不是純 immediate-mode。  
但它符合 embedded UI 的現實：

```text
小範圍 state tracking
小範圍 dirty update
足夠快
足夠可控
```

---


# 24. Layer 記憶體最佳化：Sparse Layer、Dirty Metadata、Display List

前面提到一種簡單的動靜分離做法：

```text
static layer : 1024 bytes
dynamic layer: 1024 bytes
final buffer : 1024 bytes
-------------------------
total        : 3072 bytes
```

對 128×64、1bpp OLED 來說，一張 framebuffer 是：

```text
128 * 64 / 8 = 1024 bytes
```

如果 MCU 是 ESP32、RP2040、STM32F4 這類 SRAM 較充裕的平台，3 KB 通常不是問題。  
但如果是 ATmega328P / Arduino UNO 這種 SRAM 只有 2 KB 左右的平台，甚至一張完整 framebuffer 都會有壓力。

因此自然會出現一個問題：

```text
能不能不要每個 layer 都存一整張 bitmap？
```

答案是可以。這個思考點可以擴充成一個新的主題：

```text
不要把所有中間結果都存成 dense framebuffer。
改用 metadata、dirty region、draw command 或 compressed resource 表示畫面。
```

---

## 24.1 這是不是「稀疏矩陣」？

概念上接近，但 graphics / embedded UI 裡通常不會稱為 sparse matrix。  
更常見的關鍵字是：

```text
Sparse framebuffer
Sparse bitmap
Sparse layer
Dirty tile map
Dirty rectangle list
Damage region
Span buffer
Run-length encoding, RLE
Display list
Draw command list
Sprite list
Object list
```

「稀疏矩陣」的典型想法是：

```text
大部分元素是 0，只存非 0 元素。
```

如果硬套到 1-bit OLED，可以設計成：

```cpp
struct SparsePixel {
    uint8_t x;
    uint8_t y;
    bool value;
};
```

也就是只存白色 pixel：

```text
(10, 3) = 1
(11, 3) = 1
(12, 3) = 1
```

但這在 1bpp framebuffer 上通常不划算，因為 dense framebuffer 的一個 pixel 只需要 1 bit，而 sparse pixel 需要存 `x`、`y`、`value`，通常至少 2～3 bytes。

對比：

```text
dense framebuffer:
1 pixel = 1 bit

sparse pixel:
1 pixel ≈ 16~24 bits 甚至更多
```

所以 per-pixel sparse representation 只適合極端稀疏的情境，例如只畫幾個點。  
對 UI 來說，文字、反白框、icon 通常是連續區塊，不是隨機散點，因此更適合用 region / command 表示。

---

## 24.2 Dense bitmap layer vs Sparse metadata

原本的 full layer 是：

```text
static_fb   = 整張固定畫面 bitmap
dynamic_fb  = 整張動態畫面 bitmap
final_fb    = 合成後輸出 bitmap
```

這種做法簡單，但每個 layer 都佔完整 framebuffer。

替代方向是：

```text
static_fb       = 固定背景 bitmap
final_fb        = 實際輸出 bitmap
dirty metadata  = 哪些區域要修復 / 重畫 / flush
draw commands   = 動態元素怎麼畫
```

也就是不要存：

```text
dynamic_fb: 1024 bytes
```

改存：

```text
DrawRect(highlight_rect)
DrawText(fps_text)
DrawBitmap(cursor_icon)
```

這會把 dynamic layer 從「bitmap layer」改成「display list / draw command list」。

---

## 24.3 方案一：Dirty Rect List

最實用的第一步是 dirty rectangle list。

```cpp
struct Rect {
    int x;
    int y;
    int w;
    int h;
};

struct DirtyRegion {
    Rect rects[8];
    int count;
};
```

選單反白從第 1 項移到第 2 項時：

```text
old selected item rect dirty
new selected item rect dirty
```

流程：

```text
state changed
  ↓
mark old item rect dirty
mark new item rect dirty
  ↓
copy dirty rect from static_fb to final_fb
  ↓
redraw dynamic widgets in dirty rect
  ↓
flush dirty rect to OLED / LCD
```

這樣可以省掉一張 dynamic framebuffer：

```text
static framebuffer: 1024 bytes
final framebuffer : 1024 bytes
dirty rect list   : 很小
```

優點：

```text
RAM 省
概念直接
適合 menu / cursor / highlight / small animation
```

缺點：

```text
rect 合併有成本
容易漏標 old region
對 SSD1306 這類 page-based OLED 不一定最貼近硬體
```

---

## 24.4 方案二：Dirty Page Mask

對 SSD1306 / SH1106 這類 OLED，更硬體友善的方式是 dirty page。

128×64 OLED 通常分成 8 個 page：

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

可以只用一個 byte 表示哪些 page 髒掉：

```cpp
uint8_t dirty_pages = 0;

void mark_page_dirty(int page) {
    dirty_pages |= (1 << page);
}
```

如果 UI item 高度設計成 8 或 16 pixels，就會剛好對齊 page：

```text
menu item height = 16 px
  ↓
exactly 2 SSD1306 pages
```

這會讓 partial flush 很簡單：

```text
for each dirty page:
    set OLED page address
    write 128 bytes of that page
```

記憶體成本：

```text
dirty page mask = 1 byte
```

這是小 OLED UI 非常實用的表示法。

---

## 24.5 方案三：Span List

如果要比 dirty rect 更細，可以存 horizontal span：

```cpp
struct Span {
    uint8_t y;
    uint8_t x0;
    uint8_t x1;
};
```

意思是：

```text
第 y 行，x0 到 x1 這段需要更新。
```

例如：

```text
y = 12, x = 10..80
y = 13, x = 10..80
y = 14, x = 10..80
```

它比 per-pixel sparse 更合理，因為 UI 圖形常是連續區段。  
但在 SSD1306 上，span 最後仍然要轉成 page-based write，所以小 OLED 不一定需要做到這麼細。

適合情境：

```text
更高解析度 monochrome framebuffer
軟體 rasterizer 需要精細 region tracking
需要分析每行 coverage / scanline update
```

---

## 24.6 方案四：RLE / 壓縮 bitmap resource

RLE（Run-Length Encoding）適合壓縮固定圖像資源，例如 icon、logo、bitmap font glyph。

例如：

```text
000000001111111100000000
```

可以存成：

```text
8 zeros
8 ones
8 zeros
```

資料結構：

```cpp
struct Run {
    uint8_t length;
    uint8_t value;
};
```

適合：

```text
icon
logo
bitmap font glyph
大面積空白的固定圖片
```

不適合：

```text
每 frame 頻繁修改的 final framebuffer
```

原因是 framebuffer 被修改後可能要重新壓縮，成本不一定划算。  
因此 RLE 比較適合「儲存 resource」，不適合當主要 render target。

---

## 24.7 方案五：Display List / Draw Command List

這是最值得放進 Pixel-Renderer 架構思考的方案。

不要把 dynamic layer 存成 bitmap，而是存成「要畫什麼」：

```cpp
enum class DrawType {
    Rect,
    Text,
    Bitmap,
};

struct DrawCommand {
    DrawType type;
    Rect bounds;
    // payload: color, text pointer, bitmap pointer, etc.
};
```

例如 dynamic layer 可以表示成：

```text
DrawRect(highlight_rect)
DrawText(fps_text)
DrawBitmap(cursor_icon)
```

而不是：

```text
dynamic_fb: 1024 bytes
```

資料流：

```text
UI state
  ↓
build draw command list
  ↓
compute dirty bounds from commands
  ↓
rasterize only dirty commands into final_fb
  ↓
flush dirty pages / rects
```

這很接近 browser / game engine 的思路：

```text
UI tree / scene state
  ↓
display list / render commands
  ↓
raster / composite
  ↓
present
```

因此這個方向比「sparse matrix framebuffer」更有系統設計價值。

---

## 24.8 幾種表示法比較

| 方法 | 存什麼 | RAM 成本 | 適合情境 |
|---|---:|---:|---|
| Full layer | 整張 bitmap | 高 | 簡單、RAM 足夠 |
| Sparse pixels | 非零 pixel | 通常不划算 | 極少數散點 |
| Dirty rect list | 髒掉的矩形 | 低 | 選單、局部重畫 |
| Dirty page mask | 髒掉的 OLED page | 極低 | SSD1306 / SH1106 |
| Span list | 每行髒區段 | 中 | 更細緻的 scanline update |
| RLE bitmap | 壓縮圖片 | 低 | icon / font / logo resource |
| Display list | draw commands | 低 | dynamic layer / UI 系統 / renderer 架構 |

---

## 24.9 對 128×64 OLED 的推薦架構

不要一開始做 per-pixel sparse matrix。  
比較實際的設計是：

```text
static_fb       : 1024 bytes
final_fb        : 1024 bytes
dirty_page_mask : 1 byte
draw_commands   : 小陣列
```

架構：

```text
static_fb ─────────────┐
                       v
                 restore dirty pages
                       │
draw_commands ────────>│
                       v
                  final_fb
                       │
dirty_page_mask ──────>│
                       v
                  OLED flush
```

這會把記憶體從：

```text
static + dynamic + final = 3072 bytes
```

降成：

```text
static + final + metadata ≈ 2048 bytes + 少量 command
```

如果 RAM 更緊，也可以只保留：

```text
final_fb + dirty_page_mask + redraw functions
```

但代價是每次修復 dirty region 時，要重新呼叫 static UI 的繪製邏輯，程式會更複雜。

---

## 24.10 判斷原則：你到底要省什麼？

不同表示法最佳化的目標不同。

```text
省 RAM:
  不要多張 full layer
  用 display list / dirty page mask / compressed resource

省 CPU:
  用 static framebuffer cache
  避免每 frame 重畫文字和 icon

省 I2C/SPI 傳輸量:
  用 dirty rect / dirty page partial flush

省程式複雜度:
  full framebuffer + full redraw
```

所以工程上不是「稀疏一定比較好」，而是要先問：

```text
瓶頸是 RAM？CPU？Bus bandwidth？還是 code complexity？
```

---

## 24.11 和大概念的關係

這個思考點其實是前面 invalidation-based rendering 的延伸。

前面問的是：

```text
哪些資料需要重算？
哪些區域需要重畫？
哪些 bytes 需要重傳？
```

這裡再加上一個問題：

```text
哪些中間結果根本不需要以完整 bitmap 存下來？
```

因此完整問題變成：

```text
State change
  ↓
Invalidate affected region / node / command
  ↓
Use cached static data when useful
  ↓
Represent dynamic data as commands when cheaper than bitmap
  ↓
Rasterize only necessary output
  ↓
Flush only necessary bytes
```

這會把 OLED UI 從「小玩具 UI」提升成一個非常完整的 rendering system 練習題。

---

# 25. 最終總結

本次對話真正整理出的不是單純「OLED UI 怎麼做」，而是 rendering system 的一個核心問題：

```text
Rendering 不是只有怎麼畫 pixel。
還包括：
哪些資料需要重算？
哪些區域需要重畫？
哪些 bytes 需要重傳？
哪些 cache 可以保留？
哪些 dependency 需要傳播？
```

一句話總結：

```text
當系統裡有大量 derived data / cached output 時，
狀態改變後如何精準地讓受影響部分失效，
並只重算、重畫、重傳必要的部分。
```

在 graphics 裡，它表現成：

```text
transform dirty
layout dirty
paint dirty
raster dirty
dirty rectangle
dirty page
layer cache
compositor
render graph
```

在更廣泛的軟體工程裡，它就是：

```text
dependency tracking + cache invalidation + incremental recomputation
```

對 Pixel-Renderer 來說，這是從「會寫 rasterizer」走向「會設計 rendering system」的重要入口。
