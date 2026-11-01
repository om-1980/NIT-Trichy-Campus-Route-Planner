#pragma once
// Campus graph model (modules M1 load/validate, M2 build adjacency, M4 path status).
//
// Three separate ID spaces are used on purpose:
//   * graph-node ID  - integer from nodes.txt (any non-negative int; need not be contiguous)
//   * physical edge ID - text from edges.txt, e.g. "E05" (case-insensitive, stored upper-case)
//   * place ID       - small integer from places.txt that the user types in the menu
// Internally every node/edge is addressed by a dense 0-based index.

#include <cstddef>
#include <iosfwd>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace campus {

enum class Direction { TwoWay, OneWay };
enum class StepFree { Yes, No, Unknown };
enum class PathType { Walkway, Road, Ramp, Stairs };

const char* toString(Direction d);
const char* toString(StepFree s);
const char* toString(PathType t);

struct Node {
    int id = 0;             // graph-node ID from nodes.txt
    std::string name;
    bool hasCoords = false;
    double x = 0.0;         // metres in a local planar frame (optional)
    double y = 0.0;
};

struct PhysicalEdge {
    std::string id;          // unique physical edge ID, upper-case
    int from = -1;           // internal node index
    int to = -1;             // internal node index
    double distance = 0.0;   // metres, finite and >= 0
    Direction direction = Direction::TwoWay;  // one-way means from -> to only
    bool open = true;        // current status
    bool openInData = true;  // status as written in edges.txt
    StepFree stepFree = StepFree::Unknown;
    PathType type = PathType::Walkway;
    std::string label;
};

struct Place {
    int id = 0;              // menu ID
    int node = -1;           // internal index of the mapped approach node
    std::string name;
};

// One directed adjacency entry. Both directions of a two-way path refer to the
// same PhysicalEdge, so one status flag controls both directions.
struct AdjEntry {
    int to;
    int edge;
};

struct LoadReport {
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    bool ok() const { return errors.empty(); }
};

enum class StatusChange { Changed, Unchanged, UnknownEdge };

class CampusGraph {
public:
    // Loads nodes.txt, edges.txt, places.txt and the optional metadata.txt from
    // `dir`. On any error `out` is left untouched and the problems are listed.
    static bool loadDirectory(const std::string& dir, CampusGraph& out, LoadReport& report);

    // Same as loadDirectory but from streams. `places` and `metadata` may be null.
    static bool loadStreams(std::istream& nodes, std::istream& edges, std::istream* places,
                            std::istream* metadata, CampusGraph& out, LoadReport& report);

    int nodeCount() const { return static_cast<int>(nodes_.size()); }
    int edgeCount() const { return static_cast<int>(edges_.size()); }
    std::size_t adjacencyEntryCount() const;

    const std::vector<Node>& nodes() const { return nodes_; }
    const std::vector<PhysicalEdge>& edges() const { return edges_; }
    const std::vector<Place>& places() const { return places_; }
    const std::vector<AdjEntry>& neighbours(int index) const { return adj_[index]; }
    const std::vector<std::pair<std::string, std::string>>& metadata() const { return metadata_; }
    std::string metadataValue(const std::string& key) const;

    int indexOfNodeId(int nodeId) const;                 // -1 if unknown
    int indexOfEdgeId(const std::string& edgeId) const;  // -1 if unknown
    const Place* findPlace(int placeId) const;           // nullptr if unknown

    // Closing/reopening a physical edge bumps the graph version when it changes.
    StatusChange setEdgeOpen(const std::string& edgeId, bool open);
    int resetToDataStatus();  // returns the number of edges whose status changed
    int closedEdgeCount() const;
    unsigned version() const { return version_; }

private:
    std::vector<Node> nodes_;
    std::vector<PhysicalEdge> edges_;
    std::vector<Place> places_;
    std::vector<std::vector<AdjEntry>> adj_;
    std::vector<std::pair<std::string, std::string>> metadata_;
    std::unordered_map<int, int> nodeIndex_;
    std::unordered_map<std::string, int> edgeIndex_;
    std::unordered_map<int, int> placeIndex_;
    unsigned version_ = 1;
};

struct GraphSummary {
    int nodes = 0;
    int physicalEdges = 0;
    int twoWay = 0;
    int oneWay = 0;
    int closed = 0;
    int places = 0;
    int isolatedNodes = 0;
    int components = 0;  // weakly connected components of the whole physical network
    std::size_t adjacencyEntries = 0;
    int stepFreeYes = 0;
    int stepFreeNo = 0;
    int stepFreeUnknown = 0;
    double totalLengthM = 0.0;
};

GraphSummary summarize(const CampusGraph& g);

}  // namespace campus
