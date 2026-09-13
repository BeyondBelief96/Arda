add_library(vg_core src/Geodetic3D.cpp src/Geodetic2D.cpp src/Trig.cpp src/Ellipsoid.cpp)
target_include_directories(vg_core PUBLIC include)
