@echo off
setlocal EnableDelayedExpansion

set BUILD_DIR=build-win

:: ── Parse arguments ──────────────────────────────────────────────────────────
set NO_PACKAGE=0
:arg_loop
if "%~1"=="--no-package" ( set NO_PACKAGE=1 & shift & goto arg_loop )
if not "%~1"=="" ( echo Unknown argument: %~1 & exit /b 1 )

:: ── Configure ────────────────────────────────────────────────────────────────
echo =^> Configuring...
cmake -B %BUILD_DIR% ^
    -DCMAKE_BUILD_TYPE=Release ^
    || exit /b 1

:: ── Build ────────────────────────────────────────────────────────────────────
echo =^> Building...
cmake --build %BUILD_DIR% --config Release -j %NUMBER_OF_PROCESSORS% || exit /b 1

echo =^> Build done. Artefacts in %BUILD_DIR%\MidiEnvelopePlugin_artefacts\Release\

:: ── Package ──────────────────────────────────────────────────────────────────
if "%NO_PACKAGE%"=="0" (
    echo.
    powershell -ExecutionPolicy Bypass -File package-win.ps1 || exit /b 1
)

endlocal
