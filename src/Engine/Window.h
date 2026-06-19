#pragma once
#include <string>
#include "Core/Input.h"

struct GLFWwindow;

// Owns the GLFW window and OpenGL context.
//
// THREADING: every method here must be called from the main thread (GLFW
// requires window creation and event polling on the thread that called
// glfwInit). The exception is input(), whose returned object is itself
// thread-safe and is read from the simulation thread.
class Window {
public:
    bool create(int width, int height, const std::string& title);
    void destroy();

    void makeContextCurrent();   // bind GL context to the calling thread
    void swapBuffers();
    void pollEvents();

    bool shouldClose() const;
    void requestClose();

    int width() const { return width_; }
    int height() const { return height_; }
    Input& input() { return input_; }

private:
    GLFWwindow* window_ = nullptr;
    Input input_;
    int width_ = 0, height_ = 0;
};
