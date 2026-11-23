// Leaflet-powered Realistic Interactive Campus Map Engine for NIT Trichy

class CampusMap {
    constructor(containerId, campusData, onPlaceClick, onEdgeClick) {
        this.containerId = containerId;
        this.data = campusData;
        this.nodes = campusData.nodes;
        this.places = campusData.places;
        this.edges = campusData.edges;
        this.gps = campusData.gps;
        this.onPlaceClick = onPlaceClick;
        this.onEdgeClick = onEdgeClick;

        this.nodeMap = new Map();
        this.nodes.forEach(n => this.nodeMap.set(n.id, n));

        this.placeMap = new Map();
        this.places.forEach(p => this.placeMap.set(p.id, p));

        this.selectedPlaceA = null;
        this.selectedPlaceB = null;
        this.currentRoute = null;
        this.activeCategory = "all";

        this.map = null;
        this.tileLayers = {};
        this.roadLayerGroup = null;
        this.markerLayerGroup = null;
        this.routeLayerGroup = null;
        this.simulationMarker = null;
        this.simInterval = null;

        this.initMap();
        this.renderRoads();
        this.renderPlaces();
    }

    initMap() {
        const center = this.gps ? this.gps.center : [10.7645, 78.8145];
        
        // Leaflet Map instance
        this.map = L.map(this.containerId, {
            center: center,
            zoom: 16,
            minZoom: 13,
            maxZoom: 20,
            zoomControl: false,
            attributionControl: false
        });

        // Add custom attribution at bottom right
        L.control.attribution({ position: 'bottomright', prefix: false }).addTo(this.map);

        // Completely Free, High-Performance Tile Layers (NO API KEY REQUIRED, NO "Map data not available" on Zoom)
        // 1. Satellite & Hybrid (Real High-Res Aerial Photography + Road Overlays up to Zoom 20)
        this.tileLayers.satellite = L.tileLayer('https://mt1.google.com/vt/lyrs=y&x={x}&y={y}&z={z}', {
            attribution: '&copy; Google Satellite Imagery &bull; NIT Trichy',
            maxZoom: 20,
            maxNativeZoom: 20
        });

        // 2. Crisp Streets Cartography (High-Contrast Campus Roads & Labels up to Zoom 20)
        this.tileLayers.streets = L.tileLayer('https://mt1.google.com/vt/lyrs=m&x={x}&y={y}&z={z}', {
            attribution: '&copy; Google Maps &bull; NIT Trichy',
            maxZoom: 20,
            maxNativeZoom: 20
        });

        // 3. OpenStreetMap Standard (Community Cartography with client-side overzoom to avoid missing tile errors)
        this.tileLayers.osm = L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
            attribution: '&copy; OpenStreetMap contributors &bull; NIT Trichy',
            maxZoom: 20,
            maxNativeZoom: 19
        });

        // 4. Esri Satellite (Clean aerial view with maxNativeZoom=18 so Leaflet overzooms without "not available" watermark)
        this.tileLayers.esri = L.tileLayer('https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}', {
            attribution: 'Esri Satellite &bull; NIT Trichy Campus',
            maxZoom: 20,
            maxNativeZoom: 18
        });

        // Default to Google Streets (high-res, crisp, zero watermark, zero missing data on zoom)
        this.tileLayers.streets.addTo(this.map);
        this.activeBaseLayer = 'streets';

        // Layer Groups
        this.roadLayerGroup = L.layerGroup().addTo(this.map);
        this.routeLayerGroup = L.layerGroup().addTo(this.map);
        this.markerLayerGroup = L.layerGroup().addTo(this.map);

        // Fit campus bounds
        if (this.gps) {
            const bounds = [
                [this.gps.minLat - 0.001, this.gps.minLon - 0.001],
                [this.gps.maxLat + 0.001, this.gps.maxLon + 0.001]
            ];
            this.map.fitBounds(bounds, { padding: [40, 40] });
        }
    }

    setBaseMap(layerName) {
        const viewport = document.getElementById(this.containerId);
        
        let actualLayer = layerName;
        if (layerName === 'dark') {
            if (viewport) viewport.classList.add('map-dark-theme');
            actualLayer = 'streets';
        } else {
            if (viewport) viewport.classList.remove('map-dark-theme');
        }

        if (!this.tileLayers[actualLayer]) return;
        Object.values(this.tileLayers).forEach(layer => {
            if (this.map.hasLayer(layer)) this.map.removeLayer(layer);
        });
        this.tileLayers[actualLayer].addTo(this.map);
        this.activeBaseLayer = layerName;

        // Re-render roads with optimal contrast for the layer
        this.renderRoads();
    }

    renderRoads() {
        this.roadLayerGroup.clearLayers();

        const isSatellite = (this.activeBaseLayer === 'satellite');
        const isDark = (this.activeBaseLayer === 'dark');

        this.edges.forEach(e => {
            const u = this.nodeMap.get(e.from);
            const v = this.nodeMap.get(e.to);
            if (!u || !v || !u.lat || !v.lat) return;

            const latlngs = (e.coords && e.coords.length >= 2) ? e.coords : [
                [u.lat, u.lon],
                [v.lat, v.lon]
            ];

            const isOpen = window.campusRouter ? window.campusRouter.isEdgeOpen(e.id) : (e.status === "open");
            const isMainSpine = (u.x === 0 && v.x === 0);

            let color = isSatellite ? "#ffffff" : (isDark ? "#475569" : "#64748b");
            let weight = 3.5;
            let dashArray = null;
            let opacity = isSatellite ? 0.8 : 0.85;

            if (isMainSpine) {
                color = isSatellite ? "#fbbf24" : (isDark ? "#60a5fa" : "#3b82f6");
                weight = 5.5;
                opacity = 0.95;
            } else if (e.type === "walkway") {
                color = isSatellite ? "#38bdf8" : (isDark ? "#38bdf8" : "#0284c7");
                weight = 2.5;
                dashArray = "4, 4";
            } else if (e.type === "ramp") {
                color = "#0d9488";
                weight = 3.0;
                dashArray = "3, 3";
            } else if (e.type === "stairs") {
                color = "#ea580c";
                weight = 3.5;
                dashArray = "6, 4";
            }

            if (!isOpen) {
                color = "#ef4444";
                weight = 4.5;
                dashArray = "8, 6";
                opacity = 1.0;
            }

            const poly = L.polyline(latlngs, {
                color: color,
                weight: weight,
                dashArray: dashArray,
                opacity: opacity,
                lineCap: "round"
            });

            // Hover tooltip & click to toggle closure
            poly.bindTooltip(`<strong>${e.id}</strong>: ${e.label || e.type} (${e.distance} m)<br><span style="font-size:10px; color:${isOpen ? '#10b981' : '#ef4444'}; font-weight:700;">${isOpen ? '● OPEN' : '■ CLOSED'} &bull; Click to toggle</span>`, {
                sticky: true,
                className: 'road-leaflet-tooltip'
            });

            poly.on("click", (evt) => {
                L.DomEvent.stopPropagation(evt);
                if (this.onEdgeClick) this.onEdgeClick(e.id, evt);
            });

            this.roadLayerGroup.addLayer(poly);
        });
    }

    getCategoryStyle(category) {
        const styles = {
            hostels: { bg: "#f97316", icon: "🛏️", label: "Hostel" },
            departments: { bg: "#2563eb", icon: "🏛️", label: "Department" },
            workshops: { bg: "#0891b2", icon: "⚙️", label: "Workshop/Lab" },
            libraries: { bg: "#7c3aed", icon: "📚", label: "Library/LHC" },
            messes: { bg: "#059669", icon: "🍽️", label: "Mess" },
            canteens: { bg: "#d97706", icon: "☕", label: "Canteen/Cafe" },
            sports: { bg: "#16a34a", icon: "⚽", label: "Sports" },
            admin: { bg: "#dc2626", icon: "🏢", label: "Admin" },
            amenities: { bg: "#0d9488", icon: "🏪", label: "Shopping/Bank" },
            worship: { bg: "#b45309", icon: "🛕", label: "Worship" },
            gates: { bg: "#475569", icon: "🛡️", label: "Gate" },
            other: { bg: "#6b7280", icon: "📍", label: "Place" }
        };
        return styles[category] || styles.other;
    }

    renderPlaces() {
        this.markerLayerGroup.clearLayers();

        this.places.forEach(p => {
            if (this.activeCategory !== "all" && p.category !== this.activeCategory) {
                return;
            }

            const style = this.getCategoryStyle(p.category);
            const isStart = this.selectedPlaceA && this.selectedPlaceA.id === p.id;
            const isDest = this.selectedPlaceB && this.selectedPlaceB.id === p.id;

            let markerHtml = "";
            let iconSize = [28, 28];
            let iconAnchor = [14, 14];

            if (isStart) {
                markerHtml = `
                    <div class="custom-pin pin-start">
                        <div class="pulse-ring ring-start"></div>
                        <div class="pin-badge bg-start">A</div>
                    </div>
                `;
                iconSize = [36, 36];
                iconAnchor = [18, 18];
            } else if (isDest) {
                markerHtml = `
                    <div class="custom-pin pin-dest">
                        <div class="pulse-ring ring-dest"></div>
                        <div class="pin-badge bg-dest">B</div>
                    </div>
                `;
                iconSize = [36, 36];
                iconAnchor = [18, 18];
            } else {
                markerHtml = `
                    <div class="custom-pin pin-normal" style="background-color: ${style.bg};">
                        <span class="pin-icon">${style.icon}</span>
                    </div>
                `;
            }

            const customIcon = L.divIcon({
                html: markerHtml,
                className: 'campus-div-icon',
                iconSize: iconSize,
                iconAnchor: iconAnchor
            });

            const marker = L.marker([p.lat, p.lon], { icon: customIcon });

            // Tooltip
            marker.bindTooltip(`
                <div class="place-leaflet-tooltip">
                    <span class="tooltip-cat" style="color: ${style.bg}">${style.label}</span>
                    <strong>${p.name}</strong>
                    <span class="tooltip-sub">#${p.id} &bull; Node [${p.nodeId}]</span>
                </div>
            `, {
                direction: 'top',
                offset: [0, -14],
                className: 'custom-leaflet-tooltip'
            });

            marker.on("click", (evt) => {
                L.DomEvent.stopPropagation(evt);
                if (this.onPlaceClick) this.onPlaceClick(p, evt);
            });

            this.markerLayerGroup.addLayer(marker);
        });
    }

    renderRoute(routeResult) {
        this.currentRoute = routeResult;
        this.routeLayerGroup.clearLayers();
        if (this.simulationMarker) {
            this.map.removeLayer(this.simulationMarker);
            this.simulationMarker = null;
        }
        if (this.simInterval) {
            clearInterval(this.simInterval);
            this.simInterval = null;
        }

        if (!routeResult || routeResult.status !== "OK" || routeResult.pathNodes.length === 0) {
            this.renderPlaces();
            return;
        }

        let latlngs = [];
        if (routeResult.pathEdges && routeResult.pathEdges.length > 0) {
            routeResult.pathEdges.forEach((edge, idx) => {
                const u = this.nodeMap.get(edge.from);
                const v = this.nodeMap.get(edge.to);
                const fromNode = routeResult.pathNodes[idx];
                const isForward = (fromNode && fromNode.id === edge.from);
                let pts = (edge.coords && edge.coords.length >= 2)
                    ? edge.coords
                    : [[u.lat, u.lon], [v.lat, v.lon]];
                if (!isForward) {
                    pts = [...pts].reverse();
                }
                if (latlngs.length > 0 && pts.length > 0) {
                    latlngs.push(...pts.slice(1));
                } else {
                    latlngs.push(...pts);
                }
            });
        } else {
            latlngs = routeResult.pathNodes.map(n => [n.lat, n.lon]);
        }
        routeResult.detailedCoords = latlngs;

        // 1. Glowing outer shadow polyline
        const glowLine = L.polyline(latlngs, {
            color: "#2563eb",
            weight: 12,
            opacity: 0.35,
            lineCap: "round",
            lineJoin: "round"
        });

        // 2. Main vivid blue polyline
        const mainLine = L.polyline(latlngs, {
            color: "#3b82f6",
            weight: 6,
            opacity: 0.95,
            lineCap: "round",
            lineJoin: "round"
        });

        // 3. Animated dashed flow line
        const animatedLine = L.polyline(latlngs, {
            color: "#93c5fd",
            weight: 3.5,
            dashArray: "10, 12",
            opacity: 0.9,
            className: "leaflet-animated-route-flow"
        });

        this.routeLayerGroup.addLayer(glowLine);
        this.routeLayerGroup.addLayer(mainLine);
        this.routeLayerGroup.addLayer(animatedLine);

        // Turn waypoint dots
        routeResult.pathNodes.forEach((n, idx) => {
            if (idx === 0 || idx === routeResult.pathNodes.length - 1) return;
            const turnDot = L.circleMarker([n.lat, n.lon], {
                radius: 4.5,
                fillColor: "#ffffff",
                color: "#1d4ed8",
                weight: 2,
                opacity: 1,
                fillOpacity: 1
            });
            turnDot.bindTooltip(`Step ${idx}: ${n.name}`, { direction: "top", offset: [0, -5] });
            this.routeLayerGroup.addLayer(turnDot);
        });

        this.renderPlaces();
        this.focusOnRoute();
    }

    focusOnRoute() {
        if (!this.currentRoute || !this.currentRoute.pathNodes || this.currentRoute.pathNodes.length < 2) return;
        const latlngs = (this.currentRoute.detailedCoords && this.currentRoute.detailedCoords.length >= 2)
            ? this.currentRoute.detailedCoords
            : this.currentRoute.pathNodes.map(n => [n.lat, n.lon]);
        this.map.fitBounds(L.latLngBounds(latlngs), { padding: [60, 60], maxZoom: 17 });
    }

    focusOnPlace(place) {
        if (!place || !place.lat) return;
        this.map.setView([place.lat, place.lon], 17, { animate: true });
    }

    resetView() {
        if (this.gps) {
            const bounds = [
                [this.gps.minLat - 0.0008, this.gps.minLon - 0.0008],
                [this.gps.maxLat + 0.0008, this.gps.maxLon + 0.0008]
            ];
            this.map.fitBounds(bounds, { padding: [30, 30] });
        }
    }

    setSelection(placeA, placeB) {
        this.selectedPlaceA = placeA;
        this.selectedPlaceB = placeB;
        this.renderPlaces();
    }

    setCategoryFilter(category) {
        this.activeCategory = category;
        this.renderPlaces();
    }

    simulateRoute(speedFactor = 1.0) {
        if (!this.currentRoute || !this.currentRoute.pathNodes || this.currentRoute.pathNodes.length < 2) return;

        if (this.simulationMarker) {
            this.map.removeLayer(this.simulationMarker);
        }
        if (this.simInterval) {
            clearInterval(this.simInterval);
        }

        const latlngs = (this.currentRoute.detailedCoords && this.currentRoute.detailedCoords.length >= 2)
            ? this.currentRoute.detailedCoords
            : this.currentRoute.pathNodes.map(n => [n.lat, n.lon]);
        let step = 0;
        const totalSteps = latlngs.length;

        const simIcon = L.divIcon({
            html: `<div class="sim-person-marker">🏃</div>`,
            className: 'sim-div-icon',
            iconSize: [30, 30],
            iconAnchor: [15, 15]
        });

        this.simulationMarker = L.marker(latlngs[0], { icon: simIcon, zIndexOffset: 1000 }).addTo(this.map);

        this.simInterval = setInterval(() => {
            step++;
            if (step >= totalSteps) {
                clearInterval(this.simInterval);
                this.simInterval = null;
                return;
            }
            this.simulationMarker.setLatLng(latlngs[step]);
        }, Math.max(150, 600 / speedFactor));
    }
}

if (typeof window !== "undefined") {
    window.CampusMap = CampusMap;
}
