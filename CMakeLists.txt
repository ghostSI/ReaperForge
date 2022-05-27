cmake_minimum_required(VERSION 3.5)

project(ReaperForge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

if (MSVC)
else ()
    set(CMAKE_CXX_FLAGS "")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra")
endif ()

if (CMAKE_HOST_UNIX)
    set(LINUX ON)
elseif (CMAKE_HOST_WIN32)
    set(WINDOWS ON)
endif ()

set(HEADER_FILES
		src/_kiss_fft_guts.h
		src/arrangement.h
		src/camera.h
		src/chordDetector.h
		src/chords.h
		src/chromagram.h
		src/collection.h
		src/configuration.h
		src/data.h
		src/debug.h
		src/file.h
		src/font.h
		src/getopt.h
		src/global.h
		src/helper.h
		src/highway.h
		src/inflate.h
		src/input.h
		src/installer.h
		src/json.h
		src/kiss_fft.h
		src/manifest.h
		src/nuklear.h
		src/oggvorbis.h
        src/opengl.h
        src/pcm.h
        src/player.h
        src/psarc.h
        src/rijndael.h
        src/saves.h
        src/settings.h
        src/shader.h
        src/sng.h
        src/song.h
        src/sound.h
        src/type.h
        src/typedefs.h
        src/ui.h
        src/wem.h
        src/xblock.h
        src/xml.h
        )

set(SOURCE_FILES
		src/arrangement.cpp
		src/camera.cpp
		src/chordDetector.cpp
		src/chords.cpp
		src/chromagram.cpp
		src/collection.cpp
		src/data.cpp
		src/debug.cpp
		src/file.cpp
		src/font.cpp
		src/getopt.cpp
		src/global.cpp
		src/helper.cpp
		src/highway.cpp
		src/inflate.cpp
        src/input.cpp
        src/installer.cpp
        src/json.cpp
        src/kiss_fft.cpp
        src/main.cpp
        src/manifest.cpp
        src/oggvorbis.cpp
        src/opengl.cpp
        src/pcm.cpp
        src/player.cpp
        src/psarc.cpp
        src/rijndael.cpp
        src/saves.cpp
        src/settings.cpp
        src/shader.cpp
        src/sng.cpp
        src/song.cpp
        src/sound.cpp
        src/test.cpp
        src/ui.cpp
        src/wem.cpp
        src/xblock.cpp
        src/xml.cpp
        )

set(RESOURCE_FILES
        )

add_executable(ReaperForge WIN32 ${HEADER_FILES} ${SOURCE_FILES} ${RESOURCE_FILES} ReaperForge.rc)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/native)
set_target_properties(${TARGETNAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/native)

file(COPY ${RESOURCE_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/res)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

if (${LINUX})
    target_link_libraries(ReaperForge stdc++fs)

    ## USE INSTALLED SDL2 ##
    #find_library(SDL2 ClassLibrary)
    #target_link_libraries(ReaperForge SDL2)

    ## USE OWN SDL2 ##
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    target_include_directories(ReaperForge PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/SDL2-2.0.12/Linux/include/)
    target_link_libraries(ReaperForge ${CMAKE_SOURCE_DIR}/3rdparty/SDL2-2.0.12/Linux/lib/libSDL2.a ${CMAKE_DL_LIBS})

    find_package(OpenGL REQUIRED)
    set(OpenGL_GL_PREFERENCE "GLVND")
    include_directories(${OPENGL_INCLUDE_DIRS})
    target_link_libraries(ReaperForge ${OPENGL_LIBRARIES})

elseif (${WINDOWS})

    target_include_directories(ReaperForge PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/SDL2-2.0.12/Windows/include/)
    target_link_directories(ReaperForge PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/SDL2-2.0.12/Windows/lib/)
    target_link_libraries(ReaperForge SDL2.lib)
    target_link_libraries(ReaperForge SDL2main.lib)

    # static link
    target_link_libraries(ReaperForge Ws2_32.lib)
    target_link_libraries(ReaperForge Setupapi.lib)
    target_link_libraries(ReaperForge version.lib)
    target_link_libraries(ReaperForge imm32.lib)
    target_link_libraries(ReaperForge winmm.lib)

    find_package(OpenGL REQUIRED)
    include_directories(${OPENGL_INCLUDE_DIRS})
    target_link_libraries(ReaperForge ${OPENGL_LIBRARIES})

    # static link runtime
    if (MSVC)
        set(CMAKE_SUPPRESS_REGENERATION true)

        target_compile_options(ReaperForge PRIVATE "/MP")
        foreach (flag_var CMAKE_C_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELEASE)
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
            string(REGEX REPLACE "/MDd" "/MT" ${flag_var} "${${flag_var}}")
            string(REGEX REPLACE "/MTd" "/MT" ${flag_var} "${${flag_var}}")
        endforeach (flag_var)
        SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}" CACHE STRING "MSVC C Debug MT flags " FORCE)
        SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}" CACHE STRING "MSVC CXX Debug MT flags " FORCE)
        SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}" CACHE STRING "MSVC C Release MT flags " FORCE)
        SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}" CACHE STRING "MSVC CXX Release MT flags " FORCE)
    endif ()

endif ()
