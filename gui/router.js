// Dijkstra Shortest Path Router for NIT Trichy Campus Route Planner
// Implements the exact logic of campus::Router from router.cpp (binary min-heap, tie-breaking, closures, step-free)

class PriorityQueue {
    constructor() {
        this.heap = [];
    }

    push(item) {
        this.heap.push(item);
        this._siftUp(this.heap.length - 1);
    }

    pop() {
        if (this.heap.length === 0) return null;
        const top = this.heap[0];
        const bottom = this.heap.pop();
        if (this.heap.length > 0) {
            this.heap[0] = bottom;
            this._siftDown(0);
        }
        return top;
    }

    isEmpty() {
        return this.heap.length === 0;
    }

    _compare(a, b) {
        // [dist, hops, nodeIndex]
        if (Math.abs(a.dist - b.dist) > 1e-9) {
            return a.dist < b.dist ? -1 : 1;
        }
        if (a.hops !== b.hops) {
            return a.hops < b.hops ? -1 : 1;
        }
        return 0;
    }

    _siftUp(idx) {
        let current = idx;
        while (current > 0) {
            const parent = Math.floor((current - 1) / 2);
            if (this._compare(this.heap[current], this.heap[parent]) < 0) {
                const tmp = this.heap[current];
                this.heap[current] = this.heap[parent];
                this.heap[parent] = tmp;
                current = parent;
            } else {
                break;
            }
        }
    }

    _siftDown(idx) {
        let current = idx;
        const length = this.heap.length;
        while (true) {
            let left = 2 * current + 1;
            let right = 2 * current + 2;
            let smallest = current;

            if (left < length && this._compare(this.heap[left], this.heap[smallest]) < 0) {
                smallest = left;
            }
            if (right < length && this._compare(this.heap[right], this.heap[smallest]) < 0) {
                smallest = right;
            }
            if (smallest !== current) {
                const tmp = this.heap[current];
                this.heap[current] = this.heap[smallest];
                this.heap[smallest] = tmp;
                current = smallest;
            } else {
                break;
            }
        }
    }
}

class CampusRouter {
    constructor(campusData) {
        this.data = campusData;
        this.nodes = campusData.nodes;
        this.edges = campusData.edges;
        this.places = campusData.places;
        
        // Maps
        this.nodeIndex = new Map();
        this.nodes.forEach((n, idx) => {
            this.nodeIndex.set(n.id, idx);
        });

        this.placeMap = new Map();
        this.places.forEach(p => {
            this.placeMap.set(p.id, p);
        });

        this.edgeMap = new Map();
        this.edges.forEach(e => {
            this.edgeMap.set(e.id.toUpperCase(), e);
        });

        // Dynamic edge status state (can be changed by GUI)
        this.edgeOpenState = new Map();
        this.edges.forEach(e => {
            this.edgeOpenState.set(e.id.toUpperCase(), e.status === "open");
        });

        this.buildAdjacency();
    }

    buildAdjacency() {
        this.adj = Array.from({ length: this.nodes.length }, () => []);
        this.edges.forEach((e, edgeIdx) => {
            const uIdx = this.nodeIndex.get(e.from);
            const vIdx = this.nodeIndex.get(e.to);
            if (uIdx === undefined || vIdx === undefined) return;

            // Outgoing u -> v
            this.adj[uIdx].push({
                toNode: vIdx,
                edgeIdx: edgeIdx,
                edge: e,
                dist: e.distance
            });

            // If two-way, outgoing v -> u
            if (e.direction === "two-way") {
                this.adj[vIdx].push({
                    toNode: uIdx,
                    edgeIdx: edgeIdx,
                    edge: e,
                    dist: e.distance
                });
            }
        });
    }

    isEdgeOpen(edgeId) {
        return this.edgeOpenState.get(edgeId.toUpperCase()) ?? true;
    }

    setEdgeStatus(edgeId, isOpen) {
        const key = edgeId.toUpperCase();
        if (this.edgeOpenState.has(key)) {
            this.edgeOpenState.set(key, isOpen);
            return true;
        }
        return false;
    }

    toggleEdgeStatus(edgeId) {
        const key = edgeId.toUpperCase();
        if (this.edgeOpenState.has(key)) {
            const current = this.edgeOpenState.get(key);
            this.edgeOpenState.set(key, !current);
            return !current;
        }
        return null;
    }

    resetAllEdges() {
        this.edges.forEach(e => {
            this.edgeOpenState.set(e.id.toUpperCase(), e.status === "open");
        });
    }

