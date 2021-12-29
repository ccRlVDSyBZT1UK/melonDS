project(sdl)

SET(SOURCES_SDL
    Platform.cpp
    Config.cpp
    main.cpp
    ${CMAKE_SOURCE_DIR}/res/melon.qrc
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if( ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set(USE_FLAGS "-s USE_SDL=2 -s ALLOW_MEMORY_GROWTH=1  --preload-file assetdir -s ASSERTIONS=1 -s USE_PTHREADS=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${USE_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${USE_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${USE_FLAGS}")
    set(CMAKE_EXECUTABLE_SUFFIX .html)
else()
    find_package(SDL2 REQUIRED)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SLIRP REQUIRED slirp)
pkg_check_modules(LIBARCHIVE REQUIRED libarchive)
add_compile_definitions(ARCHIVE_SUPPORT_ENABLED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(melonDS_sdl ${SOURCES_SDL})

target_include_directories(melonDS_sdl PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_PREFIX}/include ${SLIRP_INCLUDE_DIRS} ${LIBARCHIVE_INCLUDE_DIRS})
#target_link_directories(melonDS_sdl PRIVATE ${SDL2_LIBRARY_DIRS})
#target_link_directories(melonDS_sdl PRIVATE ${SLIRP_LIBRARY_DIRS})
target_link_directories(melonDS_sdl PRIVATE ${LIBARCHIVE_LIBRARY_DIRS})

target_include_directories(melonDS_sdl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(melonDS_sdl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../..")
target_link_libraries(melonDS_sdl core)

if (BUILD_STATIC)
	#target_link_libraries(melonDS_sdl -static ${SDL2_STATIC_LIBRARIES} ${SLIRP_STATIC_LIBRARIES} ${LIBARCHIVE_STATIC_LIBRARIES})
else()
endif()
#target_link_libraries(melonDS_sdl ${SDL2_LIBRARIES} ${SLIRP_LIBRARIES} ${LIBARCHIVE_LIBRARIES})
target_link_libraries(melonDS_sdl ${LIBARCHIVE_LIBRARIES})

if (NOT Iconv_IS_BUILT_IN)
	target_link_libraries(melonDS_sdl ${Iconv_LIBRARIES})
endif()

if (PORTABLE)
    add_definitions(-DPORTABLE)
endif()
