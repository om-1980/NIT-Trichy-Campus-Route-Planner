#include "graph.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <istream>
#include <limits>
#include <numeric>
#include <sstream>

namespace campus {

const char* toString(Direction d) { return d == Direction::TwoWay ? "two-way" : "one-way"; }

const char* toString(StepFree s) {
    switch (s) {
        case StepFree::Yes: return "yes";
        case StepFree::No: return "no";
        default: return "unknown";
    }
}

const char* toString(PathType t) {
    switch (t) {
        case PathType::Walkway: return "walkway";
        case PathType::Road: return "road";
        case PathType::Ramp: return "ramp";
        default: return "stairs";
    }
}

namespace {

std::string trim(const std::string& s) {
    std::size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string upper(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

std::vector<std::string> splitPipe(const std::string& line) {
    std::vector<std::string> out;
    std::string field;
    std::istringstream in(line);
    while (std::getline(in, field, '|')) out.push_back(trim(field));
    if (!line.empty() && line.back() == '|') out.emplace_back();
    return out;
}

// Strict integer parse: the whole field must be an integer that fits in int.
bool parseInt(const std::string& s, int& out) {
    if (s.empty()) return false;
    std::size_t i = (s[0] == '-' || s[0] == '+') ? 1 : 0;
    if (i == s.size()) return false;
    for (std::size_t k = i; k < s.size(); ++k)
        if (!std::isdigit(static_cast<unsigned char>(s[k]))) return false;
    errno = 0;
    char* end = nullptr;
    long long v = std::strtoll(s.c_str(), &end, 10);
    if (errno == ERANGE || v < std::numeric_limits<int>::min() || v > std::numeric_limits<int>::max())
        return false;
    out = static_cast<int>(v);
    return true;
}

// Strict real parse: the whole field must be consumed. "nan"/"inf" parse here and
// are rejected by the caller's finiteness check so the message is specific.
bool parseDouble(const std::string& s, double& out) {
    if (s.empty()) return false;
    char* end = nullptr;
    errno = 0;
    out = std::strtod(s.c_str(), &end);
    return end == s.c_str() + s.size();
}

bool validEdgeId(const std::string& id) {
    if (id.empty()) return false;
    return std::all_of(id.begin(), id.end(), [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-';
    });
}

// Reads non-blank, non-comment lines. Calls fn(lineNumber, fields).
void forEachRecord(std::istream& in, const std::function<void(int, const std::string&)>& fn) {
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        if (lineNo == 1 && line.size() >= 3 && line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
        std::string t = trim(line);
        if (t.empty() || t[0] == '#') continue;
        fn(lineNo, t);
    }
}

std::string at(const char* file, int line) { return std::string(file) + ":" + std::to_string(line) + ": "; }

}  // namespace

bool CampusGraph::loadDirectory(const std::string& dir, CampusGraph& out, LoadReport& report) {
    const std::string base = dir.empty() ? std::string(".") : dir;
    std::ifstream nodes(base + "/nodes.txt"), edges(base + "/edges.txt"), places(base + "/places.txt");
    std::ifstream metadata(base + "/metadata.txt");
    if (!nodes) report.errors.push_back("cannot open file '" + base + "/nodes.txt'");
    if (!edges) report.errors.push_back("cannot open file '" + base + "/edges.txt'");
    if (!places) report.errors.push_back("cannot open file '" + base + "/places.txt'");
    if (!report.ok()) return false;
    return loadStreams(nodes, edges, &places, metadata ? &metadata : nullptr, out, report);
}

bool CampusGraph::loadStreams(std::istream& nodesIn, std::istream& edgesIn, std::istream* placesIn,
                              std::istream* metadataIn, CampusGraph& out, LoadReport& report) {
    const std::size_t errorsBefore = report.errors.size();
    auto error = [&](const std::string& msg) { report.errors.push_back(msg); };
    CampusGraph g;
    std::unordered_map<int, int> nodeLine;

    // ---- nodes.txt: node_id|name[|x_m|y_m]
    forEachRecord(nodesIn, [&](int ln, const std::string& line) {
        const auto f = splitPipe(line);
        if (f.size() != 2 && f.size() != 4) {
            error(at("nodes.txt", ln) + "expected 2 or 4 '|'-separated fields, found " + std::to_string(f.size()));
            return;
        }
        Node n;
        if (!parseInt(f[0], n.id) || n.id < 0) {
            error(at("nodes.txt", ln) + "node_id must be a non-negative integer (got '" + f[0] + "')");
            return;
        }
        n.name = f[1];
        if (n.name.empty()) {
            error(at("nodes.txt", ln) + "node name is empty");
            return;
        }
        if (f.size() == 4 && !(f[2].empty() && f[3].empty())) {
            if (f[2].empty() || f[3].empty() || !parseDouble(f[2], n.x) || !parseDouble(f[3], n.y) ||
                !std::isfinite(n.x) || !std::isfinite(n.y)) {
                error(at("nodes.txt", ln) + "coordinates must be two finite numbers or both empty");
                return;
            }
            n.hasCoords = true;
        }
        auto it = nodeLine.find(n.id);
        if (it != nodeLine.end()) {
            error(at("nodes.txt", ln) + "duplicate node_id " + std::to_string(n.id) + " (first on line " +
                  std::to_string(it->second) + ")");
            return;
        }
        nodeLine[n.id] = ln;
        g.nodeIndex_[n.id] = static_cast<int>(g.nodes_.size());
        g.nodes_.push_back(n);
    });
    if (g.nodes_.empty() && report.errors.size() == errorsBefore) error("nodes.txt: no node records found");

    // ---- edges.txt: edge_id|from|to|distance_m|direction|status|step_free|path_type[|label]
    std::unordered_map<std::string, int> edgeLine;
    forEachRecord(edgesIn, [&](int ln, const std::string& line) {
        const auto f = splitPipe(line);
        if (f.size() != 8 && f.size() != 9) {
            error(at("edges.txt", ln) + "expected 8 or 9 '|'-separated fields, found " + std::to_string(f.size()));
            return;
        }
        const std::string where = at("edges.txt", ln);
        bool ok = true;
        PhysicalEdge e;
        e.id = upper(f[0]);
        if (!validEdgeId(e.id)) {
            error(where + "edge_id must use letters, digits, '_' or '-' (got '" + f[0] + "')");
            ok = false;
        } else if (edgeLine.count(e.id)) {
            error(where + "duplicate edge_id " + e.id + " (first on line " + std::to_string(edgeLine[e.id]) + ")");
            ok = false;
        }
        int fromId = 0, toId = 0;
        for (int k = 1; k <= 2; ++k) {
            int& id = (k == 1) ? fromId : toId;
            if (!parseInt(f[k], id)) {
                error(where + (k == 1 ? "from_node" : "to_node") + " is not an integer (got '" + f[k] + "')");
                ok = false;
            } else if (!g.nodeIndex_.count(id)) {
                error(where + (k == 1 ? "from_node " : "to_node ") + std::to_string(id) + " is not in nodes.txt");
                ok = false;
            }
        }
        if (ok && fromId == toId) {
            error(where + "self-loop path (from_node equals to_node) is not allowed");
            ok = false;
        }
        if (!parseDouble(f[3], e.distance)) {
            error(where + "distance_m is not a number (got '" + f[3] + "')");
            ok = false;
        } else if (!std::isfinite(e.distance)) {
            error(where + "distance_m must be finite (got '" + f[3] + "')");
            ok = false;
        } else if (e.distance < 0.0) {
            error(where + "distance_m must be >= 0; Dijkstra requires non-negative weights (got '" + f[3] + "')");
            ok = false;
        }
        const std::string dir = lower(f[4]), status = lower(f[5]), sf = lower(f[6]), type = lower(f[7]);
        if (dir == "two-way") e.direction = Direction::TwoWay;
        else if (dir == "one-way") e.direction = Direction::OneWay;
        else { error(where + "direction must be two-way or one-way (got '" + f[4] + "')"); ok = false; }
        if (status == "open") e.open = true;
        else if (status == "closed") e.open = false;
        else { error(where + "status must be open or closed (got '" + f[5] + "')"); ok = false; }
        if (sf == "yes") e.stepFree = StepFree::Yes;
        else if (sf == "no") e.stepFree = StepFree::No;
        else if (sf == "unknown") e.stepFree = StepFree::Unknown;
        else { error(where + "step_free must be yes, no or unknown (got '" + f[6] + "')"); ok = false; }
        if (type == "walkway") e.type = PathType::Walkway;
        else if (type == "road") e.type = PathType::Road;
        else if (type == "ramp") e.type = PathType::Ramp;
        else if (type == "stairs") e.type = PathType::Stairs;
        else { error(where + "path_type must be walkway, road, ramp or stairs (got '" + f[7] + "')"); ok = false; }
        if (ok && e.type == PathType::Stairs && e.stepFree == StepFree::Yes) {
            error(where + "a stairs path cannot be step_free=yes");
            ok = false;
        }
        if (!ok) return;
        e.from = g.nodeIndex_[fromId];
        e.to = g.nodeIndex_[toId];
        e.openInData = e.open;
        if (f.size() == 9) e.label = f[8];
        const Node& a = g.nodes_[e.from];
        const Node& b = g.nodes_[e.to];
        if (a.hasCoords && b.hasCoords) {
            const double straight = std::hypot(a.x - b.x, a.y - b.y);
            if (e.distance + 1e-6 < straight) {
                std::ostringstream w;
                w << where << e.id << " distance " << e.distance << " m is shorter than the straight-line distance "
                  << straight << " m between its endpoints (check data)";
                report.warnings.push_back(w.str());
            }
        }
        edgeLine[e.id] = ln;
        g.edgeIndex_[e.id] = static_cast<int>(g.edges_.size());
        g.edges_.push_back(e);
    });

    // ---- places.txt: place_id|node_id|name
    if (placesIn) {
        std::unordered_map<int, int> placeLine;
        forEachRecord(*placesIn, [&](int ln, const std::string& line) {
            const auto f = splitPipe(line);
            const std::string where = at("places.txt", ln);
            if (f.size() != 3) {
                error(where + "expected 3 '|'-separated fields, found " + std::to_string(f.size()));
                return;
            }
            Place p;
            int nodeId = 0;
            if (!parseInt(f[0], p.id) || p.id < 1) {
                error(where + "place_id must be a positive integer (got '" + f[0] + "')");
                return;
            }
            if (!parseInt(f[1], nodeId) || !g.nodeIndex_.count(nodeId)) {
                error(where + "node_id '" + f[1] + "' is not a node in nodes.txt");
                return;
            }
            p.name = f[2];
            if (p.name.empty()) {
                error(where + "place name is empty");
                return;
            }
            if (placeLine.count(p.id)) {
                error(where + "duplicate place_id " + std::to_string(p.id) + " (first on line " +
                      std::to_string(placeLine[p.id]) + ")");
                return;
            }
            placeLine[p.id] = ln;
            p.node = g.nodeIndex_[nodeId];
            g.placeIndex_[p.id] = static_cast<int>(g.places_.size());
            g.places_.push_back(p);
        });
        if (g.places_.empty() && report.errors.size() == errorsBefore) error("places.txt: no place records found");
    }

    // ---- metadata.txt (optional): key = value
    if (metadataIn) {
        forEachRecord(*metadataIn, [&](int, const std::string& line) {
            const auto eq = line.find('=');
            if (eq == std::string::npos) return;
            g.metadata_.emplace_back(trim(line.substr(0, eq)), trim(line.substr(eq + 1)));
        });
    }

    if (report.errors.size() != errorsBefore) return false;

    // ---- M2: build adjacency lists in file order.
    g.adj_.assign(g.nodes_.size(), {});
    for (int i = 0; i < static_cast<int>(g.edges_.size()); ++i) {
        const PhysicalEdge& e = g.edges_[i];
        g.adj_[e.from].push_back({e.to, i});
        if (e.direction == Direction::TwoWay) g.adj_[e.to].push_back({e.from, i});
    }
    out = std::move(g);
    return true;
}

std::size_t CampusGraph::adjacencyEntryCount() const {
    std::size_t n = 0;
    for (const auto& list : adj_) n += list.size();
    return n;
}

std::string CampusGraph::metadataValue(const std::string& key) const {
    for (const auto& kv : metadata_)
        if (kv.first == key) return kv.second;
    return {};
}

int CampusGraph::indexOfNodeId(int nodeId) const {
    auto it = nodeIndex_.find(nodeId);
    return it == nodeIndex_.end() ? -1 : it->second;
}

int CampusGraph::indexOfEdgeId(const std::string& edgeId) const {
    auto it = edgeIndex_.find(upper(trim(edgeId)));
    return it == edgeIndex_.end() ? -1 : it->second;
}

const Place* CampusGraph::findPlace(int placeId) const {
    auto it = placeIndex_.find(placeId);
    return it == placeIndex_.end() ? nullptr : &places_[it->second];
}

StatusChange CampusGraph::setEdgeOpen(const std::string& edgeId, bool open) {
    const int i = indexOfEdgeId(edgeId);
    if (i < 0) return StatusChange::UnknownEdge;
    if (edges_[i].open == open) return StatusChange::Unchanged;
    edges_[i].open = open;
    ++version_;
    return StatusChange::Changed;
}

int CampusGraph::resetToDataStatus() {
    int changed = 0;
    for (auto& e : edges_) {
        if (e.open != e.openInData) {
            e.open = e.openInData;
            ++changed;
        }
    }
    if (changed > 0) ++version_;
    return changed;
}

int CampusGraph::closedEdgeCount() const {
    return static_cast<int>(std::count_if(edges_.begin(), edges_.end(), [](const PhysicalEdge& e) { return !e.open; }));
}

GraphSummary summarize(const CampusGraph& g) {
    GraphSummary s;
    s.nodes = g.nodeCount();
    s.physicalEdges = g.edgeCount();
    s.places = static_cast<int>(g.places().size());
    s.adjacencyEntries = g.adjacencyEntryCount();
    std::vector<int> parent(s.nodes), degree(s.nodes, 0);
    std::iota(parent.begin(), parent.end(), 0);
    std::function<int(int)> find = [&](int v) { return parent[v] == v ? v : parent[v] = find(parent[v]); };
    for (const auto& e : g.edges()) {
        (e.direction == Direction::TwoWay ? s.twoWay : s.oneWay)++;
        if (!e.open) ++s.closed;
        if (e.stepFree == StepFree::Yes) ++s.stepFreeYes;
        else if (e.stepFree == StepFree::No) ++s.stepFreeNo;
        else ++s.stepFreeUnknown;
        s.totalLengthM += e.distance;
        ++degree[e.from];
        ++degree[e.to];
        parent[find(e.from)] = find(e.to);
    }
    for (int v = 0; v < s.nodes; ++v) {
        if (degree[v] == 0) ++s.isolatedNodes;
        if (find(v) == v) ++s.components;
    }
    return s;
}

}  // namespace campus
