#include "reference.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace campus {

namespace {

struct Arc {
    int u, v;
    double w;
};

bool permitted(const PhysicalEdge& e, RouteMode mode) {
    if (!e.open) return false;
    if (mode == RouteMode::StepFree && e.stepFree != StepFree::Yes) return false;
    return true;
}

}  // namespace

ReferenceLabels bellmanFordReference(const CampusGraph& g, int source, RouteMode mode) {
    const int n = g.nodeCount();
    ReferenceLabels out;
    out.dist.assign(n, kInf);
    out.segments.assign(n, std::numeric_limits<int>::max());
    if (source < 0 || source >= n) return out;

    std::vector<Arc> arcs;
    for (const PhysicalEdge& e : g.edges()) {
        if (!permitted(e, mode)) continue;
        arcs.push_back({e.from, e.to, e.distance});
        if (e.direction == Direction::TwoWay) arcs.push_back({e.to, e.from, e.distance});
    }

    out.dist[source] = 0.0;
    out.segments[source] = 0;
    for (int round = 1; round < std::max(n, 2); ++round) {
        bool changed = false;
        for (const Arc& a : arcs) {
            if (out.dist[a.u] == kInf) continue;
            const double nd = out.dist[a.u] + a.w;
            const int nh = out.segments[a.u] + 1;
            if (nd < out.dist[a.v] || (nd == out.dist[a.v] && nh < out.segments[a.v])) {
                out.dist[a.v] = nd;
                out.segments[a.v] = nh;
                changed = true;
            }
        }
        out.rounds = round;
        if (!changed) break;
    }
    return out;
}

bool nearlyEqual(double a, double b, double tol) {
    if (std::isinf(a) || std::isinf(b)) return a == b;
    return std::fabs(a - b) <= tol * std::max({1.0, std::fabs(a), std::fabs(b)});
}

RouteCheck checkRoute(const CampusGraph& g, const Route& r, double tol) {
    auto fail = [](const std::string& why) { return RouteCheck{false, why}; };
    if (r.status != RouteStatus::Ok) return fail("route status is not OK");
    if (r.nodes.empty()) return fail("empty node list");
    if (r.nodes.front() != r.source) return fail("route does not start at the source");
    if (r.nodes.back() != r.target) return fail("route does not end at the destination");
    if (r.edges.size() + 1 != r.nodes.size()) return fail("edge count is not node count - 1");
    if (r.segments != static_cast<int>(r.edges.size())) return fail("segment count differs from edge count");

    std::unordered_set<int> seen;
    for (int v : r.nodes) {
        if (v < 0 || v >= g.nodeCount()) return fail("node index out of range");
        if (!seen.insert(v).second) return fail("route repeats a node");
    }
    double sum = 0.0;
    for (std::size_t i = 0; i < r.edges.size(); ++i) {
        const int id = r.edges[i];
        if (id < 0 || id >= g.edgeCount()) return fail("edge index out of range");
        const PhysicalEdge& e = g.edges()[id];
        const int a = r.nodes[i], b = r.nodes[i + 1];
        const bool forward = e.from == a && e.to == b;
        const bool backward = e.from == b && e.to == a;
        if (!forward && !(backward && e.direction == Direction::TwoWay))
            return fail(e.id + " does not join the reported nodes in a permitted direction");
        if (!e.open) return fail(e.id + " is closed");
        if (r.mode == RouteMode::StepFree && e.stepFree != StepFree::Yes) return fail(e.id + " is not step-free");
        sum += e.distance;
    }
    if (!nearlyEqual(sum, r.distance, tol)) return fail("edge distances do not sum to the reported distance");
    return {};
}

}  // namespace campus
