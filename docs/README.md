# Pixel-Renderer Docs

這個目錄分成兩種文件：

1. 已有的長篇教學與討論紀錄，例如 `tutorial-soft-renderer/`、`tutorial-cpp/`、`notes/`。
2. 後續要沉澱成穩定規格的短文件，例如 `foundations/`、`architecture/`、`verification/`、`mapping/`、`roadmap/`、`adr/`。

建議閱讀入口：

```text
HANDOFF.md
PROJECT_MAP.md
ARCHITECTURE.md
DEVELOPMENT.md
  -> foundations/
  -> architecture/
  -> verification/
  -> mapping/
  -> roadmap/
  -> adr/
```

## Directory Map

```text
docs/
  HANDOFF.md                 cross-session handoff and next-step checklist
  PROJECT_MAP.md              project-level overview
  ARCHITECTURE.md             short/mid/long technical target architecture
  DEVELOPMENT.md              git, branch, commit, experiment workflow
  README.md                   this file

  foundations/                conventions, math mapping, pipeline vocabulary
  architecture/               renderer core, display backend, command queue, UI
  verification/               tests, debug views, traces, golden images
  mapping/                    commercial engine mapping and career mapping
  roadmap/                    milestone planning and execution order
  adr/                        architecture decision records

  tutorial-soft-renderer/     graphics and rasterization teaching track
  tutorial-cpp/               C++ and engineering teaching track
  notes/                      dated discussion records and long-form notes
```
