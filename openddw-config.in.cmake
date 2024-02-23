@PACKAGE_INIT@

set(@PROJECT_NAME@_TARGET_LIST
  @PROJECT_TARGET_LIST@
)

include(CMakeFindDependencyMacro)

# Find OpenDDS
#This allows DDS_ROOT to be set off OpenDDSConfig.cmake location
option(OPENDDS_ALLOW_ENV_CHANGE "Allow multiple find_package(opendds) calls." ON)
find_package(OpenDDS REQUIRED)

#Include idl2dll.cmake so it can be used if/when needed
include(${CMAKE_CURRENT_LIST_DIR}/idl2library.cmake)

list(GET ${@PROJECT_NAME@_TARGET_LIST} 0 _FIRST_TARGET)
if(NOT TARGET ${_FIRST_TARGET})
  # Include the CMake targets file for @PROJECT_NAME@.
  #  All targets are specified in this file.
  include(${CMAKE_CURRENT_LIST_DIR}/@PROJECT_TARGET_FILE@)
endif(NOT TARGET ${_FIRST_TARGET})

# Iterate over all targets and set appropriate properties.
# Most projects will only have one target, but some may include multiple targets that can be imported with one find_package call.
set(_TARGET_FULL_NAME "")
foreach(_TARGET_NAME IN LISTS @PROJECT_NAME@_TARGET_LIST)
  set(_TARGET_FULL_NAME "@PROJECT_TARGET_PREFIX@::${_TARGET_NAME}")
  get_target_property(_IMPORTED_CONFIGS ${_TARGET_FULL_NAME} IMPORTED_CONFIGURATIONS)
  #Release config is required, so assume it is present
  #RelWithDebInfo is optional
  #Debug is optional
  #MinSizeRel is normally not required, but covered as optional here for completeness' sake
  if(NOT "RELWITHDEBINFO" IN_LIST _IMPORTED_CONFIGS)
    set_target_properties(${_TARGET_FULL_NAME} PROPERTIES
      MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
    )
  endif(NOT "RELWITHDEBINFO" IN_LIST _IMPORTED_CONFIGS)
 
  if(NOT "DEBUG" IN_LIST _IMPORTED_CONFIGS)
    if ("RELWITHDEBINFO" IN_LIST _IMPORTED_CONFIGS)
      set_target_properties(${_TARGET_FULL_NAME} PROPERTIES
        MAP_IMPORTED_CONFIG_DEBUG RelWithDebInfo
      )
    else("RELWITHDEBINFO" IN_LIST _IMPORTED_CONFIGS)
      set_target_properties(${_TARGET_FULL_NAME} PROPERTIES
        MAP_IMPORTED_CONFIG_DEBUG Release
      )
    endif("RELWITHDEBINFO" IN_LIST _IMPORTED_CONFIGS)
  endif(NOT "DEBUG" IN_LIST _IMPORTED_CONFIGS)
 
  if(NOT "MINSIZEREL" IN_LIST _IMPORTED_CONFIGS)
    set_target_properties(${_TARGET_FULL_NAME} PROPERTIES
      MAP_IMPORTED_CONFIG_MINSIZEREL Release
    )
  endif(NOT "MINSIZEREL" IN_LIST _IMPORTED_CONFIGS)
endforeach(_TARGET_NAME)

set(_TARGET_FULL_NAME)
set(_IMPORTED_CONFIGS)
set(@PROJECT_NAME@_TARGET_LIST)
set(_FIRST_TARGET)

