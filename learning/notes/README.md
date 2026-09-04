# Pixel-Renderer Learning Notes

這裡保存 dated learning notes, paper readings, long-form reasoning, teaching drafts, historical records, and exploratory design notes.

它們與 project 共存在 `main`, 不需要切換 branch. 這裡的內容可以很完整、很慢、保留 naive approach 與失敗推導, 但不是 current source truth 或已採納的 architecture decision.

## Reading Boundary

涉及目前 code、build、branch、檔案存在與否時, 優先檢查 current source 與 git state. Stable project rules 請看 `docs/`.

2026-09-05 以前的歷史 notes 可能仍提到舊位置 `docs/notes/`, `docs/tutorial-*`, 或舊的 `notes/journal` workflow. 這些 path 和 branch 描述是當時情境的一部分, 不代表目前規則.

## Current Useful Notes

```text
learning/notes/2026-08-21-game_ui_popup_navigation_architecture.md
learning/notes/2026-07-08-render-baseline-reference-map.md
learning/notes/2026-07-08-nyuzi-raster-paper-reading.md
learning/notes/2026-07-08-vortex-riscv-gpgpu-graphics-paper-reading.md
learning/notes/2026-06-28-current_status_and_decision_map.md
learning/notes/2026-06-29-git-merge-restore-index-worktree.md
learning/notes/2026-05-22-pixel_renderer_next_trusted_pipeline_plan.md
learning/notes/2026-05-22-rendering_systems_learning_record_and_plan.md
learning/notes/2026-05-22-pixel_renderer_architecture_and_learning_roadmap.md
learning/notes/2026-05-22-pixel_renderer_debug_testing_architecture.md
learning/notes/2026-05-22-pixel_renderer_hidden_spine.md
```

Tutorial tracks:

```text
learning/software-renderer/
learning/cpp/
```

## Writing Style

依問題選擇需要的深度. 可以使用:

```text
Question
Context
Concrete Case
Naive Model
Failure Point
Mechanism
Math / Geometry
Code Boundary
Dry Run / Trace
Tradeoffs
Current Conclusion
Open Questions
```

這不是 mandatory template. 短問題可以短, 完整教學則應保留讓理解成立所需的背景、diagram、state snapshot 與推導.

當內容成熟成 stable project rule 時, 把結論整理到適當的 `docs/foundations/`, `docs/architecture/`, `docs/verification/`, `docs/roadmap/`, 或 `docs/adr/`. 不需要刪除原始 learning note.
