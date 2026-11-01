# Module completion checklist (status on 1 November 2026)

Legend: [x] implemented and tested (evidence given) · [~] partial · [ ] not started

## Second review: working core

- [x] M1 Load nodes, edges, places, metadata; reject malformed records, duplicate IDs,
      unknown references, negative/NaN/infinite distances, bad flags and missing files (T30–T50)
- [x] M1 Straight-line distance sanity warning (T51)
- [x] M2 Weighted adjacency lists; two-way paths share one physical edge record (T11)
- [x] M2 Non-contiguous node IDs; place IDs kept separate from node IDs (T06)
- [x] M3 Source/destination by place ID; invalid input handled (T04–T06, transcript)
- [x] M4 Close, reopen and reset paths; graph version increments only on change (T08–T13)
- [x] M5 Closure filter and step-free filter (T20, T21)
- [x] M6 Dijkstra, binary min-heap, stale-entry skipping, early stop (T01–T22, T52–T71)
- [x] M6 Equal-distance tie broken by fewer segments (T16, T16b, T17)
- [x] M7 Route output: node names, node IDs, edge IDs, distance, segments, version
- [x] Same endpoints → 0 m (T03); disconnected → NO_ROUTE (T07)
- [x] One-way edge (T14, T15); zero-weight edge (T18); parallel paths (T19)
- [x] M8 Bellman–Ford oracle and route-validity checker; 61/61 tests pass
- [x] M8 Measurements exported to CSV (timing, counters, closure impact, scaling)
- [x] Synthetic demo dataset, documented and labelled
- [x] README with Windows build, run, test and benchmark commands
- [x] Review report, slides, demo script, viva preparation

## Final review (23 November 2026)

- [ ] Verified NIT Trichy dataset with provenance and licence; actual counts reported
- [ ] Reconcile the proposal's 245/329/8 claims with the measured data
- [ ] Hand-checked expected routes on real campus data
- [~] Performance evaluation: grid scaling and Bellman–Ford timings exist as CSV; plots and repeated-run spread are missing
- [ ] Comparison with existing work: A* (straight-line heuristic); optional DIMACS NY benchmark
- [ ] Performance graphs and results analysis
- [ ] Optional: map or SVG route visualization
- [ ] Final report and slides
