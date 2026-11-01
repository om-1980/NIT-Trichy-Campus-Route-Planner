# Smart Campus Route Planner (graphs + Dijkstra)

ADSA project: a C++17 terminal application. It models a campus as a weighted graph
of physical paths, finds shortest walking routes with Dijkstra's algorithm and a
binary min-heap, and handles path closures, one-way paths and a step-free mode.

> **Data notice:** the bundled dataset `data/demo/` is **synthetic**. Its names are
> generic and its geometry is invented. It is **not** real NIT Trichy geography. See
> [data/README.md](data/README.md) for the file formats and how to substitute
> verified campus data.

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

## Run the planner

```powershell
.\build\route_planner.exe                     # uses data/demo
.\build\route_planner.exe data/demo           # explicit data folder
.\build\route_planner.exe --validate data/demo   # load, validate, print counts, exit
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
