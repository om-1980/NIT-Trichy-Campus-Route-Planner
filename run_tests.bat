@echo off
rem Runs the automated tests; writes results\test_results.csv and results\correctness_metrics.csv.
setlocal
cd /d "%~dp0"
if not exist build\run_tests.exe call build.bat || exit /b 1
if not exist results mkdir results
build\run_tests.exe data/demo results
set RC=%ERRORLEVEL%
if %RC%==0 (echo ALL TESTS PASSED) else (echo %RC% TEST^(S^) FAILED)
exit /b %RC%
