# Web UI 與 Invalidation-based Rendering 教學筆記

> 主題：從 React / Browser / Renderer frontend-backend 的分層，理解 Web UI 為什麼不是單純「state 改了就畫面改了」，而是多層 invalidation、cache、diff、rendering pipeline 的組合。

---

## 0. 這篇在回答什麼問題？

這串對話主要在釐清一個問題：

> Web UI 為什麼要分成 React 處理前段、browser 處理後段？  
> 這是不是類似 renderer frontend / backend？  
> Application-level invalidation 和 Rendering-level invalidation 的分界在哪？

直覺上，我們會以為 UI 更新只是：

```text
state 改了
  ↓
畫面改了
```

但 Web 的實際路徑更像：

```text
Application state
  ↓
React / Vue / Svelte / Solid
  ↓
DOM mutation
  ↓
Browser style / layout / paint / raster / composite
  ↓
Screen pixels
```

也就是說，Web UI 至少包含兩個層級：

```text
1. Application semantic layer
   處理「UI 語義上應該長什麼樣子」

2. Platform rendering layer
   處理「這些 DOM/CSS 最後怎麼變成 pixels」
```

---

## 1. 核心大概念：Invalidation-based incremental rendering

這整串討論背後的大概念是：

> **Invalidation-based incremental rendering**  
> 基於失效標記的增量渲染

更廣義可以叫：

> **Incremental computation with dependency tracking**  
> 帶依賴追蹤的增量計算

核心問題：

```text
狀態改變後，不要全部重算。
只讓受影響的部分失效，然後重算必要的最小集合。
```

抽象模型：

```text
Source data changed
  ↓
Invalidate affected derived data
  ↓
Propagate dirty state
  ↓
Recompute dirty parts
  ↓
Update cached output
```

在 Web UI 裡，這件事會跨很多層：

```text
React:
  state → component → DOM

Browser:
  DOM/CSS → style → layout → paint → raster → composite

GPU / OS:
  layer texture → swapchain / window surface → screen
```

每一層都有自己的 dirty object 和 cache。

---

## 2. 先分清楚：React 和 Browser 不是同一層

React 不是 browser 的 renderer。  
React 是一個 **UI state → UI tree / host mutation** 的系統。

Browser 才是真正負責：

```text
DOM + CSS → pixels
```

完整流程：

```text
你的資料 / state
  ↓
React
  ↓
DOM mutation
  ↓
Browser engine
  ↓
Pixels on screen
```

### React 關心的問題

React 關心的是：

```text
Application state 改了，
哪些 component output 應該改？
哪些 DOM node 應該被 mutation？
```

例如：

```jsx
function Menu({ selected }) {
  return (
    <ul>
      <li className={selected === 0 ? "active" : ""}>顯示幀率</li>
      <li className={selected === 1 ? "active" : ""}>保存數據</li>
      <li className={selected === 2 ? "active" : ""}>此設備</li>
    </ul>
  );
}
```

當 `selected` 從 `0` 變成 `1`：

```text
舊 UI tree：
Item[0] active
Item[1] normal
Item[2] normal

新 UI tree：
Item[0] normal
Item[1] active
Item[2] normal
```

React 的工作是：

```text
比較 old tree / new tree
  ↓
找出差異
  ↓
產生最小 DOM mutation
```

結果可能是：

```text
Item[0].className: active → normal
Item[1].className: normal → active
```

這裡還沒有 pixel-level rendering。

---

## 3. Browser 關心的是 DOM/CSS 到 pixels

React 修改 DOM 之後，browser engine 才接手。

Browser 要處理：

```text
DOM structure
CSS cascade
font
image
viewport
scroll position
device pixel ratio
z-index
overflow
transform
opacity
layer
GPU composition
```

典型 browser rendering pipeline：

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

各階段的意思：

| 階段 | 負責內容 |
|---|---|
| Style calculation | 根據 CSS selector / cascade / inheritance 算出 computed style |
| Layout | 算出每個 box 的位置與尺寸 |
| Paint | 產生繪製命令，例如 draw background、draw text、draw border |
| Raster | 把 paint commands 轉成 bitmap / tiles / texture |
| Composite | 把 layers 合成到最終畫面 |
| Screen | present 到螢幕 |

