# Second Review Report: Smart Campus Route Planner Using Graphs and Dijkstra's Algorithm

Review date: 2 November 2026. Final review: 23 November 2026.
All numbers below come from programs run on 1 November 2026. Where they came from:
`results/test_results.csv`, `results/correctness_metrics.csv`, `results/bench_*.csv`,
`results/closure_*.csv` and `results/demo_transcript.txt`.

---

## 1. Gap assessment

### 1.1 Inputs inspected

| Input | Found? | What it contains |
|---|---|---|
| Original one-page proposal (`ADSA Project Proposal.pdf`) | Yes | Weighted graph, adjacency list, Dijkstra with min-heap, menu, shortest path + distance + intermediate locations, no-route handling |
| Updated proposal (`ADSA_Campus_Route_Planner_Proposal.pdf/.docx`, 12 pages) | Yes | Modules M1–M8, closures, step-free mode, (distance, segments) label, Floyd oracle, a **generic synthetic 12-node campus**, OSM/DIMACS sources |
| NIT Trichy-specific proposal (245 vertices, 329 physical edges, 8 destinations) | **No** | Not on this machine, so none of these numbers can be checked |
| `ADSA_Campus_Dataset.zip` (referenced by the updated proposal) | **No** | Not found |
| Any NIT Trichy dataset (nodes/edges/OSM extract) | **No** | Not found |
| Existing source code | **No** | The project folder was empty |

### 1.2 Conflicts and how they were resolved

| # | Conflict | Decision |
|---|---|---|
| C1 | The NIT Trichy specification was not available. The only updated proposal found describes a generic artificial campus. | The generic updated proposal and the review brief were used as the specification. NIT Trichy counts (245/329/8) are treated as **unverified proposal claims** and are not repeated as facts. |
| C2 | The proposal specifies `nodes.json`/`edges.json` plus a 12-node dataset. That dataset is missing, and the review brief asks for `nodes.txt`, `edges.txt` and `places.txt`. | Pipe-separated text files were used, which need no JSON library and work offline. A new synthetic dataset was created and labelled as synthetic everywhere. The proposal's 12-node worked examples (e.g. 240 m and 260 m) are **not** reproduced, because their full edge list is unavailable. |
| C3 | The proposal uses integer metres. The brief asks for finite, non-negative distances with float tolerance. | Real-valued metres are accepted. Integer data, as in the demo, gives exact equal-distance tie handling. With fractional data, rounding can hide an exact tie, but the distance is still optimal. |
| C4 | The proposal uses a Floyd–Warshall oracle. The brief asks for Bellman–Ford. | Bellman–Ford over the same (distance, segments) labels was implemented. Floyd–Warshall was not implemented. |
| C5 | Proposal M4/M5 copy a snapshot and build a filtered graph per query. | The filter is applied while scanning (`edgeAllowed`). This gives the same result without an O(V+A) copy. The program is single-threaded, so the query reads the current status, and the graph version is reported. |
| C6 | The brief's core list leaves out step-free mode and the fewer-segments tie-break that the proposal calls its contribution. | Both are implemented (small cost) and tested (T16, T16b, T20, T21). |
| C7 | The proposal is silent on self-loops and has no `road` path type. | Self-loops are rejected, `road` was added, and parallel paths with different IDs are allowed (T19). |
| C8 | The proposal routes by node IDs. The brief separates menu place IDs from node IDs. | `places.txt` maps place IDs to approach-point node IDs. The demo uses 1–10 for places and 101–131 for nodes, so the two are never confused (T06). |

### 1.3 Remaining gaps

1. **No verified NIT Trichy data.** This is the largest gap. The demo runs on synthetic data.
2. No map visualization, which is planned for the final review.
3. No plotted performance graphs or comparison with A*, LPA* or DIMACS yet. Raw CSVs exist.

---

## 2. Second-review requirements checklist

