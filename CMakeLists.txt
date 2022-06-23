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
		src/base64.h
		src/bnk.h
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
		src/midi.h
		src/nuklear.h
		src/ogg.h
        src/opengl.h
        src/pcm.h
        src/phrases.h
        src/player.h
        src/profile.h
        src/psarc.h
        src/rijndael.h
        src/settings.h
        src/shader.h
        src/sng.h
        src/song.h
        src/sound.h
        src/test.h
        src/type.h
        src/typedefs.h
        src/ui.h
        src/version.h
        src/vst.h
        src/wem.h
        src/xblock.h
        src/xml.h
        )

set(SOURCE_FILES
        src/arrangement.cpp
        src/base64.cpp
        src/bnk.cpp
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
        src/midi.cpp
        src/ogg.cpp
        src/opengl.cpp
        src/pcm.cpp
        src/phrases.cpp
        src/player.cpp
        src/profile.cpp
        src/psarc.cpp
        src/rijndael.cpp
        src/settings.cpp
        src/shader.cpp
        src/sng.cpp
        src/song.cpp
        src/sound.cpp
        src/test.cpp
        src/ui.cpp
        src/vst.cpp
        src/wem.cpp
        src/xblock.cpp
        src/xml.cpp
        )

set(RESOURCE_FILES
        res/ReaperForge.rc
        )

add_executable(ReaperForge WIN32 ${HEADER_FILES} ${SOURCE_FILES} ${RESOURCE_FILES} )

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