React 不直接知道這些細節。  
React 只知道它改了 DOM / className / attributes / text content。

---

## 4. 為什麼 Web 要分成 React 前段、Browser 後段？

因為 Web 平台本來就是這樣分層的：

```text
HTML 描述結構
CSS 描述外觀
JavaScript 描述行為
Browser 負責實際呈現
```

React 是後來加在上面的 library / framework。  
它插在：

```text
Application state
  ↓
DOM
```

之間，而不是取代 browser engine。

所以實際上不是：

```text
React 和 Browser 合作設計出完整 pipeline
```

而是：

```text
Browser 原本就有完整 rendering pipeline
React 插在 application state 和 DOM 之間
```

React 能操作的標準介面是：

```text
DOM API
```

而不是：

```text
GPU command buffer
font rasterizer
layout engine internal data
compositor layer tree
window system surface
```

這是一條重要邊界：

```text
React
  ↓
DOM / CSSOM API
-------------------- Web platform boundary
Browser engine
  ↓
Layout / Paint / Composite
```

---

## 5. Web UI 可以理解成「類似前後端」嗎？

可以，但這裡的「前後端」不是：

```text
Web frontend / backend
= browser client / server API
```

而比較像：

```text
Renderer frontend / renderer backend
= 高階描述處理 / 低階像素輸出
```

Web UI 至少有兩段前後端分層。

---

## 6. 第一段：Application UI frontend / Browser platform backend

這是 React 和 browser 的分工：

```text
Application state
  ↓
React / Vue / Svelte / Solid
  ↓
DOM mutation
  ↓
Browser engine
  ↓
Pixels
```

在這個視角下：

```text
React 類似 application-level frontend：
  state → component tree → DOM mutation

Browser 類似 platform rendering backend：
  DOM/CSS → layout → paint → raster → composite → screen
```

也就是：

```text
Application-level invalidation:
  state → component → DOM

Rendering-level invalidation:
  DOM/CSS → layout → paint → composite
```

更完整：

```text
State change
  ↓
[Application-level invalidation]
Component dirty
Virtual DOM / Fiber diff
DOM mutation
  ↓
[Browser rendering-level invalidation]
Style invalidation
Layout invalidation
Paint invalidation
Raster invalidation
Composite invalidation
  ↓
Screen update
```

---

## 7. 第二段：Browser 內部也能分 renderer frontend / backend

Browser 自己內部也能再拆成：

```text
DOM/CSS
  ↓
Style calculation
  ↓
Layout
  ↓
Paint records / display list
  ↓
Raster
  ↓
Compositor
  ↓
GPU / screen
```

用 renderer frontend/backend 來看：

```text
Browser rendering frontend:
  DOM/CSS
  → style calculation
  → layout
  → paint command / display list

Browser rendering backend:
  display list
  → raster into tiles/textures
  → composite layers
  → present to screen
```

完整分層：

```text
React / App layer
────────────────────────────────────
state
  ↓
component tree
  ↓
DOM mutation

Browser rendering frontend
────────────────────────────────────
DOM + CSSOM
  ↓
style
  ↓
layout
  ↓
paint list / display list

Browser rendering backend
────────────────────────────────────
raster tiles
  ↓
compositor layers
  ↓
GPU / screen
```

所以：

```text
Rendering-level invalidation 本身橫跨 browser renderer frontend/backend。

style/layout/paint 比較偏 renderer frontend。
raster/composite/present 比較偏 renderer backend。
```

---

## 8. Invalidation 在每一層的對象不同

不同層會 dirty 不同東西：

```text
React:
  dirty component
  dirty Fiber
  dirty DOM mutation

Browser style/layout:
  dirty style
  dirty layout box

Browser paint:
  dirty paint region
  dirty display item

Browser raster/compositor:
  dirty tile
  dirty layer
  dirty damage region
```

同一個 UI change 會一路轉換：

```text
selectedIndex changed
  ↓
React: MenuItem[0], MenuItem[1] dirty
  ↓
DOM: className changed
  ↓
Browser: style dirty
  ↓
如果只是 color/background:
    paint dirty
  ↓
如果是 transform:
    maybe compositor dirty only
  ↓
screen update
```

