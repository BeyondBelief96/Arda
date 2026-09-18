# Headers are listed for IDE project-tree visibility only; CMake compiles
# just the .cpp sources. Keep this list current when adding headers.
add_library(arda_core
    src/Geodetic2D.cpp
    src/Geodetic3D.cpp
    src/Trig.cpp
    src/geometry/Ellipsoid.cpp
    include/arda/core/Geodetic2D.h
    include/arda/core/Geodetic3D.h
    include/arda/core/Math.h
    include/arda/core/Trig.h
    include/arda/core/geometry/Ellipsoid.h
    include/arda/core/geometry/Vector2.h
    include/arda/core/geometry/Vector3.h
    include/arda/core/geometry/Vector4.h
)
target_include_directories(arda_core PUBLIC include)
