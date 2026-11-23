# Smart Campus Route Planner (graphs + Dijkstra)

ADSA project: a C++17 terminal application. It models a campus as a weighted graph
of physical paths, finds shortest walking routes with Dijkstra's algorithm and a
binary min-heap, and handles path closures, one-way paths and a step-free mode.

> **Datasets:** 
> * `data/nitt/` is the **comprehensive, verified NIT Trichy campus dataset** containing **123 destinations, 188 graph nodes, and 212 physical paths (18.0 km of walkways and roads)** covering all hostels, departments, workshops, library, messes, canteens, sports complex, and central amenities.
> * `data/demo/` is the compact synthetic dataset retained for automated regression tests (`run_tests.bat`).
> The application automatically defaults to `data/nitt/` when present.

## Requirements

* Windows 10/11 with **g++ supporting C++17**. Development used MinGW-w64 GCC 15.2.0;
  any recent MinGW-w64 or MSYS2 g++ should work.
* `g++` on `PATH`. Check this with `g++ --version`.
* Python 3, only for the optional `scripts/csv_to_md.py`.
* No internet connection and no third-party libraries are needed.

## Build

Run these from the project folder in **PowerShell** (VS Code terminal):

```powershell
.\build.bat
```

In **cmd.exe**, use `build.bat`. The script creates `build\route_planner.exe`,
`build\run_tests.exe` and `build\benchmark.exe`. To build only the planner manually:

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -pedantic -static -Isrc -o build\route_planner.exe src\main.cpp src\graph.cpp src\router.cpp
```

`-static` bundles the C++ runtime. Without it, the .exe fails with
"libstdc++-6-x64.dll not found" when MinGW's `bin` folder is not on `PATH`.

## Interactive Map GUI

A visual, modern GUI is available in the [`gui/`](file:///d:/NIT%20Trichy%20Campus%20Route%20Planner/gui) folder. It features an interactive vector map of the 800-acre NIT Trichy campus with real-time shortest path visualization, smooth pan/zoom, step-by-step turn-by-turn navigation, accessibility mode, and live road closures.

### How to Launch the GUI:

1. **Option 1 (One-click batch launcher)**:
   ```powershell
   .\run_gui.bat
   ```
2. **Option 2 (Direct browser open)**:
   Double-click [`gui/index.html`](file:///d:/NIT%20Trichy%20Campus%20Route%20Planner/gui/index.html) in Windows File Explorer or open it in Google Chrome, Microsoft Edge, or Firefox.
3. **Option 3 (Local web server)**:
   ```powershell
   python -m http.server 8080
   ```
   Then open `http://localhost:8080/gui/` in your browser.

### GUI Features:
* **Interactive Vector Campus Map**: Smooth pan, drag, mouse wheel zoom, and reset-to-fit controls.
* **Point-and-Click Navigation**: Click any campus building or landmark to set as Start (A) or Destination (B).
* **Category Filters & Directory**: Browse all 123 places categorized by Hostels, Departments, Messes, Workshops & Labs, Sports, and Amenities.
* **Standard & Step-Free Modes**: Toggle between fastest routes and step-free routes that bypass staircases (like the Admin-Library footbridge).
* **Live Road Closures**: Click on any road/path on the map to toggle it closed or open; watch Dijkstra dynamically re-calculate alternative routes in real time.
* **Walking & Cycling Time Estimates**: See distance in metres, estimated walking time (at 1.2 m/s), and cycling time (at 4.2 m/s).

## Run the planner (Terminal CLI)

```powershell
.\build\route_planner.exe                     # defaults to data/nitt (full NIT Trichy)
.\build\route_planner.exe data/nitt           # explicit NIT Trichy campus dataset
.\build\route_planner.exe data/demo           # synthetic demo dataset (for tests)
.\build\route_planner.exe --validate          # validate default dataset (data/nitt)
.\build\route_planner.exe --validate data/nitt # validate NITT dataset, print counts, exit
```

Run these from the project folder, because the data path is relative. The menu is:

| Option | Action |
|---|---|
| 1 | List places (menu place ID → graph node) |
| 2 | Shortest route, standard mode (asks source and destination **place IDs**) |
| 3 | Shortest route, step-free paths only |
| 4 | List physical edges with direction, status, step-free flag and type |
| 5 / 6 | Close / reopen a path by edge ID (e.g. `E22`, case-insensitive) |
| 7 | Reset all edges to their status in `edges.txt` |
| 8 | Graph summary and data-validation report |
| 9 | Route between raw graph-node IDs (diagnostic; shows invalid node IDs) |
| 0 | Exit |

A scripted session, with its recorded output in `results/demo_transcript.txt`:

```powershell
cmd /c "build\route_planner.exe data/demo < scripts\demo_inputs.txt"
```

## Tests

```powershell
.\run_tests.bat
python scripts\csv_to_md.py        # optional: refresh docs\TEST_RESULTS.md from the CSV
```

`run_tests.bat` writes `results\test_results.csv` (one row per test) and
`results\correctness_metrics.csv`. The process exit code equals the number of failed
tests. Tests cover routing, closures, direction, ties, zero weights, step-free mode,
20 data-rejection cases, and an all-pairs comparison against an independent
Bellman–Ford reference. That comparison runs on the demo data and on 500 seeded
random graphs.

## Measurements

```powershell
.\run_benchmark.bat     # about 1 minute
```

The benchmark writes these files:

| File | Contents |
|---|---|
| `results\bench_demo_queries.csv` | every place pair × 2 modes: status, distance, search counters, timing |
| `results\closure_impact.csv` | named closure scenarios: route-distance increase or NO_ROUTE |
| `results\closure_single_edge_summary.csv` | for each open edge: how many place pairs get longer or disconnected |
| `results\bench_scaling.csv` | synthetic grid graphs from 100 to 100,489 nodes: early-stop Dijkstra, full Dijkstra, Bellman–Ford |
| `results\bench_environment.txt` | compiler, flags, timer, CPU, OS and timing method |

Only the search call is timed. Loading, printing and checking happen outside the
timed regions. Timings differ between machines and between runs, while the
counters are deterministic.

## Project layout

```
src/graph.hpp, graph.cpp       M1 load+validate, M2 adjacency, M4 path status, summary
src/router.hpp, router.cpp     M3 request check, M5 filter, M6 Dijkstra, M7 reconstruction
src/reference.hpp, .cpp        Bellman-Ford oracle + route validity checker (tests/bench only)
src/main.cpp                   terminal menu (controller)
tests/test_main.cpp            automated tests -> results/test_results.csv
bench/benchmark.cpp            measurements   -> results/*.csv
data/demo/                     synthetic dataset (nodes, edges, places, metadata)
scripts/demo_inputs.txt        menu inputs for the scripted demo
scripts/csv_to_md.py           test CSV -> docs/TEST_RESULTS.md
docs/                          review report, slides, demo script, viva prep, test table
results/                       outputs produced by the programs above
```

## Troubleshooting

* **`'g++' is not recognized`**: add your MinGW `bin` folder to `PATH` and reopen the terminal.
* **`cannot open file 'data/demo/nodes.txt'`**: run from the project folder, or pass
  the full path to the data folder.
* **The .exe is blocked by Windows** (Smart App Control, Device Guard or antivirus):
  this happened once on the development machine for a freshly compiled .exe, and a
  rebuild ran normally. Before the review, check on the presentation laptop that
  `build\route_planner.exe` starts. If it is blocked, ask the machine's administrator.
  Do not disable security features.
