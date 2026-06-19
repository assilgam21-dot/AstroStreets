#include "Core/Input.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <unordered_map>

namespace {

const std::unordered_map<std::string, int>& namedKeys() {
    static const std::unordered_map<std::string, int> m = {
        {"up", GLFW_KEY_UP}, {"down", GLFW_KEY_DOWN},
        {"left", GLFW_KEY_LEFT}, {"right", GLFW_KEY_RIGHT},
        {"space", GLFW_KEY_SPACE}, {"enter", GLFW_KEY_ENTER},
        {"escape", GLFW_KEY_ESCAPE}, {"esc", GLFW_KEY_ESCAPE},
        {"tab", GLFW_KEY_TAB}, {"lshift", GLFW_KEY_LEFT_SHIFT},
    };
    return m;
}

int nameToKey(const std::string& name) {
    if (name.size() == 1) {
        char c = name[0];
        if (c >= 'a' && c <= 'z') return GLFW_KEY_A + (c - 'a');
        if (c >= 'A' && c <= 'Z') return GLFW_KEY_A + (c - 'A');
        if (c >= '0' && c <= '9') return GLFW_KEY_0 + (c - '0');
    }
    auto it = namedKeys().find(name);
    return it == namedKeys().end() ? GLFW_KEY_UNKNOWN : it->second;
}

std::string keyToName(int key) {
    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z)
        return std::string(1, char('a' + (key - GLFW_KEY_A)));
    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
        return std::string(1, char('0' + (key - GLFW_KEY_0)));
    for (auto& kv : namedKeys())
        if (kv.second == key) return kv.first;
    return "";
}

} // namespace

void Input::onKey(int glfwKey, int glfwAction) {
    if (glfwKey < 0 || glfwKey >= (int)held_.size()) return;
    std::lock_guard<std::mutex> lk(mutex_);
    if (glfwAction == GLFW_PRESS) {
        held_[glfwKey] = true;
        std::string name = keyToName(glfwKey);
        if (!name.empty()) presses_.push_back(name);
    } else if (glfwAction == GLFW_RELEASE) {
        held_[glfwKey] = false;
    }
    // GLFW_REPEAT: leave held_ as-is, no discrete press.
}

bool Input::keyDown(const std::string& name) const {
    int k = nameToKey(name);
    if (k == GLFW_KEY_UNKNOWN || k >= (int)held_.size()) return false;
    std::lock_guard<std::mutex> lk(mutex_);
    return held_[k];
}

std::vector<std::string> Input::drainPresses() {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<std::string> out;
    out.swap(presses_);
    return out;
}
