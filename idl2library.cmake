# idl2library(IDLS <files> [BUILD_STATIC])
# Generate a shared or static library for each IDL in a list.
#
# Example:
#           idl2library(IDLS idl/example.idl idl/std_doc.idl)
#           idl2library(IDLS idl/example.idl idl/std_doc.idl BUILD_STATIC)
#           idl2library(IDLS idl/example.idl idl/std_doc.idl INCLUDE_DIRS "${PATH_TO_STD_QOS_IDL}/idl/")
#

cmake_minimum_required(VERSION 3.20)

set (IDL_TARGET_SUFFIX "_idl")

#Converts idl file to a target name:  lowervase with no path or extension with IDL_TARGET_SUFFIX appended
macro(get_idl_target_name idlfile output)
    get_filename_component(current_idl_shortname ${idlfile} NAME_WE)
    string(TOLOWER "${current_idl_shortname}" current_idl_shortname)
    set(temp_output "${current_idl_shortname}")
    string(APPEND temp_output "${IDL_TARGET_SUFFIX}")
    set(${output} ${temp_output})
endmacro()

#Finds all of the input file's (absolute path) dependencies and returns them in IDL_TARGET_DEPENDENCIES
function(find_idl_dependencies input_file)
    unset(search_list)
    unset(return_list)
    LIST(APPEND current_list ${input_file}) #Files that need to be searched (full path)
    LIST(APPEND search_list ${input_file}) #Don't search more than once (full path)

    while(current_list)
        SET(file_list ${current_list})
        unset(current_list)
        foreach(current_file ${file_list})
            set (INCLUDE_REGEX "^[ \t]*\#[ \t]*include[ \t]+[\"|<](.+\\.[I|i][D|d][L|l])[\"|>].*$")
            file(STRINGS ${current_file} included_files REGEX ${INCLUDE_REGEX})
            foreach(included_file ${included_files})
                string(REGEX REPLACE ${INCLUDE_REGEX} "\\1" included_filename "${included_file}")
                get_idl_target_name(${included_filename} current_include_target)
                if (NOT ${${current_include_target}_ABSPATH} IN_LIST search_list)
                    LIST(APPEND current_list ${${current_include_target}_ABSPATH})
                    LIST(APPEND return_list ${current_include_target})
                    LIST(APPEND search_list ${${current_include_target}_ABSPATH}) #The file should now never be searched again
                endif()
            endforeach()
        endforeach()
        unset(file_list)
    endwhile()
    set(IDL_TARGET_DEPENDENCIES ${return_list} PARENT_SCOPE)
endfunction()