對照 embedded OLED UI：

```text
selectedIndex changed
  ↓
MenuItem dirty
  ↓
dirty rect / dirty page
  ↓
framebuffer partial redraw
  ↓
OLED partial flush
```

兩者同構，但層級和資源模型不同。

---

## 9. React-level invalidation：state → component → DOM

React 的世界可以簡化成：

```text
State / Props
  ↓
Component render
  ↓
Virtual tree / Fiber tree
  ↓
Diff / Reconciliation
  ↓
Commit DOM mutation
```

React 解決的是：

```text
Application data 改了，
如何讓 UI description 和 data 保持一致？
```

它的核心不是畫 pixel，而是：

```text
state ↔ UI tree / host tree
```

常見關鍵字：

```text
Reconciliation
Virtual DOM
Fiber
Render phase
Commit phase
Host config
DOM mutation
Keyed diff
State batching
Memoization
React.memo
useMemo
useCallback
```

### React 的兩個重要階段

```text
Render phase:
  計算新的 UI tree
  可被中斷 / 可被重新開始

Commit phase:
  將變更提交到 host environment
  對 React DOM 來說就是修改 DOM
```

簡化：

```text
State update
  ↓
Render phase: 算出新 tree
  ↓
Diff: 找出變更
  ↓
Commit phase: 修改 DOM
```

---

## 10. React 不一定只能接 Browser DOM

這點可以幫助理解 React 的本質。

React 的核心是：

```text
state update
  ↓
reconciliation
  ↓
commit to some host environment
```

不同 target 有不同 renderer：

```text
React DOM         → output to browser DOM
React Native      → output to native mobile UI
React Three Fiber → output to Three.js scene
Ink               → output to terminal UI
```

所以 React 比較像：

```text
UI tree diff engine / declarative UI runtime
```

而不是：

```text
pixel renderer
```

React DOM 只是 React 的其中一個 host renderer。

---

## 11. Browser rendering-level invalidation

Browser 接到 DOM/CSS 變化後，要判斷哪些階段失效。

### 改 color / background-color

```css
.active {
  background-color: black;
  color: white;
}
```

可能需要：

```text
style invalidation
  ↓
paint invalidation
  ↓
raster
  ↓
composite
```

### 改 width / height

```css
.active {
  width: 300px;
}
```

可能需要：

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

### 改 transform

```css
.active {
  transform: translateX(10px);
}
```

在某些條件下可能只需要：

```text
composite
```

這就是為什麼網頁動畫常建議用：

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

因為前者比較容易走 compositor path。

---

## 12. Browser pipeline 的分段成本

可以大致這樣理解：

| 改動 | 常見成本 |
|---|---|
| `transform` | composite |
| `opacity` | composite |
| `color` | paint + raster + composite |
| `background-color` | paint + raster + composite |
| `box-shadow` | paint + raster + composite，可能很貴 |
| `width` / `height` | layout + paint + raster + composite |
| `font-size` | layout + paint + raster + composite |
| DOM 新增/刪除節點 | style + layout + paint + raster + composite，視範圍而定 |

重點：

```text
不同 CSS / DOM 改動會讓 browser pipeline 從不同階段重新開始。
```

這就是 rendering-level invalidation 的核心。

---

## 13. Compositor layer：Web 的動靜分離

Browser 為了降低重畫成本，會把畫面拆成 layer。

概念：

```text
Static content layer
Moving panel layer
Cursor / overlay layer
```

動畫時，如果只有 panel 在動：

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
Cursor layer      ── small update ────┘
```

這和 embedded OLED 的 static/dynamic layer 很像：

```text
OLED:
  static framebuffer
  dynamic drawing
  dirty page flush

Browser:
  rasterized layer texture
  compositor layer
  damage region
  GPU composite
```

概念相通，實作完全不同。

---

## 14. Layer promotion 與 transform 動畫

Browser 可能會把某些元素提升成 compositor layer。

常見原因：

```text
transform animation
opacity animation
position fixed / sticky
video / canvas
will-change
3D transform
complex compositing context
```

常見 CSS 提示：

```css
.panel {
  will-change: transform;
}
```

但 `will-change` 不是萬靈丹。  
它可能增加 GPU memory 和 layer management 成本。

更精準的觀念：

```text
Layer promotion 可以減少 repaint，
但會增加 memory、compositor workload、layer management 成本。
```

所以不是所有元素都該 promotion。

---

## 15. Paint command / Display list

Browser 不一定直接從 DOM 畫 pixels。  
通常中間會產生某種 paint records / display list。

可以理解成：

```text
DOM/CSS/Layout
  ↓
