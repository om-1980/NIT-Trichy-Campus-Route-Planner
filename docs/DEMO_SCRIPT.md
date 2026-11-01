# Five-minute demonstration script

**Setup (before the panel arrives):**

1. Open a PowerShell terminal in the project folder.
2. Run `.\build.bat`, then `.\run_tests.bat`, and confirm that it prints **ALL TESTS PASSED**.
3. Start the planner with `.\build\route_planner.exe` and leave it at the menu.
4. Use a large terminal font.

The expected outputs below come from `results/demo_transcript.txt`, which was
recorded with the same inputs (`scripts/demo_inputs.txt`).

**Say first:** "This dataset is synthetic. It is generic campus layout data
used to show every case, not real NIT Trichy distances."

| # | Time | Type | What appears | What to say |
|---|---|---|---|---|
| 1 | 0:00 | `1` | 10 places. Place 1 = Main Gate → node [101]. Place 10 = Remote Research Station → node [130]. | "Places have menu IDs that are separate from graph-node IDs. Each place maps to an entrance node." |
| 2 | 0:30 | `2`, `1`, `3` | **OK 590.0 m, 4 segments.** E01, E02, E03, E22 (E22 is **stairs**). Search: 11 vertices settled, 32 entries scanned. | "Dijkstra with a binary min-heap. Each output line shows the physical edge ID, the nodes and the length." |
| 3 | 1:00 | `3`, `1`, `3` | Step-free: **608.0 m**, E01, E02, E04, E05. | "Step-free mode excludes stairs and unverified paths, so the route is 18 m longer but has no steps." |
| 4 | 1:30 | `5`, `E22` | "E22 … is now CLOSED **in both directions**. Graph version 2." | "Both directions share one physical-edge record, so a single flag closes both." |
| 5 | 1:45 | `2`, `1`, `3` | **608.0 m** via E04, E05. Graph version 2. | "The route is recomputed immediately. Closing the path added 18 m." |
| 6 | 2:10 | `5`, `E05`, then `5`, `E24` | Versions 3 and 4 | "Now I close the other two library approaches." |
| 7 | 2:30 | `2`, `1`, `3` | **Status: NO_ROUTE** | "Instead of returning a wrong route, the planner reports that no route exists." |
| 8 | 2:45 | `6`, `E22` then `2`, `1`, `3` | Reopened (version 5) → **590.0 m** again | "Reopening restores the original shortest distance." |
| 9 | 3:10 | `7` | "2 edge status(es) restored. Graph version 6." | "Reset returns every path to its status in the data file." |
| 10 | 3:20 | `2`, `1`, `10` | **NO_ROUTE** to the Research Station | "This place is in a separate component of the graph." |
| 11 | 3:35 | `2`, `3`, `3` | **0.0 m, 0 segments**, same location | "When the source equals the destination, the answer is the single location." |
| 12 | 3:45 | `2`, `1`, `99` | "Invalid destination: enter a place ID from option 1." | "Invalid input is rejected without crashing." |
| 13 | 4:00 | `2`, `8`, `7` then `2`, `7`, `8` | 382.0 m via **E23** (one-way); reverse **491.0 m** without E23 | "E23 is one-way, so the two directions give different routes." |
| 14 | 4:30 | `8` | Summary: 22 nodes, 31 physical edges (30 two-way, 1 one-way), 61 adjacency entries, 0 errors, 0 warnings | "Validation counts. 61 entries = 2 × 30 + 1." |
| 15 | 4:45 | `0` | Goodbye | "The test suite runs 61 automated tests, all passing, plus 154,985 Bellman–Ford comparisons." |

**Optional extras if time allows:**

* `9`, `101`, `555` → `INVALID_DESTINATION: no graph node with ID 555.`
* `4` → the full edge table.

**Backup plans:**

* If the live .exe cannot run (no compiler, or a security block), open
  `results/demo_transcript.txt` and `docs/TEST_RESULTS.md`.
* To replay the whole demo automatically:
  `cmd /c "build\route_planner.exe data/demo < scripts\demo_inputs.txt"`
