add_library(vg_core src/Geodetic3D.cpp src/Geodetic2D.cpp src/Trig.cpp src/Ellipsoid.cpp
    include/vg/core/Math.h)
target_include_directories(vg_core PUBLIC include)