| Requirement | Status | Evidence |
|---|---|---|
| ≥ 60 % implementation | **Met under the weights in §3** (73 % by weight) | Modules M1–M8 implemented and tested |
| Test cases | **Met.** 61 automated tests executed, 61 passed | `results/test_results.csv`, `docs/TEST_RESULTS.md` |
| Evaluation metrics defined | **Met.** 8 metrics defined, 7 measured | §6, `results/*.csv` |
| Working demonstration | **Met.** A scripted run was recorded | `results/demo_transcript.txt`, `docs/DEMO_SCRIPT.md` |
| Load and validate graph and place data | Implemented, tested | T00, T30–T51 |
| Weighted adjacency lists | Implemented, tested | T00, T11, T19 |
| Accept source/destination places | Implemented, demonstrated | Menu options 2/3, T06 |
| Dijkstra + min-priority queue | Implemented, tested | T01–T22, T52–T71 |
| Show route, node names, edge IDs, distance | Implemented, demonstrated | Transcript |
| Close/reopen individual paths | Implemented, tested | T08–T13 |
| Recompute after changes | Implemented, tested | T09, T10 |
| Invalid input, same endpoints, disconnected | Implemented, tested | T03–T07, transcript |
| Automated correctness tests + basic measurements | Implemented, executed | §5, §6 |

---

## 3. Module status and completion

### 3.1 Status by module

The status words mean the following. **Implemented & tested**: the code exists and
automated tests ran and passed. **Partial**: some code or results exist but not the
full deliverable. **Planned**: not started.

| ID | Module | Status | Evidence |
|---|---|---|---|
| M1 | Load and validate data (`graph.cpp`) | Implemented & tested | T00, T30–T51 (20 rejections, missing files, 1 warning case) |
| M2 | Build adjacency lists with shared physical-edge IDs | Implemented & tested | T00 (61 entries), T11 (2 entries share E22), T19 |
| M3 | Request validation and terminal menu (`main.cpp`) | Implemented & tested (API) and demonstrated (menu) | T03–T07; transcript |
| M4 | Close, reopen and reset paths, plus graph version | Implemented & tested | T08–T13 |
| M5 | Feasibility filter (closure, step-free) | Implemented & tested | T20, T21, sweeps T52–T63 |
| M6 | Dijkstra with binary heap, lazy deletion and (distance, segments) labels | Implemented & tested | T01–T22, T52–T71; mutation check (§5.3) |
| M7 | Route reconstruction and display | Implemented & tested | `checkRoute` on every OK route (98,112 checked) |
| M8a | Test suite + Bellman–Ford oracle + route checker | Implemented & executed | 61/61 pass |
| M8b | Measurement program, CSV export | Implemented & executed | `results/bench_*.csv`, `closure_*.csv` |
| D | Verified NIT Trichy campus dataset | **Not started (blocked: no source data)** | n/a |
| V | Map or graphical route visualization | **Planned** | n/a |
| F | Final evaluation: plotted graphs, results analysis, comparison with A*/existing work, larger benchmarks | **Partial** (scaling and Bellman–Ford timings exist as CSV; no plots or A*) | `bench_scaling.csv` |

### 3.2 How the percentage is calculated

The weights are the team's own planning estimate of relative effort and importance.
They are **not an official rubric**. Each module counts only if its status is
"Implemented & tested" with evidence. Partial modules count as zero.

| Scope item | Weight | Counted |
|---|---|---|
| M1 Load and validate | 10 | 10 |
| M2 Adjacency / physical edges | 8 | 8 |
| M3 Request handling + menu | 7 | 7 |
| M4 Closure management | 8 | 8 |
| M5 Feasibility filter | 5 | 5 |
| M6 Dijkstra routing | 15 | 15 |
| M7 Reconstruction + display | 5 | 5 |
| M8a Tests + oracle | 10 | 10 |
| M8b Basic measurements | 5 | 5 |
| D Verified NIT Trichy dataset | 15 | 0 |
| V Visualization | 5 | 0 |
| F Final evaluation and comparison | 7 | 0 |
| **Total** | **100** | **73** |

Under these weights, **73 of 100** points are implemented with evidence, which is above
the 60 % milestone. The working core (M1–M8) is complete. Everything missing concerns
real campus data and final-review presentation work.

