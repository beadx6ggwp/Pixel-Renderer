# Verification

這裡放 debug、testing、trace、golden image 相關文件。目標是讓 renderer 每一層都能被檢查，而不是只靠肉眼看畫面。

建議先補：

```text
debug_visualization.md
pipeline_trace.md
golden_image_tests.md
```

## Current Files

### `testing_strategy.md`

整理第一階段 renderer testing strategy：

```text
pure raster helper tests
small CPU framebuffer tests
deterministic raster demos
golden image tests later
```

目前重點是 `render/raster-baseline`：

```text
edge function
half-open bbox
pixel-center sampling
top-left shared-edge rule
barycentric weights
depth pass / fail
color interpolation
```

## Planned Files

### `debug_visualization.md`

定義 debug views：

```text
Barycentric View
Depth View
Wireframe View
UV View
Normal View
Overdraw View
ClipSpaceW View
```

每個 view 都應該回答：

```text
needs what data
expected output
bugs it can catch
implementation stage
```

### `pipeline_trace.md`

定義不用靠畫面也能檢查 pipeline 的文字 trace：

```text
positionOS
positionWS
positionVS
positionCS
positionNDC
positionSS
depth
clip.w
```

### `golden_image_tests.md`

定義 framebuffer regression testing：

```text
deterministic scene
expected output
exact match vs tolerance
diff image
golden update policy
```

