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

## Comprehensive NIT Trichy Campus Dataset (`data/nitt/`)

`data/nitt/` is the **comprehensive, verified NIT Trichy campus dataset** containing **all campus locations, hostels, PG accommodations, departments, workshops, library, lecture halls, canteens, messes, and sports/civic amenities**.

Counts from `build\route_planner.exe --validate data/nitt`:

| Item | Count |
|---|---|
| Graph nodes | 188 (123 approach points + 65 junctions/road intersections) |
| Physical edges | 212 (211 two-way, 1 one-way) |
| Adjacency entries | 423 (= 2 × 211 + 1) |
| Places (menu entries) | 123 |
| Edges closed in the file | 1 (N212, "closed for renovation") |
| Step-free yes / no / unknown | 210 / 1 / 1 |
| Weakly connected components | 1 (100% connected campus network) |
| Total path length | 18,038.7 m (~18 km across 800 acres) |
| Load errors / warnings | 0 / 0 |

### Included NITT Categories in `data/nitt/places.txt`:

1. **Hostels (UG, PG, Scholars & Girls)**:
   * **UG Boys Hostels**: Agate (1st Year), Coral (1st Year), Diamond (1st Year), Jade (1st Year), Garnet (A, B, C), Zircon (A, B, C), Amber (A, B), Aquamarine (A, B), Topaz, Sapphire, Ruby, Lapis.
   * **PG Boys Hostels**: Emerald, Pearl.
   * **Research Scholars Hostel**: Jasper (PhD Scholars).
   * **Girls Hostels (Opal Complex & Beryl)**: Opal A, Opal B, Opal C (East Wing), Opal C (West Wing), Opal D, Opal E, Opal F, Opal G, Beryl Hostel, and Opal Security Gate.
2. **Academic Departments (All 17 Departments)**:
   * Architecture (Arch), Chemical, Chemistry, Civil, Computer Applications (CA/MCA), Computer Science (CSE), Electrical & Electronics (EEE), Electronics & Communication (ECE), Energy & Environment (CEESAT), Humanities & Social Sciences (HSS), Instrumentation & Control (ICE), Management Studies (DoMS/MBA), Mathematics, Mechanical, Metallurgical & Materials (MME), Physics, Production.
3. **Workshops, Labs & Computing**:
   * Central Workshop (Foundry, Machine Shop, Carpentry, Fitting, Welding), Production Machine Tool Lab, Siemens Centre of Excellence in Manufacturing (CoE), The Octagon (CSG), Param Porul Supercomputing Center, CECASE, Civil Structures Lab, High Voltage Lab.
4. **Libraries & Lecture Hall Complexes**:
   * Central Library Entrance & Digital Knowledge Centre, Orion Lecture Hall Complex (UG Mega LHC), Orion PG Wing, Old Lecture Hall Complex (Old LHC), New Lecture Hall Complex (NLHC / Third Eye), MiG-23 Fighter Jet Landmark.
5. **Messes & Dining Halls**:
   * Mega Mess I (Ground Floor & First Floor), Mega Mess II (Ground Floor & First Floor), A Mess (Nilgiri / North Indian), B Mess, Kailash Mess (South Indian), Sabari Mess (Kerala Cuisine), Annapurna Mess (Andhra Cuisine), Opal Girls Mess 1 & 2.
6. **Canteens & Food Outlets**:
   * Central Institute Canteen & Fast Food Annex, Nescafe Coffee Kiosk, Aavin Milk Parlour, Night Canteens (Mega Mess Plaza, Amber/Garnet, Opal Complex), BHEL Gate Canteen.
7. **Administrative & Central Landmarks**:
   * Main Gate (NH 67 Checkpost), Security Office & Visitors Reception, Administrative Building & Clock Tower, Director's Office & Senate Hall, Golden Jubilee Convention Hall (GJCH), Barn Hall Auditorium, Training & Placement Cell, SAC, Estate Maintenance (EMD), Central Campus Roundabout & Fountain.
8. **Health, Sports & Recreation**:
   * 24/7 Health Centre / NITT Hospital, Hospital Pharmacy & Ambulance Bay, Sports Complex & Central Gym, Indoor Badminton Stadium, Cricket Ground & Pavilion, Football Ground & Track, Tennis Courts, Basketball Courts, Swimming Pool Complex, Jade Playground.
9. **Shopping, Banking & Conveniences**:
   * Shopping Complex (2K Market), SBI NITT Branch & 24/7 ATM, Canara Bank ATM, India Post Office, Co-operative Stores, Bookstall, Campus Pharmacy, Guest House 1 (Kurunji), Guest House 2 (Marutham), Director's Bungalow, Staff Quarters.
10. **Places of Worship & Security Gates**:
    * Sri Vidya Ganapathi Temple (Historic 1968), Sri Hanuman Shrine, NITT Campus Mosque, Christian Prayer Fellowship, Staff Gate / East Gate, Thuvakudi Rear Gate.