---

## 4. Dataset

`data/demo/` is a synthetic demonstration dataset. It is not NIT Trichy data.
See [../data/README.md](../data/README.md) for formats, provenance and the steps to
replace it with verified data.

| Measured by `--validate` | Value |
|---|---|
| Graph nodes | 22 |
| Physical edges | 31 (30 two-way, 1 one-way) |
| Adjacency entries | 61 |
| Places | 10 |
| Closed in file | 1 (E31) |
| Step-free yes/no/unknown | 29/1/1 |
| Components | 2 (main campus + disconnected research station) |
| Total path length | 5,859 m |
| Errors / warnings | 0 / 0 |

Provenance: invented layout, AI-assisted, with no survey or map source. Units are metres.
Every path is two-way unless marked `one-way`. Each place is mapped to an
**approach-point** node (its entrance), not to a building centroid.

---

## 5. Test cases and actual outcomes

The full table, with expected and actual values for every test, is in
[TEST_RESULTS.md](TEST_RESULTS.md). It is generated from `results/test_results.csv`.
**61 tests executed, 61 passed, 0 failed, 0 not run.**

### 5.1 Coverage of the required scenarios

| Required scenario | Test(s) | Actual result (from CSV) |
|---|---|---|
| Normal shortest route | T01 | 101→106: OK 590.0 m, E01,E02,E03,E22 |
| Multi-hop shorter than direct edge | T02, T02b | 7.0 m via A>C>B vs 10 m direct; 473.0 m vs 520 m direct E21 |
| Source = destination | T03 | OK 0.0 m, 0 segments, node 106 |
| Invalid source / destination IDs | T04, T05, T06 | INVALID_SOURCE; INVALID_DESTINATION; place 99 and 0 not found |
| Unreachable destination | T07 | NO_ROUTE; Bellman–Ford distance = ∞ |
| Closure → alternative route | T08 | Close E22: 608.0 m via E01,E02,E04,E05 |
| Close until no route | T09 | 608 → 620 → 655 → 1229 → 1262 → NO_ROUTE |
| Reopen restores distance | T10 | 608.0 closed → 590.0 reopened |
| Two-way closure blocks both directions | T11 | 2 adjacency entries share E22; 608.0 m both ways after closing |
| One-way edge | T14, T15 | 118→116 382.0 m via E23; 116→118 491.0 m without E23; reverse of a lone one-way is NO_ROUTE |
| Equal-cost alternatives | T16, T16b, T17 | Fewer segments chosen (555 m in 2 segments); equal-segment tie accepts either route |
| Zero-weight edge | T18 | A→C 5.0 m via a 0 m edge |
| Negative weights / invalid data | T30–T51 | All 20 bad inputs rejected with specific messages, missing files reported, straight-line warning raised |
| Extra: step-free mode | T20, T21 | Stairs and `unknown` excluded (608.0 m, 468.0 m) |
| Extra: oracle sweeps | T52–T63, T70, T71 | 154,985/154,985 pairs agree |

### 5.2 How correctness is judged

* **Oracle.** `bellmanFordReference` rebuilds the arcs directly from the edge records,
  without the adjacency lists, the heap or the router's filter. It computes the
  lexicographic (distance, segments) labels. For integer-weight graphs, the distance
  and the segment count must match exactly. For fractional weights, the distance must
  match within relative tolerance 1e-9.
* **Route checker.** `checkRoute` verifies that every OK route:
  * starts at the source and ends at the destination;
  * uses edges that join consecutive nodes in a permitted direction;
  * uses only open edges, and only step-free edges in step-free mode;
  * visits no node twice;
  * has edge lengths that sum to the reported distance;
  * has a segment count equal to its number of edges.
* **Ties.** When several shortest routes have equal distance and equal segments, any
  of them is accepted (T17), provided it passes the checks above.

### 5.3 Can the tests detect bugs? (mutation check, run once during development)

Deliberately broken copies of `router.cpp` were compiled against the same tests:

