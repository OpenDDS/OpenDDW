cmake_minimum_required(VERSION 3.20)

project(OpenDDW VERSION 0.0.1 LANGUAGES CXX)

find_package(OpenDDS REQUIRED)

set(MANAGER_HEADER
  src/dds_callback.h
  src/dds_listeners.h
  src/dds_logging.h
  src/dds_manager.h
  src/dds_simple.h
  src/participant_monitor.h
  src/platformIndependent.h
  src/qos_dictionary.h
)

set(MANAGER_SOURCE
  src/dds_callback.cpp
  src/dds_listeners.cpp
  src/dds_logging.cpp
  src/dds_manager.cpp
  src/participant_monitor.cpp
  src/qos_dictionary.cpp
)

add_library(OpenDDW
  ${MANAGER_HEADER}
  ${MANAGER_SOURCE}
)

target_compile_features(OpenDDW PRIVATE cxx_std_17)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(OpenDDW PRIVATE
    /W4 # Set the warning level to "Level4"
    /wd4251 #Must have dll-interface to be used by clients of struct.
    /wd4244 #Conversion warning from ACE.
    /WX # Warnings are errors!!!!
  )

  target_compile_definitions(OpenDDW PRIVATE _CRT_SECURE_NO_WARNINGS)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(OpenDDW PRIVATE
    -Wall
    -Wextra
    -Wshadow
    -Wpedantic
    -Werror # Warnings are errors!!!!
  )
endif()

target_compile_options(OpenDDW PRIVATE $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>: -Wall -Wextra -Wpedantic -Wno-unused> $<$<CXX_COMPILER_ID:MSVC>: /W4>)

set(OPENDDS_TARGETS
  OpenDDS::OpenDDS
)

target_include_directories(OpenDDW INTERFACE src)

target_link_libraries(OpenDDW
  ${OPENDDS_TARGETS}
  Threads::Threads
)

OPENDDS_TARGET_SOURCES(OpenDDW idl/std_qos.idl)

INSTALL(FILES
	"${CMAKE_SOURCE_DIR}/idl2library.cmake"
	DESTINATION "share/cmake/${PROJECT_NAME}"
)
