#include "Game/GameLoop.h"
#include "Game/World.h"
#include "Core/SnapshotBuffer.h"
#include "Core/RenderFrame.h"
#include "Core/Input.h"

#include <chrono>
#include <thread>

GameLoop::GameLoop(SnapshotBuffer& snapshot, Input& input, std::atomic<bool>& running,
                   std::string scriptsDir, int cols, int rows, int viewportW, int viewportH)
    : snapshot_(snapshot), input_(input), running_(running),
      scriptsDir_(std::move(scriptsDir)), cols_(cols), rows_(rows),
      viewW_(viewportW), viewH_(viewportH) {}

void GameLoop::run() {
    // The World (all game state) lives entirely on this simulation thread.
    World world(input_, cols_, rows_, viewW_, viewH_, scriptsDir_);
    world.Setup();

    RenderFrame frame;
    world.Render(frame);
    snapshot_.publish(frame);   // give the renderer a first frame

    using clock = std::chrono::steady_clock;
    const double TICK = 1.0 / 60.0;   // fixed simulation step
    const int    MAX_STEPS = 5;       // clamp catch-up to avoid a spiral of death

    auto prev = clock::now();
    double accumulator = 0.0;

    while (running_.load()) {
        auto now = clock::now();
        double elapsed = std::chrono::duration<double>(now - prev).count();
        prev = now;
        accumulator += elapsed;

        int steps = 0;
        while (accumulator >= TICK && steps < MAX_STEPS) {
            world.Update((float)TICK);
            accumulator -= TICK;
            ++steps;
        }
        if (accumulator > TICK * MAX_STEPS) accumulator = 0.0;

        world.Render(frame);
        snapshot_.publish(frame);

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}
