import React, { useState } from "react";
import { Terminal, Copy, Check, Download, Layers, ShieldCheck, Cpu } from "lucide-react";

interface NativeWindowsHubProps {
  onClose: () => void;
}

export const NativeWindowsHub: React.FC<NativeWindowsHubProps> = ({ onClose }) => {
  const [copiedSection, setCopiedSection] = useState<string | null>(null);
  const [activeTab, setActiveTab] = useState<"build" | "source" | "ci">("build");

  const copyToClipboard = (text: string, section: string) => {
    navigator.clipboard.writeText(text);
    setCopiedSection(section);
    setTimeout(() => setCopiedSection(null), 2000);
  };

  const windowsBuildBatch = `@echo off
echo =======================================================
echo   ChronoStrider: Quantum Breach - Windows 10/11 Native Build
echo =======================================================
echo Checking Visual Studio 2022 / C++ Build Tools...

where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] CMake not found. Please install CMake 3.22+ from https://cmake.org/download/
    pause
    exit /b 1
)

echo Configuring CMake for Windows x64 (MSVC Statically Linked)...
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed.
    pause
    exit /b 1
)

echo Building ChronoStrider.exe in Release mode...
cmake --build build --config Release
if %errorlevel% neq 0 (
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

echo =======================================================
echo   BUILD SUCCESSFUL!
echo   Executable location: build\\bin\\Release\\ChronoStrider.exe
echo =======================================================
build\\bin\\Release\\ChronoStrider.exe
pause
`;

  const cmakeListsContent = `cmake_minimum_required(VERSION 3.22)
project(ChronoStrider LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static MSVC runtime linking for zero-dependency Windows 10/11 portability
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(/W4 /WX- /permissive-)
endif()

# Fetch Raylib 5.5 for high-performance native rendering & audio
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)

add_executable(ChronoStrider
    native/src/main.cpp
    native/src/game.cpp
    native/src/renderer.cpp
    native/src/audio.cpp
)

target_include_directories(ChronoStrider PRIVATE native/src)
target_link_libraries(ChronoStrider PRIVATE raylib)

set_target_properties(ChronoStrider PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "\${CMAKE_BINARY_DIR}/bin"
)
`;

  const githubActionsWorkflow = `name: Windows 10/11 Native Release

on:
  push:
    branches: [ main, master ]
  workflow_dispatch:

jobs:
  build-windows:
    runs-on: windows-latest
    steps:
      - name: Checkout repository
        uses: actions/checkout@v4

      - name: Setup MSVC developer command prompt
        uses: ilammy/msvc-dev-cmd@v1

      - name: Configure CMake
        run: cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release

      - name: Build Standalone Executable
        run: cmake --build build --config Release

      - name: Archive Windows Release
        run: |
          mkdir release
          copy build\\bin\\Release\\ChronoStrider.exe release\\
          copy README.md release\\
          Compress-Archive -Path release\\* -DestinationPath ChronoStrider-Windows-x64.zip

      - name: Upload Release Artifact
        uses: actions/upload-artifact@v4
        with:
          name: ChronoStrider-Windows-x64
          path: ChronoStrider-Windows-x64.zip
`;

  return (
    <div className="absolute inset-0 bg-slate-950/90 backdrop-blur-md flex items-center justify-center p-6 z-50 select-none">
      <div className="bg-slate-900 border border-cyan-500/30 rounded-2xl max-w-4xl w-full max-h-[90vh] flex flex-col shadow-2xl overflow-hidden">
        {/* Modal Header */}
        <div className="p-6 border-b border-slate-800 flex items-center justify-between">
          <div className="flex items-center gap-3">
            <div className="p-2 bg-cyan-950/80 border border-cyan-500/40 rounded-xl text-cyan-400">
              <Cpu className="w-6 h-6" />
            </div>
            <div>
              <h2 className="font-heading font-black text-xl text-white tracking-wide">
                WINDOWS 10/11 NATIVE ENGINE & SOURCE HUB
              </h2>
              <p className="text-xs text-slate-400 font-mono">
                Pure C++20 + Raylib 5.5 + CMake + Standalone MSVC x64 Executable
              </p>
            </div>
          </div>

          <button
            onClick={onClose}
            className="px-4 py-2 bg-slate-800 hover:bg-slate-700 text-slate-300 rounded-xl text-xs font-mono font-semibold transition-colors cursor-pointer"
          >
            CLOSE [ESC]
          </button>
        </div>

        {/* Tab Selection */}
        <div className="flex border-b border-slate-800 bg-slate-950/50 px-6 pt-3 gap-2">
          <button
            onClick={() => setActiveTab("build")}
            className={`px-4 py-2 text-xs font-mono font-bold rounded-t-xl transition-all border-b-2 cursor-pointer ${
              activeTab === "build"
                ? "border-cyan-400 text-cyan-300 bg-slate-900"
                : "border-transparent text-slate-400 hover:text-slate-200"
            }`}
          >
            BUILDING FOR WINDOWS 10/11
          </button>
          <button
            onClick={() => setActiveTab("source")}
            className={`px-4 py-2 text-xs font-mono font-bold rounded-t-xl transition-all border-b-2 cursor-pointer ${
              activeTab === "source"
                ? "border-cyan-400 text-cyan-300 bg-slate-900"
                : "border-transparent text-slate-400 hover:text-slate-200"
            }`}
          >
            C++ CMAKE SPECIFICATION
          </button>
          <button
            onClick={() => setActiveTab("ci")}
            className={`px-4 py-2 text-xs font-mono font-bold rounded-t-xl transition-all border-b-2 cursor-pointer ${
              activeTab === "ci"
                ? "border-cyan-400 text-cyan-300 bg-slate-900"
                : "border-transparent text-slate-400 hover:text-slate-200"
            }`}
          >
            GITHUB ACTIONS RELEASE (.EXE)
          </button>
        </div>

        {/* Content Area */}
        <div className="p-6 overflow-y-auto space-y-6 flex-1 text-slate-300">
          {activeTab === "build" && (
            <div>
              <div className="bg-cyan-950/40 border border-cyan-500/30 rounded-xl p-4 mb-6 flex items-start gap-3">
                <ShieldCheck className="w-5 h-5 text-cyan-400 shrink-0 mt-0.5" />
                <div className="text-xs space-y-1">
                  <div className="font-bold text-white uppercase tracking-wider font-mono">
                    Zero-Dependency Windows 10/11 Deployment
                  </div>
                  <p className="text-slate-300">
                    The native engine uses static C runtime linkage (<code className="text-cyan-300 font-mono">/MT</code>), guaranteeing that <code className="text-cyan-300 font-mono">ChronoStrider.exe</code> runs directly on any clean Windows 10/11 gaming PC without needing Visual C++ redistributable packages or external DLLs.
                  </p>
                </div>
              </div>

              <div className="flex items-center justify-between mb-2">
                <span className="text-xs font-mono text-slate-400 uppercase font-semibold">
                  1-Click Windows Batch Script (`build_windows.bat`)
                </span>
                <button
                  onClick={() => copyToClipboard(windowsBuildBatch, "bat")}
                  className="px-3 py-1 bg-slate-800 hover:bg-slate-700 text-xs font-mono text-cyan-400 rounded-lg flex items-center gap-1.5 transition-colors cursor-pointer"
                >
                  {copiedSection === "bat" ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                  {copiedSection === "bat" ? "COPIED" : "COPY SCRIPT"}
                </button>
              </div>

              <pre className="bg-slate-950 rounded-xl p-4 font-mono-code text-xs text-slate-300 overflow-x-auto border border-slate-800">
                {windowsBuildBatch}
              </pre>

              <div className="mt-6 space-y-2">
                <h4 className="font-heading font-bold text-sm text-white uppercase tracking-wider">
                  Manual Build Steps (Command Prompt / PowerShell)
                </h4>
                <div className="bg-slate-950 p-4 rounded-xl border border-slate-800 space-y-2 text-xs font-mono">
                  <div className="text-slate-400"># 1. Generate Visual Studio 2022 x64 Solution:</div>
                  <div className="text-cyan-300">cmake -S . -B build -A x64</div>
                  <div className="text-slate-400"># 2. Build Release Executable:</div>
                  <div className="text-cyan-300">cmake --build build --config Release</div>
                  <div className="text-slate-400"># 3. Launch Native Game:</div>
                  <div className="text-emerald-400">build\bin\Release\ChronoStrider.exe</div>
                </div>
              </div>
            </div>
          )}

          {activeTab === "source" && (
            <div>
              <div className="flex items-center justify-between mb-2">
                <span className="text-xs font-mono text-slate-400 uppercase font-semibold">
                  CMake Configuration (`CMakeLists.txt`)
                </span>
                <button
                  onClick={() => copyToClipboard(cmakeListsContent, "cmake")}
                  className="px-3 py-1 bg-slate-800 hover:bg-slate-700 text-xs font-mono text-cyan-400 rounded-lg flex items-center gap-1.5 transition-colors cursor-pointer"
                >
                  {copiedSection === "cmake" ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                  {copiedSection === "cmake" ? "COPIED" : "COPY CMAKE"}
                </button>
              </div>

              <pre className="bg-slate-950 rounded-xl p-4 font-mono-code text-xs text-slate-300 overflow-x-auto border border-slate-800">
                {cmakeListsContent}
              </pre>

              <div className="mt-4 p-4 bg-slate-950/70 border border-slate-800 rounded-xl text-xs space-y-1">
                <div className="font-heading font-bold text-white uppercase">Engine Structure</div>
                <div className="font-mono text-slate-400">native/src/main.cpp — Application entry, Raylib window, and frame loop</div>
                <div className="font-mono text-slate-400">native/src/game.cpp — 60 FPS deterministic Newtonian space physics & weapon logic</div>
                <div className="font-mono text-slate-400">native/src/renderer.cpp — Neon line renderer, spring warp grid, bloom pipeline</div>
                <div className="font-mono text-slate-400">native/src/audio.cpp — Procedural sound synthesis via Raylib AudioStream</div>
              </div>
            </div>
          )}

          {activeTab === "ci" && (
            <div>
              <div className="flex items-center justify-between mb-2">
                <span className="text-xs font-mono text-slate-400 uppercase font-semibold">
                  GitHub Actions CI Pipeline (`.github/workflows/windows-release.yml`)
                </span>
                <button
                  onClick={() => copyToClipboard(githubActionsWorkflow, "ci")}
                  className="px-3 py-1 bg-slate-800 hover:bg-slate-700 text-xs font-mono text-cyan-400 rounded-lg flex items-center gap-1.5 transition-colors cursor-pointer"
                >
                  {copiedSection === "ci" ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                  {copiedSection === "ci" ? "COPIED" : "COPY WORKFLOW"}
                </button>
              </div>

              <pre className="bg-slate-950 rounded-xl p-4 font-mono-code text-xs text-slate-300 overflow-x-auto border border-slate-800">
                {githubActionsWorkflow}
              </pre>

              <div className="mt-4 p-4 bg-slate-950/70 border border-slate-800 rounded-xl text-xs space-y-1">
                <div className="font-heading font-bold text-white uppercase">Automated Artifact Publishing</div>
                <p className="text-slate-400 leading-relaxed">
                  Every git push triggers GitHub Actions runners to compile on a real Windows Server VM using MSVC, zip the output executable, and publish a downloadable <code className="text-cyan-300 font-mono">ChronoStrider-Windows-x64.zip</code> release artifact for judges!
                </p>
              </div>
            </div>
          )}
        </div>
      </div>
    </div>
  );
};
