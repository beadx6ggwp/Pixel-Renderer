# Roadmap

這裡放未來實作順序、milestone、短中長期計畫。它應該比 `notes/journal` 上的 rough notes 更穩定，比 `PROJECT_MAP.md` 更具體。

建議先補：

```text
milestones.md
branching_map.md
```

## Current Files

### `next_steps.md`

整理下一個近期 source slice：

```text
render/raster-baseline
```

這份文件只回答近期問題：

```text
what to implement next
what files or modules are likely involved
what tests to add
what is explicitly out of scope
when to split arch/render-target
```

它不是完整 project roadmap。

## Planned Files

### `milestones.md`

整理主要 milestone：

```text
Milestone A: Screen-space Triangle Baseline
Milestone B: NDC / Viewport Baseline
Milestone C: Math / Camera Baseline
Milestone D: Perspective Baseline
Milestone E: IShader / Stage Data
Milestone F: Material / Renderer Skeleton
Milestone G: CommandQueue / Debug UI
```

### `branching_map.md`

整理中長期分支：

```text
Material / shader
Engine architecture
UI / tooling
Software GPU
FPGA / hardware GPU
Engine Mirror Demo
```
