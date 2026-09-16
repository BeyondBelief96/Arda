add_library(arda_renderer src/Window.cpp)
target_include_directories(arda_renderer PUBLIC include)

# PUBLIC: renderer's headers use core's types, so consumers need core too.
# PRIVATE: glfw/glad appear only in .cpp files, so consumers never see them.
target_link_libraries(arda_renderer PUBLIC arda_core PRIVATE glfw glad::glad)
