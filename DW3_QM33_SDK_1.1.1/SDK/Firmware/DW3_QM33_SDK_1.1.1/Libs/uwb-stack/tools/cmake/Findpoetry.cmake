# SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
# SPDX-License-Identifier: LicenseRef-Qorvo-2

# check_poetry_version - validator for finding poetry.
#
# This module is used to find poetry executable and check its version. Check cmake documentation for
# more details about validators and its parameters.
function(check_poetry_version validator_result_var item)
  execute_process(COMMAND ${item} --version OUTPUT_VARIABLE poetry_version)
  if(NOT poetry_FIND_VERSION)
    # Set the default version to 1.3.2
    set(poetry_FIND_VERSION "1.3.2")
  endif()
  string(STRIP ${poetry_version} poetry_version)

  if(poetry_version MATCHES "^Poetry \\(version ([0-9]+\.[0-9]+\.[0-9]+)\\)$")
    include(FindPackageHandleStandardArgs)
    find_package_check_version(
      ${CMAKE_MATCH_1} result HANDLE_VERSION_RANGE RESULT_MESSAGE_VARIABLE reason
    )
    if(NOT result)
      message(SEND_ERROR ${reason})
      set(${validator_result_var}
          FALSE
          PARENT_SCOPE
      )
    endif()
  else()
    message(
      SEND_ERROR "Unable to retrieve poetry version (invalid version output: ${poetry_version})"
    )
    set(${validator_result_var}
        FALSE
        PARENT_SCOPE
    )
  endif()
endfunction()

message(CHECK_START "Looking for poetry executable")
find_program(
  poetry_EXECUTABLE poetry PATHS ${POSSIBLE_POETRY_EXE_PATHS} VALIDATOR check_poetry_version
)

if(${poetry_EXECUTABLE} STREQUAL "poetry_EXECUTABLE-NOTFOUND")
  message(CHECK_FAIL "not found")
  set(MESSAGE_TYPE WARNING)
  if(poetry_FIND_REQUIRED)
    set(MESSAGE_TYPE FATAL_ERROR)
  endif()

  message(
    ${MESSAGE_TYPE}
    "poetry ${poetry_FIND_VERSION} binary has not been found in your system
(see cmake manual to find which directory is searched. Variable POSSIBLE_POETRY_EXE_PATHS
can be used to add additonnal paths too).
To install it depending on your system you can use the following commands:
pip install poetry (if you have pip installed)
or
apt install python3-poetry
or
pipx install poetry (if you have pipx installed)"
  )
else()
  message(CHECK_PASS "found")
  # have to disable formatting here because of the name must match the module

  # cmake-lint: disable=C0103
  set(poetry_FOUND TRUE)
  mark_as_advanced(poetry_EXECUTABLE poetry_FOUND)
  return()
endif()
