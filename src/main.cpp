// Smart Campus Route Planner - terminal menu (controller connecting M1-M7).
//
// Usage:  route_planner [data_dir]            interactive menu (default data/demo)
//         route_planner --validate [data_dir] load, validate and print a summary only

#include "graph.hpp"
#include "router.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

using namespace campus;

namespace {

enum class Input { Ok, Invalid, Eof };

std::string trimmed(const std::string& s) {
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return {};
    const auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

Input readLine(const std::string& prompt, std::string& line) {
    std::cout << prompt << std::flush;
    if (!std::getline(std::cin, line)) return Input::Eof;
    line = trimmed(line);
    return Input::Ok;
}

Input readInt(const std::string& prompt, int& value) {
    std::string line;
    if (readLine(prompt, line) == Input::Eof) return Input::Eof;
    std::istringstream in(line);
    char extra;
    if (!(in >> value) || (in >> extra)) return Input::Invalid;
    return Input::Ok;
}

std::string metres(double m) {
    std::ostringstream o;
    o << std::fixed << std::setprecision(1) << m << " m";
    return o.str();
}

std::string nodeLabel(const CampusGraph& g, int index) {
    const Node& n = g.nodes()[index];
    return n.name + " [" + std::to_string(n.id) + "]";
}

void printLoadMessages(const LoadReport& rep) {
    for (const auto& e : rep.errors) std::cout << "  ERROR   " << e << "\n";
    for (const auto& w : rep.warnings) std::cout << "  WARNING " << w << "\n";
}

void printSummary(const CampusGraph& g, const LoadReport& rep) {
    const GraphSummary s = summarize(g);
    std::cout << "\n--- Graph summary (version " << g.version() << ") ---\n";
    for (const auto& kv : g.metadata()) std::cout << "  " << std::left << std::setw(22) << kv.first << ": " << kv.second << "\n";
    std::cout << "  Graph nodes            : " << s.nodes << "\n"
              << "  Physical edges         : " << s.physicalEdges << " (" << s.twoWay << " two-way, " << s.oneWay
              << " one-way)\n"
              << "  Adjacency entries      : " << s.adjacencyEntries << " (= 2 x two-way + one-way)\n"
              << "  Places (menu entries)  : " << s.places << "\n"
              << "  Closed edges now       : " << s.closed << "\n"
              << "  Step-free yes/no/unknown: " << s.stepFreeYes << "/" << s.stepFreeNo << "/" << s.stepFreeUnknown << "\n"
              << "  Total path length      : " << metres(s.totalLengthM) << "\n"
              << "  Connected components   : " << s.components << " (ignoring direction and status)\n"
              << "  Isolated nodes         : " << s.isolatedNodes << "\n"
              << "  Load errors / warnings : " << rep.errors.size() << " / " << rep.warnings.size() << "\n";
    printLoadMessages(rep);
}

void listPlaces(const CampusGraph& g) {
    std::cout << "\n  Place  Name                                     Graph node (approach point)\n";
    for (const Place& p : g.places())
        std::cout << "  " << std::setw(5) << p.id << "  " << std::left << std::setw(40) << p.name << std::right << " "
                  << nodeLabel(g, p.node) << "\n";
}

void listEdges(const CampusGraph& g) {
    std::cout << "\n  Edge  From -> To            Length    Direction Status  Step-free Type     Label\n";
    for (const PhysicalEdge& e : g.edges()) {
        std::ostringstream ends;
        ends << g.nodes()[e.from].id << (e.direction == Direction::TwoWay ? " <-> " : "  -> ") << g.nodes()[e.to].id;
        std::cout << "  " << std::left << std::setw(5) << e.id << " " << std::setw(18) << ends.str() << std::right
                  << std::setw(9) << metres(e.distance) << "  " << std::left << std::setw(9) << toString(e.direction)
                  << " " << std::setw(7) << (e.open ? "open" : "CLOSED") << " " << std::setw(9) << toString(e.stepFree)
                  << " " << std::setw(8) << toString(e.type) << " " << e.label << std::right << "\n";
    }
}

void printRoute(const CampusGraph& g, const Route& r) {
    std::cout << "\n  Mode: " << toString(r.mode) << "   Graph version: " << r.graphVersion << "\n";
    std::cout << "  Status: " << toString(r.status) << "\n";
    if (r.status == RouteStatus::NoRoute) {
        std::cout << "  No permitted route exists from " << nodeLabel(g, r.source) << " to " << nodeLabel(g, r.target)
                  << " with the current closures and mode.\n";
    }
    if (r.status != RouteStatus::Ok) return;
    std::cout << "  Total distance: " << metres(r.distance) << "   Segments: " << r.segments << "\n";
    if (r.edges.empty()) std::cout << "  Source and destination are the same location: " << nodeLabel(g, r.source) << "\n";
    for (std::size_t i = 0; i < r.edges.size(); ++i) {
        const PhysicalEdge& e = g.edges()[r.edges[i]];
        std::cout << "  " << std::setw(2) << i + 1 << ". " << std::left << std::setw(4) << e.id << " "
                  << std::setw(38) << nodeLabel(g, r.nodes[i]) << " -> " << std::setw(38) << nodeLabel(g, r.nodes[i + 1])
                  << std::right << std::setw(9) << metres(e.distance) << "  " << toString(e.type) << "\n";
    }
    std::cout << "  Node IDs: ";
    for (std::size_t i = 0; i < r.nodes.size(); ++i) std::cout << (i ? " -> " : "") << g.nodes()[r.nodes[i]].id;
    std::cout << "\n  Edge IDs: ";
    for (std::size_t i = 0; i < r.edges.size(); ++i) std::cout << (i ? ", " : "") << g.edges()[r.edges[i]].id;
    std::cout << (r.edges.empty() ? "(none)" : "") << "\n";
    std::cout << "  Search: " << r.stats.settled << " vertices settled, " << r.stats.adjacencyScanned
              << " adjacency entries scanned (" << r.stats.edgesFiltered << " filtered), " << r.stats.heapPushes
              << " heap pushes\n";
}

// Returns false on end of input.
bool routeByPlaces(const CampusGraph& g, RouteMode mode) {
    int s = 0, t = 0;
    Input in = readInt("  Source place ID: ", s);
    if (in == Input::Eof) return false;
    const Place* ps = in == Input::Ok ? g.findPlace(s) : nullptr;
    if (!ps) {
        std::cout << "  Invalid source: enter a place ID from option 1.\n";
        return true;
    }
    in = readInt("  Destination place ID: ", t);
    if (in == Input::Eof) return false;
    const Place* pt = in == Input::Ok ? g.findPlace(t) : nullptr;
    if (!pt) {
        std::cout << "  Invalid destination: enter a place ID from option 1.\n";
        return true;
    }
    std::cout << "\n  Route: " << ps->name << "  ->  " << pt->name;
    printRoute(g, findRoute(g, ps->node, pt->node, mode));
    return true;
}

bool routeByNodeIds(const CampusGraph& g) {
    int s = 0, t = 0;
    Input in = readInt("  Source graph-node ID: ", s);
    if (in == Input::Eof) return false;
    if (in == Input::Invalid) { std::cout << "  Invalid source: not a number.\n"; return true; }
    in = readInt("  Destination graph-node ID: ", t);
    if (in == Input::Eof) return false;
    if (in == Input::Invalid) { std::cout << "  Invalid destination: not a number.\n"; return true; }
    const Route r = findRouteByNodeId(g, s, t, RouteMode::Standard);
    if (r.status == RouteStatus::InvalidSource) std::cout << "  INVALID_SOURCE: no graph node with ID " << s << ".\n";
    else if (r.status == RouteStatus::InvalidDestination) std::cout << "  INVALID_DESTINATION: no graph node with ID " << t << ".\n";
    else printRoute(g, r);
    return true;
}

bool changeEdge(CampusGraph& g, bool open) {
    std::string id;
    if (readLine(open ? "  Edge ID to reopen (e.g. E22): " : "  Edge ID to close (e.g. E22): ", id) == Input::Eof) return false;
    switch (g.setEdgeOpen(id, open)) {
        case StatusChange::UnknownEdge:
            std::cout << "  Unknown edge ID '" << id << "'. Use option 4 to list edges. Nothing changed.\n";
            break;
        case StatusChange::Unchanged:
            std::cout << "  Edge " << id << " is already " << (open ? "open" : "closed") << ". Nothing changed.\n";
            break;
        case StatusChange::Changed: {
            const PhysicalEdge& e = g.edges()[g.indexOfEdgeId(id)];
            std::cout << "  Edge " << e.id << " (" << nodeLabel(g, e.from) << (e.direction == Direction::TwoWay ? " <-> " : " -> ")
                      << nodeLabel(g, e.to) << ") is now " << (open ? "OPEN" : "CLOSED")
                      << (e.direction == Direction::TwoWay ? " in both directions" : "") << ". Graph version "
                      << g.version() << ".\n";
            break;
        }
    }
    return true;
}

void printMenu(const CampusGraph& g) {
    const std::string name = g.metadataValue("name");
    std::cout << "\n=========== Smart Campus Route Planner ===========\n";
    if (!name.empty()) std::cout << " Dataset: " << name << "\n";
    std::cout << " Graph version " << g.version() << " | " << g.nodeCount() << " nodes | " << g.edgeCount()
              << " physical edges | " << g.closedEdgeCount() << " closed\n"
              << "  1. List places\n"
              << "  2. Find shortest route (standard)\n"
              << "  3. Find shortest route (step-free paths only)\n"
              << "  4. List physical edges and status\n"
              << "  5. Close a path\n"
              << "  6. Reopen a path\n"
              << "  7. Reset all paths to dataset status\n"
              << "  8. Graph summary and data-validation report\n"
              << "  9. Route between graph-node IDs (diagnostic)\n"
              << "  0. Exit\n";
}

}  // namespace

int main(int argc, char** argv) {
    std::string dataDir = "data/demo";
    bool validateOnly = false;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--validate") validateOnly = true;
        else if (a == "--help" || a == "-h") {
            std::cout << "Usage: route_planner [--validate] [data_dir]   (default data_dir: data/demo)\n";
            return 0;
        } else dataDir = a;
    }

