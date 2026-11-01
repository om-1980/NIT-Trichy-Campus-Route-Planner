# Second-review slides (10 slides, about 8–10 minutes)

The bullets are for the slides and the notes are for the speaker. Every number comes
from `results/`. Text in [brackets] must be checked or filled in before presenting.

---

## Slide 1: Smart Campus Route Planner Using Graphs and Dijkstra's Algorithm

* ADSA project, second review, 2 November 2026
* [Name: Vivek Kumar, Roll No: 306426001 (from the proposal; confirm)]
* **Objective:** compute the shortest permitted walking route between campus places,
  handling closed paths, one-way paths, step-free requests and unreachable places

*Notes:* State the objective in one sentence. Mention that the target campus is NIT
Trichy and that today's demo uses a synthetic dataset. Slide 5 explains why.

---

## Slide 2: Problem and need

* Visitors and new students know building names, not routes
* Choosing a route by memory, or by counting buildings passed, can give a longer route
* Paths can be **closed** (construction, events), **one-way**, or have **stairs**
* The system must never suggest a closed path, and must say clearly when no route exists

*Notes:* The two parts of the problem are representing the campus correctly and
finding the shortest route that satisfies the current constraints.

---

## Slide 3: Literature recap and limitations

| Work | Idea | Limitation for this project |
|---|---|---|
| Dijkstra 1959, *Numer. Math.* 1:269–271 | Grow a set of vertices with final shortest distances | Non-negative weights only; one fixed graph per run; no campus attributes |
| Hart, Nilsson, Raphael 1968, *IEEE TSSC* 4(2):100–107 (A*) | f = g + h; optimal if h never overestimates | Needs trustworthy coordinates; does not model accessibility or closures |
| Koenig, Likhachev, Furcy 2004, *Artif. Intell.* 155:93–146 (LPA*) | Reuse search results after edge changes | Extra state and complexity; benefit depends on what changes; one fixed start and goal |

*Notes:* The citation details were checked against Crossref. Say: "These are general
algorithms. None of them defines how to model a campus, which is our focus."

---

## Slide 4: Architecture and contribution

* Pipeline: **M1** load/validate → **M2** adjacency → **M4** path status →
  **M3** request → **M5** filter → **M6** Dijkstra → **M7** reconstruct. **M8** handles tests and measurements offline.
* Contribution: **campus-specific modelling and closure-aware routing using existing
  algorithms**. It is *not* a new shortest-path algorithm.
  * One physical-edge record shared by both directions, so one flag closes both
  * Places map to entrance (approach-point) nodes
  * Step-free mode, with equal distances broken by fewer segments
  * Strict data validation, an independent oracle and closure-impact analysis

*Notes:* Show the block diagram from `REVIEW2_REPORT.md` §8. Be explicit that
Dijkstra and skipping closed edges are standard.

---

## Slide 5: Dataset and graph model

* G = (V, A). Nodes are junctions and entrances. Each physical path is one edge record.
  A two-way path gives two adjacency entries that both point to that record.
* Files: `nodes.txt`, `edges.txt` (distance in m, direction, status, step-free, type), `places.txt`
* **Demo data is synthetic, not real NIT Trichy geography:** 22 nodes,
  31 physical edges (1 one-way), 61 adjacency entries, 10 places, 2 components,
  0 errors and 0 warnings
* Built-in cases: a stairs shortcut, a one-way exit, a disconnected place, a path
  closed in the data file, and an equal-distance tie

*Notes:* Say this directly: "The NIT Trichy data and the specific proposal were not
available when this was built. The proposal mentions 245 vertices and 329 edges, but
those numbers have not been verified, so I am not presenting them as results. The
data format and the `--validate` command are ready for verified campus data."

---

## Slide 6: Implemented modules and progress evidence

| Module | Status | Evidence |
|---|---|---|
| M1–M7 core | Implemented & tested | 61 automated tests |
| M8 tests and oracle | Executed | 61/61 pass; 154,985 oracle pairs agree |
| M8 measurements | Executed | CSVs in `results/` |
| Verified NIT Trichy data | Not started | Source data needed |
| Visualization, final comparison | Planned / partial | Final review |

* Completion by stated weights: **73/100**. Weights are our own planning estimate; see the report §3.2.

*Notes:* Explain that the percentage is calculated from defined weights and counts
only modules with test evidence.

---

## Slide 7: Working demonstration

* Live: Gate → Library **590 m** (uses the stairs E22). Step-free: **608 m**.
* Close E22, giving **608 m**. Close E05 and E24, giving **NO_ROUTE**. Reopen E22, giving **590 m**.
* One-way E23: Sports → Hostel **382 m**; Hostel → Sports **491 m**
* Invalid place, same place (0 m), disconnected place (NO_ROUTE)

*Notes:* Follow `DEMO_SCRIPT.md`. Keep `results/demo_transcript.txt` open as a backup.

---

## Slide 8: Test cases and actual outcomes

* **61 tests executed: 61 passed, 0 failed, 0 not run**
* Covered: normal route; multi-hop shorter than direct; same endpoints; invalid IDs;
  unreachable; closure and alternative; close until no route; reopen; two-way
  closure; one-way; ties; zero weight; parallel paths; step-free; 20 invalid-data
  rejections
* The oracle is Bellman–Ford, built independently from the edge records. A
  route-validity checker covers endpoints, direction, open status, sum and no repeated node.
* Mutation check: deliberately broken versions of the router are caught (for
  example, a max-heap fails 24 tests and ignoring closures fails 19)

*Notes:* Show 4–5 rows from `TEST_RESULTS.md`, such as T01, T09, T11, T14 and T30.

---

## Slide 9: Evaluation metrics and available measurements

| Metric | Value |
|---|---|
| Test pass rate | 61/61 (100 %) |
| Agreement with Bellman–Ford | 154,985/154,985 pairs (100 %) |
| Route validity rate | 98,112/98,112 OK routes (100 %) |
| Search time, demo (median) | ≈ 0.7 µs per query |
| Settled vertices, demo (mean) | 11.5 of 22 |
| 100,489-node grid, early stop | ≈ 8.3 ms median; 48,001 settled |
| Closure impact, Gate→Library | +18 m → +672 m → NO_ROUTE |

* Environment: i5-12400F, Windows 11, g++ 15.2 -O2. Times vary between runs
  (0.64 vs 1.06 µs for the same query), but the counters are deterministic.
* Process memory **not measured**. The complexity is O(V + A) space and O(V + A log(1 + A)) time.

*Notes:* Explain that settled vertices and scanned entries are machine-independent.
Unreachable pairs are recorded as a status, never as a distance error.

---

## Slide 10: Remaining work and final-review plan (23 Nov)

* Verified NIT Trichy dataset (OSM export and/or survey, with ODbL attribution if OSM is used); report the actual counts
* Real-campus expected routes and an oracle sweep; closure-criticality analysis
* Comparison: A* (straight-line heuristic) vs Dijkstra vs Bellman–Ford; optional DIMACS NY for scale
* Performance graphs with repeated-run spread; results analysis
* Optional: SVG map of the route on node coordinates

*Notes:* End with: "The core is complete and verified. The main remaining work is
real campus data and the final comparison."
