@echo off

setlocal enabledelayedexpansion

REM Captured now because "shift" (used below during argument parsing) can end up
REM shifting %0 too once all positional args are consumed, which would corrupt %~dp0.
set "ScriptDir=%~dp0"

REM All the relative paths below (src, bin, data, -I./include) are relative to the repo root.
REM The cwd change is undone by the implicit endlocal when the script exits.
cd /d "%ScriptDir%.." || (
    echo Error: could not change directory to the repo root.
    exit /b 1
)

set "BuildMode=debug"
set "FileName=sys_win"
set "FilePath=./src/%FileName%.c"
set "Outdir=./bin"
set "Datadir=./data"
set "OutFileName=tix_win"
set "OutFilePath=%Outdir%/%OutFileName%.exe"
set "FlagsFile=%ScriptDir%../compile_flags.txt"
set "DebugFlags=-g -gcodeview -O0 -DDEBUG -Wl,/DEBUG:FULL -fms-runtime-lib=static_dbg"
@REM  set "DebugFlags=!DebugFlags! -fsanitize=address,undefined -fno-omit-frame-pointer"
set "ReleaseFlags=-O3 -DNDEBUG -Werror -Wl,/opt:ref -Wl,/opt:icf -fms-runtime-lib=static"
set "Flags=-mavx2 -luser32 -lgdi32 -lwinmm -ldwmapi -Wl,/subsystem:windows -Wl,/MAP:%Outdir%/%OutFileName%.map,/MAPINFO:EXPORTS"

:parse_args

if "%~1"=="" goto :done_args
if "%~1"=="/?" goto :usage
if /i "%~1"=="-h" goto :usage
if /i "%~1"=="--help" goto :usage
if /i "%~1"=="/m" (
    if "%~2"=="" (
        echo Error: /m requires a value. & exit /b 1
    )
    set "BuildMode=%~2" & shift & shift & goto :parse_args
)
echo Error: Unknown argument "%~1".
echo.

:usage
echo Usage: build.bat [/m debug^|release]
echo.
echo.  /m          Build mode. Defaults to debug.
echo.  /?, -h      Show this help.
if "%~1"=="" exit /b 0
if "%~1"=="/?" exit /b 0
if /i "%~1"=="-h" exit /b 0
if /i "%~1"=="--help" exit /b 0
exit /b 1

:done_args

if /i not "%BuildMode%"=="debug" if /i not "%BuildMode%"=="release" (
    echo Error: Invalid build mode "%BuildMode%". Must be "debug" or "release".
    exit /b 1
)

echo.
REM VCToolsInstallDir is unique to the MSVC dev environment, unlike LIB which other tools (e.g. curl) may already set
REM An already initialized environment is reused only if it targets x64,
REM otherwise LIB/INCLUDE/PATH would point at the wrong architecture and the link would fail.
set "NeedVcvars=0"
if not defined VCToolsInstallDir (
    set "NeedVcvars=1"
) else if /i not "%VSCMD_ARG_TGT_ARCH%"=="x64" (
    echo The current MSVC environment targets "%VSCMD_ARG_TGT_ARCH%" but x64 is required.
    set "NeedVcvars=1"
)

if "%NeedVcvars%"=="1" (
    set "VsWhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist "!VsWhere!" (
        echo Error: vswhere.exe not found. Install Visual Studio Build Tools, or run this script from a Developer Command Prompt.
        exit /b 1
    )

    set "VsInstallPath="
    for /f "usebackq tokens=*" %%i in (`"!VsWhere!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VsInstallPath=%%i"
    )

    if not defined VsInstallPath (
        echo Error: no Visual Studio installation with the C++ build tools component was found.
        exit /b 1
    )

    echo Setting up the MSVC build environment for x64...
    call "!VsInstallPath!\VC\Auxiliary\Build\vcvarsall.bat" x64
    if errorlevel 1 (
        echo Error: vcvarsall.bat failed to initialize the build environment.
        exit /b 1
    )
)
echo.

where clang >nul 2>&1 || (
    echo Error: clang was not found on PATH. Install LLVM or the Visual Studio "C++ Clang tools" component.
    exit /b 1
)

REM Opening the exe for append fails while it is running or held by a debugger; catch that now
REM instead of getting a cryptic link error after the clean step silently skipped it.
if exist "%OutFilePath%" (
    2>nul (>>"%OutFilePath%" type nul) || (
        echo Error: %OutFilePath% is locked. Close the running app or stop the debugger and try again.
        exit /b 1
    )
)

if not exist "%Outdir%" (
    echo Creating %Outdir%...
    mkdir "%Outdir%"
) else (
    echo Cleaning %Outdir%...
    del /s /q "%Outdir%\*" 2>nul
)

if not exist "%Datadir%" (
    echo Creating %Datadir%...
    mkdir "%Datadir%"
) else (
    echo Cleaning %Datadir%...
    del /q "%Datadir%\log.txt" 2>nul
)

if "%BuildMode%"=="debug" (
    REM -fsanitize=address has no static-link option on Windows: the runtime is only ever a DLL,
    REM which must sit next to the exe (or on PATH) or the process fails with STATUS_DLL_NOT_FOUND.
    set "AsanDllName=clang_rt.asan_dynamic-x86_64.dll"

    for /f "usebackq tokens=*" %%R in (`clang -print-resource-dir`) do (
        set "AsanDllPath=%%R\lib\windows\!AsanDllName!"
    )

    if exist "!AsanDllPath!" (
        echo Copying !AsanDllName! into %Outdir%...
        copy /y "!AsanDllPath!" "%Outdir%\" >nul
        if errorlevel 1 (
            echo Error: could not copy !AsanDllName! into %Outdir%. Is it in use by a running process?
            exit /b 1
        )
    ) else (
        echo Warning: could not find !AsanDllName! next to clang; the debug build may fail to start.
    )
)

REM Read flags from file (path is relative to this script's location, not the caller's cwd).
REM Delayed expansion is on, so a "!" inside compile_flags.txt is not supported.
for /f "usebackq tokens=*" %%A in ("%FlagsFile%") do (
    set "line=%%A"
    set "line=!line: =!"
    if not "!line!"=="" if not "!line:~0,2!"=="//" (
        set "Flags=!Flags! %%A"
    )
)

set "Flags=!Flags! -m64"
echo Building for 64-bit ^(x64^)...

if "%BuildMode%"=="debug" (
    set "Flags=!Flags! %DebugFlags%"
    echo Building in DEBUG mode...
) else (
    set "Flags=!Flags! %ReleaseFlags%"
    echo Building in RELEASE mode...
)

echo ================================================================================
echo.

echo Building %OutFilePath% ...
echo.
echo clang !Flags! %FilePath% -o %OutFilePath%
echo.

clang !Flags! %FilePath% -o %OutFilePath%

if errorlevel 1 (
    echo Building %OutFilePath% failed!
    echo If the error mentions access denied, close the running app or debugger and retry.
    exit /b %errorlevel%
)

echo.
echo Building %OutFilePath% succeeded!
