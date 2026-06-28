# Pixel-Renderer Handoff

Last updated: 2026-06-29

This file is a cross-machine / cross-session orientation note. It should preserve the broad project direction and durable decisions, not prescribe a rigid next task.

For detailed workflow, architecture, or implementation rules, follow the linked docs instead of expanding this file indefinitely.

For general documentation workflow and note organization, follow `docs/AGENTS.md`; do not expand this handoff into a full docs rulebook.

Read after opening a fresh session:

```text
AGENTS.md
README.md
docs/README.md
docs/PROJECT_MAP.md
docs/ARCHITECTURE.md
docs/DEVELOPMENT.md
docs/foundations/rendering_conventions.md
docs/foundations/rasterization_edge_rules.md
```

---

## 1. Project Framing

Pixel-Renderer should be treated as a `Rendering Systems Lab`.

The central learning loop is:

```text
build a concept directly in Pixel-Renderer
  -> inspect or reproduce the same concept in a commercial / production-style engine
  -> document the mapping between mechanism, abstraction, workflow, tooling, and tradeoffs
```

This is the `Pixel-Renderer + Engine Mirror Demo` strategy.

The project should remain open-ended. FPGA, software GPU, Filament-style renderer architecture, Unity / Unreal engine fluency, UI/tooling, and technical art are all possible branches. The shared core is the ability to implement, test, debug, and explain each rendering layer.

---

## 2. Durable Direction

Current high-level direction:

```text
build a trustworthy renderer core
preserve clear ownership and data-flow boundaries
make behavior observable through tests, traces, or debug views
connect low-level renderer work to real engine abstractions
avoid adding architecture only because it sounds engine-like
```

Important architectural boundary:

```text
renderer core calculates pixels
Framebuffer owns pixel/depth memory
RenderDevice writes Framebuffer
DisplayBackend presents Framebuffer
```

Important learning boundary:

```text
Vulkan is a useful reference for precise post-projection conventions.
Vulkan API architecture is reference material, not an implementation template.
```

---

## 3. Current Code Context

The code is still an early Win32 software renderer prototype. Verify the current source before relying on this snapshot.

Known current shape:

```text
Application / ScreenManager / RenderDevice / Rasterizer
Win32 window + DIBSection framebuffer
Clear / SetPixel
Bresenham-style DrawLine
bounding-box / barycentric DrawTriangle
```

Likely missing or incomplete areas:

```text
owned Framebuffer
depth buffer
edge-function rasterizer
tests
debug views
MVP / viewport / IShader / material / renderer skeleton
DisplayBackend abstraction
```

This snapshot should orient the next session, not replace reading the code.

---

## 4. Current Docs / README Context

Recent docs work adjusted the project entry points.

Current `README.md` intent:

```text
stay close to the original compact project-intro style
describe what Pixel-Renderer does, not the user's personal background
explain current prototype state and near-term plan
include simple current/future ASCII architecture diagrams
mention SDL / Win32 display backend switching as planned work, not current source state
avoid claiming depth, MVP, IShader, Material, Renderer, or DisplayBackend are already implemented
```

Current docs workflow intent:

```text
docs/README.md is the normal docs directory entry
docs/AGENTS.md is the docs subtree workflow rule
docs/HANDOFF.md is only for cross-machine / cross-session / stale-context orientation
rough ideas and learning traces start in docs/notes/
durable decisions are extracted later into stable topic docs or ADRs
```

Most recent current-state analysis:

```text
docs/notes/2026-06-28-current_status_and_decision_map.md
```

That note is still rough analysis, but it is the best single document for understanding why the recommended next source branch is `render/raster-baseline`, with `test/raster-core` style tests folded into it.

---

## 5. Key Docs

Primary project map:

```text
docs/PROJECT_MAP.md
```

Technical target architecture:

```text
docs/ARCHITECTURE.md
```

Development workflow:

```text
docs/DEVELOPMENT.md
```

Rendering conventions:

```text
docs/foundations/rendering_conventions.md
```

Rasterization edge rules:

```text
docs/foundations/rasterization_edge_rules.md
```

Dated records:

```text
docs/notes/
```

Documentation workflow:

```text
docs/AGENTS.md
```

Use these as durable context instead of relying on chat memory.

---

## 6. Preserved Decisions

Rendering convention:

```text
Vulkan-inspired, learning-first
NDC x/y: [-1, 1]
NDC z: [0, 1]
screen origin: top-left
screen y: down
depth: [0, 1], smaller is closer
```

Rasterization convention:

```text
sample triangles at pixel centers
prefer edge-function coverage for triangle fill
use deterministic shared-edge rules
start with clarity before fixed-point / hardware-style optimization
```

Git workflow:

```text
use engineering branch names
keep main readable and preferably buildable
prefer rebase + fast-forward merge for clean personal branches
use exp/* as disposable experiment branches
extract useful experiment results into docs, tests, or clean feature branches
```

---

## 7. Choosing The Next Task

Do not treat this handoff as a command queue. Choose the next task by asking what the current session needs most:

```text
correctness:
  improve raster / line / triangle / depth behavior

observability:
  add tests, trace dumps, debug views, or deterministic examples

architecture clarity:
  clarify framebuffer ownership, display backend, resource lifetime, or command boundaries

learning bridge:
  map a completed Pixel-Renderer concept to Vulkan / Unity / Unreal / Filament-style abstractions

experimentation:
  prototype a small idea in exp/* and extract only useful results
```

The strongest near-term center of gravity is still the trusted raster core.

Recommended default branch:

```text
render/raster-baseline
```

Recommended implementation style:

```text
small testable raster helpers
edge-function coverage
pixel-center sampling
top-left shared-edge rule
color interpolation
depth interpolation
depth buffer and depth test
simple debug views or deterministic demos
```

Architecture or tooling work is valid when it directly supports correctness, observability, or future implementation clarity. If depth ownership, tests, or headless output become awkward, split a small `arch/render-target` branch for owned `Framebuffer` / `RenderTarget` before broad `DisplayBackend` work.

---

## 8. Windows Startup

When reopening on Windows:

```bash
git status --short --branch
git pull --ff-only
git log --oneline --decorate -5
```

Then read the relevant docs and inspect the current source:

```bash
rg -n "DrawLine|DrawTriangle|SetPixel|Framebuffer|Vertex" src
```

GitHub authentication may differ by machine:

```text
Mac session used SSH over port 443.
Windows may use GitHub Desktop / HTTPS credentials or its own SSH key.
Either is fine if push/pull works reliably.
```

Before making code changes, choose a branch name that describes the engineering intent and keep the branch scope small enough to explain.

---

## 9. If Context Conflicts

Prefer current source code over older planning notes.

Prefer stable docs over chat memory:

```text
README.md
docs/README.md
PROJECT_MAP.md
ARCHITECTURE.md
DEVELOPMENT.md
rendering_conventions.md
rasterization_edge_rules.md
HANDOFF.md
```

If a future session changes a durable direction, update the relevant doc instead of leaving the decision only in the chat.
