cmake_minimum_required(VERSION 3.20)

option(OpenDDWStatic "OpenDDWStatic" OFF) #Build openddw to be compatible with a static build of opendds
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
  src/dds_manager.hpp
)

set(MANAGER_SOURCE
  src/dds_callback.cpp
  src/dds_listeners.cpp
  src/dds_logging.cpp
  src/dds_manager.cpp
  src/participant_monitor.cpp
  src/qos_dictionary.cpp
)

SET(CONFIG_FILE_INSTALL_LOCATION        "share/cmake/${PROJECT_NAME}")
SET(PROJECT_PACKAGE_NAME                "${PROJECT_NAME}")

STRING(TOLOWER "${PROJECT_PACKAGE_NAME}" PROJECT_PACKAGE_NAME)
STRING(TOUPPER "${PROJECT_NAME}"         PROJECT_MACRO_PREFIX)

SET(PROJECT_PACKAGE_CONFIG_IN_FILE      "${PROJECT_PACKAGE_NAME}-config.in.cmake")
SET(PROJECT_PACKAGE_CONFIG_FILE         "${PROJECT_PACKAGE_NAME}-config.cmake")
SET(PROJECT_PACKAGE_CONFIG_VERSION_FILE "${PROJECT_PACKAGE_NAME}-config-version.cmake")
SET(PROJECT_TARGET_FILE                 "${PROJECT_PACKAGE_NAME}-targets.cmake")
SET(PROJECT_TARGET_PREFIX               ${PROJECT_NAME})
SET(PROJECT_TARGET_LIST                 ${PROJECT_NAME})

SET(PROJECT_PUBLIC_HEADER_LOCATION  "include/${PROJECT_NAME}")

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
  OpenDDS::Rtps_Udp
)

target_include_directories(${PROJECT_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:include>
)

set_property(TARGET ${PROJECT_NAME}
	APPEND PROPERTY PUBLIC_HEADER
	${MANAGER_HEADER}
)

target_link_libraries(OpenDDW
  ${OPENDDS_TARGETS}
  Threads::Threads
)

OPENDDS_TARGET_SOURCES(OpenDDW idl/std_qos.idl)

INCLUDE(CMakePackageConfigHelpers)

install(TARGETS ${PROJECT_NAME}
	EXPORT ${PROJECT_NAME}

	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	PUBLIC_HEADER DESTINATION include
)

INSTALL(EXPORT ${PROJECT_NAME}
	NAMESPACE ${PROJECT_TARGET_PREFIX}::
	FILE ${PROJECT_TARGET_FILE}
	DESTINATION ${CONFIG_FILE_INSTALL_LOCATION}
)

INSTALL(FILES
	idl/std_qos.idl
	DESTINATION idl
)

CONFIGURE_PACKAGE_CONFIG_FILE(${PROJECT_PACKAGE_CONFIG_IN_FILE}
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_PACKAGE_CONFIG_FILE}"
  INSTALL_DESTINATION ${CONFIG_FILE_INSTALL_LOCATION}
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

WRITE_BASIC_PACKAGE_VERSION_FILE(
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_PACKAGE_CONFIG_VERSION_FILE}"
  COMPATIBILITY AnyNewerVersion
)

INSTALL(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_PACKAGE_CONFIG_FILE}"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_PACKAGE_CONFIG_VERSION_FILE}"
  "${PROJECT_SOURCE_DIR}/idl2library.cmake"
  DESTINATION "share/cmake/${PROJECT_NAME}"
)
