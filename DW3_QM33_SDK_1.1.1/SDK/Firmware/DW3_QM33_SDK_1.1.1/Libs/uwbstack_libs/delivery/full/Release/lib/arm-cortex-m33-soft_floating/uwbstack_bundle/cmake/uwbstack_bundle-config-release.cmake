#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "uwbstack_bundle" for configuration "Release"
set_property(TARGET uwbstack_bundle APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(uwbstack_bundle PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/arm-cortex-m33-soft_floating/uwbstack_bundle/libuwbstack_bundle_full_arm-cortex-m33-soft_floating_rtos_R12.7.0-00405-gb33c5c42726c.a"
  )

list(APPEND _cmake_import_check_targets uwbstack_bundle )
list(APPEND _cmake_import_check_files_for_uwbstack_bundle "${_IMPORT_PREFIX}/lib/arm-cortex-m33-soft_floating/uwbstack_bundle/libuwbstack_bundle_full_arm-cortex-m33-soft_floating_rtos_R12.7.0-00405-gb33c5c42726c.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
