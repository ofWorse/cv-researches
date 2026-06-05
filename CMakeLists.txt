cmake_minimum_required(VERSION 3.10)
project(cv-researches)

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 REQUIRED)
find_package(OpenCV REQUIRED)

add_executable(cv-researches 
    src/main.cpp
    src/application.cpp
    src/interpolator.cpp
    src/interpolation_window.cpp
    src/image_viewer.cpp
    src/texture_manager.cpp

    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/imgui_tables.cpp

    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(cv-researches PRIVATE 
    external
    external/imgui
    external/imgui/backends
)

target_link_libraries(cv-researches PRIVATE glfw ${OpenCV_LIBS})

if(APPLE)
    target_link_libraries(cv-researches PRIVATE
        "-framework Cocoa"
        "-framework OpenGL"
        "-framework IOKit"
        "-framework CoreVideo"
    )
elseif(UNIX)
    find_package(OpenGL REQUIRED)
    target_link_libraries(cv-researches PRIVATE OpenGL::GL)
endif()