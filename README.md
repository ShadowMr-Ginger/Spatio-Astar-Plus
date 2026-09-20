# Spatio A* Plus — Multi-AGV Spatio-Temporal A* Scheduling with a Web Demo Platform

A multi-AGV path planning and cargo dispatching algorithm built on **Spatio-Temporal A\***, together with a full-stack demo platform that runs it online. The algorithm prototype is C++ competition code (Huawei Software Elite Challenge, port dispatch track); this repository simplifies and ports it to a C# scheduling engine, with a Next.js frontend covering the full workflow: map generation → upload → scheduling → animated playback → schedule export.

**Demo: https://jianqiaoxu.xyz/spatio-astar**

[中文指南](中文指南/README.md)

---

## Features

- **One-click random map generation**: density-parameterized, guaranteed-connected valid task maps (CSV), auto-loaded into the preview, with optional manual download
- **Run the algorithm online**: upload a CSV map and get a coordinated multi-AGV schedule from the backend
- **Animated playback**: frame-by-frame Canvas rendering with 1/3-substep interpolation for smooth movement; play/pause, step-through, timeline scrubbing, 0.5x–4x speed
- **Camera controls**: wheel zoom anchored at the cursor (0.5x–8x), drag-to-pan, double-click to reset
- **Performance insights**: estimated solve time (X~Y s) shown before running, a live elapsed timer during the request, and result stats (makespan / moves / delivery rate / elapsed ms)
- **Schedule export**: full schedule JSON and per-frame action CSV
- **Bilingual UI**: defaults to the browser language (Chinese or English), manually switchable
- **Rules page**: explains the map format, CSV cell values, and task rules

## Map Format (CSV)

A rectangular grid, no header, comma-separated values:

| Value | Meaning | AGV-passable |
|-------|---------|--------------|
| 0 | Path cell | ✔ |
| 1 | Obstacle | ✘ |
| 2 | Cargo | ✔ |
| 3 | AGV starting position | ✔ |
| 4 | Port (delivery destination) | ✔ |

**Task rules**: multiple AGVs start at their respective `3` cells and cooperate to deliver all `2` cargos to any `4` port; each AGV carries at most one cargo at a time (it must deliver to a port before picking the next one); the objective is to minimize the makespan (frames until all cargos are delivered).

## Tech Stack

| Layer | Technology |
|-------|------------|
| Algorithm engine | C# (.NET 8) — spatio-temporal A* with reservation tables, dynamic task assignment |
| Backend | ASP.NET Core 8 Web API, layered Controllers / Services / Core |
| Frontend | Next.js 16 + React 19 + Tailwind CSS v4 + Canvas 2D |
| Tests | xUnit (30+ cases) |

## Quick Start

Prerequisites: .NET SDK 8, Node.js 18+

```bash
# 1. Start the backend (port 5080)
cd platform/backend/src/SpatioAstar.Api
dotnet run --urls http://localhost:5080

# 2. Start the frontend (port 3000, /api proxies to 5080)
cd platform/frontend
npm install
npm run dev
```

Open http://localhost:3000 in a browser.

## API Overview

| Method | Path | Description |
|--------|------|-------------|
| GET | `/api/maps/generate?width=&height=&agvCount=&cargoCount=&portCount=&obstacleRatio=` | Generate a random valid map, returned as a CSV attachment |
| POST | `/api/schedules/run` (multipart form, field `file`) | Upload a map and compute a schedule; returns per-frame JSON |
| GET | `/api/schedules/estimate?cargoCount=&openCount=&agvCount=` | Estimated solve time range `{minMs, maxMs}` |

Errors return 400 with `{ "errors": ["..."] }`, distinguishing three causes: statically unreachable, solve budget exhausted, and conflict-infeasible.

## Algorithm Notes

### Spatio-Temporal A* and Conflict Avoidance

- Search state is the 3-tuple `(x, y, t)` with four-directional moves and wait actions; the heuristic is a multi-source BFS distance field (admissible, with static dead-end pruning)
- **Reservation table**: already-planned paths occupy vertices and directed edges over time; later-planned AGVs yield to earlier ones, eliminating vertex conflicts and same-frame edge swaps
- Each AGV's start cell is shielded from others from activation time onward, constructively eliminating return-home docking conflicts

### Dynamic Task Assignment

- Two strategies combined and best-of selected (16 combinations, minimum makespan wins):
  - **DynamicNearest**: each round assigns a cargo to the *earliest-available* AGV — efficient AGVs keep taking new tasks, balancing load naturally
  - **StaticQuota**: static quota baseline, serves as fallback
- DynamicNearest reduces makespan by 16%–43% versus static assignment in benchmarks
- Solve-time guardrails: 2M expansions as the primary budget (load-independent), 50 s hard fuse, deterministic exit on pathological maps

### Solve Performance

| Scenario | End-to-end time |
|----------|-----------------|
| 30×30 / 100 cargos / 10 AGVs (high density) | ≈ 3.8 s |
| 60×60 / 100 cargos / 10 AGVs | ≈ 0.85 s |

## Repository Layout

```
├── main.cpp                  # Original C++ competition algorithm (final version)
├── 往期版本/                  # Archive of earlier iterations
└── platform/
    ├── backend/
    │   ├── src/SpatioAstar.Core/    # Engine: map parse/generate, spatio-temporal A*, scheduler
    │   ├── src/SpatioAstar.Api/     # ASP.NET Core Web API
    │   └── tests/SpatioAstar.Core.Tests/
    └── frontend/
        ├── src/app/                 # Home page + /rules
        ├── src/components/          # Canvas, player controls, control panel
        └── src/lib/                 # API client, i18n, map CSV, render utils
```

## Tests

```bash
cd platform/backend && dotnet test
```

Coverage: map parsing/validation, generator validity (connectivity and cell counts), schedule correctness (full delivery, no vertex/swap conflicts, frame continuity, carrying-state consistency), failure semantics, and performance budgets.

## History

The algorithm prototype was originally written for the 2024 Huawei Software Elite Challenge port-dispatch problem (single-file C++, including cargo value/expiry and boat-berth scheduling). The demo platform drops value/expiry/ships and returns to pure path planning, while the core spatio-temporal A* conflict-avoidance mechanism is fully preserved and ported to C#.

## Maintenance Log - 2026-09-20

- Updated by `README Maintainer` at 14:35:31.
- Repository health check passed.