    CampusGraph g;
    LoadReport rep;
    std::cout << "Loading campus data from '" << dataDir << "' ...\n";
    if (!CampusGraph::loadDirectory(dataDir, g, rep)) {
        std::cout << "DATA_ERROR: the dataset was rejected (" << rep.errors.size() << " error(s)).\n";
        printLoadMessages(rep);
        return 1;
    }
    std::cout << "Loaded " << g.nodeCount() << " nodes, " << g.edgeCount() << " physical edges, "
              << g.adjacencyEntryCount() << " adjacency entries, " << g.places().size() << " places ("
              << rep.warnings.size() << " warning(s)).\n";
    if (validateOnly) {
        printSummary(g, rep);
        return 0;
    }

    for (;;) {
        printMenu(g);
        int choice = -1;
        const Input in = readInt("Choose an option: ", choice);
        if (in == Input::Eof) break;
        if (in == Input::Invalid) {
            std::cout << "  Please enter a number from the menu.\n";
            continue;
        }
        bool more = true;
        switch (choice) {
            case 1: listPlaces(g); break;
            case 2: more = routeByPlaces(g, RouteMode::Standard); break;
            case 3: more = routeByPlaces(g, RouteMode::StepFree); break;
            case 4: listEdges(g); break;
            case 5: more = changeEdge(g, false); break;
            case 6: more = changeEdge(g, true); break;
            case 7: {
                const int n = g.resetToDataStatus();
                std::cout << "  " << n << " edge status(es) restored. Graph version " << g.version() << ".\n";
                break;
            }
            case 8: printSummary(g, rep); break;
            case 9: more = routeByNodeIds(g); break;
            case 0: std::cout << "Goodbye.\n"; return 0;
            default: std::cout << "  Unknown option " << choice << ".\n";
        }
        if (!more) break;
    }
    std::cout << "\nEnd of input. Exiting.\n";
    return 0;
}
