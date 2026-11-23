// Main Application Controller for NIT Trichy Campus Route Planner GUI

document.addEventListener("DOMContentLoaded", () => {
    // 1. Check data availability
    const campusData = window.CAMPUS_DATA;
    if (!campusData) {
        alert("Error: Campus dataset not found in window.CAMPUS_DATA!");
        return;
    }

    // 2. Instantiate Router and Map
    const router = new CampusRouter(campusData);
    window.campusRouter = router; // make available globally

    let mapInstance = null;
    let selectedPlaceA = null;
    let selectedPlaceB = null;
    let currentMode = "standard"; // "standard" | "step-free"
    let activeCategory = "all";

    // 3. Elements
    const selectStart = document.getElementById("selectStart");
    const selectDest = document.getElementById("selectDest");
    const btnFindRoute = document.getElementById("btnFindRoute");
    const btnClearRoute = document.getElementById("btnClearRoute");
    const btnSwap = document.getElementById("btnSwap");
    const modeStandard = document.getElementById("modeStandard");
    const modeStepFree = document.getElementById("modeStepFree");
    const routeResultCard = document.getElementById("routeResultCard");
    const directionsList = document.getElementById("directionsList");
    const metricDistance = document.getElementById("metricDistance");
    const metricWalk = document.getElementById("metricWalk");
    const metricCycle = document.getElementById("metricCycle");
    const stepFreeBadge = document.getElementById("stepFreeBadge");
    const btnSimulateWalk = document.getElementById("btnSimulateWalk");
    const categoryChips = document.getElementById("categoryChips");
    const placesDirectory = document.getElementById("placesDirectory");
    const closuresList = document.getElementById("closuresList");
    const btnResetClosures = document.getElementById("btnResetClosures");
    const popover = document.getElementById("mapPopover");
    const focusBtn = document.getElementById("btnFocusRoute");

    // Basemap Switcher Buttons
    const btnBaseStreets = document.getElementById("btnBaseStreets");
    const btnBaseSatellite = document.getElementById("btnBaseSatellite");
    const btnBaseDark = document.getElementById("btnBaseDark");
    const btnBaseOsm = document.getElementById("btnBaseOsm");

    // 4. Initialize Map
    mapInstance = new CampusMap("mapViewport", campusData, onPlaceClicked, onEdgeClicked);

    // Basemap Switching Events
    const basemapButtons = [
        { btn: btnBaseStreets, name: 'streets' },
        { btn: btnBaseSatellite, name: 'satellite' },
        { btn: btnBaseDark, name: 'dark' },
        { btn: btnBaseOsm, name: 'osm' }
    ];

    basemapButtons.forEach(({ btn, name }) => {
        if (!btn) return;
        btn.addEventListener("click", () => {
            basemapButtons.forEach(b => b.btn.classList.remove("active"));
            btn.classList.add("active");
            mapInstance.setBaseMap(name);
        });
    });

    // Zoom and Navigation controls
    document.getElementById("btnZoomIn").addEventListener("click", () => mapInstance.map.zoomIn());
    document.getElementById("btnZoomOut").addEventListener("click", () => mapInstance.map.zoomOut());
    document.getElementById("btnResetView").addEventListener("click", () => mapInstance.resetView());
    if (focusBtn) focusBtn.addEventListener("click", () => mapInstance.focusOnRoute());

    // 5. Populate Dropdowns
    function populateDropdowns() {
        const sortedPlaces = [...campusData.places].sort((a, b) => a.name.localeCompare(b.name));

        const createOptions = (defaultText) => {
            let html = `<option value="">${defaultText}</option>`;
            sortedPlaces.forEach(p => {
                html += `<option value="${p.id}">#${p.id} - ${p.name}</option>`;
            });
            return html;
        };

        selectStart.innerHTML = createOptions("Select Start Location (A)...");
        selectDest.innerHTML = createOptions("Select Destination Location (B)...");
    }
    populateDropdowns();

    // 6. Populate Category Chips & Directory
    const categories = [
        { id: "all", label: "All Places (123)" },
        { id: "hostels", label: "Hostels (31)" },
        { id: "departments", label: "Departments (17)" },
        { id: "workshops", label: "Labs & Workshops (11)" },
        { id: "libraries", label: "Libraries & LHCs (7)" },
        { id: "messes", label: "Messes (11)" },
        { id: "canteens", label: "Canteens & Cafes (8)" },
        { id: "sports", label: "Sports & Gym (10)" },
        { id: "admin", label: "Administration (10)" },
        { id: "amenities", label: "Shopping & Bank (12)" },
        { id: "worship", label: "Places of Worship (4)" }
    ];

    function renderCategoryChips() {
        categoryChips.innerHTML = categories.map(c => `
            <button class="chip-btn ${c.id === activeCategory ? 'active' : ''}" data-cat="${c.id}">
                ${c.label}
            </button>
        `).join("");

        categoryChips.querySelectorAll(".chip-btn").forEach(btn => {
            btn.addEventListener("click", () => {
                activeCategory = btn.getAttribute("data-cat");
                renderCategoryChips();
                mapInstance.setCategoryFilter(activeCategory);
                renderDirectory();
            });
        });
    }

    function renderDirectory() {
        let filtered = campusData.places;
        if (activeCategory !== "all") {
            filtered = campusData.places.filter(p => p.category === activeCategory);
        }

        placesDirectory.innerHTML = filtered.map(p => {
            const style = mapInstance.getCategoryStyle(p.category);
            return `
                <div class="directory-item" data-place-id="${p.id}">
                    <div class="dir-item-icon" style="background-color: ${style.bg}"></div>
                    <div class="dir-item-info">
                        <div class="dir-item-name">${p.name}</div>
                        <div class="dir-item-sub">Place #${p.id} &bull; ${style.label}</div>
                    </div>
                    <button class="dir-quick-route" title="Route to here">Go</button>
                </div>
            `;
        }).join("");

        placesDirectory.querySelectorAll(".directory-item").forEach(item => {
            const pid = parseInt(item.getAttribute("data-place-id"), 10);
            const place = campusData.places.find(x => x.id === pid);

            item.addEventListener("click", (e) => {
                if (e.target.classList.contains("dir-quick-route")) {
                    setDestination(place);
                    executeRoute();
                } else {
                    mapInstance.focusOnPlace(place);
                    showPopover(place);
                }
            });
        });
    }

    renderCategoryChips();
    renderDirectory();

    // 7. Place Selection Handlers
    function setStart(place) {
        selectedPlaceA = place;
        selectStart.value = place ? place.id : "";
        mapInstance.setSelection(selectedPlaceA, selectedPlaceB);
        hidePopover();
    }

    function setDestination(place) {
        selectedPlaceB = place;
        selectDest.value = place ? place.id : "";
        mapInstance.setSelection(selectedPlaceA, selectedPlaceB);
        hidePopover();
    }

    selectStart.addEventListener("change", (e) => {
        const pid = parseInt(e.target.value, 10);
        selectedPlaceA = campusData.places.find(p => p.id === pid) || null;
        mapInstance.setSelection(selectedPlaceA, selectedPlaceB);
    });

    selectDest.addEventListener("change", (e) => {
        const pid = parseInt(e.target.value, 10);
        selectedPlaceB = campusData.places.find(p => p.id === pid) || null;
        mapInstance.setSelection(selectedPlaceA, selectedPlaceB);
    });

    btnSwap.addEventListener("click", () => {
        const temp = selectedPlaceA;
        selectedPlaceA = selectedPlaceB;
        selectedPlaceB = temp;

        selectStart.value = selectedPlaceA ? selectedPlaceA.id : "";
        selectDest.value = selectedPlaceB ? selectedPlaceB.id : "";
        mapInstance.setSelection(selectedPlaceA, selectedPlaceB);

        if (selectedPlaceA && selectedPlaceB) {
            executeRoute();
        }
    });

    // 8. Mode Toggles
    modeStandard.addEventListener("click", () => {
        currentMode = "standard";
        modeStandard.classList.add("active");
        modeStepFree.classList.remove("active");
        if (selectedPlaceA && selectedPlaceB) executeRoute();
    });

    modeStepFree.addEventListener("click", () => {
        currentMode = "step-free";
        modeStepFree.classList.add("active");
        modeStandard.classList.remove("active");
        if (selectedPlaceA && selectedPlaceB) executeRoute();
    });

    // 9. Route Execution & Rendering
    function executeRoute() {
        if (!selectedPlaceA || !selectedPlaceB) {
            alert("Please select both a Start (A) and Destination (B) location.");
            return;
        }

        const res = router.findRouteBetweenPlaces(selectedPlaceA.id, selectedPlaceB.id, currentMode);
        mapInstance.renderRoute(res);

        if (res.status === "OK") {
            routeResultCard.style.display = "flex";
            if (focusBtn) focusBtn.style.display = "flex";

            metricDistance.textContent = `${res.distance} m`;
            metricWalk.textContent = `${res.durationWalkMin} min`;
            metricCycle.textContent = `${res.durationCycleMin} min`;

            if (res.isStepFree) {
                stepFreeBadge.className = "step-free-badge badge-yes";
                stepFreeBadge.innerHTML = `
                    <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><polyline points="20 6 9 17 4 12"></polyline></svg>
                    100% Step-Free & Accessible
                `;
            } else {
                stepFreeBadge.className = "step-free-badge badge-no";
                stepFreeBadge.innerHTML = `
                    <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M18 6L6 18M6 6l12 12"/></svg>
                    Route Includes Stairs (Admin-Library Footbridge)
                `;
            }

            // Step by step
            directionsList.innerHTML = res.pathEdges.map((e, idx) => {
                const fromNode = res.pathNodes[idx];
                const toNode = res.pathNodes[idx + 1];
                let typeIcon = "🚶";
                if (e.type === "road") typeIcon = "🛣️";
                else if (e.type === "ramp") typeIcon = "♿";
                else if (e.type === "stairs") typeIcon = "🪜";

                return `
                    <div class="direction-item">
                        <div class="dir-step-num">${idx + 1}</div>
                        <div class="dir-step-body">
                            <div class="dir-step-name">${typeIcon} ${e.label || (e.type + ' path')}</div>
                            <div class="dir-step-meta">
                                ${fromNode.name.split('(')[0].trim()} &rarr; ${toNode.name.split('(')[0].trim()} &bull; 
                                <strong>${e.distance} m</strong> (${e.type})
                            </div>
                        </div>
                    </div>
                `;
            }).join("");
        } else if (res.status === "SameNode") {
            routeResultCard.style.display = "flex";
            metricDistance.textContent = "0 m";
            metricWalk.textContent = "0 min";
            metricCycle.textContent = "0 min";
            directionsList.innerHTML = `<div style="padding:10px; color:#94a3b8;">Start and destination are the same location.</div>`;
        } else {
            routeResultCard.style.display = "flex";
            metricDistance.textContent = "No Route";
            metricWalk.textContent = "--";
            metricCycle.textContent = "--";
            directionsList.innerHTML = `
                <div style="padding: 12px; color: #ef4444; background: rgba(239,68,68,0.1); border-radius: 6px;">
                    No accessible path found under current closures / step-free filters. Try reopening closed paths or switching to standard mode.
                </div>
            `;
        }
    }

    btnFindRoute.addEventListener("click", executeRoute);

    btnClearRoute.addEventListener("click", () => {
        selectedPlaceA = null;
        selectedPlaceB = null;
        selectStart.value = "";
        selectDest.value = "";
        mapInstance.setSelection(null, null);
        mapInstance.renderRoute(null);
        routeResultCard.style.display = "none";
        if (focusBtn) focusBtn.style.display = "none";
        hidePopover();
    });

    if (btnSimulateWalk) {
        btnSimulateWalk.addEventListener("click", () => {
            mapInstance.simulateRoute(1.5);
        });
    }

    // 10. Map Interaction Callbacks
    function onPlaceClicked(place, evt) {
        showPopover(place, evt);
    }

    function onEdgeClicked(edgeId) {
        router.toggleEdgeStatus(edgeId);
        mapInstance.renderRoads();
        renderClosuresList();
        
        if (selectedPlaceA && selectedPlaceB) {
            executeRoute();
        }
    }

    // Popover on place click
    function showPopover(place, evt) {
        const point = mapInstance.map.latLngToContainerPoint([place.lat, place.lon]);
        const style = mapInstance.getCategoryStyle(place.category);

        popover.style.left = `${Math.min(window.innerWidth - 260, Math.max(460, point.x - 120))}px`;
        popover.style.top = `${Math.min(window.innerHeight - 200, Math.max(80, point.y - 120))}px`;
        popover.style.display = "flex";

        popover.innerHTML = `
            <div style="display: flex; justify-content: space-between; align-items: flex-start;">
                <span class="popover-cat" style="color:${style.bg}">${style.icon} ${style.label}</span>
                <button id="btnClosePopover" style="background:none; border:none; color:#94a3b8; cursor:pointer; font-size:16px;">&times;</button>
            </div>
            <div class="popover-title">${place.name}</div>
            <div style="font-size: 11px; color: #94a3b8;">Place #${place.id} &bull; Node [${place.nodeId}]</div>
            <div class="popover-actions">
                <button class="btn-popover-start" id="btnPopStart">Start (A)</button>
                <button class="btn-popover-dest" id="btnPopDest">Dest (B)</button>
            </div>
        `;

        document.getElementById("btnClosePopover").addEventListener("click", hidePopover);
        document.getElementById("btnPopStart").addEventListener("click", () => {
            setStart(place);
            if (selectedPlaceA && selectedPlaceB) executeRoute();
        });
        document.getElementById("btnPopDest").addEventListener("click", () => {
            setDestination(place);
            if (selectedPlaceA && selectedPlaceB) executeRoute();
        });
    }

    function hidePopover() {
        popover.style.display = "none";
    }

    mapInstance.map.on("click", (e) => {
        hidePopover();
    });

    // 11. Closures List
    function renderClosuresList() {
        const closed = router.getClosedEdges();
        if (closed.length === 0) {
            closuresList.innerHTML = `<div style="font-size: 11.5px; color: #94a3b8; font-style: italic;">All physical campus paths are currently open.</div>`;
            return;
        }

        closuresList.innerHTML = closed.map(e => `
            <div class="closure-item">
                <div>
                    <strong>Path ${e.id}</strong>: ${e.label || (e.type + ' path')}
                </div>
                <button class="btn-reopen" data-edge-id="${e.id}">Reopen</button>
            </div>
        `).join("");

        closuresList.querySelectorAll(".btn-reopen").forEach(btn => {
            btn.addEventListener("click", () => {
                const eid = btn.getAttribute("data-edge-id");
                router.setEdgeStatus(eid, true);
                mapInstance.renderRoads();
                renderClosuresList();
                if (selectedPlaceA && selectedPlaceB) executeRoute();
            });
        });
    }

    btnResetClosures.addEventListener("click", () => {
        router.resetAllEdges();
        mapInstance.renderRoads();
        renderClosuresList();
        if (selectedPlaceA && selectedPlaceB) executeRoute();
    });

    renderClosuresList();

    // 12. Preset Quick Routes
    const presets = [
        { name: "Main Gate &rarr; Orion LHC", a: 1, b: 41 },
        { name: "Main Gate &rarr; Mega Mess I", a: 1, b: 77 },
        { name: "Agate Hostel &rarr; Kailash Mess", a: 46, b: 83 },
        { name: "Amber Hostel &rarr; Mega Mess II", a: 56, b: 79 },
        { name: "Opal Hostels &rarr; Central Library", a: 67, b: 39 },
        { name: "CSE Dept &rarr; Central Canteen", a: 16, b: 88 },
        { name: "Admin Block &rarr; Barn Hall", a: 3, b: 6 },
        { name: "Main Gate &rarr; Sports Complex", a: 1, b: 98 }
    ];

    const presetGrid = document.getElementById("presetGrid");
    presetGrid.innerHTML = presets.map((pr, idx) => `
        <button class="preset-chip" data-preset-idx="${idx}">
            ${pr.name}
        </button>
    `).join("");

    presetGrid.querySelectorAll(".preset-chip").forEach(btn => {
        btn.addEventListener("click", () => {
            const idx = parseInt(btn.getAttribute("data-preset-idx"), 10);
            const pr = presets[idx];
            const pA = campusData.places.find(x => x.id === pr.a);
            const pB = campusData.places.find(x => x.id === pr.b);
            if (pA && pB) {
                setStart(pA);
                setDestination(pB);
                executeRoute();
            }
        });
    });
});
