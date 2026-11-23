#pragma once

#include <cstddef>
#include <iosfwd>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;
namespace campus {

enum class Direction { TwoWay, OneWay };
enum class StepFree { Yes, No, Unknown };
enum class PathType { Walkway, Road, Ramp, Stairs };

const char* toString(Direction d);
const char* toString(StepFree s);
const char* toString(PathType t);

struct Node {
    int id = 0;             // graph-node ID from nodes.txt
    string name;
    bool hasCoords = false;
    double x = 0.0;         // metres in a local planar frame (optional)
    double y = 0.0;
};

struct PhysicalEdge {
    string id;          // unique physical edge ID, upper-case
    int from = -1;           // internal node index
    int to = -1;             // internal node index
    double distance = 0.0;   // metres, finite and >= 0
    Direction direction = Direction::TwoWay;  // one-way means from -> to only
    bool open = true;        // current status
    bool openInData = true;  // status as written in edges.txt
    StepFree stepFree = StepFree::Unknown;
    PathType type = PathType::Walkway;
    string label;
};

struct Place {
    int id = 0;              // menu ID
    int node = -1;           // internal index of the mapped approach node
    string name;
};

// One directed adjacency entry. Both directions of a two-way path refer to the
// same PhysicalEdge, so one status flag controls both directions.
struct AdjEntry {
    int to;
    int edge;
};

struct LoadReport {
    vector<string> errors;
    vector<string> warnings;
    bool ok() const { return errors.empty(); }
};

enum class StatusChange { Changed, Unchanged, UnknownEdge };

class CampusGraph {
public:
    // Loads nodes.txt, edges.txt, places.txt and the optional metadata.txt from
    // `dir`. On any error `out` is left untouched and the problems are listed.
    static bool loadDirectory(const string& dir, CampusGraph& out, LoadReport& report);

    // Same as loadDirectory but from streams. `places` and `metadata` may be null.
    static bool loadStreams(istream& nodes, istream& edges, istream* places,
                            istream* metadata, CampusGraph& out, LoadReport& report);

    int nodeCount() const { return static_cast<int>(nodes_.size()); }
    int edgeCount() const { return static_cast<int>(edges_.size()); }
    size_t adjacencyEntryCount() const;

    const vector<Node>& nodes() const { return nodes_; }
    const vector<PhysicalEdge>& edges() const { return edges_; }
    const vector<Place>& places() const { return places_; }
    const vector<AdjEntry>& neighbours(int index) const { return adj_[index]; }
    const vector<pair<string, string>>& metadata() const { return metadata_; }
    string metadataValue(const string& key) const;

    int indexOfNodeId(int nodeId) const;                 // -1 if unknown
    int indexOfEdgeId(const string& edgeId) const;  // -1 if unknown
    const Place* findPlace(int placeId) const;           // nullptr if unknown

    // Closing/reopening a physical edge bumps the graph version when it changes.
    StatusChange setEdgeOpen(const string& edgeId, bool open);
    int resetToDataStatus();  // returns the number of edges whose status changed
    int closedEdgeCount() const;
    unsigned version() const { return version_; }

private:
    vector<Node> nodes_;
    vector<PhysicalEdge> edges_;
    vector<Place> places_;
    vector<vector<AdjEntry>> adj_;
    vector<pair<string, string>> metadata_;
    unordered_map<int, int> nodeIndex_;
    unordered_map<string, int> edgeIndex_;
    unordered_map<int, int> placeIndex_;
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
    size_t adjacencyEntries = 0;
    int stepFreeYes = 0;
    int stepFreeNo = 0;
    int stepFreeUnknown = 0;
    double totalLengthM = 0.0;
};

GraphSummary summarize(const CampusGraph& g);

}  // namespace campus
