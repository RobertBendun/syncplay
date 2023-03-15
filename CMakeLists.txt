cmake_minimum_required(VERSION 3.12)
project(syncplay VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)

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

include_directories(midifile/include/)

set(SRCS
   midifile/src/Options.cpp
   midifile/src/Binasc.cpp
   midifile/src/MidiEvent.cpp
   midifile/src/MidiEventList.cpp
   midifile/src/MidiFile.cpp
   midifile/src/MidiMessage.cpp
)
set(HDRS
    midifile/include/Binasc.h
    midifile/include/MidiEvent.h
    midifile/include/MidiEventList.h
    midifile/include/MidiFile.h
    midifile/include/MidiMessage.h
    midifile/include/Options.h
)
add_library(midifile STATIC ${SRCS} ${HDRS})

include_directories(.)

add_subdirectory(rtmidi)
add_subdirectory(portable-file-dialogs)

hello_imgui_add_app(syncplay syncplay.cc)
target_link_libraries(syncplay PRIVATE Ableton::Link midifile rtmidi portable_file_dialogs)
