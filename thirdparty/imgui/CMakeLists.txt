file(GLOB SOURCE_FILES
     repo/*.h
     repo/*.cpp
     repo/examples/imgui_impl_glfw.h
     repo/examples/imgui_impl_glfw.cpp
     repo/examples/imgui_impl_opengl3.h
     repo/examples/imgui_impl_opengl3.cpp)
add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD)
add_library(imgui ${SOURCE_FILES})