function(idl2library)
    set(options BUILD_STATIC)
    set(singleValueArgs)
    set(multiValueArgs IDLS INCLUDE_DIRS)

    cmake_parse_arguments(PARSE_ARGV 0 idl2library "${options}" "${singleValueArgs}" "${multiValueArgs}")

    if (${idl2library_BUILD_STATIC})
        message("Configured to build static libraries.")
    else()
        message("Configured to build shared libraries.")
    endif()

    unset(IDL_WISHLIST)
    #Add idl excention to each file in the list if there is no extension
    foreach(IDL_ARG ${idl2library_IDLS})
        string(REGEX MATCH "\.[I|i][D|d][L|l]$" IDL_ARG_EXTENSION ${IDL_ARG})
        if(IDL_ARG_EXTENSION STREQUAL "")
            #If they didn't then add it..
            set(IDL_ARG "${IDL_ARG}.idl")
        endif()
        list(APPEND IDL_WISHLIST ${IDL_ARG})
    endforeach()

    unset(IDL_DIR_WISHLIST)
    foreach(IDL_DIR_ARG ${idl2library_INCLUDE_DIRS})
        list(APPEND IDL_DIR_WISHLIST ${IDL_DIR_ARG})
    endforeach()

    option(OPENDDS_CPP11_IDL_MAPPING "Use C++11 IDL mapping" OFF)
    option(OPENDDS_CMAKE_VERBOSE "Print verbose output when loading the OpenDDS Config Package" ON)

    find_package(OpenDDS REQUIRED)

    if(NOT IDL_WISHLIST)
        message("No IDLs specified.  Update your CMakeLists.txt to include a list of the required IDLs, and pass that list to idl2library()")
    endif()

    list(REMOVE_DUPLICATES IDL_WISHLIST)
    message("The following idls are being used: ${IDL_WISHLIST}")

    # For each input idl create the following variables:
    # ${current_idl_target}_ABSPATH, ${current_idl_target}_ABSDIR, ${current_idl_target}_RELPATH, ${current_idl_target}_RELDIR
    foreach(SINGLE_IDL ${IDL_WISHLIST})
        get_idl_target_name(${SINGLE_IDL} current_idl_target)
        unset(${current_idl_target}_ABSPATH)
        unset(${current_idl_target}_ABSDIR)
        unset(${current_idl_target}_RELPATH)
        unset(${current_idl_target}_RELDIR)

        cmake_path(IS_ABSOLUTE SINGLE_IDL SINGLE_IDL_IS_ABS)
        if(SINGLE_IDL_IS_ABS)
            set(${current_idl_target}_ABSPATH ${SINGLE_IDL})
        else()
            find_file(${current_idl_target}_ABSPATH
                ${SINGLE_IDL}
                PATHS ${CMAKE_CURRENT_SOURCE_DIR}
                NO_CACHE
                REQUIRED
                NO_DEFAULT_PATH)
            if(NOT ${current_idl_target}_ABSPATH)
                message(FATAL_ERROR "Unable to find ${SINGLE_IDL}. ")
            endif()
        endif()
        if(NOT EXISTS "${${current_idl_target}_ABSPATH}")
            message(FATAL_ERROR "File does not exist: ${${current_idl_target}_ABSPATH}. ")
        else()
           message(DEBUG "    Absolute path: ${${current_idl_target}_ABSPATH}")
        endif()
        #For reasons that I do not understand, cmake_path segfaults on release builds on sles11
        if(NOT CMAKE_PATH_BROKEN)
            cmake_path(RELATIVE_PATH
                ${current_idl_target}_ABSPATH
                BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                OUTPUT_VARIABLE ${current_idl_target}_RELPATH)
        else() #Use old deprecated cmake file() command
            file(RELATIVE_PATH ${current_idl_target}_RELPATH ${CMAKE_CURRENT_SOURCE_DIR} ${${current_idl_target}_ABSPATH})
        endif()
        if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${${current_idl_target}_RELPATH}")
            message(FATAL_ERROR "File does not exist: ${${current_idl_target}_RELPATH}. (Relative to ${CMAKE_CURRENT_SOURCE_DIR}) ")
        endif()
        # Get the directory path of the IDL file we're operating on
        #Save off all directories where idls may be
        get_filename_component(${current_idl_target}_ABSDIR ${${current_idl_target}_ABSPATH} DIRECTORY)
        get_filename_component(${current_idl_target}_RELDIR ${${current_idl_target}_RELPATH} DIRECTORY)
        list(APPEND            ALL_IDL_RELDIRS ${${current_idl_target}_RELDIR})
        list(APPEND            ALL_IDL_PATHS ${${current_idl_target}_ABSPATH})
    endforeach()
    list(REMOVE_DUPLICATES ALL_IDL_RELDIRS)
    list(REMOVE_DUPLICATES ALL_IDL_PATHS)

    #Add a library to build for each input idl
    foreach(SINGLE_IDL ${IDL_WISHLIST})
        get_idl_target_name(${SINGLE_IDL} current_idl_target)

        # If this target has already been created, we're done
        if(TARGET ${current_idl_target})
            unset(${current_idl_target}_ABSPATH)
            message(WARNING "Already created a target for ${current_idl_target}..  Skipping")
            break()
        endif(TARGET ${current_idl_target})

        # Second duplicate target check
        if(TARGET ${current_idl_target})
            unset(${current_idl_target}_ABSPATH)
            message(WARNING "Already created a target for ${current_idl_target}..  Skipping")
            break()
        endif(TARGET ${current_idl_target})

        unset(IDL_TARGET_DEPENDENCIES)
        unset(current_idl_include_opts)

        find_idl_dependencies("${${current_idl_target}_ABSPATH}") #Dependencies are returned in IDL_TARGET_DEPENDENCIES

        #Note:  current_idl_include_opts needs to be a list.  Previously it was a string and that will no longer work correctly.
        foreach(target_dependency ${IDL_TARGET_DEPENDENCIES})
            if(NOT "${${target_dependency}_ABSDIR}" STREQUAL "${${current_idl_target}_ABSDIR}")
                list(APPEND current_idl_include_opts "-I${${target_dependency}_ABSDIR}")
            endif()
        endforeach()
        list(REMOVE_DUPLICATES current_idl_include_opts)

        foreach(include_dir ${IDL_DIR_WISHLIST})
            list(APPEND current_idl_include_opts "-I${include_dir}")
        endforeach()

        if(OPENDDS_CPP11_IDL_MAPPING)
            list(APPEND current_idl_include_opts "-Lc++11 ")
        endif()

        message("Adding library: ${current_idl_target}")
        message("current_idl_include_opts:  ${current_idl_include_opts}")
        message("Dependencies: ${IDL_TARGET_DEPENDENCIES}\n")

        if(idl2library_BUILD_STATIC)
            add_library(${current_idl_target} STATIC)
            set_property(TARGET ${current_idl_target} PROPERTY POSITION_INDEPENDENT_CODE ON)
        else()
            add_library(${current_idl_target} SHARED)
        endif()

        opendds_target_sources(${current_idl_target}
            ${${current_idl_target}_RELPATH}
            OPENDDS_IDL_OPTIONS ${current_idl_include_opts} -Gxtypes-complete
            TAO_IDL_OPTIONS ${current_idl_include_opts}
            INCLUDE_BASE ${${current_idl_target}_ABSDIR}
        )
        target_link_libraries(${current_idl_target}
           ${IDL_TARGET_DEPENDENCIES}
            OpenDDS::Dcps
        )

        # Group the IDL projects together
        set_target_properties(${current_idl_target} PROPERTIES FOLDER IDL)
        target_compile_definitions(${current_idl_target} PUBLIC _HAS_AUTO_PTR_ETC=1 _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)

        # Add this project to the IDL list so it's only loaded 1x
        list(APPEND IDL_TARGETS "${current_idl_target}")
        list(APPEND IDL_RUNTIMES "$<TARGET_FILE:${current_idl_target}>")

        if(USING_INSTALL_HELPER)
            SET_NAME_PROPERTIES(${current_idl_target})
        else()
            if(WIN32)
                set_target_properties(${current_idl_target} PROPERTIES DEBUG_OUTPUT_NAME ${current_idl_target}d)
                set_target_properties(${current_idl_target} PROPERTIES RELEASE_OUTPUT_NAME ${current_idl_target})
                set_target_properties(${current_idl_target} PROPERTIES RELWITHDEBINFO_OUTPUT_NAME ${current_idl_target})

                # Name PDBs appropriately
                set_target_properties(${current_idl_target} PROPERTIES COMPILE_PDB_NAME_DEBUG ${current_idl_target}d)
                set_target_properties(${current_idl_target} PROPERTIES COMPILE_PDB_NAME_RELWITHDEBINFO ${current_idl_target})
                set_target_properties(${current_idl_target} PROPERTIES COMPILE_PDB_NAME_RELEASE ${current_idl_target})
            endif(WIN32)
        endif()
    endforeach()
    list(REMOVE_DUPLICATES IDL_LIBS)
    list(REMOVE_DUPLICATES IDL_RUNTIMES)
    set(IDL_LIBS ${IDL_TARGETS} CACHE INTERNAL "List of idl targets")
    set(IDL_RUNTIMES ${IDL_RUNTIMES} CACHE INTERNAL "list of idl runtime libraries")
    message("idl libs: ${IDL_LIBS}")
    message("idl runtimes: ${IDL_RUNTIMES}")
endfunction(idl2library)
