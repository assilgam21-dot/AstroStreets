#pragma once
#include <string>
#include "Core/Input.h"

struct GLFWwindow;

class Window {
public:
    bool create(int width, int height, const std::string& title);
    void destroy();

    void pollEvents();
    bool shouldClose() const;
    void requestClose();

    void* nativeHandle() const;

    int width() const { return width_; }
    int height() const { return height_; }
    Input& input() { return input_; }

private:
    GLFWwindow* window_ = nullptr;
    Input input_;
    int width_ = 0, height_ = 0;
};