set(Ak OFF)
if(${Ak})
	if(CMAKE_GENERATOR MATCHES "Visual Studio 12 2013")
	add_definitions(-DUNICODE -D_UNICODE -DAK_OPTIMIZED)
		target_include_directories(ReaperForge PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/Wwise\ 2013.2.9.4872/include/)
		target_link_directories(ReaperForge PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/Wwise\ 2013.2.9.4872/lib/Release\(StaticCRT\)/lib/)
		target_link_libraries(ReaperForge AkAudioInputSource.lib)
		target_link_libraries(ReaperForge AkCompressorFX.lib)
		target_link_libraries(ReaperForge AkConvolutionReverbFX.lib)
		target_link_libraries(ReaperForge AkDelayFX.lib)
		target_link_libraries(ReaperForge AkExpanderFX.lib)
		target_link_libraries(ReaperForge AkFlangerFX.lib)
		target_link_libraries(ReaperForge AkGainFX.lib)
		target_link_libraries(ReaperForge AkGuitarDistortionFX.lib)
		target_link_libraries(ReaperForge AkHarmonizerFX.lib)
		target_link_libraries(ReaperForge AkMP3Source.lib)
		target_link_libraries(ReaperForge AkMatrixReverbFX.lib)
		target_link_libraries(ReaperForge AkMemoryMgr.lib)
		target_link_libraries(ReaperForge AkMeterFX.lib)
		target_link_libraries(ReaperForge AkMotionGenerator.lib)
		target_link_libraries(ReaperForge AkMusicEngine.lib)
		target_link_libraries(ReaperForge AkParametricEQFX.lib)
		target_link_libraries(ReaperForge AkPeakLimiterFX.lib)
		target_link_libraries(ReaperForge AkPitchShifterFX.lib)
		target_link_libraries(ReaperForge AkRoomVerbFX.lib)
		target_link_libraries(ReaperForge AkRumble.lib)
		target_link_libraries(ReaperForge AkSilenceSource.lib)
		target_link_libraries(ReaperForge AkSineSource.lib)
		target_link_libraries(ReaperForge AkSoundEngine.lib)
		target_link_libraries(ReaperForge AkSoundSeedImpactFX.lib)
		target_link_libraries(ReaperForge AkSoundSeedWind.lib)
		target_link_libraries(ReaperForge AkSoundSeedWoosh.lib)
		target_link_libraries(ReaperForge AkStereoDelayFX.lib)
		target_link_libraries(ReaperForge AkStreamMgr.lib)
		target_link_libraries(ReaperForge AkTimeStretchFX.lib)
		target_link_libraries(ReaperForge AkToneSource.lib)
		target_link_libraries(ReaperForge AkTremoloFX.lib)
		target_link_libraries(ReaperForge AkVorbisDecoder.lib)
		target_link_libraries(ReaperForge AstoundsoundExpanderFX.lib)
		target_link_libraries(ReaperForge AstoundsoundFolddownFX.lib)
		target_link_libraries(ReaperForge AstoundsoundRTIFX.lib)
		target_link_libraries(ReaperForge AstoundsoundShared.lib)
		target_link_libraries(ReaperForge CrankcaseAudioREVModelPlayerFX.lib)
		target_link_libraries(ReaperForge McDSPFutzBoxFX.lib)
		target_link_libraries(ReaperForge McDSPLimiterFX.lib)
		target_link_libraries(ReaperForge iZHybridReverbFX.lib)
		target_link_libraries(ReaperForge iZTrashBoxModelerFX.lib)
		target_link_libraries(ReaperForge iZTrashDelayFX.lib)
		target_link_libraries(ReaperForge iZTrashDistortionFX.lib)
		target_link_libraries(ReaperForge iZTrashDynamicsFX.lib)
		target_link_libraries(ReaperForge iZTrashFiltersFX.lib)
		target_link_libraries(ReaperForge iZTrashMultibandDistortionFX.lib)
	else()
		add_definitions(-DUNICODE -D_UNICODE -DAK_OPTIMIZED)
		target_include_directories(ReaperForge PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/Wwise\ 2022.1.0.7929/include/)
		target_link_directories(ReaperForge PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/Wwise\ 2022.1.0.7929/lib/Release\(StaticCRT\)/lib/)
		target_link_libraries(ReaperForge Ak3DAudioBedMixerFX.lib)
		target_link_libraries(ReaperForge AkAudioInputSource.lib)
		target_link_libraries(ReaperForge AkCompressorFX.lib)
		target_link_libraries(ReaperForge AkDelayFX.lib)
		target_link_libraries(ReaperForge AkExpanderFX.lib)
		target_link_libraries(ReaperForge AkFlangerFX.lib)
		target_link_libraries(ReaperForge AkGainFX.lib)
		target_link_libraries(ReaperForge AkGuitarDistortionFX.lib)
		target_link_libraries(ReaperForge AkHarmonizerFX.lib)
		target_link_libraries(ReaperForge AkMatrixReverbFX.lib)
		target_link_libraries(ReaperForge AkMeterFX.lib)
		target_link_libraries(ReaperForge AkParametricEQFX.lib)
		target_link_libraries(ReaperForge AkPeakLimiterFX.lib)
		target_link_libraries(ReaperForge AkPitchShifterFX.lib)
		target_link_libraries(ReaperForge AkRecorderFX.lib)
		target_link_libraries(ReaperForge AkRoomVerbFX.lib)
		target_link_libraries(ReaperForge AkSilenceSource.lib)
		target_link_libraries(ReaperForge AkSineSource.lib)
		target_link_libraries(ReaperForge AkStereoDelayFX.lib)
		target_link_libraries(ReaperForge AkSynthOneSource.lib)
		target_link_libraries(ReaperForge AkTimeStretchFX.lib)
		target_link_libraries(ReaperForge AkToneSource.lib)
		target_link_libraries(ReaperForge AkTremoloFX.lib)
		target_link_libraries(ReaperForge AkMemoryMgr.lib)
		target_link_libraries(ReaperForge AkMusicEngine.lib)
		target_link_libraries(ReaperForge AkSoundEngine.lib)
		target_link_libraries(ReaperForge AkSpatialAudio.lib)
		target_link_libraries(ReaperForge AkStreamMgr.lib)
		target_link_libraries(ReaperForge AkVorbisDecoder.lib)
		target_link_libraries(ReaperForge AkOpusDecoder.lib)
	endif()
endif()

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
