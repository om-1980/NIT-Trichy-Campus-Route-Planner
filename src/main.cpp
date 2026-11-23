// Smart Campus Route Planner - terminal menu (controller connecting M1-M7).
//
// Usage:  route_planner [data_dir]            interactive menu (default data/demo)
//         route_planner --validate [data_dir] load, validate and print a summary only

#include "graph.hpp"
#include "router.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;
using namespace campus;

namespace {

enum class Input { Ok, Invalid, Eof };

string trimmed(const string& s) {
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == string::npos) return {};
    const auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

Input readLine(const string& prompt, string& line) {
    cout << prompt << flush;
    if (!getline(cin, line)) return Input::Eof;
    line = trimmed(line);
    return Input::Ok;
}

Input readInt(const string& prompt, int& value) {
    string line;
    if (readLine(prompt, line) == Input::Eof) return Input::Eof;
    istringstream in(line);
    char extra;
    if (!(in >> value) || (in >> extra)) return Input::Invalid;
    return Input::Ok;
}

string metres(double m) {
    ostringstream o;
    o << fixed << setprecision(1) << m << " m";
    return o.str();
}

string nodeLabel(const CampusGraph& g, int index) {
    const Node& n = g.nodes()[index];
    return n.name + " [" + to_string(n.id) + "]";
}

void printLoadMessages(const LoadReport& rep) {
    for (const auto& e : rep.errors) cout << "  ERROR   " << e << "\n";
    for (const auto& w : rep.warnings) cout << "  WARNING " << w << "\n";
}

void printSummary(const CampusGraph& g, const LoadReport& rep) {
    const GraphSummary s = summarize(g);
    cout << "\n--- Graph summary (version " << g.version() << ") ---\n";
    for (const auto& kv : g.metadata()) cout << "  " << left << setw(22) << kv.first << ": " << kv.second << "\n";
    cout << "  Graph nodes            : " << s.nodes << "\n"
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
    cout << "\n  Place  Name                                              Graph node (approach point)\n";
    for (const Place& p : g.places())
        cout << "  " << setw(5) << p.id << "  " << left << setw(48) << p.name << right << " "
                  << nodeLabel(g, p.node) << "\n";
}

void listEdges(const CampusGraph& g) {
    cout << "\n  Edge  From -> To            Length    Direction Status  Step-free Type     Label\n";
    for (const PhysicalEdge& e : g.edges()) {
        ostringstream ends;
        ends << g.nodes()[e.from].id << (e.direction == Direction::TwoWay ? " <-> " : "  -> ") << g.nodes()[e.to].id;
        cout << "  " << left << setw(5) << e.id << " " << setw(18) << ends.str() << right
                  << setw(9) << metres(e.distance) << "  " << left << setw(9) << toString(e.direction)
                  << " " << setw(7) << (e.open ? "open" : "CLOSED") << " " << setw(9) << toString(e.stepFree)
                  << " " << setw(8) << toString(e.type) << " " << e.label << right << "\n";
    }
}

void printRoute(const CampusGraph& g, const Route& r) {
    cout << "\n  Mode: " << toString(r.mode) << "   Graph version: " << r.graphVersion << "\n";
    cout << "  Status: " << toString(r.status) << "\n";
    if (r.status == RouteStatus::NoRoute) {
        cout << "  No permitted route exists from " << nodeLabel(g, r.source) << " to " << nodeLabel(g, r.target)
                  << " with the current closures and mode.\n";
    }
    if (r.status != RouteStatus::Ok) return;
    cout << "  Total distance: " << metres(r.distance) << "   Segments: " << r.segments << "\n";
    if (r.edges.empty()) cout << "  Source and destination are the same location: " << nodeLabel(g, r.source) << "\n";
    for (size_t i = 0; i < r.edges.size(); ++i) {
        const PhysicalEdge& e = g.edges()[r.edges[i]];
        cout << "  " << setw(2) << i + 1 << ". " << left << setw(4) << e.id << " "
                  << setw(38) << nodeLabel(g, r.nodes[i]) << " -> " << setw(38) << nodeLabel(g, r.nodes[i + 1])
                  << right << setw(9) << metres(e.distance) << "  " << toString(e.type) << "\n";
    }
    cout << "  Node IDs: ";
    for (size_t i = 0; i < r.nodes.size(); ++i) cout << (i ? " -> " : "") << g.nodes()[r.nodes[i]].id;
    cout << "\n  Edge IDs: ";
    for (size_t i = 0; i < r.edges.size(); ++i) cout << (i ? ", " : "") << g.edges()[r.edges[i]].id;
    cout << (r.edges.empty() ? "(none)" : "") << "\n";
    cout << "  Search: " << r.stats.settled << " vertices settled, " << r.stats.adjacencyScanned
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
        cout << "  Invalid source: enter a place ID from option 1.\n";
        return true;
    }
    in = readInt("  Destination place ID: ", t);
    if (in == Input::Eof) return false;
    const Place* pt = in == Input::Ok ? g.findPlace(t) : nullptr;
    if (!pt) {
        cout << "  Invalid destination: enter a place ID from option 1.\n";
        return true;
    }
    cout << "\n  Route: " << ps->name << "  ->  " << pt->name;
    printRoute(g, findRoute(g, ps->node, pt->node, mode));
    return true;
}

