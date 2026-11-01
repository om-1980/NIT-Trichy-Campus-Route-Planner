// Automated tests (module M8). Writes results/test_results.csv and
// results/correctness_metrics.csv; exit code is the number of failed tests.
//
// Usage: run_tests [data_dir] [results_dir]   (defaults: data/demo results)

#include "graph.hpp"
#include "reference.hpp"
#include "router.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace campus;

namespace {

struct Record {
    std::string id, category, scenario, setup, expected, actual;
    bool pass;
};

std::vector<Record> records;

void record(const std::string& id, const std::string& category, const std::string& scenario, const std::string& setup,
            const std::string& expected, const std::string& actual, bool pass) {
    records.push_back({id, category, scenario, setup, expected, actual, pass});
    std::cout << (pass ? "[PASS] " : "[FAIL] ") << id << "  " << scenario << "\n";
    if (!pass) std::cout << "         expected: " << expected << "\n         actual:   " << actual << "\n";
}

std::string fmt(double v) {
    std::ostringstream o;
    o << std::fixed << std::setprecision(1) << v;
    return o.str();
}

std::string join(const std::vector<std::string>& v, const std::string& sep) {
    std::string s;
    for (std::size_t i = 0; i < v.size(); ++i) s += (i ? sep : "") + v[i];
    return s;
}

std::vector<std::string> edgeIds(const CampusGraph& g, const Route& r) {
    std::vector<std::string> out;
    for (int e : r.edges) out.push_back(g.edges()[e].id);
    return out;
}

std::vector<int> nodeIds(const CampusGraph& g, const Route& r) {
    std::vector<int> out;
    for (int v : r.nodes) out.push_back(g.nodes()[v].id);
    return out;
}

std::string describe(const CampusGraph& g, const Route& r) {
    if (r.status != RouteStatus::Ok) return toString(r.status);
    std::vector<std::string> ids;
    for (int id : nodeIds(g, r)) ids.push_back(std::to_string(id));
    std::string edges = join(edgeIds(g, r), ",");
    return "OK " + fmt(r.distance) + " m, " + std::to_string(r.segments) + " seg, nodes " + join(ids, ">") +
           ", edges " + (edges.empty() ? "-" : edges);
}

// Same check as the oracle comparison, for a single route.
bool matchesOracle(const CampusGraph& g, const Route& r, bool compareSegments = true) {
    const ReferenceLabels ref = bellmanFordReference(g, r.source, r.mode);
    if (r.status == RouteStatus::NoRoute) return ref.dist[r.target] == kInf;
    if (r.status != RouteStatus::Ok) return false;
    if (!checkRoute(g, r).valid) return false;
    if (!nearlyEqual(ref.dist[r.target], r.distance)) return false;
    return !compareSegments || ref.segments[r.target] == r.segments;
}

bool loadText(const std::string& nodes, const std::string& edges, const std::string& places, CampusGraph& g,
              LoadReport& rep) {
    std::istringstream n(nodes), e(edges), p(places);
    return CampusGraph::loadStreams(n, e, places.empty() ? nullptr : &p, nullptr, g, rep);
}

const std::string kNodes3 = "1|A\n2|B\n3|C\n";

// ---------------------------------------------------------------- oracle sweep
struct SweepStats {
    long pairs = 0, agree = 0, okRoutes = 0, validRoutes = 0, noRoute = 0;
    std::string firstFailure;
};

// Every ordered (source, target) pair: Dijkstra (with early stop) vs Bellman-Ford.
void sweep(const CampusGraph& g, RouteMode mode, bool compareSegments, SweepStats& s) {
    const int n = g.nodeCount();
    for (int src = 0; src < n; ++src) {
        const ReferenceLabels ref = bellmanFordReference(g, src, mode);
        const SearchResult full = dijkstra(g, src, -1, mode);  // full tree must also agree
        for (int dst = 0; dst < n; ++dst) {
            const Route r = findRoute(g, src, dst, mode);
            ++s.pairs;
            bool ok;
            std::string why;
            if (r.status == RouteStatus::NoRoute) {
                ++s.noRoute;
                ok = ref.dist[dst] == kInf && full.dist[dst] == kInf;
                if (!ok) why = "Dijkstra NO_ROUTE but reference reachable";
            } else {
                ++s.okRoutes;
                const RouteCheck c = checkRoute(g, r);
                if (c.valid) ++s.validRoutes;
                ok = c.valid && nearlyEqual(ref.dist[dst], r.distance) && nearlyEqual(full.dist[dst], r.distance) &&
                     (!compareSegments || (ref.segments[dst] == r.segments && full.segments[dst] == r.segments));
                if (!c.valid) why = "invalid route: " + c.reason;
                else if (!ok) why = "label mismatch: dijkstra " + fmt(r.distance) + "/" + std::to_string(r.segments) +
                                    " reference " + fmt(ref.dist[dst]) + "/" + std::to_string(ref.segments[dst]);
            }
            if (ok) ++s.agree;
            else if (s.firstFailure.empty())
                s.firstFailure = std::string(toString(mode)) + " " + std::to_string(g.nodes()[src].id) + "->" +
                                 std::to_string(g.nodes()[dst].id) + ": " + why;
        }
    }
}

std::string sweepText(const SweepStats& s) {
    return "agree " + std::to_string(s.agree) + "/" + std::to_string(s.pairs) + " pairs; valid routes " +
           std::to_string(s.validRoutes) + "/" + std::to_string(s.okRoutes) + "; NO_ROUTE pairs " +
           std::to_string(s.noRoute) + (s.firstFailure.empty() ? "" : "; first failure: " + s.firstFailure);
}

SweepStats totalSweep;  // aggregated across all sweep tests for the metrics file

void addTotals(const SweepStats& s) {
    totalSweep.pairs += s.pairs;
    totalSweep.agree += s.agree;
    totalSweep.okRoutes += s.okRoutes;
    totalSweep.validRoutes += s.validRoutes;
    totalSweep.noRoute += s.noRoute;
}

// Random graph text with non-contiguous node IDs, parallel edges, zero weights,
// one-way edges, closed edges, mixed step-free flags and possibly isolated nodes.
void randomGraphText(std::mt19937& rng, int n, int m, bool fractional, std::string& nodes, std::string& edges) {
    std::ostringstream ns, es;
    std::vector<int> ids(n);
    for (int i = 0; i < n; ++i) ids[i] = 1000 + 7 * i;
    std::shuffle(ids.begin(), ids.end(), rng);
    for (int i = 0; i < n; ++i) ns << ids[i] << "|N" << ids[i] << "\n";
    std::uniform_int_distribution<int> pick(0, n - 1), pct(0, 99), w(1, 100);
    std::uniform_real_distribution<double> wr(0.0, 100.0);
    for (int k = 0; k < m && n > 1; ++k) {
        int u = pick(rng), v = pick(rng);
        while (v == u) v = pick(rng);
        std::ostringstream dist;
        if (pct(rng) < 10) dist << 0;
        else if (fractional) dist << std::setprecision(17) << wr(rng);
        else dist << w(rng);
        const int sf = pct(rng);
        const bool stairs = sf < 15;
        es << "R" << k << "|" << ids[u] << "|" << ids[v] << "|" << dist.str() << "|"
           << (pct(rng) < 25 ? "one-way" : "two-way") << "|" << (pct(rng) < 15 ? "closed" : "open") << "|"
           << (stairs ? "no" : (sf < 75 ? "yes" : (sf < 90 ? "unknown" : "no"))) << "|"
           << (stairs ? "stairs" : "walkway") << "\n";
    }
    nodes = ns.str();
    edges = es.str();
}

}  // namespace

