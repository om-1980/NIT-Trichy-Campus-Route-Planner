#pragma once
// Independent correctness oracle used only by tests and benchmarks (M8).
// It does not use the adjacency lists, the heap or edgeAllowed(): arcs and the
// mode filter are rebuilt directly from the physical edge records.

#include "graph.hpp"
#include "router.hpp"

#include <string>
#include <vector>

namespace campus {

struct ReferenceLabels {
    std::vector<double> dist;   // kInf if unreachable
    std::vector<int> segments;  // segment count of the lexicographically best label
    int rounds = 0;             // Bellman-Ford passes performed
};

// Bellman-Ford over (distance, segments) labels from one source.
ReferenceLabels bellmanFordReference(const CampusGraph& g, int source, RouteMode mode);

// |a - b| <= tol * max(1, |a|, |b|); both infinite counts as equal.
bool nearlyEqual(double a, double b, double tol = 1e-9);

struct RouteCheck {
    bool valid = true;
    std::string reason;
};

// Checks an OK route: correct endpoints, consecutive edges join the reported
// nodes in a permitted direction, every edge is open and allowed in the mode,
// no repeated node, edge distances sum to the reported distance, and the
// segment count equals the number of edges.
RouteCheck checkRoute(const CampusGraph& g, const Route& r, double tol = 1e-9);

}  // namespace campus
