cmake_minimum_required(VERSION 3.0)

project(OpenDDW VERSION 0.0.1 LANGUAGES CXX)

find_package(OpenDDS REQUIRED)

if (NOT EXISTS ${OPENDDW_ROOT})
  set (OPENDDW_ROOT ${CMAKE_SOURCE_DIR})
endif()

set(OPENDDW_INCLUDE_DIR ${OPENDDW_ROOT}/src)

set(MANAGER_HEADER
  ${OPENDDW_ROOT}/src/dds_callback.h
  ${OPENDDW_ROOT}/src/dds_listeners.h
  ${OPENDDW_ROOT}/src/dds_logging.h
  ${OPENDDW_ROOT}/src/dds_manager.h
  ${OPENDDW_ROOT}/src/dds_simple.h
  ${OPENDDW_ROOT}/src/filesystem.h
  ${OPENDDW_ROOT}/src/participant_monitor.h
  ${OPENDDW_ROOT}/src/platformIndependent.h
  ${OPENDDW_ROOT}/src/qos_dictionary.h
)

set(MANAGER_SOURCE
  ${OPENDDW_ROOT}/src/dds_callback.cpp
  ${OPENDDW_ROOT}/src/dds_listeners.cpp
  ${OPENDDW_ROOT}/src/dds_logging.cpp
  ${OPENDDW_ROOT}/src/dds_manager.cpp
  ${OPENDDW_ROOT}/src/participant_monitor.cpp
  ${OPENDDW_ROOT}/src/qos_dictionary.cpp
)

add_library(manager
  ${MANAGER_HEADER}
  ${MANAGER_SOURCE}
)

if (MSVC)
  target_compile_definitions(manager PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
  target_compile_features(manager PRIVATE cxx_std_17)
endif()

target_compile_options(manager PRIVATE $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>: -Wall -pedantic -Wno-unused> $<$<CXX_COMPILER_ID:MSVC>: /W4>)

set(OPENDDS_TARGETS
  OpenDDS::Dcps
  OpenDDS::Rtps
  OpenDDS::Rtps_Udp
)

target_link_libraries(manager
  ${OPENDDS_TARGETS}
  Threads::Threads
)

OPENDDS_TARGET_SOURCES(manager idl/std_qos.idl)
