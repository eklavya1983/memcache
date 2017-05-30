#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "mstch::mstch" for configuration "Release"
set_property(TARGET mstch::mstch APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(mstch::mstch PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libmstch.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS mstch::mstch )
list(APPEND _IMPORT_CHECK_FILES_FOR_mstch::mstch "${_IMPORT_PREFIX}/lib/libmstch.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
