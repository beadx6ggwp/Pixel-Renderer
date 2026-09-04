# Pixel-Renderer Learning Workspace

`learning/` 是 Pixel-Renderer 的學習空間. 它保存完整推導、tutorial、paper reading、historical reasoning、naive examples 與 small labs.

這個目錄直接存在於 `main`. 寫 learning material 不需要特別建立 branch.

## AI Collaboration

在這裡, AI 預設是 tutor, coach, professor, reviewer, debugger, and research partner first. Coding agent second.

AI 應依目前問題調整教學方式, 而不是強迫每次使用固定模板. 視需要使用:

```text
concrete case
  -> naive model
  -> failure point
  -> deeper mechanism
  -> math / geometry
  -> code / engineering consequence
```

也可以使用 Prediction, minimal experiment, Dry Run, ASCII diagram, counterexample, trace, code review, 或 engine/hardware mapping. 這些是教學工具, 不是使用者必須完成的流程.

使用者可以自由探索 rasterization, graphics math, C++, GPU architecture, FPGA, UI, tooling, browser rendering, 或 commercial engines. Roadmap 提供方向與 dependency, 不限制可以學什麼.

## Directory Map

```text
learning/
  README.md

  notes/
    dated notes, readings, reasoning records, and topic explorations

  cpp/
    C++ theory and renderer-oriented implementation tutorial track

  software-renderer/
    first-principles software renderer tutorial track
```

需要新的 topic folder 時再建立. 不必預先做出完整 curriculum hierarchy.

## Learning vs Stable Project Truth

```text
learning/
  explains why
  may preserve naive approaches, alternatives, and historical context

docs/
  states what the current project can rely on
  owns stable conventions, architecture boundaries, verification, roadmap, and ADRs

src/ + tests/ + examples/
  show what the formal renderer currently implements and verifies
```

Learning material 可以引用 current source, 但不要只靠舊 note 斷言目前行為. 如果 source、stable docs 與舊 note 不一致, 優先檢查 source 和 git state.

## Learning Notes And Labs

Learning entry 可以很自由:

```text
a short question
a complete teaching chapter
a failed attempt
a paper reading
a comparison table
a tiny console program
a deliberately naive implementation
a frame-by-frame Dry Run
```

不要求每篇都 polished, 也不要求每個問題變成正式功能.

如果內容成熟成 durable convention 或 architecture decision, 把結論整理進 `docs/`. 完整推導仍可保留在這裡.

## When A Branch Is Useful

Branch 是 code-state isolation tool, 不是 learning category.

以下情況才考慮 branch:

```text
the work modifies formal source in incompatible ways
the experiment may temporarily break the build
two implementations need side-by-side repository states
the work needs independent review or a multi-day integration path
```

單純新增 note、tutorial、reading 或 self-contained lab, 通常直接放進 `learning/` 即可.

## Entry Points

```text
learning/notes/README.md
learning/cpp/index.html
learning/software-renderer/index.html
```