Paint commands / Display list
  ↓
Raster
```

Display list 類似：

```text
DrawRect(background)
DrawText("顯示幀率")
DrawBorder(...)
DrawImage(...)
```

這和我們在 OLED UI 討論過的 `draw command list` 很接近：

```text
dynamic layer as bitmap:
  直接存一整張 dynamic framebuffer

dynamic layer as commands:
  存 DrawRect / DrawText / DrawBitmap
```

Browser 的 display list 是一種 intermediate representation。

對 Pixel-Renderer 的啟發：

```text
不要讓 UI widget 直接寫 framebuffer。
可以先產生 draw commands，再交給 rasterizer。
```

---

## 16. Browser raster：從 paint commands 到 tiles/textures

Raster 階段會把 paint commands 轉成 bitmap。  
在現代 browser 裡，畫面常常被切成 tiles。

概念：

```text
Paint commands
  ↓
Raster tiles
  ↓
GPU textures
```

這樣的好處：

```text
局部更新
tile cache
平行 raster
scroll 時重用舊 tile
```

這類似 OLED 的 dirty page / dirty rect，但 scale 更大：

```text
OLED:
  dirty page = 8px 高的一段 buffer

Browser:
  dirty tile = 螢幕上的一塊 raster cache
```

兩者都在處理：

```text
哪些 pixel 資料需要重新生成？
哪些 cached bitmap 可以保留？
```

---

## 17. Damage region：Browser 裡的 dirty rectangle

Browser / compositor 裡常見詞是：

```text
damage region
```

這和 embedded GUI 裡的：

```text
dirty rectangle
invalid region
update region
```

是同一類概念。

當某一小塊畫面變化：

```text
old position dirty
new position dirty
```

Browser 會盡量讓 damage region 小一點，避免整頁重新 raster / composite / present。

概念：

```text
Visual change
  ↓
Damage region
  ↓
Only damaged tiles / layers need update
```

---

## 18. React-level diff 和 Browser-level damage 不同

這點很重要。

React diff 的對象是：

```text
component / virtual tree / DOM
```

Browser damage 的對象是：

```text
paint region / raster tile / compositor layer
```

所以：

```text
React diff 小，不代表 browser repaint 小。
React diff 大，也不一定代表 browser repaint 大。
```

例子：

### React 改一個 className，但造成 layout 大改

```text
React mutation 很小：
  className changed

Browser cost 很大：
  layout whole container
  repaint many descendants
```

### React 重新 render 很多 component，但 DOM 幾乎沒變

```text
React render cost 大
DOM mutation 小
Browser rendering cost 小
```

這就是為什麼 Web performance 要分層看。

---

## 19. Framework-level optimization vs Browser-level optimization

React / Vue / Svelte 主要優化：

```text
state update
component render
diff
DOM mutation
```

Browser 主要優化：

```text
style
layout
paint
raster
composite
```

所以常見效能問題也分兩類。

### Application-level 問題

```text
太多 component re-render
state 放太高導致大範圍更新
list 沒有 virtualization
memoization 不當
key 設錯造成大量 remount
```

### Rendering-level 問題

```text
layout thrashing
forced synchronous layout
頻繁改 width/height/top/left
大面積 paint
大量 box-shadow/filter
過多 compositor layers
圖片過大
```

兩者需要不同工具和思路。

---

## 20. Layout thrashing：跨層錯誤的典型例子

Web UI 很常見的 performance bug：

```js
for (const el of items) {
  const h = el.offsetHeight;      // read layout
  el.style.height = h + 10 + "px"; // write layout
}
```

問題是 read/write 混在一起，browser 可能被迫同步 layout。

更好的方式：

```js
const heights = items.map(el => el.offsetHeight);

for (let i = 0; i < items.length; ++i) {
  items[i].style.height = heights[i] + 10 + "px";
}
```

概念：

```text
不要在同一段流程裡交錯：
read layout → write style → read layout → write style

