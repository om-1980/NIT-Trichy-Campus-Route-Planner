#include "router.hpp"

#include <algorithm>
#include <functional>
#include <queue>
#include <stdexcept>
#include <tuple>

namespace campus {

const char* toString(RouteMode m) { return m == RouteMode::Standard ? "standard" : "step-free"; }

const char* toString(RouteStatus s) {
    switch (s) {
        case RouteStatus::Ok: return "OK";
        case RouteStatus::NoRoute: return "NO_ROUTE";
        case RouteStatus::InvalidSource: return "INVALID_SOURCE";
        default: return "INVALID_DESTINATION";
    }
}

bool edgeAllowed(const PhysicalEdge& e, RouteMode mode) {
    if (!e.open) return false;
    return mode == RouteMode::Standard || e.stepFree == StepFree::Yes;
}

SearchResult dijkstra(const CampusGraph& g, int source, int target, RouteMode mode) {
    const int n = g.nodeCount();
    SearchResult r;
    r.dist.assign(n, kInf);
    r.segments.assign(n, std::numeric_limits<int>::max());
    r.parent.assign(n, -1);
    r.parentEdge.assign(n, -1);
    if (source < 0 || source >= n) return r;

    using Entry = std::tuple<double, int, int>;  // (distance, segments, node index)
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> heap;
    std::vector<char> settled(n, 0);
    SearchStats& s = r.stats;
    const std::vector<PhysicalEdge>& edges = g.edges();

    r.dist[source] = 0.0;
    r.segments[source] = 0;
    heap.emplace(0.0, 0, source);
    s.heapPushes = 1;
    s.maxHeapSize = 1;

    while (!heap.empty()) {
        const auto [d, h, u] = heap.top();
        heap.pop();
        if (settled[u] || d != r.dist[u] || h != r.segments[u]) {  // stale entry
            ++s.stalePops;
            continue;
        }
        settled[u] = 1;
        ++s.settled;
        if (u == target) break;

        for (const AdjEntry& a : g.neighbours(u)) {
            ++s.adjacencyScanned;
            const PhysicalEdge& e = edges[a.edge];
            if (!edgeAllowed(e, mode)) {
                ++s.edgesFiltered;
                continue;
            }
            ++s.relaxAttempts;
            const double nd = d + e.distance;
            const int nh = h + 1;
            if (nd < r.dist[a.to] || (nd == r.dist[a.to] && nh < r.segments[a.to])) {
                r.dist[a.to] = nd;
                r.segments[a.to] = nh;
                r.parent[a.to] = u;
                r.parentEdge[a.to] = a.edge;
                heap.emplace(nd, nh, a.to);
                ++s.relaxSuccesses;
                ++s.heapPushes;
                s.maxHeapSize = std::max(s.maxHeapSize, heap.size());
            }
        }
    }
    return r;
}

Route findRoute(const CampusGraph& g, int source, int target, RouteMode mode) {
    Route route;
    route.mode = mode;
    route.source = source;
    route.target = target;
    route.graphVersion = g.version();
    const int n = g.nodeCount();
    if (source < 0 || source >= n) {
        route.status = RouteStatus::InvalidSource;
        return route;
    }
    if (target < 0 || target >= n) {
        route.status = RouteStatus::InvalidDestination;
        return route;
    }

    const SearchResult r = dijkstra(g, source, target, mode);
    route.stats = r.stats;
    if (r.dist[target] == kInf) {
        route.status = RouteStatus::NoRoute;
        return route;
    }

    // M7: follow parent pointers back from the target, then reverse once.
    for (int v = target, steps = 0; v != source; v = r.parent[v], ++steps) {
        if (r.parent[v] < 0 || steps > n) throw std::logic_error("broken parent chain in route reconstruction");
        route.nodes.push_back(v);
        route.edges.push_back(r.parentEdge[v]);
    }
    route.nodes.push_back(source);
    std::reverse(route.nodes.begin(), route.nodes.end());
    std::reverse(route.edges.begin(), route.edges.end());
    route.distance = r.dist[target];
    route.segments = r.segments[target];
    route.status = RouteStatus::Ok;
    return route;
}

Route findRouteByNodeId(const CampusGraph& g, int sourceNodeId, int targetNodeId, RouteMode mode) {
    return findRoute(g, g.indexOfNodeId(sourceNodeId), g.indexOfNodeId(targetNodeId), mode);
}

}  // namespace campus
