#pragma once
#include <atomic>
#include <string>

class SnapshotBuffer;
class Input;

// The simulation loop — the body of the dedicated sim thread.
//
// It owns its World (created inside run(), so all game state lives on this
// thread), steps it at a fixed timestep, and publishes a finished RenderFrame
// each iteration. It stops when `running` becomes false.
class GameLoop {
public:
    GameLoop(SnapshotBuffer& snapshot, Input& input, std::atomic<bool>& running,
             std::string scriptsDir, int cols, int rows, int viewportW, int viewportH);

    void run();

private:
    SnapshotBuffer& snapshot_;
    Input& input_;
    std::atomic<bool>& running_;
    std::string scriptsDir_;
    int cols_, rows_;
    int viewW_, viewH_;
};
