import json
import os
import re
import math
import xml.etree.ElementTree as ET

DATA_DIR = r"D:\NIT Trichy Campus Route Planner\data\nitt"
GUI_DIR = r"D:\NIT Trichy Campus Route Planner\gui"
OSM_FILE = r"D:\NIT Trichy Campus Route Planner\osm_nitt_real.xml"
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

def dist_m(p1, p2):
    return math.hypot((p1[0] - p2[0])*METRES_PER_DEG_LAT, (p1[1] - p2[1])*METRES_PER_DEG_LON)

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

# Load OSM road ways if available
osm_highways = []
if os.path.exists(OSM_FILE):
    try:
        tree = ET.parse(OSM_FILE)
        root = tree.getroot()
        osm_nodes = {}
        for n in root.findall('node'):
            osm_nodes[n.attrib['id']] = (float(n.attrib['lat']), float(n.attrib['lon']))

        for w in root.findall('way'):
            tags = {t.attrib['k']: t.attrib['v'] for t in w.findall('tag')}
            if 'highway' in tags:
                nds = [osm_nodes[nd.attrib['ref']] for nd in w.findall('nd') if nd.attrib['ref'] in osm_nodes]
                osm_highways.append({
                    'id': w.attrib['id'],
                    'pts': nds
                })
        print(f"Loaded {len(osm_highways)} physical highway ways from OSM for curve calibration.")
    except Exception as ex:
        print(f"Warning: could not parse OSM XML: {ex}")

def point_to_segment(p, a, b):
    px, py = p[1]*METRES_PER_DEG_LON, p[0]*METRES_PER_DEG_LAT
    ax, ay = a[1]*METRES_PER_DEG_LON, a[0]*METRES_PER_DEG_LAT
    bx, by = b[1]*METRES_PER_DEG_LON, b[0]*METRES_PER_DEG_LAT
    dx, dy = bx - ax, by - ay
    if dx == 0 and dy == 0:
        return dist_m(p, a), 0.0, a
    t = max(0.0, min(1.0, ((px - ax)*dx + (py - ay)*dy) / (dx*dx + dy*dy)))
    proj_lat = a[0] + t*(b[0] - a[0])
    proj_lon = a[1] + t*(b[1] - a[1])
    d = math.hypot(px - (proj_lon*METRES_PER_DEG_LON), py - (proj_lat*METRES_PER_DEG_LAT))
    return d, t, (proj_lat, proj_lon)

def get_edge_polyline(u_obj, v_obj):
    u_pt = (u_obj['lat'], u_obj['lon'])
    v_pt = (v_obj['lat'], v_obj['lon'])

    if not osm_highways:
        return [[round(u_pt[0], 6), round(u_pt[1], 6)], [round(v_pt[0], 6), round(v_pt[1], 6)]]

    best_way = None
    best_dist = float('inf')
    best_u_info = None
    best_v_info = None

    for h in osm_highways:
        pts = h['pts']
        u_d, u_seg, u_proj = float('inf'), None, None
        v_d, v_seg, v_proj = float('inf'), None, None

        for i in range(len(pts) - 1):
            d, t, proj = point_to_segment(u_pt, pts[i], pts[i+1])
            if d < u_d:
                u_d = d
                u_seg = (i, t)
                u_proj = proj

            d2, t2, proj2 = point_to_segment(v_pt, pts[i], pts[i+1])
            if d2 < v_d:
                v_d = d2
                v_seg = (i, t2)
                v_proj = proj2

        if u_d < 12.0 and v_d < 12.0:
            if u_d + v_d < best_dist:
                best_dist = u_d + v_d
                best_way = h
                best_u_info = (u_seg, u_proj)
                best_v_info = (v_seg, v_proj)

    if best_way:
        (i_u, t_u), u_proj = best_u_info
        (i_v, t_v), v_proj = best_v_info
        pts = best_way['pts']
        pos_u = i_u + t_u
        pos_v = i_v + t_v

        path = [u_pt]
        if pos_u < pos_v:
            path.append(u_proj)
            for idx in range(i_u + 1, i_v + 1):
                path.append(pts[idx])
            path.append(v_proj)
        else:
            path.append(u_proj)
            for idx in range(i_u, i_v, -1):
                path.append(pts[idx])
            path.append(v_proj)
        path.append(v_pt)

        cleaned = [path[0]]
        for pt in path[1:]:
            if dist_m(cleaned[-1], pt) > 0.8:
                cleaned.append(pt)
        if len(cleaned) >= 2:
            return [[round(p[0], 6), round(p[1], 6)] for p in cleaned]

    return [[round(u_pt[0], 6), round(u_pt[1], 6)], [round(v_pt[0], 6), round(v_pt[1], 6)]]

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

        u_obj = node_map.get(u)
        v_obj = node_map.get(v)
        coords = get_edge_polyline(u_obj, v_obj) if (u_obj and v_obj) else []

        edges.append({
            "id": eid,
            "from": u,
            "to": v,
            "distance": dist,
            "direction": direction,
            "status": status,
            "stepFree": step_free,
            "type": ptype,
            "label": label,
            "coords": coords
        })

# Compute campus bounding box
all_x = [n["x"] for n in nodes]
all_y = [n["y"] for n in nodes]
all_lat = [n["lat"] for n in nodes]
all_lon = [n["lon"] for n in nodes]

campus_data = {
    "metadata": metadata,
    "nodes": nodes,
    "places": places,
    "edges": edges,
    "bounds": {
        "minX": min(all_x),
        "maxX": max(all_x),
        "minY": min(all_y),
        "maxY": max(all_y)
    },
    "gps": {
        "refLat": REF_LAT,
        "refLon": REF_LON,
        "minLat": min(all_lat),
        "maxLat": max(all_lat),
        "minLon": min(all_lon),
        "maxLon": max(all_lon),
        "center": [round(sum(all_lat)/len(all_lat), 6), round(sum(all_lon)/len(all_lon), 6)]
    }
}

# Write gui/data.json
json_path = os.path.join(GUI_DIR, "data.json")
with open(json_path, "w", encoding="utf-8") as f:
    json.dump(campus_data, f, indent=2)

# Write gui/data.js
js_path = os.path.join(GUI_DIR, "data.js")
with open(js_path, "w", encoding="utf-8") as f:
    f.write("// NIT Trichy Verified Campus Map Dataset for Web GUI\n")
    f.write("window.CAMPUS_DATA = ")
    json.dump(campus_data, f, indent=2)
    f.write(";\n")

print(f"Exported data for GUI: {len(nodes)} nodes, {len(places)} places, {len(edges)} edges.")
curved = sum(1 for e in edges if len(e.get("coords", [])) > 2)
print(f"Edges with physical road curvature: {curved} / {len(edges)}")
print(f"Campus bounds: X[{min(all_x)} .. {max(all_x)}], Y[{min(all_y)} .. {max(all_y)}]")
