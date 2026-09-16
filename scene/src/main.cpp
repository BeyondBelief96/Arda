#include <arda/core/geometry/Vector3.h>
#include <arda/renderer/Window.h>

#include <cstdio>
#include <exception>

int main() {
    try {
        // WGS84 ellipsoid radii, in metres.
        const arda::core::Vector3<double> wgs84(6378137.0, 6378137.0, 6356752.314245);
        std::printf("WGS84 radii magnitude: %.3f m\n", wgs84.Magnitude());

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
