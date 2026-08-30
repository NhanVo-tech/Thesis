# SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-Qorvo-2

# This append existing cmake library to zephyr.
function(qorvo_append_cmake_library_to_zephyr lib)
  zephyr_append_cmake_library(${lib})
  # As Zephyr macros don't add INCLUDE_DIRECTORIES from these library, we need to manually add them.
  zephyr_include_directories($<TARGET_PROPERTY:${lib},INTERFACE_INCLUDE_DIRECTORIES>)
  zephyr_compile_definitions($<TARGET_PROPERTY:${lib},INTERFACE_COMPILE_DEFINITIONS>)
  target_link_libraries(${lib} PRIVATE zephyr_interface)
endfunction()

# This function returns the current git tag or hash of the current caller directory in the variable
# GIT_TAG
function(qorvo_get_tag_hash complete_version)
  if(complete_version)
    execute_process(
      COMMAND git describe --exact-match --tags --dirty --match "R*"
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      OUTPUT_VARIABLE git_tag
      ERROR_QUIET
    )

    if(NOT git_tag)
      execute_process(
        COMMAND git describe --tags --dirty --match "R*" --abbrev=12
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE git_tag_hash
        ERROR_QUIET
      )

      if(NOT git_tag_hash)
        execute_process(
          COMMAND git log -1 --format=%H
          WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
          OUTPUT_VARIABLE git_hash
          OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(git_tag_hash ${git_hash})
      else()
        # Add leading zeroes to the number of commits between the last tag and the HEAD
        string(REGEX MATCH "-[0-9]+-g" git_nb_commits ${git_tag_hash})
        string(REGEX MATCH "[0-9]+" git_nb_commits ${git_nb_commits})
        string(REGEX REPLACE "^.*(.....)\$" "\\1" git_nb_commits "00000${git_nb_commits}")

        # Replace -<nb commits tag/head>-g with fixed length element
        string(REGEX REPLACE "-[0-9]+-g" "-${git_nb_commits}-g" git_tag_hash ${git_tag_hash})
      endif()

      set(git_tag ${git_tag_hash})
    endif()
  else()
    execute_process(
      COMMAND git describe --dirty --always --match "R*"
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      OUTPUT_VARIABLE git_tag
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  endif()

  string(STRIP "${git_tag}" git_tag)
  set(GIT_TAG
      ${git_tag}
      PARENT_SCOPE
  )
endfunction()

# qorvo_install_target - helper to install the specific target in predefined directory.
#
# Automatically export the target archive, headers and provide a cmake file to import it. Only run
# if QORVO_EXPORT_LIBRARY is set.
#
# Parameters: target: cmake target to export.
#
function(qorvo_install_target target)
  if(NOT QORVO_EXPORT_LIBRARY)
    return()
  endif()
  cmake_minimum_required(VERSION 3.23)
  get_target_property(${target}_INCLUDES ${target} INTERFACE_INCLUDE_DIRECTORIES)
  get_target_property(${target}_SOURCE_DIR ${target} SOURCE_DIR)

  if(NOT ${target}_INCLUDES STREQUAL "${target}_INCLUDES-NOTFOUND")
    # Reset target interface to rewrite it with correct BUILD and INSTALL interface
    set_target_properties(${target} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "")
    # filter to remove relative path as it is not possible to include them it should be removed once
    # the build system is cleaned
    foreach(dir ${${target}_INCLUDES})
      # Re-build the build interface identically, it will not impact the build
      target_include_directories(${target} INTERFACE $<BUILD_INTERFACE:${dir}>)

      # Ignore directories outside the current directory
      if(NOT dir MATCHES "/\\.\\." OR dir MATCHES "\\.\\./include")
        # Convert absolute path to relative
        string(REPLACE "${${target}_SOURCE_DIR}/" "" dir "${dir}")

        if(dir MATCHES "^include$")
          # Take into account all files and directories under include. The folder "include" cannot
          # be used directly in target_sources as it is also the base directory
          file(
            GLOB_RECURSE subdirectories_and_files
            RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
            ${dir}/*.h ${dir}/*.hh ${dir}/*.hpp
          )
          set(base_directory ${CMAKE_CURRENT_SOURCE_DIR}/${dir})
          set(install_dirs include/${target})
        elseif(dir MATCHES "^\\.\\./include$")
          # Take into account all files and directories under ../include. The folder "include"
          # cannot be used directly in target_sources as it is also the base directory
          file(
            GLOB_RECURSE subdirectories_and_files
            RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
            ${dir}/*.h ${dir}/*.hh ${dir}/*.hpp
          )
          set(base_directory ${CMAKE_CURRENT_SOURCE_DIR}/${dir})
          set(install_dirs include/${target})

        elseif("${dir}" MATCHES "^include/")
          # Take into account directories like include/<dirname>
          set(subdirectories_and_files ${dir})
          set(base_directory ${CMAKE_CURRENT_SOURCE_DIR}/include)
          file(RELATIVE_PATH install_dirs "${CMAKE_CURRENT_SOURCE_DIR}/include"
               "${CMAKE_CURRENT_SOURCE_DIR}/${dir}"
          )
          set(install_dirs "include/${target}/${install_dirs}")

        else()
          # Take into account directories that are not inside "include/" They will be "moved" inside
          # the include directory in the install folder.
          file(
            GLOB_RECURSE subdirectories_and_files
            RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
            ${dir}/*.h ${dir}/*.hh ${dir}/*.hpp
          )
          set(base_directory ${CMAKE_CURRENT_SOURCE_DIR}/${dir})
          set(install_dirs include/${target})
        endif()

        # Select files and directories to copy when installing
        if(subdirectories_and_files)
          target_sources(
            ${target}
            PUBLIC FILE_SET
                   HEADERS
                   TYPE
                   HEADERS
                   BASE_DIRS
                   ${base_directory}
                   FILES
                   "${subdirectories_and_files}"
          )
        endif()

        # Re-build the install interface with only relative pathnames
        target_include_directories(${target} INTERFACE $<INSTALL_INTERFACE:${install_dirs}>)
      endif()
    endforeach()

  endif()

  # Argument DESTINATION is supported for each target type and install has to be called once

  # cmake-lint: disable=E1122
  install(
    TARGETS ${target}
    OPTIONAL
    EXPORT ${target}-config
    ARCHIVE DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/${target}
            FILE_SET HEADERS
            DESTINATION include/${target}
  )

  install(EXPORT ${target}-config DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}/${target}/cmake)
endfunction()

# Shortcut macro to check set a variable if not already set
macro(SET_IF_NOT_SET var value)
  if(NOT ${var})
    set(${var} ${value})
  endif()
endmacro()

# qorvo_generate_config_file - helper for processing Kconfig file into .config file.
#
# This function utilizes olddefconfig program to generate .config file from the input written in
# Kconfig language. File will be saved in CMAKE_BINARY_DIR.
#
# Parameters: kconfig_file: path to Kconfig file.
function(qorvo_generate_config_file kconfig_file)
  find_package(Python 3.8 QUIET REQUIRED COMPONENTS Interpreter)
  message(CHECK_START "Checking kconfiglib module is installed")
  execute_process(
    COMMAND ${Python_EXECUTABLE} -c "import olddefconfig" ERROR_VARIABLE MODULE_CHECK_ERROR
  )
  if(MODULE_CHECK_ERROR)
    message(CHECK_FAIL "not found")
    message(
      FATAL_ERROR
        "olddefconfig module has not been found in your system.
Install it on a Debian-based system, you can use the following commands:
  apt install python3-kconfiglib"
    )
    message(DEBUG ${MODULE_CHECK_ERROR})
  else()
    message(CHECK_PASS "found")
  endif()

  set(config_file "${CMAKE_BINARY_DIR}/.config")
  message(CHECK_START "Generate default '${config_file}' file  ${kconfig_file}")
  execute_process(
    COMMAND ${Python_EXECUTABLE} -m olddefconfig ${kconfig_file}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    ERROR_VARIABLE GENERATION_ERROR
  )
  if(GENERATION_ERROR)
    message(CHECK_FAIL "generation failure")
    message(FATAL_ERROR ${GENERATION_ERROR})
  else()
    message(CHECK_PASS "generation successful")
  endif()
endfunction()

# qorvo_retrieve_config_options - helper for building set of options.
#
# If .config file exists, its values are set in PARENT_SCOPE. If there is no .config in
# CMAKE_BINARY_DIR, helper looks for it in CMAKE_CURRENT_SOURCE_DIR. If both don't exist, .config is
# generated from given Kconfig default values. Config file can be passed also as CONFIG_FILE
# parameter.
#
# Parameters: KCONFIG_FILE: path to Kconfig file. CONFIG_FILE: path to .config file.
#
function(qorvo_retrieve_config_options)
  # cmake-lint: disable=R0912,R0915
  if(QORVO_DISABLE_KCONFIG)
    return()
  endif()
  set(prefix ARGS)
  set(options)
  set(oneValueArgs KCONFIG_FILE CONFIG_FILE)
  set(multiValueArgs)
  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT ${prefix}_KCONFIG_FILE)
    set(kconfig_file "${CMAKE_CURRENT_SOURCE_DIR}/Kconfig")
  else()
    set(kconfig_file ${${prefix}_KCONFIG_FILE})
  endif()

  message(CHECK_START "Looking for \"${kconfig_file}\" file")
  if(EXISTS ${kconfig_file})
    message(CHECK_PASS "found")
  else()
    message(CHECK_FAIL "not found")
    message(FATAL_ERROR "Kconfig file is missing, unable to set options")
  endif()

  if(NOT ${prefix}_CONFIG_FILE)
    message(CHECK_START "Looking for '.config' file in build directory '${CMAKE_BINARY_DIR}'")
    file(GLOB_RECURSE configfile_list "${CMAKE_BINARY_DIR}/.config")

    if(configfile_list)
      list(POP_FRONT configfile_list config_file)
      message(CHECK_PASS "found '${config_file}")
    else()
      message(CHECK_FAIL "not found")
      message(CHECK_START "Looking for '.config' file in source directory
        '${CMAKE_CURRENT_SOURCE_DIR}'"
      )
      file(GLOB configfile_list "${CMAKE_CURRENT_SOURCE_DIR}/.config")
      if(configfile_list)
        list(POP_FRONT configfile_list config_file)
        message(CHECK_PASS "found '${config_file}")
      else()
        message(CHECK_FAIL "not found - generating from kconfig default values")
        qorvo_generate_config_file(${kconfig_file})
        set(config_file "${CMAKE_BINARY_DIR}/.config")
      endif()
    endif()
  else()
    set(config_file ${${prefix}_CONFIG_FILE})
  endif()

  if(EXISTS ${config_file})
    file(STRINGS ${config_file} configFileContent REGEX "^[^#].*$")
    message(DEBUG "Configuration variables are:")
    foreach(line ${configFileContent})
      if(${line} MATCHES "^([A-Za-z0-9_]*)=y$")
        if(NOT ${CMAKE_MATCH_1})
          set(${CMAKE_MATCH_1}
              ON
              PARENT_SCOPE
          )
          message(DEBUG "\t${CMAKE_MATCH_1}=y")
        else()
          message(DEBUG "\t${CMAKE_MATCH_1} is overriden with value '${${CMAKE_MATCH_1}}'.
            Value from '${config_file}' is ignored."
          )
        endif()
      elseif(${line} MATCHES "^([A-Za-z0-9_]*)=([0-9]+)$")
        set(${CMAKE_MATCH_1}
            ${CMAKE_MATCH_2}
            PARENT_SCOPE
        )
        message(DEBUG "\t${CMAKE_MATCH_1}=${CMAKE_MATCH_2}")
      endif()
    endforeach()
  else()
    message(FATAL_ERROR "Unable to read '${config_file}'")
  endif()
endfunction()

#
# qorvo_check_required_parameters - helper to check that required parameters of a function have been
# given. It shall be used with cmake_parse_arguments.
#
# Parameters:
#
# * prefix: prefix that has been set by cmake_parse_arguments.
# * ARGN: a list of argument to check.
#
function(qorvo_check_required_parameters prefix)
  if(NOT ARGN)
    message(FATAL_ERROR "At least one parameter is required in addition to the prefix")
  endif()
  foreach(var ${ARGN})
    if(NOT DEFINED ${prefix}_${var})
      message(FATAL_ERROR "${var} parameter is required")
    endif()
  endforeach()
endfunction()

# qorvo_run_poetry_based_generator - helper to run poetry based generator. This function will
# install dependencies and run a code generator using poetry.
#
# Parameters:
#
# * NAME: name of the generator.
# * PATH: path where the generator is stored.
# * ARGS: arguments to pass to the generator.
# * OUTPUT: output of the generator.
# * COMMENT: comment to display running the generator.
# * DEPENDS: dependencies of the generator.
#
function(qorvo_run_poetry_based_generator)
  set(options)
  set(oneValueArgs NAME PATH COMMENT)
  set(multiValueArgs ARGS OUTPUT DEPENDS)
  set(requiredArgs NAME OUTPUT)
  set(prefix GEN)
  cmake_parse_arguments(${prefix} "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  qorvo_check_required_parameters(${prefix} ${requiredArgs})
  # cmake-lint: disable=C0103
  if(NOT ${prefix}_PATH)
    # Have to disable formatting here because of the name must use uppercase.

    # cmake-lint: disable=C0103
    set(${prefix}_PATH ".")
  endif()

  find_package(poetry 1.3.2...1.8.2 REQUIRED)

  message(
    STATUS "Running '${poetry_EXECUTABLE} install -C ${${prefix}_PATH}' to install dependencies"
  )
  # The command below is needed in order to prevent poetry from using the compiler set by cmake to
  # build the dependencies. It also avoid any prompt from keyring that can block the build.
  execute_process(
    COMMAND
      ${CMAKE_COMMAND} -E env --unset=CC --unset=CXX --unset=VIRTUAL_ENV
      PYTHON_KEYRING_BACKEND=keyring.backends.null.Keyring ${poetry_EXECUTABLE} install -C
      ${${prefix}_PATH} RESULTS_VARIABLE results
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
  )

  foreach(res ${results})
    if(NOT res EQUAL 0)
      message(
        FATAL_ERROR
          "Failed to install dependencies of poetry based generator named ${${prefix}_NAME}:
        ${output} ${error}"
      )
    endif()
  endforeach()

  set(generator_command ${CMAKE_COMMAND} -E env --unset=VIRTUAL_ENV ${poetry_EXECUTABLE} run -C
                        ${${prefix}_PATH} -- ${${prefix}_NAME} ${${prefix}_ARGS}
  )
  list(JOIN generator_command " " gen_command_str)
  message(STATUS "Build will run '${gen_command_str}'")

  add_custom_command(
    OUTPUT ${${prefix}_OUTPUT}
    COMMENT ${${prefix}_COMMENT}
    DEPENDS ${${prefix}_DEPENDS}
    COMMAND ${generator_command}
    COMMAND_EXPAND_LISTS VERBATIM
  )
endfunction()

# This function enables iwyu on specified target
function(qorvo_target_enable_iwyu)
  set(options)
  set(oneValueArgs)
  set(multiValueArgs TARGETS)
  set(requiredArgs TARGETS)
  set(prefix ARGS)
  cmake_parse_arguments(${prefix} "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  qorvo_check_required_parameters(${prefix} ${requiredArgs})

  get_property(iwyu_path_and_options GLOBAL PROPERTY IWYU_PATH_AND_OPTIONS)
  if(NOT iwyu_path_and_options)

    find_program(iwyu_path NAMES include-what-you-use iwyu REQUIRED)

    if(NOT iwyu_path-NOTFOUND)
      execute_process(COMMAND ${iwyu_path} --version OUTPUT_VARIABLE iwyu_output)
      string(REGEX MATCH "include-what-you-use 0\.18.*" iwyu_path_valid_version ${iwyu_output})
      set(iwyu_path-NOTFOUND NOT iwyu_path_valid_version)
    endif()
    if(iwyu_path-NOTFOUND)
      message(
        FATAL_ERROR
          "
        Could not find the 'include-what-you-use ${IWYU_VERSION}'.
        To install do:
        - 'apt install iwyu' except on Ubuntu 22.04
        - For Ubuntu 22.04, see user manual documentation
      "
      )
    endif()
    set(iwyu_path_and_options ${iwyu_path} -Xiwyu --error=0 -Xiwyu
                              --mapping_file=${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../iwyu/gtest.imp
    )
    set_property(GLOBAL PROPERTY IWYU_PATH_AND_OPTIONS ${IWYU_PATH_AND_OPTIONS})
  endif()

  foreach(libname ${${prefix}_TARGETS})
    set_property(TARGET ${libname} PROPERTY C_INCLUDE_WHAT_YOU_USE ${iwyu_path_and_options})
  endforeach()
endfunction()

# Enable only one CONFIG flags to be similar to KConfig.
function(qorvo_enable_component component impl)
  unset(CONFIG_Q_${component} PARENT_SCOPE)
  get_cmake_property(variableNames VARIABLES)
  foreach(variableName ${variableNames})
    if(variableName MATCHES "^CONFIG_Q_${component}_IMPL_.*")
      unset(${variableName} PARENT_SCOPE)
    endif()
  endforeach()
  set(CONFIG_Q_${component}_IMPL_${impl}
      TRUE
      PARENT_SCOPE
  )
  set(CONFIG_Q_${component}
      TRUE
      PARENT_SCOPE
  )
endfunction()

# qorvo_collect_dependencies - helper to retrieve all deps of a target.
#
# Parameters:
#
# * target: the target to retrieve the dependencies from.
#
function(qorvo_collect_dependencies target)
  list(APPEND static_libs ${target})
  get_target_property(libs ${target} LINK_LIBRARIES)
  if(libs)
    list(APPEND deps ${libs})
  endif()
  get_target_property(libs ${target} INTERFACE_LINK_LIBRARIES)
  if(libs)
    list(APPEND deps ${libs})
  endif()

  foreach(dep IN LISTS deps)
    if(TARGET ${dep})
      get_target_property(alias ${dep} ALIASED_TARGET)
      if(TARGET ${alias})
        set(dep ${alias})
      endif()
      get_target_property(type ${dep} TYPE)
      if(${type} STREQUAL "STATIC_LIBRARY")
        list(APPEND static_libs ${dep})
      endif()

      get_property(library_already_added GLOBAL PROPERTY _${tgt_name}_static_bundle_${dep})
      if(NOT library_already_added)
        set_property(GLOBAL PROPERTY _${tgt_name}_static_bundle_${dep} ON)
        qorvo_collect_dependencies(${dep})
      endif()
    endif()
  endforeach()

  set(static_libs
      ${static_libs}
      PARENT_SCOPE
  )
endfunction()

# qorvo_add_static_library_bundle - helper to create a bundle of static libraries.
#
# Parameters:
#
# * bundle: the bundle name
# * TARGETS: The libraries to bundle
#
function(qorvo_add_static_library_bundle bundle)
  set(options)
  set(oneValueArgs)
  set(multiValueArgs TARGETS)
  set(requiredArgs TARGETS)
  set(prefix ARGS)
  cmake_parse_arguments(${prefix} "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  qorvo_check_required_parameters(${prefix} ${requiredArgs})

  foreach(target IN LISTS ${prefix}_TARGETS)
    qorvo_collect_dependencies(${target})
  endforeach()

  list(REMOVE_DUPLICATES static_libs)

  file(TOUCH ${CMAKE_BINARY_DIR}/dummy.c)
  add_library(${bundle} STATIC ${CMAKE_BINARY_DIR}/dummy.c)

  set_target_properties(${bundle} PROPERTIES INTERFACE_LINK_LIBRARIES "" LINK_LIBRARIES "")

  add_custom_target(${bundle}_target ALL COMMENT "Creating bundle ${bundle}")
  foreach(lib IN LISTS static_libs)

    get_target_property(lib_type ${lib} TYPE)
    if(NOT lib_type STREQUAL "INTERFACE_LIBRARY")
      add_custom_command(
        TARGET ${bundle}
        POST_BUILD
        COMMAND mkdir -p ${CMAKE_BINARY_DIR}/${lib}
        COMMAND ${CMAKE_AR} t $<TARGET_FILE:${lib}> | tee ${CMAKE_BINARY_DIR}/${lib}/objects_list
        COMMAND ${CMAKE_AR} x $<TARGET_FILE:${lib}> --output ${CMAKE_BINARY_DIR}/${lib}/
        COMMAND cat ${CMAKE_BINARY_DIR}/${lib}/objects_list | xargs -I{} ${CMAKE_AR} q
                $<TARGET_FILE:${bundle}> ${CMAKE_BINARY_DIR}/${lib}/{}
        COMMENT "Creating bundle for ${lib}"
      )
    endif()
    add_dependencies(${bundle}_target ${lib})
    get_target_property(publish_lib ${lib} PUBLISHED)
    if(publish_lib)

      get_target_property(dep_headers ${lib} HEADER_SET)
      get_target_property(dep_header_dirs ${lib} HEADER_DIRS)
      target_sources(
        ${bundle} INTERFACE FILE_SET HEADERS BASE_DIRS ${dep_header_dirs} FILES ${dep_headers}
      )
    endif()
  endforeach()

  add_dependencies(${bundle}_target ${static_libs})
endfunction()
