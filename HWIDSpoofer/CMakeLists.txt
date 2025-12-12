cmake_minimum_required(VERSION 3.16)
project(HWIDSpoofer VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Windows specific settings
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Third-party libraries
add_subdirectory(ThirdParty/minhook)

# nlohmann json (header only)
include_directories(ThirdParty/nlohmann_json/include)

# Common includes
include_directories(Common)
include_directories(SpooferDLL/include)

# ========== Spoofer DLL ==========
file(GLOB_RECURSE SPOOFER_DLL_SOURCES 
    "SpooferDLL/src/*.cpp"
    "SpooferDLL/src/hooks/*.cpp"
    "SpooferDLL/src/spoof/*.cpp"
    "SpooferDLL/src/utils/*.cpp"
)

add_library(SpooferDLL SHARED ${SPOOFER_DLL_SOURCES})
target_link_libraries(SpooferDLL PRIVATE 
    minhook
    wbemuuid      # WMI
    iphlpapi      # Network adapters
    setupapi      # Setup API
    ole32
    oleaut32
    advapi32
)
target_include_directories(SpooferDLL PRIVATE SpooferDLL/include)

# ========== Injector (CLI) ==========
add_executable(HWIDInjector 
    Injector/src/main.cpp
    Injector/src/injector.cpp
    Injector/src/process_utils.cpp
)
target_include_directories(HWIDInjector PRIVATE Injector/include)
target_link_libraries(HWIDInjector PRIVATE advapi32)

# ========== Injector GUI (Optional) ==========
# add_executable(HWIDSpooferGUI WIN32
#     GUI/src/main.cpp
#     GUI/src/gui.cpp
# )

# Copy config file to output
configure_file(Config/hwid_config.json ${CMAKE_BINARY_DIR}/bin/hwid_config.json COPYONLY)

# Installation
install(TARGETS SpooferDLL HWIDInjector
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
)
install(FILES Config/hwid_config.json DESTINATION bin)