int main(int argc, char** argv) {
    const std::string dataDir = argc > 1 ? argv[1] : "data/demo";
    const std::string resultsDir = argc > 2 ? argv[2] : "results";

    CampusGraph demo;
    LoadReport demoRep;
    if (!CampusGraph::loadDirectory(dataDir, demo, demoRep)) {
        std::cout << "Cannot load demo dataset from " << dataDir << "\n";
        for (const auto& e : demoRep.errors) std::cout << "  " << e << "\n";
        return 100;
    }
    const RouteMode STD = RouteMode::Standard, SF = RouteMode::StepFree;
    auto fresh = [&]() { CampusGraph g = demo; return g; };

    // ===================================================== dataset integrity
    {
        const GraphSummary s = summarize(demo);
        std::ostringstream a;
        a << s.nodes << " nodes, " << s.physicalEdges << " edges (" << s.oneWay << " one-way), " << s.adjacencyEntries
          << " adjacency entries, " << s.places << " places, " << s.closed << " closed, " << s.components
          << " components, " << demoRep.warnings.size() << " warnings";
        const bool pass = s.nodes == 22 && s.physicalEdges == 31 && s.oneWay == 1 && s.adjacencyEntries == 61 &&
                          s.places == 10 && s.closed == 1 && s.components == 2 && demoRep.warnings.empty();
        record("T00", "data", "Demo dataset loads with expected counts", "load data/demo",
               "22 nodes, 31 edges (1 one-way), 61 adjacency entries, 10 places, 1 closed, 2 components, 0 warnings",
               a.str(), pass);
    }

    // ===================================================== core routing
    {
        const Route r = findRouteByNodeId(demo, 101, 106, STD);
        record("T01", "routing", "Normal shortest route", "demo; 101 Main Gate -> 106 Library; standard",
               "OK 590.0 m via E01,E02,E03,E22 (matches oracle, valid)", describe(demo, r),
               r.status == RouteStatus::Ok && nearlyEqual(r.distance, 590) &&
                   edgeIds(demo, r) == std::vector<std::string>{"E01", "E02", "E03", "E22"} && matchesOracle(demo, r));
    }
    {
        CampusGraph g;
        LoadReport rep;
        loadText(kNodes3, "D1|1|2|10|two-way|open|yes|walkway\nD2|1|3|3|two-way|open|yes|walkway\nD3|3|2|4|two-way|open|yes|walkway\n", "", g, rep);
        const Route r = findRouteByNodeId(g, 1, 2, STD);
        record("T02", "routing", "Multi-hop route shorter than a direct edge", "A-B 10 direct; A-C 3, C-B 4",
               "OK 7.0 m via A>C>B (not the 10 m direct edge)", describe(g, r),
               r.status == RouteStatus::Ok && nearlyEqual(r.distance, 7) && nodeIds(g, r) == std::vector<int>{1, 3, 2} &&
                   matchesOracle(g, r));
    }
    {
        const Route r = findRouteByNodeId(demo, 101, 105, STD);
        record("T02b", "routing", "Multi-hop beats direct perimeter road (demo)", "demo; 101 -> 105; E21 direct = 520 m",
               "OK 473.0 m via E01,E02,E04 (not E21)", describe(demo, r),
               r.status == RouteStatus::Ok && nearlyEqual(r.distance, 473) &&
                   edgeIds(demo, r) == std::vector<std::string>{"E01", "E02", "E04"} && matchesOracle(demo, r));
    }
    {
        const Route r = findRouteByNodeId(demo, 106, 106, STD);
        record("T03", "routing", "Source equals destination", "demo; 106 -> 106",
               "OK 0.0 m, 0 segments, single node 106", describe(demo, r),
               r.status == RouteStatus::Ok && r.distance == 0.0 && r.segments == 0 && nodeIds(demo, r) == std::vector<int>{106} &&
                   r.edges.empty());
    }
    {
        const Route a = findRouteByNodeId(demo, 999, 106, STD);
        const Route b = findRouteByNodeId(demo, 101, -5, STD);
        record("T04", "input", "Invalid source graph-node ID", "demo; 999 -> 106", "INVALID_SOURCE", describe(demo, a),
               a.status == RouteStatus::InvalidSource);
        record("T05", "input", "Invalid destination graph-node ID", "demo; 101 -> -5", "INVALID_DESTINATION",
               describe(demo, b), b.status == RouteStatus::InvalidDestination);
    }
    {
        const Place* p1 = demo.findPlace(1);
        const Place* p99 = demo.findPlace(99);
        const Place* p0 = demo.findPlace(0);
        std::string actual = std::string("place 1 -> node ") + (p1 ? std::to_string(demo.nodes()[p1->node].id) : "none") +
                             "; place 99 " + (p99 ? "found" : "not found") + "; place 0 " + (p0 ? "found" : "not found") +
                             "; node ID 1 " + (demo.indexOfNodeId(1) >= 0 ? "exists" : "does not exist");
        record("T06", "input", "Invalid place IDs rejected; place IDs distinct from node IDs",
               "demo; lookup place 1, 99, 0 and node 1",
               "place 1 -> node 101; place 99 not found; place 0 not found; node ID 1 does not exist", actual,
               p1 && demo.nodes()[p1->node].id == 101 && !p99 && !p0 && demo.indexOfNodeId(1) < 0);
    }
    {
        const Route r = findRouteByNodeId(demo, 101, 130, STD);
        const ReferenceLabels ref = bellmanFordReference(demo, demo.indexOfNodeId(101), STD);
        record("T07", "routing", "Unreachable destination (disconnected component)", "demo; 101 -> 130",
               "NO_ROUTE; reference distance = infinity", describe(demo, r) + "; reference " +
               (ref.dist[demo.indexOfNodeId(130)] == kInf ? "infinity" : fmt(ref.dist[demo.indexOfNodeId(130)])),
               r.status == RouteStatus::NoRoute && ref.dist[demo.indexOfNodeId(130)] == kInf);
    }

    // ===================================================== closures
    {
        CampusGraph g = fresh();
        g.setEdgeOpen("E22", false);
        const Route r = findRouteByNodeId(g, 101, 106, STD);
        record("T08", "closure", "Closing an edge gives the alternative route", "demo; close E22; 101 -> 106",
               "OK 608.0 m via E01,E02,E04,E05 (+18 m)", describe(g, r),
               r.status == RouteStatus::Ok && nearlyEqual(r.distance, 608) &&
                   edgeIds(g, r) == std::vector<std::string>{"E01", "E02", "E04", "E05"} && matchesOracle(g, r));
    }
    {
        CampusGraph g = fresh();
        const std::vector<std::string> order = {"E22", "E04", "E20", "E21", "E06", "E24"};
        std::vector<std::string> got;
        bool pass = true;
        const std::vector<double> want = {608, 620, 655, 1229, 1262, -1};
        for (std::size_t i = 0; i < order.size(); ++i) {
            g.setEdgeOpen(order[i], false);
            const Route r = findRouteByNodeId(g, 101, 106, STD);
            got.push_back(order[i] + ":" + (r.status == RouteStatus::Ok ? fmt(r.distance) : toString(r.status)));
            pass = pass && matchesOracle(g, r) &&
                   (want[i] < 0 ? r.status == RouteStatus::NoRoute : nearlyEqual(r.distance, want[i]));
        }
        record("T09", "closure", "Closing paths one by one until no route remains",
               "demo; 101 -> 106; close E22,E04,E20,E21,E06,E24 in turn",
               "E22:608.0, E04:620.0, E20:655.0, E21:1229.0, E06:1262.0, E24:NO_ROUTE", join(got, ", "), pass);
    }
    {
        CampusGraph g = fresh();
        g.setEdgeOpen("E22", false);
        const Route closed = findRouteByNodeId(g, 101, 106, STD);
        g.setEdgeOpen("E22", true);
        const Route reopened = findRouteByNodeId(g, 101, 106, STD);
        record("T10", "closure", "Reopening restores the original shortest distance",
               "demo; close E22, route, reopen E22, route 101 -> 106", "608.0 m while closed; 590.0 m after reopening",
               fmt(closed.distance) + " m while closed; " + describe(g, reopened) + " after reopening",
               nearlyEqual(closed.distance, 608) && nearlyEqual(reopened.distance, 590) &&
                   edgeIds(g, reopened) == std::vector<std::string>{"E01", "E02", "E03", "E22"});
    }
    {
        CampusGraph g = fresh();
        const int e22 = g.indexOfEdgeId("E22");
        int refs = 0;
        for (int v = 0; v < g.nodeCount(); ++v)
            for (const AdjEntry& a : g.neighbours(v)) refs += a.edge == e22 ? 1 : 0;
        const Route before = findRouteByNodeId(g, 106, 101, STD);
        g.setEdgeOpen("E22", false);
        const Route fwd = findRouteByNodeId(g, 101, 106, STD);
        const Route back = findRouteByNodeId(g, 106, 101, STD);
        const auto uses22 = [&](const Route& r) {
            const auto ids = edgeIds(g, r);
            return std::find(ids.begin(), ids.end(), "E22") != ids.end();
        };
        record("T11", "closure", "Closing a two-way edge blocks both directions",
               "demo; E22 (104<->106) closed; route both ways",
               "2 adjacency entries share E22; reverse route used E22 before (590.0); after closing neither direction uses E22 (608.0 both ways)",
               std::to_string(refs) + " adjacency entries share E22; before: " + describe(g, before) + "; after fwd " +
                   fmt(fwd.distance) + (uses22(fwd) ? " uses E22" : "") + ", back " + fmt(back.distance) +
                   (uses22(back) ? " uses E22" : ""),
               refs == 2 && uses22(before) && !uses22(fwd) && !uses22(back) && nearlyEqual(fwd.distance, 608) &&
                   nearlyEqual(back.distance, 608) && matchesOracle(g, fwd) && matchesOracle(g, back));
    }
    {
        CampusGraph g = fresh();
        const unsigned v0 = g.version();
        const StatusChange unknown = g.setEdgeOpen("E99", false);
        const unsigned v1 = g.version();
        const StatusChange lower = g.setEdgeOpen("e22", false);
        const unsigned v2 = g.version();
        const StatusChange again = g.setEdgeOpen("E22", false);
        const unsigned v3 = g.version();
        const int restored = g.resetToDataStatus();
        const unsigned v4 = g.version();
        std::ostringstream a;
        a << "E99 " << (unknown == StatusChange::UnknownEdge ? "unknown" : "accepted") << " (v" << v1 << "); e22 "
          << (lower == StatusChange::Changed ? "closed" : "not closed") << " (v" << v2 << "); E22 again "
          << (again == StatusChange::Unchanged ? "unchanged" : "changed") << " (v" << v3 << "); reset restored "
          << restored << " (v" << v4 << "), E31 " << (g.edges()[g.indexOfEdgeId("E31")].open ? "open" : "closed");
        record("T12", "closure", "Unknown edge ID, case-insensitive ID, repeated close, reset and graph version",
               "demo; close E99, e22, E22 again, then reset",
               "E99 unknown (v1); e22 closed (v2); E22 again unchanged (v2); reset restored 1 (v3), E31 closed", a.str(),
               v0 == 1 && unknown == StatusChange::UnknownEdge && v1 == 1 && lower == StatusChange::Changed && v2 == 2 &&
                   again == StatusChange::Unchanged && v3 == 2 && restored == 1 && v4 == 3 &&
                   !g.edges()[g.indexOfEdgeId("E31")].open);
    }
    {
        CampusGraph g = fresh();
        const Route before = findRouteByNodeId(g, 101, 104, STD);
        g.setEdgeOpen("E31", true);
        const Route after = findRouteByNodeId(g, 101, 104, STD);
        record("T13", "closure", "Edge closed in the data file is not used until reopened",
               "demo; 101 -> 104; then reopen E31",
               "420.0 m via E01,E02,E03; after reopening E31: 350.0 m via E01,E31",
               describe(g, before) + " | " + describe(g, after),
               nearlyEqual(before.distance, 420) && nearlyEqual(after.distance, 350) &&
                   edgeIds(g, after) == std::vector<std::string>{"E01", "E31"});
    }

    // ===================================================== direction, ties, weights, modes
    {
        const Route a = findRouteByNodeId(demo, 118, 116, STD);
        const Route b = findRouteByNodeId(demo, 116, 118, STD);
        const auto ids = edgeIds(demo, b);
        record("T14", "direction", "One-way edge used only in its direction (demo)",
               "demo; E23 is one-way 118 -> 115; route 118->116 and 116->118",
               "118->116: 382.0 m via E23,E15; 116->118: 491.0 m without E23",
               describe(demo, a) + " | " + describe(demo, b),
               nearlyEqual(a.distance, 382) && edgeIds(demo, a) == std::vector<std::string>{"E23", "E15"} &&
                   nearlyEqual(b.distance, 491) && std::find(ids.begin(), ids.end(), "E23") == ids.end() &&
                   matchesOracle(demo, a) && matchesOracle(demo, b));
    }
    {
        CampusGraph g;
        LoadReport rep;
        loadText("1|A\n2|B\n", "W1|1|2|10|one-way|open|yes|road\n", "", g, rep);
        const Route a = findRouteByNodeId(g, 1, 2, STD);
        const Route b = findRouteByNodeId(g, 2, 1, STD);
        record("T15", "direction", "One-way edge: reverse direction has no route", "A->B one-way 10 m only",
               "A->B OK 10.0 m; B->A NO_ROUTE", describe(g, a) + " | " + describe(g, b),
               a.status == RouteStatus::Ok && nearlyEqual(a.distance, 10) && b.status == RouteStatus::NoRoute);
    }
    {
        const Route r = findRouteByNodeId(demo, 113, 120, STD);
        record("T16", "ties", "Equal distance: fewer segments preferred (demo)",
               "demo; 113 -> 120; 555 m via E29,E27 (2 seg) or E12,E11,E28,E27 (4 seg)",
               "OK 555.0 m, 2 segments via E29,E27", describe(demo, r),
               nearlyEqual(r.distance, 555) && r.segments == 2 &&
                   edgeIds(demo, r) == std::vector<std::string>{"E29", "E27"} && matchesOracle(demo, r));
    }
    {
        // The 3-segment route reaches D first (C is settled before E), so the
        // 2-segment route must replace it through the tie-break rule.
        CampusGraph g;
        LoadReport rep;
        loadText("1|A\n2|B\n3|C\n4|D\n5|E\n",
                 "K1|1|2|2|two-way|open|yes|walkway\nK2|2|3|2|two-way|open|yes|walkway\nK3|3|4|2|two-way|open|yes|walkway\n"
                 "K4|1|5|5|two-way|open|yes|walkway\nK5|5|4|1|two-way|open|yes|walkway\n", "", g, rep);
        const Route r = findRouteByNodeId(g, 1, 4, STD);
        record("T16b", "ties", "Equal distance: later-found route with fewer segments replaces earlier one",
               "A-B-C-D 2+2+2 (3 seg, found first); A-E-D 5+1 (2 seg)", "OK 6.0 m, 2 segments via A>E>D",
               describe(g, r), nearlyEqual(r.distance, 6) && nodeIds(g, r) == std::vector<int>{1, 5, 4} && matchesOracle(g, r));
    }
    {
        CampusGraph g;
        LoadReport rep;
        loadText("1|A\n2|B\n3|C\n4|D\n",
                 "Q1|1|2|5|two-way|open|yes|walkway\nQ2|2|4|5|two-way|open|yes|walkway\n"
                 "Q3|1|3|5|two-way|open|yes|walkway\nQ4|3|4|5|two-way|open|yes|walkway\n", "", g, rep);
        const Route r = findRouteByNodeId(g, 1, 4, STD);
        const auto n = nodeIds(g, r);
        record("T17", "ties", "Equal distance and equal segments: either route accepted",
               "A-B-D and A-C-D both 10 m, 2 segments", "OK 10.0 m, 2 segments, A>B>D or A>C>D, valid",
               describe(g, r),
               nearlyEqual(r.distance, 10) && r.segments == 2 &&
                   (n == std::vector<int>{1, 2, 4} || n == std::vector<int>{1, 3, 4}) && matchesOracle(g, r));
    }
    {
        CampusGraph g;
        LoadReport rep;
        loadText(kNodes3, "Z1|1|2|0|two-way|open|yes|walkway\nZ2|2|3|5|two-way|open|yes|walkway\nZ3|1|3|6|two-way|open|yes|walkway\n", "", g, rep);
        const Route a = findRouteByNodeId(g, 1, 3, STD);
        const Route b = findRouteByNodeId(g, 1, 2, STD);
        record("T18", "weights", "Zero-weight edge handled", "A-B 0 m, B-C 5 m, A-C 6 m",
               "A->C OK 5.0 m via A>B>C; A->B OK 0.0 m, 1 segment", describe(g, a) + " | " + describe(g, b),
               nearlyEqual(a.distance, 5) && nodeIds(g, a) == std::vector<int>{1, 2, 3} && b.distance == 0.0 &&
                   b.segments == 1 && matchesOracle(g, a) && matchesOracle(g, b));
    }
    {
        CampusGraph g;
        LoadReport rep;
        loadText("1|A\n2|B\n", "P1|1|2|30|two-way|open|yes|walkway\nP2|1|2|20|two-way|open|yes|walkway\n", "", g, rep);
        const Route a = findRouteByNodeId(g, 1, 2, STD);
        g.setEdgeOpen("P2", false);
        const Route b = findRouteByNodeId(g, 1, 2, STD);
        record("T19", "weights", "Parallel physical paths kept separate", "A-B by P1 30 m and P2 20 m; then close P2",
               "20.0 m via P2; after closing P2: 30.0 m via P1", describe(g, a) + " | " + describe(g, b),
               edgeIds(g, a) == std::vector<std::string>{"P2"} && edgeIds(g, b) == std::vector<std::string>{"P1"} &&
                   nearlyEqual(b.distance, 30));
    }
    {
        const Route r = findRouteByNodeId(demo, 101, 106, SF);
        bool allSf = true;
        for (int e : r.edges) allSf = allSf && demo.edges()[e].stepFree == StepFree::Yes;
        record("T20", "mode", "Step-free mode avoids the stairs shortcut", "demo; 101 -> 106; step-free",
               "OK 608.0 m via E01,E02,E04,E05; every edge step_free=yes", describe(demo, r) + (allSf ? "; all step-free" : "; NOT all step-free"),
               nearlyEqual(r.distance, 608) && allSf && matchesOracle(demo, r));
    }
    {
        const Route a = findRouteByNodeId(demo, 110, 118, STD);
        const Route b = findRouteByNodeId(demo, 110, 118, SF);
        record("T21", "mode", "Step-free mode excludes step_free=unknown", "demo; 110 -> 118; E25 is unknown",
               "standard 350.0 m via E25; step-free 468.0 m via E09,E17,E18", describe(demo, a) + " | " + describe(demo, b),
               nearlyEqual(a.distance, 350) && nearlyEqual(b.distance, 468) &&
                   edgeIds(demo, b) == std::vector<std::string>{"E09", "E17", "E18"});
    }

    {
        // Full search from 101: every reachable vertex is settled exactly once and
        // every other heap removal is counted as stale.
        const int src = demo.indexOfNodeId(101);
        const SearchResult r = dijkstra(demo, src, -1, STD);
        std::size_t reachable = 0;
        for (double d : r.dist) reachable += d < kInf ? 1 : 0;
        const SearchStats& s = r.stats;
        std::ostringstream a;
        a << "settled " << s.settled << " of " << reachable << " reachable; pushes " << s.heapPushes << " = settled + stale "
          << s.stalePops << "; scanned " << s.adjacencyScanned << " = filtered " << s.edgesFiltered << " + attempts "
          << s.relaxAttempts << "; successes " << s.relaxSuccesses << " = pushes - 1";
        record("T22", "metrics", "Search counters are consistent (stale entries skipped)",
               "demo; full Dijkstra from 101 (no early stop), standard",
               "settled = reachable vertices (20); pushes = settled + stale; scanned = filtered + attempts", a.str(),
               s.settled == reachable && reachable == 20 && s.heapPushes == s.settled + s.stalePops &&
                   s.adjacencyScanned == s.edgesFiltered + s.relaxAttempts && s.relaxSuccesses + 1 == s.heapPushes);
    }

    // ===================================================== data validation (rejections)
    struct Bad {
        const char* id;
        const char* scenario;
        std::string nodes, edges, places, mustContain;
    };
    const std::string goodEdge = "E1|1|2|10|two-way|open|yes|walkway\n";
    const std::vector<Bad> bad = {
        {"T30", "Negative distance rejected", kNodes3, "E1|1|2|-5|two-way|open|yes|walkway\n", "", "must be >= 0"},
        {"T31", "NaN distance rejected", kNodes3, "E1|1|2|nan|two-way|open|yes|walkway\n", "", "must be finite"},
        {"T32", "Infinite distance rejected", kNodes3, "E1|1|2|inf|two-way|open|yes|walkway\n", "", "must be finite"},
        {"T33", "Non-numeric distance rejected", kNodes3, "E1|1|2|12m|two-way|open|yes|walkway\n", "", "not a number"},
        {"T34", "Duplicate node ID rejected", "1|A\n2|B\n1|C\n", goodEdge, "", "duplicate node_id 1"},
        {"T35", "Duplicate edge ID rejected (case-insensitive)", kNodes3, goodEdge + "e1|2|3|5|two-way|open|yes|walkway\n", "",
         "duplicate edge_id E1"},
        {"T36", "Edge to unknown node rejected", kNodes3, "E1|1|9|10|two-way|open|yes|walkway\n", "", "to_node 9 is not in nodes.txt"},
        {"T37", "Invalid direction flag rejected", kNodes3, "E1|1|2|10|both|open|yes|walkway\n", "", "direction must be"},
        {"T38", "Invalid status rejected", kNodes3, "E1|1|2|10|two-way|blocked|yes|walkway\n", "", "status must be"},
        {"T39", "Invalid step_free flag rejected", kNodes3, "E1|1|2|10|two-way|open|maybe|walkway\n", "", "step_free must be"},
        {"T40", "Stairs marked step-free rejected", kNodes3, "E1|1|2|10|two-way|open|yes|stairs\n", "", "stairs path cannot"},
        {"T41", "Malformed edge record (missing fields) rejected", kNodes3, "E1|1|2|10\n", "", "expected 8 or 9"},
        {"T42", "Non-integer node ID rejected", "1|A\nB2|B\n", goodEdge, "", "node_id must be"},
        {"T43", "Empty node name rejected", "1|A\n2|\n", goodEdge, "", "node name is empty"},
        {"T44", "Self-loop edge rejected", kNodes3, "E1|1|1|10|two-way|open|yes|walkway\n", "", "self-loop"},
        {"T45", "Partial coordinates rejected", "1|A|0|\n2|B\n", goodEdge, "", "coordinates must be"},
        {"T46", "Duplicate place ID rejected", kNodes3, goodEdge, "1|1|Gate\n1|2|Library\n", "duplicate place_id 1"},
        {"T47", "Place mapped to unknown node rejected", kNodes3, goodEdge, "1|7|Gate\n", "not a node in nodes.txt"},
        {"T48", "Empty node file rejected", "# only comments\n", "", "", "no node records"},
        {"T49", "Node ID overflow rejected", "99999999999|A\n2|B\n", "", "", "node_id must be"},
    };
    for (const Bad& b : bad) {
        CampusGraph g = fresh();
        LoadReport rep;
        const bool loaded = loadText(b.nodes, b.edges, b.places, g, rep);
        std::string msg = rep.errors.empty() ? "(no error)" : rep.errors.front();
        bool found = false;
        for (const auto& e : rep.errors) found = found || e.find(b.mustContain) != std::string::npos;
        const bool untouched = g.nodeCount() == demo.nodeCount();  // failed load must not overwrite the target
        // Show the file that contains the bad record.
        std::string setup = !b.places.empty() ? "places.txt: " + b.places
                            : (b.edges.empty() || b.edges == goodEdge) ? "nodes.txt: " + b.nodes
                                                                       : "edges.txt: " + b.edges;
        std::replace(setup.begin(), setup.end(), '\n', ' ');
        record(b.id, "validation", b.scenario, setup, "DATA_ERROR containing '" + b.mustContain + "'",
               (loaded ? "LOADED; " : "DATA_ERROR: ") + msg + (untouched ? "" : " [target graph modified]"),
               !loaded && found && untouched);
    }
    {
        CampusGraph g;
        LoadReport rep;
        const bool loaded = CampusGraph::loadDirectory("data/does_not_exist", g, rep);
        record("T50", "validation", "Missing data files reported", "load data/does_not_exist",
               "DATA_ERROR: cannot open nodes.txt, edges.txt, places.txt",
               (loaded ? "LOADED" : "DATA_ERROR: " + std::to_string(rep.errors.size()) + " errors, first: ") +
                   (rep.errors.empty() ? "" : rep.errors.front()),
               !loaded && rep.errors.size() == 3 && rep.errors[0].find("cannot open") != std::string::npos);
    }
    {
        CampusGraph g;
        LoadReport rep;
        const bool loaded = loadText("1|A|0|0\n2|B|30|40\n", "E1|1|2|20|two-way|open|yes|walkway\n", "", g, rep);
        record("T51", "validation", "Distance shorter than straight line gives a warning (not an error)",
               "A(0,0) B(30,40) straight 50 m; E1 = 20 m", "loaded with 1 warning",
               std::string(loaded ? "loaded" : "rejected") + " with " + std::to_string(rep.warnings.size()) + " warning(s)",
               loaded && rep.warnings.size() == 1);
    }

    // ===================================================== oracle sweeps (demo dataset)
    {
        const std::vector<std::pair<std::string, std::vector<std::string>>> scenarios = {
            {"no extra closures", {}},
            {"E22 closed", {"E22"}},
            {"E22,E04 closed", {"E22", "E04"}},
            {"E05,E22,E24 closed (library cut off)", {"E05", "E22", "E24"}},
            {"E31 reopened", {}},
            {"E08,E13,E19,E23 closed", {"E08", "E13", "E19", "E23"}},
        };
        int k = 52;  // T52-T63
        for (const auto& sc : scenarios) {
            for (RouteMode mode : {STD, SF}) {
                CampusGraph g = fresh();
                for (const auto& id : sc.second) g.setEdgeOpen(id, false);
                if (sc.first == "E31 reopened") g.setEdgeOpen("E31", true);
                SweepStats s;
                sweep(g, mode, true, s);
                addTotals(s);
                std::ostringstream id;
                id << "T" << k++;
                record(id.str(), "oracle", std::string("All-pairs agreement with Bellman-Ford, ") + toString(mode),
                       "demo; " + sc.first + "; all 22x22 node pairs", "agree 484/484; all OK routes valid",
                       sweepText(s), s.agree == s.pairs && s.validRoutes == s.okRoutes && s.pairs == 484);
            }
        }
    }

    // ===================================================== randomized property tests
    {
        std::mt19937 rng(20261102);
        const int graphs = 300;
        SweepStats s;
        long nodesTotal = 0;
        for (int i = 0; i < graphs; ++i) {
            const int n = 1 + static_cast<int>(rng() % 30);
            const int m = static_cast<int>(rng() % (3 * n + 1));
            std::string nt, et;
            randomGraphText(rng, n, m, false, nt, et);
            CampusGraph g;
            LoadReport rep;
            if (!loadText(nt, et, "", g, rep)) {
                s.firstFailure = "generator produced invalid data: " + rep.errors.front();
                ++s.pairs;
                break;
            }
            nodesTotal += n;
            sweep(g, i % 2 ? SF : STD, true, s);
        }
        addTotals(s);
        record("T70", "oracle", "Random graphs, integer weights: labels match Bellman-Ford",
               "300 seeded random graphs, 1-30 nodes, zero weights, parallel/one-way/closed edges; all pairs",
               "all pairs agree (distance and segments); all OK routes valid", sweepText(s) + "; nodes total " + std::to_string(nodesTotal),
               s.agree == s.pairs && s.validRoutes == s.okRoutes && s.pairs > 0);
    }
    {
        std::mt19937 rng(7);
        SweepStats s;
        for (int i = 0; i < 200; ++i) {
            const int n = 2 + static_cast<int>(rng() % 25);
            std::string nt, et;
            randomGraphText(rng, n, 3 * n, true, nt, et);
            CampusGraph g;
            LoadReport rep;
            if (!loadText(nt, et, "", g, rep)) {
                s.firstFailure = "generator produced invalid data: " + rep.errors.front();
                ++s.pairs;
                break;
            }
            sweep(g, i % 2 ? SF : STD, false, s);
        }
        addTotals(s);
        record("T71", "oracle", "Random graphs, fractional weights: distances match within tolerance",
               "200 seeded random graphs, 2-26 nodes, real weights; relative tolerance 1e-9",
               "all pairs agree on distance; all OK routes valid", sweepText(s),
               s.agree == s.pairs && s.validRoutes == s.okRoutes && s.pairs > 0);
    }

    // ===================================================== write results
    int failed = 0;
    for (const auto& r : records) failed += r.pass ? 0 : 1;
    const int total = static_cast<int>(records.size());

    auto csv = [](const std::string& s) {
        std::string out = "\"";
        for (char c : s) out += (c == '"') ? std::string("\"\"") : std::string(1, c);
        return out + "\"";
    };
    std::ofstream f(resultsDir + "/test_results.csv");
    f << "test_id,category,scenario,setup,expected,actual,status\n";
    for (const auto& r : records)
        f << r.id << "," << r.category << "," << csv(r.scenario) << "," << csv(r.setup) << "," << csv(r.expected) << ","
          << csv(r.actual) << "," << (r.pass ? "PASS" : "FAIL") << "\n";

    std::ofstream m(resultsDir + "/correctness_metrics.csv");
    m << "metric,numerator,denominator,value_percent\n";
    auto pct = [](long a, long b) { std::ostringstream o; o << std::fixed << std::setprecision(2) << (b ? 100.0 * a / b : 0.0); return o.str(); };
    m << "test_pass_rate," << (total - failed) << "," << total << "," << pct(total - failed, total) << "\n";
    m << "oracle_agreement_rate (all sweep pairs incl. NO_ROUTE)," << totalSweep.agree << "," << totalSweep.pairs << ","
      << pct(totalSweep.agree, totalSweep.pairs) << "\n";
    m << "route_validity_rate (OK routes)," << totalSweep.validRoutes << "," << totalSweep.okRoutes << ","
      << pct(totalSweep.validRoutes, totalSweep.okRoutes) << "\n";
    m << "no_route_pairs_checked," << totalSweep.noRoute << "," << totalSweep.pairs << ","
      << pct(totalSweep.noRoute, totalSweep.pairs) << "\n";

    std::cout << "\n" << (total - failed) << "/" << total << " tests passed. Oracle pairs agreeing: " << totalSweep.agree
              << "/" << totalSweep.pairs << ". Valid OK routes: " << totalSweep.validRoutes << "/" << totalSweep.okRoutes
              << ".\nWrote " << resultsDir << "/test_results.csv and " << resultsDir << "/correctness_metrics.csv\n";
    return failed;
}