| Mutation | Tests that failed |
|---|---|
| Ignore closed edges | 19 |
| Ignore step-free filter | 10 |
| Max-heap instead of min-heap | 24 |
| Remove (distance, segments) tie-break | 2 (T16b, T70) |
| Do not skip stale heap entries | 1 (T22: counter check; routes stay correct, as expected) |
| Remove early stop at destination | 0 (expected, because only efficiency changes) |

---

## 6. Evaluation metrics

| Metric | Definition / calculation | Purpose | Value now |
|---|---|---|---|
| Test pass rate | passed tests ÷ executed tests | Overall functional status | **61/61 = 100 %** |
| Shortest-distance agreement | pairs where Dijkstra's status and label equal Bellman–Ford's ÷ pairs compared. Unreachable pairs count as agreeing only if both report unreachable, and no "distance error" is computed for them. | Correctness against an independent method | **154,985/154,985 = 100 %** (98,112 reachable + 56,873 NO_ROUTE pairs) |
| Route validity rate | OK routes passing `checkRoute` ÷ OK routes | Checks the route itself, not just the number | **98,112/98,112 = 100 %** |
| Search execution time | Wall time of `findRoute` only, with `steady_clock`. Each sample is a batch of K calls divided by K; the median over B batches is reported. Loading, printing and checking are not timed. | Responsiveness and scaling | Demo: median of per-query medians **≈ 0.7 µs** (200 queries; 31 batches × 200 calls each). Grids: see §6.2 |
| Settled vertices | Vertices removed from the heap with a current label. Each vertex is settled at most once, and stale removals are counted separately. | Machine-independent work measure | Demo mean **11.5** of 22 (standard, 100 place pairs); max 20 |
| Adjacency entries scanned / relaxation attempts | Scanned = entries read from settled vertices. Attempts = scanned minus entries filtered as closed or not allowed. Successes = labels improved. | Work that does not depend on timer noise; shows the filter's cost | Demo mean **32.4** scanned per query (standard) |
| Route-distance increase after closure | new − base (m) and % of base, **only when both routes exist**. Otherwise the new status is recorded instead. | Campus impact of closures | §6.3 |
| Memory | Complexity O(V + A). The CSV also gives an **estimate** from element counts × `sizeof`, which excludes string storage and allocator overhead. | Scalability | **Process memory was not measured.** Estimate for the 100,489-node grid: graph ≈ 31.3 MB, per-query workspace ≈ 2.1 MB |

Measurement environment (`results/bench_environment.txt`): Intel Core i5-12400F
(6 cores, 12 threads), 15.8 GB RAM, Windows 11 Pro 10.0.26300, g++ 15.2.0,
`-std=c++17 -O2 -static`, `std::chrono::steady_clock` with a 1 ns period, single thread.

### 6.1 Demo dataset: all ordered place pairs (100 per mode)

| Mode | OK | NO_ROUTE | Median of per-query median times | Mean settled | Max settled | Mean scanned | Max heap size |
|---|---|---|---|---|---|---|---|
| standard | 82 | 18 | 0.715 µs | 11.51 | 20 | 32.37 | 9 |
| step-free | 82 | 18 | 0.728 µs | 11.67 | 20 | 32.84 | 10 |

The 18 NO_ROUTE pairs per mode are the pairs that involve the disconnected research
station. For loading, the median time to read and validate the four demo files was
**129 µs**, measured separately from search.

### 6.2 Scaling on synthetic grid graphs (20 random queries each)

These are synthetic grids with 4-neighbour paths and random integer weights from 50 to 150 m.

| Nodes | Adjacency entries | Dijkstra, early stop: median time | Mean settled | Full Dijkstra: median time | Bellman–Ford (reference): median time | Agreement with Bellman–Ford |
|---|---|---|---|---|---|---|
| 100 | 360 | 1.34 µs | 45.4 | 2.02 µs | 5.0 µs | 20/20 |
| 1,024 | 3,968 | 12.24 µs | 477.1 | 48.24 µs | 176 µs | 20/20 |
| 10,000 | 39,600 | 349.2 µs | 3,899.2 | 951.4 µs | 8,353 µs | 20/20 |
| 100,489 | 400,688 | 8,295 µs | 48,001.2 | 15,443 µs | not run (too slow at this size) | 20/20 |

