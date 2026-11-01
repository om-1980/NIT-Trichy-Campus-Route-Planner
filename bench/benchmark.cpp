// Measurement program for the evaluation metrics (module M8, timing part).
//
// Only the search call is timed (findRoute / dijkstra / bellmanFordReference).
// File loading is timed separately; nothing is printed inside a timed region.
// Timing: std::chrono::steady_clock around a batch of K identical calls; the
// per-call time of a batch is (batch time / K); B batches give B samples and
// the median sample is reported.
//
// Usage: benchmark [data_dir] [results_dir]   (defaults: data/demo results)

#include "graph.hpp"
#include "reference.hpp"
#include "router.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#ifndef BENCH_FLAGS
#define BENCH_FLAGS "unknown"
#endif

using namespace campus;
using Clock = std::chrono::steady_clock;

namespace {

volatile double gSink = 0.0;  // keeps timed calls from being optimised away

struct Timing {
    double medianUs = 0, minUs = 0, maxUs = 0;
    int batches = 0, perBatch = 0;
};

Timing timeCall(const std::function<double()>& call, int batches, int perBatch) {
    std::vector<double> samples;
    samples.reserve(batches);
    for (int b = 0; b < batches; ++b) {
        double acc = 0.0;
        const auto t0 = Clock::now();
        for (int k = 0; k < perBatch; ++k) acc += call();
        const auto t1 = Clock::now();
        gSink = gSink + acc;
        samples.push_back(std::chrono::duration<double, std::micro>(t1 - t0).count() / perBatch);
    }
    std::sort(samples.begin(), samples.end());
    return {samples[samples.size() / 2], samples.front(), samples.back(), batches, perBatch};
}

std::string f2(double v, int p = 3) {
    std::ostringstream o;
    o << std::fixed << std::setprecision(p) << v;
    return o.str();
}

std::string distText(const Route& r) { return r.status == RouteStatus::Ok ? f2(r.distance, 1) : ""; }

// Grid graph: side x side junctions, 4-neighbour two-way paths, integer metres.
void gridText(int side, unsigned seed, std::string& nodes, std::string& edges) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> w(50, 150);
    std::ostringstream ns, es;
    for (int r = 0; r < side; ++r)
        for (int c = 0; c < side; ++c) ns << r * side + c << "|J" << r << "_" << c << "\n";
    int k = 0;
    for (int r = 0; r < side; ++r)
        for (int c = 0; c < side; ++c) {
            const int v = r * side + c;
            if (c + 1 < side) es << "G" << k++ << "|" << v << "|" << v + 1 << "|" << w(rng) << "|two-way|open|yes|walkway\n";
            if (r + 1 < side) es << "G" << k++ << "|" << v << "|" << v + side << "|" << w(rng) << "|two-way|open|yes|walkway\n";
        }
    nodes = ns.str();
    edges = es.str();
}

}  // namespace

