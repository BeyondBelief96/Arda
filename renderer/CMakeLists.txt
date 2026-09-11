add_library(vg_renderer src/Window.cpp)
target_include_directories(vg_renderer PUBLIC include)

# PUBLIC: renderer's headers use core's types, so consumers need core too.
# PRIVATE: glfw/glad appear only in .cpp files, so consumers never see them.
target_link_libraries(vg_renderer PUBLIC vg_core PRIVATE glfw glad::glad)
