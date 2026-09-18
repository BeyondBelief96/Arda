#pragma once

namespace arda::renderer {

    enum class GraphicsApi {
        OpenGL33,
        Direct3D11
    };

    enum class WindowType {
        Default,
        Hidden, // for tests;
    };
}
