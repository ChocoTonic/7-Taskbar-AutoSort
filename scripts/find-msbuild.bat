@echo off
setlocal EnableDelayedExpansion

where MSBuild.exe >nul 2>&1
if not errorlevel 1 (
    where MSBuild.exe
    exit /b 0
)

set _MSBUILD=

if exist "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe" (
    set "_MSBUILD=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe"
    goto :found
)
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe" (
    set "_MSBUILD=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe"
    goto :found
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" (
    set "_MSBUILD=C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"
    goto :found
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\amd64\MSBuild.exe" (
    set "_MSBUILD=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\amd64\MSBuild.exe"
    goto :found
)
if exist "C:\Program Files (x86)\Microsoft Visual Studio\17\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe" (
    set "_MSBUILD=C:\Program Files (x86)\Microsoft Visual Studio\17\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe"
    goto :found
)

echo MSBuild.exe not found. Install VS Build Tools or use a Developer Command Prompt. >&2
exit /b 1

:found
echo !_MSBUILD!
exit /b 0
