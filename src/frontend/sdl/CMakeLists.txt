project(sdl)

SET(SOURCES_SDL
    Platform.cpp
    Config.cpp
    main.cpp
    Input.cpp
    ${CMAKE_SOURCE_DIR}/res/melon.qrc
)

if( ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set(USE_FLAGS "-s USE_SDL=2 -s ALLOW_MEMORY_GROWTH=1 -s ASSERTIONS=1  -lidbfs.js -s EXPORTED_FUNCTIONS='[\"_HandleButton\", \"_HandleTouch\", \"_LoadState\", \"_LoadRom\", \"_SaveState\", \"_main\"]' -s EXPORTED_RUNTIME_METHODS='[\"allocate\", \"ALLOC_NORMAL\"]'")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${USE_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${USE_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${USE_FLAGS}")
    set(CMAKE_EXECUTABLE_SUFFIX .html)
else()
    find_package(SDL2 REQUIRED)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(LIBARCHIVE REQUIRED libarchive)
add_compile_definitions(ARCHIVE_SUPPORT_ENABLED)

add_executable(melonDS_sdl ${SOURCES_SDL})

target_include_directories(melonDS_sdl PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_PREFIX}/include ${LIBARCHIVE_INCLUDE_DIRS})
target_link_directories(melonDS_sdl PRIVATE ${LIBARCHIVE_LIBRARY_DIRS})
if( ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    target_link_libraries(melonDS_sdl ${LIBARCHIVE_LIBRARIES})
else()
    target_link_directories(melonDS_sdl PRIVATE ${SDL2_LIBRARY_DIRS})
    target_link_libraries(melonDS_sdl ${SDL2_LIBRARIES} ${LIBARCHIVE_LIBRARIES})
endif()

target_include_directories(melonDS_sdl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(melonDS_sdl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../..")
target_link_libraries(melonDS_sdl core)
