#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "uci_bundle" for configuration "Release"
set_property(TARGET uci_bundle APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(uci_bundle PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/arm-cortex-m4-soft_floating/uci_bundle/libuci_bundle_full_arm-cortex-m4-soft_floating_rtos_R12.7.0-00405-gb33c5c42726c.a"
  )

list(APPEND _cmake_import_check_targets uci_bundle )
list(APPEND _cmake_import_check_files_for_uci_bundle "${_IMPORT_PREFIX}/lib/arm-cortex-m4-soft_floating/uci_bundle/libuci_bundle_full_arm-cortex-m4-soft_floating_rtos_R12.7.0-00405-gb33c5c42726c.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
