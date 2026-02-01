# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\uniquness_maximizer_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\uniquness_maximizer_autogen.dir\\ParseCache.txt"
  "uniquness_maximizer_autogen"
  )
endif()
