#pragma once
#include <mutex>
#include <string>
#include <vector>

// Thread-safe input state.
//
// The main (window) thread feeds raw GLFW key events in through onKey(); the
// simulation thread reads held-key state and drains discrete presses. A single
// mutex guards both — input volume is tiny, so contention is irrelevant.
//
// The header is GLFW-free on purpose: only the .cpp knows about key codes, so
// gameplay/sim code depends on plain strings ("w", "left", "space", ...).
class Input {
public:
    void onKey(int glfwKey, int glfwAction);     // main thread (GLFW callback)

    bool keyDown(const std::string& name) const; // any thread — is key held now?
    std::vector<std::string> drainPresses();     // sim thread — discrete presses

private:
    mutable std::mutex mutex_;
    std::vector<bool> held_ = std::vector<bool>(512, false); // indexed by GLFW key
    std::vector<std::string> presses_;
};