**Run-to-run variation.** The benchmark ran twice. Counters were identical in both
runs, but times differed: the Main Gate → Library query took 0.64 µs and 1.06 µs, and
the 100k-node early-stop median took 7.27 ms and 8.30 ms. Treat the times as
order-of-magnitude figures for this machine. Plots with spread across repeated runs
are planned for the final review.

### 6.3 Closure impact (standard mode, from `closure_impact.csv`)

| Scenario | Route | Base | After closure | Increase |
|---|---|---|---|---|
| Close E22 (stairs footbridge) | Gate → Library | 590 m | 608 m | +18 m (+3.05 %) |
| + E04 | Gate → Library | 590 m | 620 m | +30 m (+5.08 %) |
| + E20 | Gate → Library | 590 m | 655 m | +65 m (+11.02 %) |
| + E21 | Gate → Library | 590 m | 1,229 m | +639 m (+108.31 %) |
| + E06 | Gate → Library | 590 m | 1,262 m | +672 m (+113.90 %) |
| + E24 | Gate → Library | 590 m | NO_ROUTE | not computed (no route) |
| Close E23 (one-way exit) | Sports → Hostel | 382 m | 491 m | +109 m (+28.53 %) |
| Close E03 | Gate → Admin | 420 m | 778 m | +358 m (+85.24 %) |
| Close E29 | Canteen → Health | 555 m | 555 m | 0 m (equal-length alternative) |

The single-edge sensitivity results are in `closure_single_edge_summary.csv`. That run
closes each open edge in turn and checks all 72 reachable place pairs. Two edges,
**E15 and E27**, each disconnect **16 of the 72** pairs, because each is the only
approach walk to its building. **E08** lengthens the most pairs (21, up to +634 m).
Closing **E21** or **E29** changes no shortest place-to-place distance. This analysis
shows which campus paths are critical.

---

## 7. Complexity of the implementation

Notation: V is the number of nodes. E is the number of physical edges. A is the number
of adjacency entries, where A = 2·(two-way edges) + (one-way edges) ≤ 2E.

| Operation | Time | Space | Notes |
|---|---|---|---|
| Load + validate (M1) | O(V + E) expected | O(V + E) | Hash maps for ID lookup |
| Build adjacency (M2) | O(V + A) | O(V + A) | One record per physical edge. Each adjacency entry stores (neighbour, edge index), 8 bytes. |
| Close / reopen (M4) | O(1) expected | O(1) | Hash lookup plus one flag. No rebuild. Both directions read the same flag. |
| Filter (M5) | O(1) per scanned entry | none | Applied during the scan instead of copying the graph |
| Dijkstra (M6), lazy binary heap | **O(V + A log(1 + A))** | O(V + A) | O(V) to initialise the label arrays. Each settled vertex scans its entries once, so pushes ≤ 1 + A, and each push or pop costs O(log heap size). Because A ≤ V² on simple graphs, this is O((V + E) log V). Parallel edges are covered by the A-based bound. |
| Reconstruction (M7) | O(path length) ≤ O(V) | O(V) | Follows parent pointers, then reverses once |
| Bellman–Ford oracle | O(V · A) | O(V + A) | Testing only |

Why the lazy heap is used: `std::priority_queue` has no decrease-key operation. The
code inserts a new entry on every improvement and later discards stale entries, using
a `settled` flag and a label comparison. This makes the heap hold up to 1 + A entries
instead of V, but the asymptotic bound stays the same. The stale removals are counted
(`stale_pops` in the CSVs).

Why it is correct: weights are non-negative. Every arc adds (w ≥ 0, +1 segment) to a
label, and adding the same pair to two labels preserves their lexicographic order. So
when a vertex is removed from the heap with its current label, that label is optimal.
The early stop is therefore safe for the destination.

---

## 8. Block diagram (matches the code)

