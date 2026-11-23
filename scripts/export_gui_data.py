import json
import os
import re

DATA_DIR = r"D:\NIT Trichy Campus Route Planner\data\nitt"
GUI_DIR = r"D:\NIT Trichy Campus Route Planner\gui"
os.makedirs(GUI_DIR, exist_ok=True)

# Parse metadata
metadata = {}
with open(os.path.join(DATA_DIR, "metadata.txt"), "r", encoding="utf-8") as f:
    for line in f:
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if "=" in line:
            k, v = line.split("=", 1)
            metadata[k.strip()] = v.strip()

# Categorizer helper
def categorize(name):
    n = name.lower()
    if any(k in n for k in ["hostel", "beryl", "opal"]):
        return "hostels"
    if any(k in n for k in ["dept of", "department of"]):
        return "departments"
    if any(k in n for k in ["workshop", "siemens", "octagon", "computing", "lab", "cecase", "supercomputing"]):
        return "workshops"
    if any(k in n for k in ["library", "lecture hall", "orion", "lhc", "nlhc"]):
        return "libraries"
    if any(k in n for k in ["mess"]):
        return "messes"
    if any(k in n for k in ["canteen", "nescafe", "aavin", "food", "stalls"]):
        return "canteens"
    if any(k in n for k in ["sports", "gym", "stadium", "ground", "tennis", "basketball", "swimming", "court", "track"]):
        return "sports"
    if any(k in n for k in ["temple", "shrine", "mosque", "church", "prayer", "fellowship"]):
        return "worship"
    if any(k in n for k in ["shopping", "sbi", "atm", "post office", "store", "bookstall", "pharmacy", "guest house", "quarters", "bungalow"]):
        return "amenities"
    if any(k in n for k in ["gate"]):
        return "gates"
    if any(k in n for k in ["admin", "clock tower", "senate", "convention hall", "barn hall", "placement", "activity centre", "estate"]):
        return "admin"
    return "other"

# Reference GPS coordinate for NIT Trichy Main Gate (0, 0) on NH 67 (Trichy-Tanjore Road)
REF_LAT = 10.756873
REF_LON = 78.813229
METRES_PER_DEG_LAT = 111320.0
METRES_PER_DEG_LON = 109360.0

def to_gps(x_m, y_m):
    lat = REF_LAT + (y_m / METRES_PER_DEG_LAT)
    lon = REF_LON + (x_m / METRES_PER_DEG_LON)
    return round(lat, 6), round(lon, 6)

# Parse nodes
nodes = []
node_map = {}
with open(os.path.join(DATA_DIR, "nodes.txt"), "r", encoding="utf-8") as f:
    for line in f:
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        parts = [p.strip() for p in line.split("|")]
        nid = int(parts[0])
        name = parts[1]
        x = float(parts[2]) if len(parts) > 2 and parts[2] else 0.0
        y = float(parts[3]) if len(parts) > 3 and parts[3] else 0.0
        lat, lon = to_gps(x, y)
        cat = categorize(name)
        is_junction = "junction" in name.lower() or "cross" in name.lower()
        node_obj = {
            "id": nid,
            "name": name,
            "x": x,
            "y": y,
            "lat": lat,
            "lon": lon,
            "category": "junction" if is_junction else cat,
            "isJunction": is_junction
        }
        nodes.append(node_obj)
        node_map[nid] = node_obj

# Parse places
places = []
with open(os.path.join(DATA_DIR, "places.txt"), "r", encoding="utf-8") as f:
    for line in f:
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        parts = [p.strip() for p in line.split("|")]
        pid = int(parts[0])
        nid = int(parts[1])
        name = parts[2]
        cat = categorize(name)
        n = node_map.get(nid, {})
        places.append({
            "id": pid,
            "nodeId": nid,
            "name": name,
            "category": cat,
            "x": n.get("x", 0.0),
            "y": n.get("y", 0.0),
            "lat": n.get("lat", REF_LAT),
            "lon": n.get("lon", REF_LON)
        })

# Parse edges
edges = []
with open(os.path.join(DATA_DIR, "edges.txt"), "r", encoding="utf-8") as f:
    for line in f:
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        parts = [p.strip() for p in line.split("|")]
        eid = parts[0]
        u = int(parts[1])
        v = int(parts[2])
        dist = float(parts[3])
        direction = parts[4].lower()
        status = parts[5].lower()
        step_free = parts[6].lower()
        ptype = parts[7].lower()
        label = parts[8] if len(parts) > 8 else ""
        edges.append({
            "id": eid,
            "from": u,
            "to": v,
            "distance": dist,
            "direction": direction,
            "status": status,
            "stepFree": step_free,
            "type": ptype,
            "label": label
        })

campus_data = {
    "metadata": metadata,
    "nodes": nodes,
    "places": places,
    "edges": edges,
    "bounds": {
        "minX": min(n["x"] for n in nodes),
        "maxX": max(n["x"] for n in nodes),
        "minY": min(n["y"] for n in nodes),
        "maxY": max(n["y"] for n in nodes)
    },
    "gps": {
        "center": [round(REF_LAT + 700 / METRES_PER_DEG_LAT, 6), round(REF_LON + 150 / METRES_PER_DEG_LON, 6)],
        "minLat": min(n["lat"] for n in nodes),
        "maxLat": max(n["lat"] for n in nodes),
        "minLon": min(n["lon"] for n in nodes),
        "maxLon": max(n["lon"] for n in nodes)
    }
}

with open(os.path.join(GUI_DIR, "data.json"), "w", encoding="utf-8") as f:
    json.dump(campus_data, f, indent=2)

with open(os.path.join(GUI_DIR, "data.js"), "w", encoding="utf-8") as f:
    f.write("// Auto-generated from data/nitt\n")
    f.write("const dataObj = ")
    json.dump(campus_data, f, indent=2)
    f.write(";\n")
    f.write("if (typeof window !== 'undefined') { window.CAMPUS_DATA = dataObj; }\n")
    f.write("if (typeof module !== 'undefined') { module.exports = dataObj; }\n")

print(f"Exported data for GUI: {len(nodes)} nodes, {len(places)} places, {len(edges)} edges.")
print(f"Campus bounds: X[{campus_data['bounds']['minX']} .. {campus_data['bounds']['maxX']}], Y[{campus_data['bounds']['minY']} .. {campus_data['bounds']['maxY']}]")
