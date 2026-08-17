cmake_minimum_required(VERSION 3.16)

project(uniquness_maximizer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

set(PROJECT_SOURCES
        #source code:
        src/main.cpp
        src/mainwindow.cpp
        src/maximizer.cpp
        src/core/document.cpp
        src/core/json_utils.cpp
        src/document/docx_reader.cpp
        src/document/document_chunker.cpp
        src/analysis/prompt_builder.cpp
        src/analysis/response_parser.cpp
        src/analysis/analysis_engine.cpp
        src/providers/ollama_provider.cpp

        #headers:
        include/mainwindow.h
        include/maximizer.h
        include/core/document.h
        include/core/document_chunk.h
        include/core/json_utils.h
        include/document/docx_reader.h
        include/document/document_chunker.h
        include/core/ai_types.h
        include/core/analysis_types.h
        include/analysis/prompt_builder.h
        include/analysis/response_parser.h
        include/analysis/analysis_engine.h
        include/providers/ai_provider.h
        include/providers/ollama_provider.h

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
  Qt${QT_VERSION_MAJOR}::Network
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

include(CTest)
if(BUILD_TESTING)
    # Each test is an executable built from tests/<name>.cpp plus the production
    # sources it exercises, linked against Qt Core and the shared test support headers.
    function(add_project_test name)
        cmake_parse_arguments(TEST "" "" "SOURCES;LIBRARIES;INCLUDE_DIRS" ${ARGN})
        add_executable(${name} tests/${name}.cpp ${TEST_SOURCES})
        target_link_libraries(${name} PRIVATE Qt${QT_VERSION_MAJOR}::Core ${TEST_LIBRARIES})
        target_include_directories(${name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/support
            ${TEST_INCLUDE_DIRS}
        )
        add_test(NAME ${name} COMMAND ${name})
    endfunction()

    add_project_test(docx_reader_test
        SOURCES
            src/core/document.cpp
            src/document/docx_reader.cpp
            lib/pugixml/pugixml.cpp
        LIBRARIES
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/libzip/lib/zip.lib
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/lib/zlib.lib
        INCLUDE_DIRS
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/pugixml
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/libzip/include
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/include
    )

    add_project_test(document_chunker_test
        SOURCES
            src/core/document.cpp
            src/document/document_chunker.cpp
    )

    add_project_test(analysis_contract_test
        SOURCES
            src/core/json_utils.cpp
            src/analysis/prompt_builder.cpp
            src/analysis/response_parser.cpp
    )

    add_project_test(analysis_engine_test
        SOURCES
            src/core/document.cpp
            src/core/json_utils.cpp
            src/document/document_chunker.cpp
            src/analysis/prompt_builder.cpp
            src/analysis/response_parser.cpp
            src/analysis/analysis_engine.cpp
            include/analysis/analysis_engine.h
    )

    add_project_test(ollama_provider_test
        SOURCES
            src/core/json_utils.cpp
            src/providers/ollama_provider.cpp
            include/providers/ollama_provider.h
        LIBRARIES
            Qt${QT_VERSION_MAJOR}::Network
    )

    add_project_test(ollama_integration_test
        SOURCES
            src/core/json_utils.cpp
            src/providers/ollama_provider.cpp
            include/providers/ollama_provider.h
            src/analysis/prompt_builder.cpp
            src/analysis/response_parser.cpp
        LIBRARIES
            Qt${QT_VERSION_MAJOR}::Network
    )
endif()
