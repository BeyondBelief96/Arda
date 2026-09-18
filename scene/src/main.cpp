#include <arda/core/geometry/Vector3.h>
#include <arda/renderer/Window.h>
#include <arda/core/geometry/Ellipsoid.h>
#include <cstdio>
#include <exception>

using arda::core::geometry::Ellipsoid;
int main() {
    try {
        Ellipsoid wgs84 = Ellipsoid::WGS84();
        arda::renderer::Window window(1280, 720, "Arda");
        while (!window.ShouldClose()) {
            window.Clear(0.02f, 0.05f, 0.12f);
            window.SwapBuffers();
            window.PollEvents();
        }
    } catch (const std::exception& error) {
        std::fprintf(stderr, "fatal: %s\n", error.what());
        return 1;
    }
    return 0;
}
