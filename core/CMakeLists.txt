add_library(arda_core
    src/Geodetic2D.cpp
    src/Geodetic3D.cpp
    src/Trig.cpp
    src/geometry/Ellipsoid.cpp
)
target_include_directories(arda_core PUBLIC include)
