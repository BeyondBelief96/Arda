#pragma once

#include <string>

// Forward declaration instead of #include <GLFW/glfw3.h>. This is what lets
// GLFW stay a PRIVATE dependency in CMakeLists.txt: the type is only ever a
// pointer here, so the compiler does not need the definition.
struct GLFWwindow;

namespace vg::renderer {

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    bool ShouldClose() const;
    void Clear(float red, float green, float blue, float alpha = 1.0f);
    void SwapBuffers();
    void PollEvents();

private:
    GLFWwindow* m_handle = nullptr;
};

} // namespace vg::renderer