應該 batch：
read all → write all
```

這和 invalidation 有關，因為你強迫 browser 在還沒準備好的時候把 dirty layout 算完。

---

## 21. React 的 batching 與 Browser 的 batching

React 會 batch state updates：

```text
multiple setState
  ↓
batch
  ↓
one render / commit
```

Browser 也會 batch rendering work：

```text
multiple DOM/CSS changes
  ↓
style/layout/paint scheduled
  ↓
next frame update
```

理想情況：

```text
Application updates batch together
Browser rendering updates batch together
```

錯誤情況：

```text
每次小變化都強迫 layout / paint / sync measurement
```

這會破壞 pipeline。

---

## 22. React 和 Browser 的同步邊界

React commit DOM mutation 後，browser 不一定立刻重畫。  
Browser 通常會在自己的 frame lifecycle 中處理。

大致流程：

```text
JavaScript task
  ↓
React state update / render / commit
  ↓
DOM mutated
  ↓
Browser schedules style/layout/paint
  ↓
requestAnimationFrame
  ↓
rendering update
  ↓
paint/composite
```

實際 browser event loop 很複雜，但這個模型足夠用來理解：

```text
DOM mutation 不是等於 pixels 立刻改變。
```

React 的 commit 和 browser 的 rendering 是不同階段。

---

## 23. 對照 Cocos / Game Engine

Cocos / game engine 也有類似分層：

```text
Game state
  ↓
Scene graph / ECS
  ↓
Transform update
  ↓
Render queue / draw commands
  ↓
GPU command buffer
  ↓
Screen
```

對照 Web：

| Game / Renderer | Web UI |
|---|---|
| Game state | Application state |
| Scene graph / ECS | Component tree / DOM |
| Transform dirty | Layout dirty |
| Render queue dirty | Paint / display list dirty |
| Vertex buffer dirty | Raster tile / layer dirty |
| GPU composite | Browser compositor |

同樣是：

```text
source data → derived data → cached output
```

當 source data 改變，就要 invalidated derived data。

---

## 24. 對照 Embedded OLED UI

OLED UI 可能是：

```text
selected_index
  ↓
Menu state
  ↓
dirty item rect
  ↓
draw commands
  ↓
1bpp framebuffer
  ↓
dirty page flush
  ↓
OLED
```

Web UI 是：

```text
selected_index
  ↓
React state / component
  ↓
DOM className mutation
  ↓
style / paint invalidation
  ↓
raster tile / layer update
  ↓
compositor
  ↓
screen
```

對照表：

| Embedded OLED | Web |
|---|---|
| `selected_index` | React state |
| Menu logic | React component |
| Widget tree | Virtual DOM / DOM |
| Dirty widget | Dirty component / dirty DOM |
| Dirty rect / page | Paint damage / raster tile dirty |
| Framebuffer | Rasterized layer / tile |
| OLED flush | Compositor present |

概念同構：

```text
狀態改變後，只更新受影響的部分。
```

但資源模型不同：

```text
OLED:
  少 RAM、低頻 CPU、I2C/SPI bus、1-bit framebuffer

Browser:
  大量 RAM、GPU、複雜 layout、font、compositor、high DPI
```

---

## 25. 跟 Pixel-Renderer 的架構啟發

你之後做 Pixel-Renderer / UI system 時，可以刻意分成：

```text
Application state
  ↓
UI tree / scene tree
  ↓
Layout
  ↓
Paint commands / Display list
  ↓
Rasterization
  ↓
Framebuffer / Texture
  ↓
Display backend
```

不要讓 UI widget 直接操作 pixel：

```cpp
// 不好的方向：widget 直接畫 framebuffer
button.drawPixelDirectly(framebuffer);
```

比較好的方向：

```text
Button state
  ↓
Layout rect
  ↓
Paint command: DrawRect, DrawText
  ↓
Rasterizer
  ↓
Framebuffer
```

這樣每一層都可以做 dirty tracking：

```text
state dirty
layout dirty
paint dirty
raster dirty
flush dirty
```

這就是 browser 架構對 renderer 設計的最大啟發。

---

## 26. 一個 mini Web-like UI pipeline 設計

你可以在自己的 renderer 做一個簡化版：

```text
UI State
  ↓
