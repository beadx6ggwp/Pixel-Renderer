# Pixel-Renderer Handoff

Last updated: 2026-05-28

This file is a cross-machine / cross-session orientation note. It should preserve the broad project direction and durable decisions, not prescribe a rigid next task.

For detailed workflow, architecture, or implementation rules, follow the linked docs instead of expanding this file indefinitely.

Read after opening a fresh session:

```text
AGENTS.md
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
implement a concept from first principles in Pixel-Renderer
  -> inspect or reproduce the same concept in a commercial / production-style engine
  -> document the mapping between mechanism, abstraction, workflow, tooling, and tradeoffs
```

This is the `Pixel-Renderer + Engine Mirror Demo` strategy.

The project should remain open-ended. FPGA, software GPU, Filament-style renderer architecture, Unity / Unreal engine fluency, UI/tooling, and technical art are all possible branches. The shared core is first-principles rendering knowledge plus the ability to test, debug, and explain each layer.

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

## 4. Key Docs

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

Use these as durable context instead of relying on chat memory.

---

## 5. Preserved Decisions

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

## 6. Choosing The Next Task

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

The strongest near-term center of gravity is still the trusted raster core, but architecture or tooling work is valid when it directly supports correctness, observability, or future implementation clarity.

---

## 7. Windows Startup

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

## 8. If Context Conflicts

Prefer current source code over older planning notes.

Prefer stable docs over chat memory:

```text
PROJECT_MAP.md
ARCHITECTURE.md
DEVELOPMENT.md
rendering_conventions.md
rasterization_edge_rules.md
HANDOFF.md
```

If a future session changes a durable direction, update the relevant doc instead of leaving the decision only in the chat.
