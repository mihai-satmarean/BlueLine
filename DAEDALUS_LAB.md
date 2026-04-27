# Daedalus Lab — Blueprint Graph Beautifier Study
**Date**: 2026-04-27  
**Ticket**: special-agent#41 (Brancusi engine plugin pilot)  
**Goal**: Choose a base for developing a unified Blueprint graph beautifier for HoloTalks UE 5.7 projects.

---

## Candidates Evaluated

| Candidate | Source available | License | UE 5.7 | Maintenance |
|---|---|---|---|---|
| **BlueLine** | Full C++ on GitHub | MIT | Yes (declared `5.7.0`) | Active (Jan 2026) |
| **Auto Node Arranger** | Documentation only | Closed/Marketplace | UE4 era, unknown | Stale (June 2024) |
| **Blueprint Assist** | Closed | Paid | Yes | Active |
| **Graph Formatter** | GitHub fork available | MIT | Max 5.4 | Abandoned |

---

## Verdict

**Base: BlueLine.** Everything else is either closed, outdated, or paywalled.

Fork: `mihai-satmarean/BlueLine`  
Clone: `blueprint-beautify-blueline/` in `unreal_unified_mcp`

---

## BlueLine — What Already Exists (MIT, ready to use)

### Module: `BlueLineGraph`

| Class | Command | What it does |
|---|---|---|
| `FBlueLineFormatter` | Shift+Q | Soft format: aligns selected nodes by X-spacing + top-align to parent. Exec pins take priority over data pins. Collision pass (2x) resolves overlaps. |
| `FBlueLineGraphCleaner` | Shift+C | Full graph clean: Sugiyama layered layout + **Evolutionary Algorithm** for crossing minimization. |
| `FBlueLineManhattanRouter` | Shift+R | "Rigidify wires": replaces diagonal spaghetti with Manhattan L-path using `UK2Node_Knot` reroute nodes. |
| `FBlueLineWireSnapper` | on drag | Magnetic wire snapping: pins snap to nearby pins when dragging. |
| `FBlueLineConnectionPolicy` | visual | Custom pin/wire drawing (pin decoration, colour). |

### Module: `BlueLineCore`

| Class | What it does |
|---|---|
| `FBlueLineGraphAnalyzer` | Analyzes a graph: counts wire crossings, detects execution chains, data clusters, isolated nodes. Outputs `ComplexityScore` (0-100) and `ReadabilityScore`. Detects issues and generates suggestions. |
| `FBlueLineContextUtils` | Gets the focused `SGraphPanel` / active `UEdGraph` — shared util. |
| `UBlueLineEditorSettings` | Editor Preferences: HorizontalSpacing, VerticalSpacing, MagnetDist, GridSnapSize, CollisionPadding, bEnableAutoFormat. |

### Module: `BlueLineSmartTags`

GameplayTag-aware node tagging system. Not immediately relevant to beautify work, but useful for categorising nodes by domain (e.g., `holotalks.ai.stt`, `holotalks.audio.tts`).

---

## Auto Node Arranger — What to Port (algorithm, not code)

The GitHub repo has **no source code** (documentation only). However, the Readme documents the algorithms well enough to implement them as a new formatter class inside BlueLine.

### Features worth porting

#### 1. Multi-mode Layout: Straight / Center / Compact

AutoNodeArranger's 3 modes map to Y-alignment strategies:

| Mode | Y-alignment strategy | Shortcut (ANA) | Target shortcut |
|---|---|---|---|
| **Straight** | All nodes in exec chain aligned to same Y (top-align) | Shift+Q | Already in `FBlueLineFormatter` (partial) |
| **Center** | Node Y = parent_Y + (parent_height/2) - (self_height/2) | Shift+X | **TO IMPLEMENT** |
| **Compact** | Stack data-pin nodes vertically, minimize vertical spread | Shift+V | **TO IMPLEMENT** |

Implementation target: new class `FBlueLineMultiModeFormatter` with enum `EFormatMode { Straight, Center, Compact }`.

#### 2. Select Connected Graph (Shift+F)

Selects all nodes reachable from the current selection via any pin connection (BFS/DFS).  
`FBlueLineGraphAnalyzer` already does chain/cluster detection — expose as a select action.

#### 3. Reroute Node Arrangement

When a reroute (knot) node exists inside an exec chain, include it in the layout pass (currently `FBlueLineFormatter` only casts to `UEdGraphNode` — knots are included but not specially handled).  
Port: lock reroute Y to the wire Y it belongs to, avoid pushing it off-axis.

#### 4. Custom Spacing per Graph Type

ANA stores separate spacing configs per graph type (BlueprintGraph, AnimGraph, BehaviorTree, etc.).  
BlueLine currently has a single global setting. Port: extend `UBlueLineEditorSettings` with a `TMap<FName, FBlueLineGraphSpacing>` per graph schema class name.

---

## Blueprint Assist — Features to Implement Free (skip the paid plugin)

From the public docs at [blueprintassist.github.io](https://blueprintassist.github.io/):

| BA feature | Implementation in BlueLine |
|---|---|
| Auto-format on new node added | Hook into `UEdGraph::OnGraphChanged` delegate; call `FBlueLineFormatter::AutoAlignSelectedNodes` on the new node's connected subgraph |
| Format Compact / Expanded style | Map to Compact / Straight modes in `FBlueLineMultiModeFormatter` |
| Helix parameter stacking | In Compact mode: stack data-input nodes vertically to the left of the exec node they feed |
| Keyboard shortcut customization | Already in BlueLine via `FBlueLineCommands` |

---

## Development Backlog (priority order)

| Priority | Task | Class target |
|---|---|---|
| P0 | Port ANA Center + Compact layout modes | `FBlueLineMultiModeFormatter` (new) |
| P0 | Expose "Select Connected Graph" as editor action | extend `FBlueLineCommands` + BFS in `FBlueLineGraphAnalyzer` |
| P1 | Auto-format on node creation (BA parity) | `FBlueLineGraphModule` — hook `OnGraphChanged` |
| P1 | Per-graph-type spacing config | extend `UBlueLineEditorSettings` |
| P2 | Helix stacking for data pins | `FBlueLineMultiModeFormatter` Compact mode |
| P2 | Reroute node lock-to-wire in layout pass | `FBlueLineFormatter` |
| P3 | SmartTags for HoloTalks domains | `BlueLineSmartTags` module — add HoloTalks tag set |

---

## Upstream Contribution Strategy

Changes that are generic (multi-mode formatter, per-graph spacing, select connected) should be  
proposed back to `gregorik/BlueLine` as PRs. HoloTalks-specific features (SmartTags for `holotalks.*`) stay in the fork.

---

## Folder Structure

```
unreal_unified_mcp/
├── blueprint-beautify-blueline/   ← fork mihai-satmarean/BlueLine (base)
│   ├── DAEDALUS_LAB.md            ← this file
│   ├── Source/
│   │   ├── BlueLineCore/
│   │   ├── BlueLineGraph/         ← main beautify logic
│   │   └── BlueLineSmartTags/
│   └── BlueLineCore.uplugin       ← EngineVersion: 5.7.0
│
└── blueprint-beautify-autonode/   ← fork mihai-satmarean/AutoNodeArranger
                                      (docs reference only — no source code)
```