```
  data/<dir>/  nodes.txt  edges.txt  places.txt  [metadata.txt]
        |
        v
 +------------------------+   any error    DATA_ERROR: list of file:line messages,
 | M1 Load and validate   |-------------->  program exits (no partial graph)
 |    (graph.cpp)         |
 +------------------------+
        | valid records
        v
 +------------------------+   nodes[], edges[] (one record per physical path, with status),
 | M2 Build graph         |   places[] (place ID -> approach node), adj[u] = (v, edgeIndex)
 +------------------------+
        |                               +-------------- Terminal menu (main.cpp) --------------+
        v                               |                                                       |
 +------------------------+  close /    |                                                       |
 | M4 Path status         |<-- reopen --+  user                                                 |
 |  edges[i].open,        |  / reset    |                                                       |
 |  version++ on change   |             +-- source place, destination place, mode --+           |
 +------------------------+                                                         v           |
        |                                                          +----------------------+     |
        |  status read during search                               | M3 Validate request  |     |
        v                                                          | place -> node index  |     |
 +------------------------+   edgeAllowed(e, mode)                 +----------------------+     |
 | M5 Feasibility filter  |<-----------------------+                          |                 |
 |  open AND (standard OR |                        |                          v                 |
 |  step_free = yes)      |                 +---------------------------------------------+   |
 +------------------------+                 | M6 Dijkstra: binary min-heap of             |   |
                                            |    (distance, segments, node), lazy deletion,|   |
                                            |    early stop at destination, counters      |   |
                                            +---------------------------------------------+   |
                                                    | dist[], segments[], parent[], parentEdge[]
                                                    v                                         |
                                            +---------------------------------------------+   |
                                            | M7 Reconstruct and display: node names,     |---+
                                            |    node IDs, edge IDs, distance, segments,  |
                                            |    graph version, or NO_ROUTE / INVALID     |
                                            +---------------------------------------------+

 Offline (not used for navigation):
   M8a tests/test_main.cpp : expected cases + Bellman-Ford oracle + checkRoute -> results/test_results.csv
   M8b bench/benchmark.cpp : timed searches, counters, closure impact          -> results/bench_*.csv
```

---

## 9. Pseudocode (one block per module in the diagram)