Widget Tree
  ↓
Layout Tree
  ↓
Display List
  ↓
Raster Cache
  ↓
Compositor
  ↓
Framebuffer
```

### Data structures

```cpp
struct Widget {
    WidgetId id;
    Style style;
    std::vector<Widget*> children;

    bool style_dirty;
    bool layout_dirty;
    bool paint_dirty;
};

struct LayoutBox {
    Rect bounds;
    bool dirty;
};

enum class PaintOpType {
    FillRect,
    DrawText,
    DrawImage,
};

struct PaintOp {
    PaintOpType type;
    Rect bounds;
};
```

### Pipeline

```text
1. State changes
2. Mark affected widgets dirty
3. Recompute style/layout if needed
4. Generate paint ops for dirty widgets
5. Rasterize dirty regions
6. Composite/present final framebuffer
```

這可以讓你用很小規模理解 browser rendering pipeline。

---

## 27. 最重要的錯誤觀念修正

### 錯誤觀念 A：React 負責畫畫

不精準。

```text
React 負責 state → host mutation。
React DOM 的 host mutation 是 DOM mutation。
Browser 負責 DOM/CSS → pixels。
```

### 錯誤觀念 B：React diff 小，畫面更新就一定便宜

不一定。

```text
React diff 小，只代表 DOM mutation 可能小。
但 browser layout / paint 可能很貴。
```

### 錯誤觀念 C：transform 一定只走 compositor

不一定。

```text
transform 常常比較容易走 compositor path，
但是否真的只 composite，取決於 browser、layer、元素狀態、其他 CSS。
```

### 錯誤觀念 D：layer 越多越好

不一定。

```text
layer 可以減少 repaint，
但會增加 memory、compositor management、GPU workload。
```

### 錯誤觀念 E：Virtual DOM 就是 Web performance 的核心

不完整。

```text
Virtual DOM 只解 application-level diff。
真正 Web performance 還包含 browser rendering pipeline。
```

---

## 28. 關鍵字整理

### React / Application-level

```text
React reconciliation
Virtual DOM
Fiber
Render phase
Commit phase
DOM mutation
State batching
Keyed diff
Memoization
React.memo
useMemo
useCallback
Host renderer
React DOM
React Native
```

### Browser rendering-level

```text
Style calculation
Layout
Reflow
Paint
Raster
Composite
Display list
Paint record
Damage region
Invalidation
Layerization
Layer promotion
Compositor layer
Raster tile
Tile cache
GPU compositing
```

### Performance

```text
Layout thrashing
Forced synchronous layout
requestAnimationFrame
will-change
transform animation
opacity animation
paint invalidation
layout invalidation
compositor-only animation
```

### Cross-domain

```text
Dirty flag
Dirty rectangle
Incremental rendering
Incremental computation
Dependency tracking
Cache invalidation
Scene graph
Render graph
Frame graph
Display list
```

---

## 29. 最終總結

Web UI 的更新不是單一階段，而是多層 pipeline：

```text
Application state
  ↓
React / framework
  ↓
DOM mutation
  ↓
Browser style/layout/paint
  ↓
Raster/compositor
  ↓
Screen
```

用 invalidation 來看：

```text
Application-level invalidation:
  state → component → DOM

Rendering-level invalidation:
  DOM/CSS → style → layout → paint → raster → composite
```

用 renderer frontend/backend 來看：

```text
App UI frontend:
  state → component tree → DOM mutation

Browser renderer frontend:
  DOM/CSS → style → layout → paint/display list

Browser renderer backend:
  display list → raster → compositor → screen
```

關鍵不是背名詞，而是看清楚每一層在同步什麼：

```text
React:
  state ↔ DOM

Browser:
  DOM/CSS ↔ pixels
```

這和 OLED UI、Cocos、game engine、Pixel-Renderer 的共同結構是：

```text
Source data changed
  ↓
Derived data invalid
  ↓
Recompute only necessary parts
  ↓
Update cached output
```

也就是：

```text
dependency tracking + cache invalidation + incremental recomputation
```

這是從「會畫圖」進入「會設計 rendering system」的核心知識點。
