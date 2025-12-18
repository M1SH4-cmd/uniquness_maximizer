# Install script for directory: C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/libzip")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/build/libzip.pc")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libzip/modules" TYPE FILE FILES
    "C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/cmake/FindNettle.cmake"
    "C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/cmake/Findzstd.cmake"
    "C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/cmake/FindMbedTLS.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/build/zipconf.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libzip" TYPE FILE FILES
    "C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/build/libzip-config.cmake"
    "C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/build/libzip-config-version.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libzip/libzip-targets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libzip/libzip-targets.cmake"
         "C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/build/CMakeFiles/Export/ab63c3a9eda5ec24a2943b813039874c/libzip-targets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libzip/libzip-targets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libzip/libzip-targets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libzip" TYPE FILE FILES "C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/build/CMakeFiles/Export/ab63c3a9eda5ec24a2943b813039874c/libzip-targets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libzip" TYPE FILE FILES "C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/build/CMakeFiles/Export/ab63c3a9eda5ec24a2943b813039874c/libzip-targets-debug.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libzip" TYPE FILE FILES "C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/build/CMakeFiles/Export/ab63c3a9eda5ec24a2943b813039874c/libzip-targets-minsizerel.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libzip" TYPE FILE FILES "C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/build/CMakeFiles/Export/ab63c3a9eda5ec24a2943b813039874c/libzip-targets-relwithdebinfo.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libzip" TYPE FILE FILES "C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/build/CMakeFiles/Export/ab63c3a9eda5ec24a2943b813039874c/libzip-targets-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libzip/libzip-bin-targets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libzip/libzip-bin-targets.cmake"
         "C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/build/CMakeFiles/Export/ab63c3a9eda5ec24a2943b813039874c/libzip-bin-targets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libzip/libzip-bin-targets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/libzip/libzip-bin-targets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libzip" TYPE FILE FILES "C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/build/CMakeFiles/Export/ab63c3a9eda5ec24a2943b813039874c/libzip-bin-targets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libzip" TYPE FILE FILES "C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/build/CMakeFiles/Export/ab63c3a9eda5ec24a2943b813039874c/libzip-bin-targets-debug.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libzip" TYPE FILE FILES "C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/build/CMakeFiles/Export/ab63c3a9eda5ec24a2943b813039874c/libzip-bin-targets-minsizerel.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libzip" TYPE FILE FILES "C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/build/CMakeFiles/Export/ab63c3a9eda5ec24a2943b813039874c/libzip-bin-targets-relwithdebinfo.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/libzip" TYPE FILE FILES "C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/build/CMakeFiles/Export/ab63c3a9eda5ec24a2943b813039874c/libzip-bin-targets-release.cmake")
  endif()
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/build/lib/cmake_install.cmake")
  include("C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/build/man/cmake_install.cmake")
  include("C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/build/src/cmake_install.cmake")
  include("C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/build/ossfuzz/cmake_install.cmake")
  include("C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/build/examples/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_COMPONENT MATCHES "^[a-zA-Z0-9_.+-]+$")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
  else()
    string(MD5 CMAKE_INST_COMP_HASH "${CMAKE_INSTALL_COMPONENT}")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INST_COMP_HASH}.txt")
    unset(CMAKE_INST_COMP_HASH)
  endif()
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
  file(WRITE "C:/Programming/Development/Projs/uniquness_maximizer/lib/libzip-1.11.4/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