```
M1 LOAD_AND_VALIDATE(dir)
  for each required file in {nodes.txt, edges.txt, places.txt}: if cannot open -> error
  for each non-comment line in nodes.txt:
      split on '|'; require 2 or 4 fields
      require integer id >= 0, non-empty name, both-or-neither finite coordinates
      reject duplicate id; nodeIndex[id] <- next index
  for each line in edges.txt:
      require 8 or 9 fields; edge_id matches [A-Za-z0-9_-]+ and is unique (case-insensitive)
      require from, to in nodeIndex and from != to
      require distance parses, is finite and >= 0
      require direction in {two-way, one-way}, status in {open, closed},
              step_free in {yes, no, unknown}, path_type in {walkway, road, ramp, stairs}
      reject stairs with step_free = yes
      if both endpoints have coordinates and distance < straight line: add WARNING
  for each line in places.txt: require unique place_id >= 1, node_id in nodeIndex, name
  if any error: return DATA_ERROR(list)            // target graph left unchanged

M2 BUILD_GRAPH(nodes, edges)
  adj[v] <- empty for every node
  for i, e in edges (file order):
      append (e.to, i) to adj[e.from]
      if e.direction = two-way: append (e.from, i) to adj[e.to]   // same physical edge i
  version <- 1

M3 VALIDATE_REQUEST(placeS, placeT, mode)
  ps <- findPlace(placeS); if none: return INVALID_SOURCE
  pt <- findPlace(placeT); if none: return INVALID_DESTINATION
  return (ps.node, pt.node, mode)                  // s = t is valid

M4 SET_EDGE_OPEN(edgeId, open)
  i <- edgeIndex[upper(edgeId)]; if none: return UNKNOWN_EDGE
  if edges[i].open = open: return UNCHANGED
  edges[i].open <- open; version <- version + 1; return CHANGED

M5 EDGE_ALLOWED(e, mode)
  return e.open and (mode = standard or e.step_free = yes)

M6 DIJKSTRA(s, t, mode)
  dist[v] <- INF; seg[v] <- INF; parent[v] <- NIL; parentEdge[v] <- NIL; settled[v] <- false
  dist[s] <- 0; seg[s] <- 0; heap <- {(0, 0, s)}
  while heap not empty:
      (d, h, u) <- pop-min(heap)
      if settled[u] or (d, h) != (dist[u], seg[u]): stalePops++; continue
      settled[u] <- true; settledCount++
      if u = t: break                               // destination label is final
      for (v, i) in adj[u]:
          scanned++
          if not EDGE_ALLOWED(edges[i], mode): filtered++; continue
          cand <- (d + edges[i].distance, h + 1)
          if cand < (dist[v], seg[v]) lexicographically:
              (dist[v], seg[v]) <- cand; parent[v] <- u; parentEdge[v] <- i
              push(heap, (cand.d, cand.h, v))
  return dist, seg, parent, parentEdge, counters

M7 RECONSTRUCT(s, t, result)
  if dist[t] = INF: return NO_ROUTE
  nodes <- []; edges <- []; v <- t
  while v != s: append v to nodes; append parentEdge[v] to edges; v <- parent[v]
  append s; reverse nodes and edges
  print names, node IDs, edge IDs, dist[t], seg[t], mode, version

M8a TEST(graph)
  for each scripted case: build graph, apply closures, route, compare with expected
  for each source s: ref <- BELLMAN_FORD(s, mode)
      for each target t: r <- DIJKSTRA+RECONSTRUCT(s, t, mode)
          agree if (r = NO_ROUTE and ref[t] = INF) or (CHECK_ROUTE(r) and label(r) = ref[t])
  BELLMAN_FORD(s, mode): arcs built from edge records; repeat V-1 rounds or until no change:
      for each arc (u, v, w): relax lexicographically
  write results/test_results.csv and results/correctness_metrics.csv

M8b MEASURE(graph)
  for each query: run once for counters; then B batches of K calls,
      sample = batch time / K; report median, min, max      // nothing printed inside timing
  closure scenarios: base route, close edges, new route, increase if both OK
  write results/*.csv and environment
```

---

## 10. Contribution (stated precisely)

This project **does not propose a new shortest-path algorithm**. Dijkstra's algorithm
(1959) and skipping closed edges are standard techniques. The contribution is
**campus-specific graph modelling and closure-aware routing built on existing
algorithms**, made reliable through validation and testing:

1. **The physical-edge model.** One record per real path is shared by both directions,
   so a single closure blocks a two-way path consistently (T11). Each path carries
   direction, status, a step-free flag and a type.
2. **Places separated from the network.** Destinations map to **approach-point
   entrance nodes** through `places.txt`, independent of graph-node IDs.
3. **Closure-aware routing with versioning.** Paths can be closed and reopened at run
   time without rebuilding the graph, and each answer reports the graph version it
   used.
4. **A step-free mode** that excludes `no` and `unknown`, and **tie-breaking by fewer
   segments** for equal distances (a lexicographic label, which is still Dijkstra).
5. **Data-quality checks for campus data.** These include rejection with file:line
   messages and a warning when a path is shorter than the straight-line distance.
6. **An evaluation method.** It combines an independent oracle, a route-validity
   checker, closure-impact analysis and a single-edge criticality sweep.

At present these are demonstrated **on synthetic data only**. Applying them to
verified NIT Trichy data is the main remaining task.

---

## 11. Important data structures

