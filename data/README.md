# Dataset files

## What is in `data/demo/`

`data/demo/` is a **synthetic demonstration dataset**. The names are generic campus
facilities. The coordinates, connections and distances are invented so that every
routing case can be shown and tested. **It is not a survey or map of NIT
Tiruchirappalli and must not be presented as real campus geography.**

Counts below come from `build\route_planner.exe --validate data/demo`:

| Item | Count |
|---|---|
| Graph nodes | 22 (10 named approach points + 12 junctions/gates) |
| Physical edges | 31 (30 two-way, 1 one-way) |
| Adjacency entries | 61 (= 2 × 30 + 1) |
| Places (menu entries) | 10 |
| Edges closed in the file | 1 (E31, "closed for construction") |
| Step-free yes / no / unknown | 29 / 1 / 1 |
| Weakly connected components | 2 (main campus + a disconnected research station) |
| Load errors / warnings | 0 / 0 |

Cases built into the layout on purpose:

| Case | Where |
|---|---|
| Stairs shortcut that standard mode uses and step-free mode avoids | E22 (Admin–Library footbridge) |
| Longer alternative after a closure | Gate→Library: 590 m → 608 m when E22 closes |
| Closing paths until no route remains | Close E05, E22, E24 to cut off the Library |
| Disconnected location | Place 10 (nodes 130–131, linked to each other only) |
| One-way path | E23 (118 → 115 only) |
| Multi-hop route shorter than a direct edge | 101→105: 473 m via E01, E02, E04 vs. 520 m direct E21 |
| Equal distance, fewer segments preferred | 113→120: 555 m by 2 or 4 segments |
| Unverified step-free status (`unknown`) | E25 |
| Edge closed in the data file | E31 |

## File formats

All files are UTF-8 text with fields separated by `|`. Blank lines and lines that start
with `#` are ignored, and spaces around fields are trimmed. Any invalid record makes the
whole load fail with `file:line` messages, and the program does not start with partial data.

### `nodes.txt`: `node_id|name|x_m|y_m`

* `node_id`: a non-negative integer and unique. IDs **do not need to be contiguous**,
  because the program maps them to dense internal indices.
* `name`: non-empty text.
* `x_m|y_m`: optional local planar coordinates in metres. Give both or leave both
  empty, or write only `node_id|name`. When both endpoints of an edge have
  coordinates, an edge shorter than the straight-line distance between them produces
  a **warning**, not an error.

### `edges.txt`: `edge_id|from_node|to_node|distance_m|direction|status|step_free|path_type|label`

* `edge_id`: unique. Letters, digits, `_` or `-` are allowed. Matching is
  case-insensitive (`e05` = `E05`).
* `from_node`, `to_node`: must be IDs in `nodes.txt`. Self-loops are rejected.
* `distance_m`: a finite number ≥ 0, in metres. Negative, `nan` and `inf` values are rejected.
* `direction`: `two-way` or `one-way`. `one-way` allows `from_node → to_node` only.
* `status`: `open` or `closed`. This is the starting status, and the menu can change it.
* `step_free`: `yes`, `no` or `unknown`. Step-free mode uses only `yes`.
* `path_type`: `walkway`, `road`, `ramp` or `stairs`. `stairs` with `step_free=yes` is rejected.
* `label`: optional free text.

A two-way record creates **two adjacency entries that point to the same physical
edge record**, so closing that one record blocks both directions.

### `places.txt`: `place_id|node_id|name`

* `place_id`: a positive integer that the user types in the menu. This is a separate
  number space from graph-node IDs: place 1 maps to node 101.
* `node_id`: the graph node used as the place's **approach point**, normally the
  entrance on the walking network.
* Two places may share one node.

### `metadata.txt` (optional): `key = value`

The planner shows these lines in its summary. Use them to record provenance, units,
assumptions and licence. The keys used here are `name`, `provenance`, `units`,
`direction_assumption` and `licence`.

## Replacing the demo with verified NIT Trichy data

1. Create a new folder such as `data/nitt/` that contains the same four files. Leave
   `data/demo/` unchanged so the tests keep working.
2. **Source the geometry.** Use either your own survey (walk and measure, or GPS
   traces) or an OpenStreetMap export of the campus area from
   <https://www.openstreetmap.org/export>. OSM data is © OpenStreetMap contributors
   and licensed under the ODbL. If you use it, keep that attribution in
   `metadata.txt` and on your slides.
3. **Choose nodes.** Use path junctions and one approach point (the entrance) per
   building. Do not route to building centroids.
4. **Compute `distance_m`** along each path's actual geometry, summing the segment
   lengths. Do not use straight-line distance between the endpoints.
5. **Set the attributes.**
   * Mark paths `one-way` only where walking is really restricted.
   * Set `step_free=yes` only where it has been checked. Otherwise use `unknown`.
6. **Map places.** Add each destination to `places.txt` with its approach node.
7. **Validate.** Run `build\route_planner.exe --validate data/nitt` and fix every
   reported error. Review the warnings and check the reported counts.
8. **Report actual counts** from that output, with their source and date. Do not
   reuse counts from the proposal.
9. **Run on the new data.** Use `build\route_planner.exe data/nitt` for the demo.
   The automated tests (`run_tests.bat`) check expected routes in the synthetic
   layout, so keep running them on `data/demo`. For real data, add a new test block
   that loads `data/nitt`, runs `sweep()` (the Bellman–Ford all-pairs comparison in
   `tests/test_main.cpp`), and adds hand-checked expected routes.
