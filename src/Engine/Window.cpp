#include "Engine/Window.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <cstdio>

static void keyCallback(GLFWwindow* w, int key, int, int action, int) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    if (self) self->input().onKey(key, action);
}

bool Window::create(int width, int height, const std::string& title) {
    if (!glfwInit()) { std::fprintf(stderr, "glfwInit failed\n"); return false; }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window_) {
        std::fprintf(stderr, "glfwCreateWindow failed\n");
        glfwTerminate();
        return false;
    }
    width_ = width; height_ = height;
    glfwSetWindowUserPointer(window_, this);
    glfwSetKeyCallback(window_, keyCallback);
    return true;
}

void Window::pollEvents() { glfwPollEvents(); }

bool Window::shouldClose() const { return window_ && glfwWindowShouldClose(window_); }
void Window::requestClose()      { if (window_) glfwSetWindowShouldClose(window_, 1); }

void* Window::nativeHandle() const {
    return window_ ? (void*)glfwGetWin32Window(window_) : nullptr;
}

void Window::destroy() {
    if (window_) glfwDestroyWindow(window_);
    glfwTerminate();
    window_ = nullptr;
}