| Structure | Code | Why |
|---|---|---|
| Node record | `Node {id, name, hasCoords, x, y}` in `nodes_` | Names for output. Optional coordinates for data checks, and later for A* or a map. |
| Physical edge record | `PhysicalEdge {id, from, to, distance, direction, open, openInData, stepFree, type, label}` | One record per real path, so both directions share one status |
| Place record | `Place {id, node, name}` | Menu IDs separate from graph IDs; approach-point mapping |
| Adjacency list | `vector<vector<AdjEntry{to, edge}>>` | O(V + A) space. Scanning a vertex's neighbours costs O(degree). |
| ID maps | `unordered_map` for node ID, edge ID and place ID to index | Non-contiguous IDs; O(1) expected lookup |
| Label arrays | `dist` (double), `segments` (int) | (distance, segments) label per vertex |
| Parent arrays | `parent`, `parentEdge` | Route reconstruction with exact edge IDs, even with parallel paths |
| Min-priority queue | `priority_queue<tuple<double,int,int>, …, greater<>>` | Binary heap, O(log n) push and pop. Lazy deletion replaces decrease-key. |
| Settled flags | `vector<char>` | Identifies stale heap entries cheaply |

---

## 12. Known limitations

* **The data is synthetic.** No real NIT Trichy distances or connections have been
  used yet, and no NIT Trichy claim has been verified.
* "Step-free" describes only the recorded flag. It is not a full accessibility
  assessment (width, slope and surface are not modelled).
* "Fewer segments" depends on how paths are split into edges, and is not the same as
  "fewer turns".
* Each query runs a fresh Dijkstra search. There is no incremental reuse such as
  LPA*, which is unnecessary at campus scale.
* The interface is terminal only, with no map.
* Times are single-machine and vary between runs. Process memory was not measured.
* With fractional distances, exact equal-distance tie-breaking may be hidden by
  floating-point rounding. The reported distance is still optimal.

---

## 13. Work remaining before 23 November 2026

| Week | Task | Output |
|---|---|---|
| 3–9 Nov | Obtain or build verified NIT Trichy data (OSM export and/or campus survey). Choose approach points for the destinations and validate with `--validate`. | `data/nitt/` with provenance; actual counts |
| 3–9 Nov | Locate the NIT Trichy proposal and reconcile its claims (245/329/8) with the measured counts | Reconciliation note |
| 10–16 Nov | Hand-checked expected routes and an oracle sweep on the real data; closure-impact runs | Updated tests and CSVs |
| 10–16 Nov | Comparison: A* with a straight-line heuristic (coordinates), Bellman–Ford, and optionally DIMACS NY for scale. Repeated runs with spread. | `bench_*.csv`, plots |
| 17–20 Nov | Performance graphs (time vs. size, settled vs. size, closure impact). Optional simple map output (SVG of the route on node coordinates). | Figures |
| 21–22 Nov | Final report: results analysis, comparison with existing work, limitations | Final slides and report |

---

## 14. Literature (papers already identified in the proposal)

The bibliographic details below were checked against Crossref on 1 November 2026
(title, authors, venue, volume, issue, pages, year). The content summaries are
standard descriptions of these papers. The full texts were **not re-read** for this
review.

1. E. W. Dijkstra, "A note on two problems in connexion with graphs," *Numerische
   Mathematik*, vol. 1, no. 1, pp. 269–271, 1959. doi:10.1007/BF01386390.
   This is the shortest-path method that grows a set of vertices with final distances.
   It needs non-negative weights.
2. P. E. Hart, N. J. Nilsson, B. Raphael, "A Formal Basis for the Heuristic
   Determination of Minimum Cost Paths," *IEEE Trans. Systems Science and
   Cybernetics*, vol. 4, no. 2, pp. 100–107, 1968. doi:10.1109/TSSC.1968.300136.
   This is A*, which orders by f = g + h. It finds an optimal path when h never
   overestimates. Using it requires reliable coordinates.
3. S. Koenig, M. Likhachev, D. Furcy, "Lifelong Planning A*," *Artificial
   Intelligence*, vol. 155, no. 1–2, pp. 93–146, 2004. doi:10.1016/j.artint.2003.12.001.
   LPA* is incremental replanning after edge-cost changes. It needs extra state, and
   its benefit depends on the change pattern.

Limitations for this project: these are general algorithms. None of them defines
campus data such as approach points, step-free flags or shared physical-edge closures.
That modelling is this project's focus.
