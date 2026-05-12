# 7-Taskbar-AutoSort build wrapper.
#
# Requires:
#   - GNU Make (Windows: choco install make, or use Git-bash/MSYS2 make)
#   - MSBuild.exe on PATH (launch "x64 Native Tools Command Prompt for VS 2022"
#     or run vcvarsall.bat first). Override with MSBUILD=... if needed.

SHELL := cmd
.SHELLFLAGS := /C

MSBUILD      ?= MSBuild.exe
CLANG_FORMAT ?= clang-format

SLN     := 7-Taskbar-AutoSort.sln
CONFIG  ?= Release
PLATFORM ?= x64

OUTDIR  := build\$(CONFIG)-$(PLATFORM)
ZIPNAME := build\7-Taskbar-AutoSort-$(CONFIG)-$(PLATFORM).zip

.PHONY: all build debug release clean package help lint test test-release format format-check

all: release

help:
	@echo Targets:
	@echo   make release           Build Release x64 (default)
	@echo   make debug             Build Debug x64
	@echo   make build             Build with current CONFIG/PLATFORM ($(CONFIG)/$(PLATFORM))
	@echo   make lint              Run MSVC static analysis checks
	@echo   make test              Build and run tests (current CONFIG/PLATFORM)
	@echo   make test-release      Build and run tests (Release x64)
	@echo   make format            Auto-format all source files with clang-format
	@echo   make format-check      Check formatting without modifying files (used in CI)
	@echo   make clean             Remove build artifacts
	@echo   make package           Zip the Release x64 binaries
	@echo.
	@echo Overrides:
	@echo   make build CONFIG=Debug PLATFORM=Win32
	@echo   make build MSBUILD="C:\path\to\MSBuild.exe"
	@echo   make format CLANG_FORMAT="C:\Program Files\LLVM\bin\clang-format.exe"
	@echo.
	@echo Pre-build validation (macOS): ./scripts/validate-build.sh

lint:
	@echo Running MSVC static analysis...
	@echo Checks: Undefined symbols, missing includes, mismatched braces, type errors
	$(MSBUILD) $(SLN) /p:Configuration=$(CONFIG) /p:Platform=$(PLATFORM) /p:RunCodeAnalysis=true /m /nologo

FORMAT_SOURCES := inject\inject.c inject\json_util.c inject\json_util.h inject\settings.c \
	inject\settings.h inject\tray.c inject\tray.h inject\update.c inject\update.h \
	inject\version_compare.c inject\version_compare.h \
	autosort\autosort_module.c autosort\autosort_module.h \
	tests\test_json_util.cpp tests\test_version_compare.cpp

format:
	$(CLANG_FORMAT) -i $(FORMAT_SOURCES)

format-check:
	$(CLANG_FORMAT) --dry-run --Werror $(FORMAT_SOURCES)

test:
	$(MSBUILD) $(SLN) /p:Configuration=$(CONFIG) /p:Platform=$(PLATFORM) /m /nologo
	build\$(CONFIG)-$(PLATFORM)\tests.exe

test-release:
	@$(MAKE) test CONFIG=Release PLATFORM=x64

build:
	$(MSBUILD) $(SLN) /p:Configuration=$(CONFIG) /p:Platform=$(PLATFORM) /m /nologo

release:
	@$(MAKE) build CONFIG=Release PLATFORM=x64

debug:
	@$(MAKE) build CONFIG=Debug PLATFORM=x64

clean:
	@if exist build rmdir /s /q build
	@echo Cleaned.

package: release
	@if not exist $(OUTDIR)\7-Taskbar-AutoSort.exe ( echo ERROR: $(OUTDIR)\7-Taskbar-AutoSort.exe missing & exit 1 )
	@if not exist $(OUTDIR)\autosort.dll ( echo ERROR: $(OUTDIR)\autosort.dll missing & exit 1 )
	@powershell -NoProfile -Command "Compress-Archive -Path '$(OUTDIR)\7-Taskbar-AutoSort.exe','$(OUTDIR)\autosort.dll','README.md','LICENSE' -DestinationPath '$(ZIPNAME)' -Force"
	@echo Packaged $(ZIPNAME)
