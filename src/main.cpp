// AstroStreets — a 2D sprite game with a C++/OpenGL engine and (later) Lua spells.
//
// Two threads:
//   * main / render thread  — owns the D3D12 device, polls window events, and
//                             draws the latest world snapshot every vsync.
//   * simulation thread     — owns the World (entities + camera), steps it at a
//                             fixed timestep, and publishes finished frames.
//
// They communicate through a lock-guarded SnapshotBuffer (RenderFrames out) and
// a thread-safe Input (key events in). This file just wires them together.

#include "Engine/Window.h"
#include "Engine/Renderer.h"
#include "Core/SnapshotBuffer.h"
#include "Core/RenderFrame.h"
#include "Game/GameLoop.h"

#include <atomic>
#include <cstdio>
#include <filesystem>
#include <optional>
#include <thread>

namespace fs = std::filesystem;

// Find a sibling folder (assets/ or scripts/) whether we run from the project
// dir or the build output.
static std::optional<fs::path> findDir(const char* name) {
    const char* prefixes[] = { "", "../", "../../", "../../../", "../../../../" };
    for (const char* p : prefixes) {
        fs::path dir = fs::path(p) / name;
        if (fs::exists(dir)) return fs::absolute(dir);
    }
    return std::nullopt;
}

int main() {
    const int WINDOW_W = 1280, WINDOW_H = 720;   // viewport in pixels
    const int COLS = 64, ROWS = 36;              // world size in tiles

    auto assetsDir  = findDir("assets");
    auto scriptsDir = findDir("scripts");
    if (!assetsDir) {
        std::fprintf(stderr, "Could not find the assets/ folder.\n");
        return 1;
    }

    // --- render thread owns the window + D3D12 device ---
    Window window;
    if (!window.create(WINDOW_W, WINDOW_H, "AstroStreets")) return 1;

    Renderer renderer;
    if (!renderer.init(window.nativeHandle(), WINDOW_W, WINDOW_H, assetsDir->generic_string())) {
        window.destroy();
        return 1;
    }

    // --- launch the simulation thread ---
    SnapshotBuffer snapshot;
    std::atomic<bool> running{ true };

    GameLoop loop(snapshot, window.input(), running,
                  scriptsDir ? scriptsDir->generic_string() : "",
                  COLS, ROWS, WINDOW_W, WINDOW_H);
    std::thread simThread([&loop] { loop.run(); });

    // --- render loop (main thread) ---
    RenderFrame frame;
    while (running.load() && !window.shouldClose()) {
        window.pollEvents();
        if (window.input().keyDown("escape")) break;

        snapshot.acquire(frame);
        renderer.draw(frame, window.width(), window.height());
    }

    // --- shut down ---
    running = false;
    if (simThread.joinable()) simThread.join();
    renderer.shutdown();
    window.destroy();
    return 0;
}
