# Verification

這裡放 debug、testing、trace、golden image 相關文件。目標是讓 renderer 每一層都能被檢查，而不是只靠肉眼看畫面。

建議先補：

```text
testing_strategy.md
debug_visualization.md
pipeline_trace.md
golden_image_tests.md
```

## Planned Files

### `testing_strategy.md`

整理測試分層：

```text
math unit tests
rasterizer unit tests
depth buffer tests
viewport tests
pipeline tests
golden image tests
regression tests
```

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

