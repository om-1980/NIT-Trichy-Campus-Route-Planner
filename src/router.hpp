#pragma once
// Route search (M5 feasibility filter, M6 Dijkstra, M7 reconstruction).

#include "graph.hpp"

#include <cstddef>
#include <limits>
#include <vector>

namespace campus {

enum class RouteMode { Standard, StepFree };
enum class RouteStatus { Ok, NoRoute, InvalidSource, InvalidDestination };

const char* toString(RouteMode m);
const char* toString(RouteStatus s);

constexpr double kInf = std::numeric_limits<double>::infinity();

// M5: an arc may be used if its physical edge is open and, in step-free mode,
// the edge is recorded as verified step-free. Applied on the fly while scanning.
bool edgeAllowed(const PhysicalEdge& e, RouteMode mode);

// Counters for one search. Definitions (also used in the evaluation report):
//   settled          vertices removed from the heap with a current label (each at most once)
//   stalePops        heap removals discarded because a better label was already recorded
//   adjacencyScanned adjacency entries read while expanding settled vertices
//   edgesFiltered    scanned entries skipped because the edge is closed / not allowed in the mode
//   relaxAttempts    scanned entries for which a candidate label was computed
//   relaxSuccesses   attempts that improved the neighbour's label (each causes one heap push)
//   heapPushes       total heap insertions including the source
//   maxHeapSize      largest heap size reached (lazy deletion keeps stale entries)
struct SearchStats {
    std::size_t settled = 0;
    std::size_t stalePops = 0;
    std::size_t adjacencyScanned = 0;
    std::size_t edgesFiltered = 0;
    std::size_t relaxAttempts = 0;
    std::size_t relaxSuccesses = 0;
    std::size_t heapPushes = 0;
    std::size_t maxHeapSize = 0;
};

struct SearchResult {
    std::vector<double> dist;     // distance part of the best label (m); kInf if unreached
    std::vector<int> segments;    // segment-count part of the best label
    std::vector<int> parent;      // previous node index on the best route, -1 if none
    std::vector<int> parentEdge;  // physical edge index used to enter the node, -1 if none
    SearchStats stats;
};

// M6: lazy-deletion Dijkstra from `source` with a binary min-heap. Labels are
// (distance, segments) compared lexicographically, so among equal-distance routes
// the one with fewer segments is preferred. If `target` >= 0 the search stops when
// the target is settled; only the target's label is then guaranteed final.
SearchResult dijkstra(const CampusGraph& g, int source, int target, RouteMode mode);

struct Route {
    RouteStatus status = RouteStatus::NoRoute;
    RouteMode mode = RouteMode::Standard;
    int source = -1;          // node index
    int target = -1;          // node index
    std::vector<int> nodes;   // node indices, source first
    std::vector<int> edges;   // physical edge indices; edges[i] joins nodes[i] -> nodes[i+1]
    double distance = kInf;
    int segments = 0;
    unsigned graphVersion = 0;
    SearchStats stats;
};

// M3 + M6 + M7: validate the request, search, and rebuild the route from parents.
Route findRoute(const CampusGraph& g, int source, int target, RouteMode mode);

// Same, addressed by graph-node IDs instead of internal indices.
Route findRouteByNodeId(const CampusGraph& g, int sourceNodeId, int targetNodeId, RouteMode mode);

}  // namespace campus