bool routeByNodeIds(const CampusGraph& g) {
    int s = 0, t = 0;
    Input in = readInt("  Source graph-node ID: ", s);
    if (in == Input::Eof) return false;
    if (in == Input::Invalid) { cout << "  Invalid source: not a number.\n"; return true; }
    in = readInt("  Destination graph-node ID: ", t);
    if (in == Input::Eof) return false;
    if (in == Input::Invalid) { cout << "  Invalid destination: not a number.\n"; return true; }
    const Route r = findRouteByNodeId(g, s, t, RouteMode::Standard);
    if (r.status == RouteStatus::InvalidSource) cout << "  INVALID_SOURCE: no graph node with ID " << s << ".\n";
    else if (r.status == RouteStatus::InvalidDestination) cout << "  INVALID_DESTINATION: no graph node with ID " << t << ".\n";
    else printRoute(g, r);
    return true;
}

bool changeEdge(CampusGraph& g, bool open) {
    string id;
    if (readLine(open ? "  Edge ID to reopen (e.g. E22): " : "  Edge ID to close (e.g. E22): ", id) == Input::Eof) return false;
    switch (g.setEdgeOpen(id, open)) {
        case StatusChange::UnknownEdge:
            cout << "  Unknown edge ID '" << id << "'. Use option 4 to list edges. Nothing changed.\n";
            break;
        case StatusChange::Unchanged:
            cout << "  Edge " << id << " is already " << (open ? "open" : "closed") << ". Nothing changed.\n";
            break;
        case StatusChange::Changed: {
            const PhysicalEdge& e = g.edges()[g.indexOfEdgeId(id)];
            cout << "  Edge " << e.id << " (" << nodeLabel(g, e.from) << (e.direction == Direction::TwoWay ? " <-> " : " -> ")
                      << nodeLabel(g, e.to) << ") is now " << (open ? "OPEN" : "CLOSED")
                      << (e.direction == Direction::TwoWay ? " in both directions" : "") << ". Graph version "
                      << g.version() << ".\n";
            break;
        }
    }
    return true;
}

void printMenu(const CampusGraph& g) {
    const string name = g.metadataValue("name");
    cout << "\n=========== Smart Campus Route Planner ===========\n";
    if (!name.empty()) cout << " Dataset: " << name << "\n";
    cout << " Graph version " << g.version() << " | " << g.nodeCount() << " nodes | " << g.edgeCount()
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
    string dataDir = "data/demo";
    {
        ifstream testNitt("data/nitt/nodes.txt");
        if (testNitt.good()) dataDir = "data/nitt";
    }
    bool validateOnly = false;
    for (int i = 1; i < argc; ++i) {
        const string a = argv[i];
        if (a == "--validate") validateOnly = true;
        else if (a == "--help" || a == "-h") {
            cout << "Usage: route_planner [--validate] [data_dir]   (default data_dir: " << dataDir << ")\n";
            return 0;
        } else dataDir = a;
    }

    CampusGraph g;
    LoadReport rep;
    cout << "Loading campus data from '" << dataDir << "' ...\n";
    if (!CampusGraph::loadDirectory(dataDir, g, rep)) {
        cout << "DATA_ERROR: the dataset was rejected (" << rep.errors.size() << " error(s)).\n";
        printLoadMessages(rep);
        return 1;
    }
    cout << "Loaded " << g.nodeCount() << " nodes, " << g.edgeCount() << " physical edges, "
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
            cout << "  Please enter a number from the menu.\n";
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
                cout << "  " << n << " edge status(es) restored. Graph version " << g.version() << ".\n";
                break;
            }
            case 8: printSummary(g, rep); break;
            case 9: more = routeByNodeIds(g); break;
            case 0: cout << "Goodbye.\n"; return 0;
            default: cout << "  Unknown option " << choice << ".\n";
        }
        if (!more) break;
    }
    cout << "\nEnd of input. Exiting.\n";
    return 0;
}
