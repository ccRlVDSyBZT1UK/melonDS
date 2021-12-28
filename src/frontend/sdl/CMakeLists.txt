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

find_package(Qt6 COMPONENTS Core Gui Widgets Network OpenGL OpenGLWidgets REQUIRED)
set(QT_LINK_LIBS Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::OpenGL Qt6::OpenGLWidgets)

find_package(PkgConfig REQUIRED)
find_package(Iconv REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SLIRP REQUIRED slirp)
pkg_check_modules(LIBARCHIVE REQUIRED libarchive)
add_compile_definitions(ARCHIVE_SUPPORT_ENABLED)

add_executable(melonDS_sdl ${SOURCES_SDL})

target_include_directories(melonDS_sdl PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_PREFIX}/include ${SLIRP_INCLUDE_DIRS} ${LIBARCHIVE_INCLUDE_DIRS})
target_link_directories(melonDS_sdl PRIVATE ${SDL2_LIBRARY_DIRS} ${SLIRP_LIBRARY_DIRS})
target_link_directories(melonDS_sdl PRIVATE ${LIBARCHIVE_LIBRARY_DIRS})

target_include_directories(melonDS_sdl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(melonDS_sdl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../..")
target_link_libraries(melonDS_sdl ${QT_LINK_LIBS})
target_link_libraries(melonDS_sdl core)

if (BUILD_STATIC)
	target_link_libraries(melonDS_sdl -static ${SDL2_STATIC_LIBRARIES} ${SLIRP_STATIC_LIBRARIES} ${LIBARCHIVE_STATIC_LIBRARIES})
else()
	target_link_libraries(melonDS_sdl ${SDL2_LIBRARIES} ${SLIRP_LIBRARIES} ${LIBARCHIVE_LIBRARIES})
endif()

if (NOT Iconv_IS_BUILT_IN)
	target_link_libraries(melonDS_sdl ${Iconv_LIBRARIES})
endif()

if (PORTABLE)
    add_definitions(-DPORTABLE)
endif()