    getClosedEdges() {
        const closed = [];
        this.edges.forEach(e => {
            if (!this.edgeOpenState.get(e.id.toUpperCase())) {
                closed.push(e);
            }
        });
        return closed;
    }

    findRoute(fromNodeId, toNodeId, mode = "standard") {
        const srcIdx = this.nodeIndex.get(fromNodeId);
        const dstIdx = this.nodeIndex.get(toNodeId);

        if (srcIdx === undefined || dstIdx === undefined) {
            return { status: "InvalidNodes", distance: 0, pathNodes: [], pathEdges: [] };
        }

        if (srcIdx === dstIdx) {
            return {
                status: "SameNode",
                distance: 0,
                pathNodes: [this.nodes[srcIdx]],
                pathEdges: [],
                segments: 0,
                durationWalkMin: 0,
                durationCycleMin: 0
            };
        }

        const N = this.nodes.length;
        const dist = new Array(N).fill(Infinity);
        const hops = new Array(N).fill(Infinity);
        const parentNode = new Array(N).fill(-1);
        const parentEdge = new Array(N).fill(-1);

        dist[srcIdx] = 0;
        hops[srcIdx] = 0;

        const pq = new PriorityQueue();
        pq.push({ dist: 0, hops: 0, u: srcIdx });

        while (!pq.isEmpty()) {
            const top = pq.pop();
            const d = top.dist;
            const h = top.hops;
            const u = top.u;

            if (d > dist[u] || (Math.abs(d - dist[u]) < 1e-9 && h > hops[u])) {
                continue;
            }

            if (u === dstIdx) break;

            for (const entry of this.adj[u]) {
                const e = entry.edge;
                const v = entry.toNode;

                // Check edge closure
                if (!this.isEdgeOpen(e.id)) continue;

                // Step-free filter: in step-free mode, only step_free === "yes" allowed
                if (mode === "step-free" && e.stepFree !== "yes") continue;

                const candDist = d + entry.dist;
                const candHops = h + 1;

                let isBetter = false;
                if (candDist < dist[v] - 1e-9) {
                    isBetter = true;
                } else if (Math.abs(candDist - dist[v]) <= 1e-9 && candHops < hops[v]) {
                    isBetter = true;
                }

                if (isBetter) {
                    dist[v] = candDist;
                    hops[v] = candHops;
                    parentNode[v] = u;
                    parentEdge[v] = entry.edgeIdx;
                    pq.push({ dist: candDist, hops: candHops, u: v });
                }
            }
        }

        if (!isFinite(dist[dstIdx])) {
            return {
                status: "NoRoute",
                distance: Infinity,
                pathNodes: [],
                pathEdges: [],
                segments: 0
            };
        }

        // Reconstruct path
        const pathNodes = [];
        const pathEdges = [];
        let curr = dstIdx;
        while (curr !== srcIdx) {
            pathNodes.push(this.nodes[curr]);
            const eIdx = parentEdge[curr];
            pathEdges.push(this.edges[eIdx]);
            curr = parentNode[curr];
        }
        pathNodes.push(this.nodes[srcIdx]);

        pathNodes.reverse();
        pathEdges.reverse();

        const walkTimeMin = Math.ceil(dist[dstIdx] / (1.2 * 60)); // ~4.3 km/h
        const cycleTimeMin = Math.max(1, Math.ceil(dist[dstIdx] / (4.2 * 60))); // ~15 km/h

        return {
            status: "OK",
            distance: Math.round(dist[dstIdx] * 10) / 10,
            pathNodes: pathNodes,
            pathEdges: pathEdges,
            segments: pathEdges.length,
            durationWalkMin: walkTimeMin,
            durationCycleMin: cycleTimeMin,
            isStepFree: pathEdges.every(e => e.stepFree === "yes")
        };
    }

    findRouteBetweenPlaces(placeAId, placeBId, mode = "standard") {
        const pA = this.placeMap.get(placeAId);
        const pB = this.placeMap.get(placeBId);
        if (!pA || !pB) {
            return { status: "InvalidPlaces", distance: 0, pathNodes: [], pathEdges: [] };
        }
        const res = this.findRoute(pA.nodeId, pB.nodeId, mode);
        res.placeA = pA;
        res.placeB = pB;
        return res;
    }
}

if (typeof window !== "undefined") {
    window.CampusRouter = CampusRouter;
}
if (typeof module !== "undefined") {
    module.exports = CampusRouter;
}
