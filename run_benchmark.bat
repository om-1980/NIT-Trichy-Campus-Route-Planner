@echo off
rem Collects the evaluation measurements into results\*.csv (takes about a minute).
rem Close other heavy programs first; timings depend on the machine and its load.
setlocal
cd /d "%~dp0"
if not exist build\benchmark.exe call build.bat || exit /b 1
if not exist results mkdir results
build\benchmark.exe data/demo results || exit /b 1
rem Append the CPU and OS so the measurements state their environment.
powershell -NoProfile -Command "$c=(Get-CimInstance Win32_Processor | Select-Object -First 1); $o=Get-CimInstance Win32_OperatingSystem; Add-Content -Encoding ascii results\bench_environment.txt ('cpu: ' + $c.Name.Trim() + ' (' + $c.NumberOfCores + ' cores, ' + $c.NumberOfLogicalProcessors + ' threads)'); Add-Content -Encoding ascii results\bench_environment.txt ('os: ' + $o.Caption + ' ' + $o.Version); Add-Content -Encoding ascii results\bench_environment.txt ('ram_gb: ' + [math]::Round($o.TotalVisibleMemorySize/1MB,1))"
type results\bench_environment.txt
