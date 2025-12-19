cmake_minimum_required(VERSION 3.16)

project(uniquness_maximizer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        #source code:
        src/main.cpp
        src/mainwindow.cpp
        src/maximizer.cpp

        #headers:
        include/mainwindow.h
        include/maximizer.h

        #qrc files:
        res_file.qrc

        #pugixml lib:
        lib/pugixml/pugixml.cpp

        #external libs:

)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(uniquness_maximizer
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}

    )
else()
    if(ANDROID)
        add_library(uniquness_maximizer SHARED
          ${PROJECT_SOURCES}
        )
    else()
        add_executable(uniquness_maximizer
          ${PROJECT_SOURCES}
        )
    endif()
endif()

if(WIN32)
    set(APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.ico")

    if(EXISTS "${APP_ICON}")
        message(STATUS "Found icon file: ${APP_ICON}")

        set(RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/resources/icon.rc")
        file(WRITE ${RC_FILE} "IDI_ICON1 ICON \"${APP_ICON}\"")

        target_sources(uniquness_maximizer PRIVATE ${RC_FILE})

        set_target_properties(uniquness_maximizer PROPERTIES
            WIN32_EXECUTABLE TRUE
            LINK_FLAGS "/MANIFEST:NO"
        )
    else()
        message(WARNING "Icon file not found: ${APP_ICON}")
    endif()
endif()

target_link_libraries(uniquness_maximizer PRIVATE
  Qt${QT_VERSION_MAJOR}::Widgets
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/libzip/lib/zip.lib
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/lib/zlib.lib
)
target_include_directories(uniquness_maximizer PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/pugixml
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/libzip/include
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/include
)

if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.uniquness_maximizer)
endif()
set_target_properties(uniquness_maximizer PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
)

include(GNUInstallDirs)
install(TARGETS uniquness_maximizer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(uniquness_maximizer)
endif()

add_custom_command(TARGET uniquness_maximizer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/libzip/lib/zip.dll
        $<TARGET_FILE_DIR:uniquness_maximizer>

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/lib/zlib.dll
        $<TARGET_FILE_DIR:uniquness_maximizer>
)