int main(int argc, char** argv) {
    const std::string dataDir = argc > 1 ? argv[1] : "data/demo";
    const std::string out = argc > 2 ? argv[2] : "results";

    // ------------------------------------------------ load time (separate from search)
    CampusGraph demo;
    LoadReport rep;
    if (!CampusGraph::loadDirectory(dataDir, demo, rep)) {
        std::cout << "cannot load " << dataDir << "\n";
        return 1;
    }
    const Timing load = timeCall([&] {
        CampusGraph g;
        LoadReport r;
        CampusGraph::loadDirectory(dataDir, g, r);
        return static_cast<double>(g.nodeCount());
    }, 21, 20);

    // ------------------------------------------------ demo: every ordered place pair, both modes
    {
        std::ofstream f(out + "/bench_demo_queries.csv");
        f << "mode,src_place,dst_place,src_node,dst_node,status,distance_m,segments,settled,stale_pops,adjacency_scanned,"
             "edges_filtered,relax_attempts,relax_successes,heap_pushes,max_heap,batches,calls_per_batch,median_us,min_us,max_us\n";
        for (RouteMode mode : {RouteMode::Standard, RouteMode::StepFree})
            for (const Place& a : demo.places())
                for (const Place& b : demo.places()) {
                    const Route r = findRoute(demo, a.node, b.node, mode);
                    const Timing t = timeCall([&] { return findRoute(demo, a.node, b.node, mode).distance; }, 31, 200);
                    const SearchStats& s = r.stats;
                    f << toString(mode) << "," << a.id << "," << b.id << "," << demo.nodes()[a.node].id << ","
                      << demo.nodes()[b.node].id << "," << toString(r.status) << "," << distText(r) << ","
                      << (r.status == RouteStatus::Ok ? std::to_string(r.segments) : "") << "," << s.settled << ","
                      << s.stalePops << "," << s.adjacencyScanned << "," << s.edgesFiltered << "," << s.relaxAttempts
                      << "," << s.relaxSuccesses << "," << s.heapPushes << "," << s.maxHeapSize << "," << t.batches << ","
                      << t.perBatch << "," << f2(t.medianUs) << "," << f2(t.minUs) << "," << f2(t.maxUs) << "\n";
                }
    }

    // ------------------------------------------------ closure scenarios (route-distance increase)
    {
        struct Scenario {
            std::string name;
            int src, dst;  // place IDs
            std::vector<std::string> close;
        };
        const std::vector<Scenario> scenarios = {
            {"S1 footbridge closed", 1, 3, {"E22"}},
            {"S2 footbridge + north road centre closed", 1, 3, {"E22", "E04"}},
            {"S3 + central cross walk closed", 1, 3, {"E22", "E04", "E20"}},
            {"S4 + perimeter road closed", 1, 3, {"E22", "E04", "E20", "E21"}},
            {"S5 + north road east closed", 1, 3, {"E22", "E04", "E20", "E21", "E06"}},
            {"S6 + library ramp closed", 1, 3, {"E22", "E04", "E20", "E21", "E06", "E24"}},
            {"S7 library approaches closed", 1, 3, {"E05", "E22", "E24"}},
            {"S8 one-way sports exit closed", 8, 7, {"E23"}},
            {"S9 admin approach closed", 1, 2, {"E03"}},
            {"S10 canteen service path closed", 6, 9, {"E29"}},
        };
        std::ofstream f(out + "/closure_impact.csv");
        f << "scenario,src_place,dst_place,closed_edges,base_status,base_distance_m,base_edges,new_status,new_distance_m,"
             "new_edges,increase_m,increase_percent\n";
        auto edgesOf = [](const CampusGraph& g, const Route& r) {
            std::string s;
            for (std::size_t i = 0; i < r.edges.size(); ++i) s += (i ? " " : "") + g.edges()[r.edges[i]].id;
            return s;
        };
        for (const Scenario& sc : scenarios) {
            CampusGraph g = demo;
            const int s = g.findPlace(sc.src)->node, t = g.findPlace(sc.dst)->node;
            const Route base = findRoute(g, s, t, RouteMode::Standard);
            std::string closed;
            for (const auto& id : sc.close) {
                g.setEdgeOpen(id, false);
                closed += (closed.empty() ? "" : " ") + id;
            }
            const Route after = findRoute(g, s, t, RouteMode::Standard);
            const bool both = base.status == RouteStatus::Ok && after.status == RouteStatus::Ok;
            f << '"' << sc.name << "\"," << sc.src << "," << sc.dst << "," << closed << "," << toString(base.status) << ","
              << distText(base) << "," << edgesOf(demo, base) << "," << toString(after.status) << "," << distText(after)
              << "," << edgesOf(g, after) << "," << (both ? f2(after.distance - base.distance, 1) : "") << ","
              << (both && base.distance > 0 ? f2(100.0 * (after.distance - base.distance) / base.distance, 2) : "") << "\n";
        }
    }

    // ------------------------------------------------ single-edge closure sensitivity over all place pairs
    {
        std::ofstream f(out + "/closure_single_edge_summary.csv");
        f << "edge_id,pairs_reachable_before,pairs_longer,pairs_disconnected,pairs_unchanged,mean_increase_m_over_longer,"
             "max_increase_m\n";
        for (const PhysicalEdge& e : demo.edges()) {
            if (!e.open) continue;
            CampusGraph g = demo;
            g.setEdgeOpen(e.id, false);
            int reachable = 0, longer = 0, disconnected = 0, unchanged = 0;
            double sum = 0, mx = 0;
            for (const Place& a : demo.places())
                for (const Place& b : demo.places()) {
                    if (a.id == b.id) continue;
                    const Route r0 = findRoute(demo, a.node, b.node, RouteMode::Standard);
                    if (r0.status != RouteStatus::Ok) continue;
                    ++reachable;
                    const Route r1 = findRoute(g, a.node, b.node, RouteMode::Standard);
                    if (r1.status != RouteStatus::Ok) ++disconnected;
                    else if (r1.distance > r0.distance + 1e-9) {
                        ++longer;
                        sum += r1.distance - r0.distance;
                        mx = std::max(mx, r1.distance - r0.distance);
                    } else ++unchanged;
                }
            f << e.id << "," << reachable << "," << longer << "," << disconnected << "," << unchanged << ","
              << (longer ? f2(sum / longer, 1) : "") << "," << (longer ? f2(mx, 1) : "") << "\n";
        }
    }

    // ------------------------------------------------ scaling on synthetic grid graphs
    {
        std::ofstream f(out + "/bench_scaling.csv");
        f << "graph,nodes,physical_edges,adjacency_entries,build_ms,algorithm,queries,batches,calls_per_batch,"
             "median_of_query_medians_us,mean_settled,mean_adjacency_scanned,mean_heap_pushes,mean_stale_pops,"
             "mean_max_heap,est_query_workspace_bytes,est_graph_bytes,oracle_agreement\n";
        const std::size_t heapEntry = sizeof(std::tuple<double, int, int>);
        for (int side : {10, 32, 100, 317}) {
            std::string nt, et;
            gridText(side, 42u + side, nt, et);
            CampusGraph g;
            LoadReport r;
            const auto b0 = Clock::now();
            std::istringstream ni(nt), ei(et);
            CampusGraph::loadStreams(ni, ei, nullptr, nullptr, g, r);
            const double buildMs = std::chrono::duration<double, std::milli>(Clock::now() - b0).count();
            const int n = g.nodeCount();
            const std::size_t graphBytes = n * sizeof(Node) + g.edgeCount() * sizeof(PhysicalEdge) +
                                           g.adjacencyEntryCount() * sizeof(AdjEntry) + n * sizeof(std::vector<AdjEntry>);
            std::mt19937 rng(1000 + side);
            std::uniform_int_distribution<int> pick(0, n - 1);
            const int queries = 20;
            std::vector<std::pair<int, int>> pairs;
            for (int q = 0; q < queries; ++q) pairs.emplace_back(pick(rng), pick(rng));
            const int batches = side >= 317 ? 5 : (side >= 100 ? 9 : 21);

            struct Algo {
                std::string name;
                bool enabled;
            };
            const std::vector<Algo> algos = {{"dijkstra_p2p_early_stop", true},
                                             {"dijkstra_full_sssp", true},
                                             {"bellman_ford_reference_full", side <= 100}};
            for (const Algo& al : algos) {
                if (!al.enabled) continue;
                const int perBatch = al.name[0] == 'b' ? 1 : (side >= 317 ? 1 : (side >= 100 ? 5 : 50));
                std::vector<double> med;
                double settled = 0, scanned = 0, pushes = 0, stale = 0, maxHeap = 0;
                int agree = 0;
                for (const auto& [s, t] : pairs) {
                    Timing tm;
                    if (al.name == "dijkstra_p2p_early_stop") {
                        const Route rt = findRoute(g, s, t, RouteMode::Standard);
                        const SearchStats& st = rt.stats;
                        settled += st.settled; scanned += st.adjacencyScanned; pushes += st.heapPushes;
                        stale += st.stalePops; maxHeap += st.maxHeapSize;
                        const ReferenceLabels ref = bellmanFordReference(g, s, RouteMode::Standard);
                        agree += nearlyEqual(ref.dist[t], rt.distance) ? 1 : 0;
                        tm = timeCall([&] { return findRoute(g, s, t, RouteMode::Standard).distance; }, batches, perBatch);
                    } else if (al.name == "dijkstra_full_sssp") {
                        const SearchResult sr = dijkstra(g, s, -1, RouteMode::Standard);
                        const SearchStats& st = sr.stats;
                        settled += st.settled; scanned += st.adjacencyScanned; pushes += st.heapPushes;
                        stale += st.stalePops; maxHeap += st.maxHeapSize;
                        agree = -1;
                        tm = timeCall([&] { return dijkstra(g, s, -1, RouteMode::Standard).dist[t]; }, batches, perBatch);
                    } else {
                        agree = -1;
                        tm = timeCall([&] { return bellmanFordReference(g, s, RouteMode::Standard).dist[t]; },
                                      side >= 100 ? 3 : batches, 1);
                    }
                    med.push_back(tm.medianUs);
                }
                std::sort(med.begin(), med.end());
                const double q = static_cast<double>(queries);
                const bool counted = al.name[0] == 'd';
                const std::size_t workspace =
                    n * (sizeof(double) + 3 * sizeof(int) + sizeof(char)) + static_cast<std::size_t>(maxHeap / q) * heapEntry;
                f << "grid_" << side << "x" << side << "," << n << "," << g.edgeCount() << "," << g.adjacencyEntryCount()
                  << "," << f2(buildMs, 2) << "," << al.name << "," << queries << ","
                  << (al.name[0] == 'b' ? (side >= 100 ? 3 : batches) : batches) << "," << perBatch << ","
                  << f2(med[med.size() / 2]) << "," << (counted ? f2(settled / q, 1) : "") << ","
                  << (counted ? f2(scanned / q, 1) : "") << "," << (counted ? f2(pushes / q, 1) : "") << ","
                  << (counted ? f2(stale / q, 1) : "") << "," << (counted ? f2(maxHeap / q, 1) : "") << ","
                  << (counted ? std::to_string(workspace) : "") << "," << graphBytes << ","
                  << (agree >= 0 ? std::to_string(agree) + "/" + std::to_string(queries) : "") << "\n";
                std::cout << "  grid " << side << "x" << side << " " << al.name << ": median " << f2(med[med.size() / 2])
                          << " us\n";
            }
        }
    }

    // ------------------------------------------------ environment record
    {
        std::ofstream f(out + "/bench_environment.txt");
        const std::time_t now = std::time(nullptr);
        f << "run_at_local: " << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S") << "\n"
          << "compiler: g++ " << __VERSION__ << "\n"
          << "flags: " << BENCH_FLAGS << "\n"
          << "cplusplus: " << __cplusplus << "\n"
          << "timer: std::chrono::steady_clock, period " << Clock::period::num << "/" << Clock::period::den << " s\n"
          << "method: per-call time = batch time / calls_per_batch; median over batches; loading, printing and\n"
          << "        result checking are outside timed regions\n"
          << "demo_load_time_us_median: " << f2(load.medianUs, 1) << " (" << load.batches << " batches x " << load.perBatch
          << " loads of nodes/edges/places/metadata files)\n"
          << "demo_dataset: " << demo.nodeCount() << " nodes, " << demo.edgeCount() << " physical edges, "
          << demo.adjacencyEntryCount() << " adjacency entries, " << demo.places().size() << " places\n"
          << "sizeof: Node " << sizeof(Node) << ", PhysicalEdge " << sizeof(PhysicalEdge) << ", AdjEntry "
          << sizeof(AdjEntry) << ", heap entry " << sizeof(std::tuple<double, int, int>) << " bytes\n";
    }
    std::cout << "Benchmark complete. Results written to " << out << "/\n";
    return 0;
}
