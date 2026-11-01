# Opening speech and viva preparation

## Opening speech (about 60 seconds)

> Good morning. My project is the Smart Campus Route Planner using graphs and
> Dijkstra's algorithm. A campus is modelled as a weighted graph. Junctions and
> building entrances are the nodes, and each real path is one edge record with its
> length in metres, its direction, whether it is open, and whether it is step-free.
>
> The planner finds the shortest permitted route between two places. It handles
> closed paths, one-way paths, a step-free mode, invalid input and places that cannot
> be reached.
>
> I am not claiming a new shortest-path algorithm. Dijkstra's algorithm is standard.
> My work is the campus model and closure-aware routing on top of it, checked by
> validation and testing.
>
> For this review, the core is implemented in C++17. Sixty-one automated tests pass,
> and the router agrees with an independent Bellman–Ford checker on all 154,985
> source-destination pairs tested.
>
> The demo uses a clearly labelled synthetic campus, because verified NIT Trichy data
> is not ready yet. Preparing that data is the main task before the final review.

## Ten likely viva questions

**1. Why Dijkstra and not BFS or Bellman–Ford?**
BFS finds the fewest edges, not the shortest distance, so it ignores weights.
Bellman–Ford handles negative weights but costs O(V·A). Walking distances are never
negative, so Dijkstra is correct and much faster. On the 10,000-node grid, the full
Dijkstra median was about 0.95 ms against about 8.4 ms for Bellman–Ford.

**2. Why does Dijkstra need non-negative weights?**
When a vertex is removed from the heap with the smallest label, Dijkstra treats that
label as final. With a negative edge, a later path could still reduce it, so that
assumption fails. The loader therefore rejects negative, NaN and infinite distances
(tests T30–T32).

**3. What is a lazy priority queue, and why use it?**
`std::priority_queue` cannot decrease a key. The code pushes a new entry each time a
label improves. When an older entry for the same vertex is popped, it no longer
matches the stored label, so it is skipped and counted as a stale pop. The heap can
hold up to 1 + A entries, but the time stays O(V + A log(1 + A)).

**4. What is the time and space complexity?**
Time is O(V + A log(1 + A)), which is O((V + E) log V) for simple graphs. Space is
O(V + A) for the graph, the label and parent arrays, and the heap. A = 2 × (two-way
edges) + (one-way edges). Closing a path is O(1) expected, with no rebuild.

**5. How does closing a two-way path affect both directions?**
Both adjacency entries store the index of the same physical edge record. The record
has one `open` flag, so closing it blocks both directions. Test T11 checks that two
entries share E22 and that neither direction uses it after closure.

**6. How do you know the answers are correct?**
* An independent Bellman–Ford oracle is built directly from the edge records. It
  agreed on 154,985 of 154,985 pairs, on the demo data under closures and on 500
  random graphs.
* A checker confirms that every returned edge joins the reported nodes in an allowed
  direction, is open, and that the lengths sum to the distance.
* Deliberately broken versions of the router are caught by the tests.

**7. What happens when two routes have the same length?**
Labels are (distance, segments), compared lexicographically, so the route with fewer
segments wins (T16, T16b). If distance and segments are both equal, either route is
correct, and the test accepts either (T17). Distance is never traded for fewer
segments.

**8. Is your dataset real NIT Trichy data?**
No. The demo dataset is synthetic and labelled as such in every file and in the
program header. The proposal's figures of 245 vertices and 329 edges have not been
verified, because that data was not available. Verified data, from OSM with
attribution and/or a survey, is planned for the final review. It will be checked with
`--validate`, and its actual counts will be reported.

**9. Why not A*?**
A* needs a heuristic that never overestimates, such as straight-line distance, and
that requires reliable coordinates. Campus-scale searches already take well under a
millisecond. A* will be added in the final review as a comparison, using coordinates
from verified data.

**10. What are the limitations?**
* The data is synthetic so far.
* "Step-free" is only a recorded flag, not a full accessibility audit.
* "Fewer segments" is not the same as "fewer turns".
* Every query is a fresh search, with no incremental LPA*-style reuse.
* There is no map display yet.
* Times are from one machine and vary between runs.
* Process memory was not measured.

## Quick facts to remember

* Demo graph: 22 nodes, 31 physical edges, 61 adjacency entries, 10 places
* Gate → Library: 590 m standard (via the stairs E22), 608 m step-free or with E22 closed
* Tests: 61/61. Oracle pairs: 154,985/154,985. Valid routes: 98,112/98,112.
* Weighted completion: 73/100 (our weights; NIT Trichy data, visualization and final comparison not counted)
