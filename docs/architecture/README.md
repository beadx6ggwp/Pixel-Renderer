# Architecture

這裡放 Pixel-Renderer 的工程架構文件。重點不是教 graphics math，而是定義 renderer core、framebuffer ownership、display backend、resource lifetime、command queue、UI overlay 等邊界。

建議先補：

```text
display_backend_architecture.md
renderer_core_architecture.md
resource_lifetime.md
command_queue_architecture.md
debug_ui_architecture.md
```

## Planned Files

### `display_backend_architecture.md`

整理 Win32 / SDL / macOS display abstraction：

```text
renderer core produces pixels
Framebuffer owns pixels
RenderDevice writes Framebuffer
DisplayBackend presents Framebuffer
Win32DisplayBackend
SDLDisplayBackend
headless test backend
```

這份文件應該把三件事拆開：

```text
renderer core: calculate pixels
framebuffer ownership: store pixels
display backend: show pixels
```

### `renderer_core_architecture.md`

整理目前與目標 core 架構：

```text
Application
ScreenManager
RenderDevice
Rasterizer
Framebuffer
Renderer
Scene
View
Camera
Material
MaterialInstance
```

### `resource_lifetime.md`

定義 ownership rule：

```text
owner
borrow
view
handle
raw pointer policy
future resource registry
CommandQueue snapshot rule
```

### `command_queue_architecture.md`

定義 renderer frontend/backend split：

```text
RenderCommand
scene_queue
ui_queue
SoftwareBackend
state deduplication
sorting
batching
future render thread
```

### `debug_ui_architecture.md`

整理 immediate-mode debug UI 如何接進 renderer：

```text
input state
UiCommandList
DrawRect / DrawText
ClipRect
RenderMode controls
RenderStats panel
```

