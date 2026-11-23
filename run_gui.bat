@echo off
rem Launches the NIT Trichy Campus Route Planner Interactive GUI.
setlocal
cd /d "%~dp0"

echo =========================================================
echo    NIT Trichy Campus Route Planner - Interactive GUI
echo =========================================================
echo Launching interactive map GUI in your default browser...

start "" "gui\index.html"

echo.
echo GUI successfully launched!
echo If you prefer running via a local web server, run:
echo    python -m http.server 8080
echo and open: http://localhost:8080/gui/
echo.
