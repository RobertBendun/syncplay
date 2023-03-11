cmake_minimum_required(VERSION 3.12)
project(syncplay VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 20)

configure_file(config.hh.in config.hh)
include_directories(build)

include(FetchContent)
FetchContent_Declare(
    hello_imgui
    GIT_REPOSITORY https://github.com/pthom/hello_imgui.git
    # Enter the desired git tag below
    # GIT_TAG
)
FetchContent_MakeAvailable(hello_imgui)
list(APPEND CMAKE_MODULE_PATH ${HELLOIMGUI_CMAKE_PATH})
include(hello_imgui_add_app)

include(link/AbletonLinkConfig.cmake)

hello_imgui_add_app(syncplay syncplay.cc)
target_link_libraries(syncplay PRIVATE Ableton::Link)
